// ==============================================================================
// Source/DSP/WavetableOscillator.h
// ==============================================================================
#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>
#include <complex>
#include <atomic>
#include <cmath> // std::isnan, std::floor, std::isfinite を確実に使うため追加
#include "../Generated/WavetableData_Generated.h"

class WavetableOscillator
{
public:
    using SIMDFloat = juce::dsp::SIMDRegister<float>;
    static constexpr int MaxVoices = 12;
    static constexpr int SimdWidth = SIMDFloat::size();
    static constexpr int MaxBlocks = (MaxVoices + SimdWidth - 1) / SimdWidth;
    static constexpr int NumLevels = 10;

    struct WavetableSet : public juce::ReferenceCountedObject {
        using Ptr = juce::ReferenceCountedObjectPtr<WavetableSet>;
        std::array<juce::AudioBuffer<float>, NumLevels> levels;
        int frameSize = 2048;
        int numFrames = 0;
        int totalSamples = 0;
        WavetableSet() = default;
    };

    WavetableOscillator() {
        formatManager.registerBasicFormats();
        auto* emptySet = new WavetableSet();
        for (int i = 0; i < NumLevels; ++i) {
            emptySet->levels[i].setSize(1, 2048);
            emptySet->levels[i].clear();
        }
        emptySet->totalSamples = 2048;
        emptySet->frameSize = 2048;
        emptySet->numFrames = 1;
        currentWavetableSet = emptySet;

        auto& random = juce::Random::getSystemRandom();
        for (int i = 0; i < MaxVoices; ++i) {
            driftRate[i] = 0.1f + random.nextFloat() * 0.4f;
        }

        createDefaultBasicShapes();
        resetPhase();
    }

    ~WavetableOscillator() {
        loadJobId++;
        backgroundPool->removeAllJobs(true, 1000);
    }

    void prepare(double sr) { sampleRate = std::max(1.0, sr); }

    void createDefaultBasicShapes() {
        const int myJobId = ++loadJobId;
        WavetableSet::Ptr newSet = new WavetableSet();
        newSet->totalSamples = 8192;
        newSet->frameSize = 2048;
        newSet->numFrames = 4;

        juce::AudioBuffer<float> tempRaw(1, newSet->totalSamples);
        auto* writePtr = tempRaw.getWritePointer(0);

        for (int i = 0; i < 2048; ++i) {
            float p = i / 2048.0f;
            writePtr[i] = std::sin(p * juce::MathConstants<float>::twoPi);
            writePtr[2048 + i] = 2.0f * std::abs(2.0f * (p - std::floor(p + 0.5f))) - 1.0f;
            writePtr[4096 + i] = 2.0f * (p - std::floor(p + 0.5f));
            writePtr[6144 + i] = p < 0.5f ? 1.0f : -1.0f;
        }

        for (int lvl = 0; lvl < NumLevels; ++lvl) {
            newSet->levels[lvl].setSize(1, newSet->totalSamples);
            newSet->levels[lvl].makeCopyOf(tempRaw);
        }
        currentWavetableSet = newSet;
        runBandlimitingTask(myJobId, newSet, tempRaw);
    }

    void loadWavetableFile(juce::String fileName) {
        const int myJobId = ++loadJobId;
        juce::File file = juce::File(EmbeddedWavetables::wavetablesDir).getChildFile(fileName);
        if (!file.existsAsFile()) return;

        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
        if (reader == nullptr) return;

        WavetableSet::Ptr newSet = new WavetableSet();
        juce::AudioBuffer<float> tempRaw(1, (int)reader->lengthInSamples);
        reader->read(&tempRaw, 0, (int)reader->lengthInSamples, 0, true, false);

        newSet->totalSamples = tempRaw.getNumSamples();
        newSet->frameSize = (newSet->totalSamples >= 2048) ? 2048 : newSet->totalSamples;
        newSet->numFrames = std::max(1, newSet->totalSamples / newSet->frameSize);

        for (int lvl = 0; lvl < NumLevels; ++lvl) {
            newSet->levels[lvl].setSize(1, newSet->totalSamples);
            newSet->levels[lvl].makeCopyOf(tempRaw);
        }
        currentWavetableSet = newSet;
        runBandlimitingTask(myJobId, newSet, tempRaw);
    }

    void loadCustomWavetableFile(const juce::File& file) {
        const int myJobId = ++loadJobId;
        if (!file.existsAsFile()) return;

        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
        if (reader == nullptr) return;

        WavetableSet::Ptr newSet = new WavetableSet();
        juce::AudioBuffer<float> tempRaw(1, (int)reader->lengthInSamples);
        reader->read(&tempRaw, 0, (int)reader->lengthInSamples, 0, true, false);

        newSet->totalSamples = tempRaw.getNumSamples();
        newSet->frameSize = (newSet->totalSamples >= 2048) ? 2048 : newSet->totalSamples;
        newSet->numFrames = std::max(1, newSet->totalSamples / newSet->frameSize);

        for (int lvl = 0; lvl < NumLevels; ++lvl) {
            newSet->levels[lvl].setSize(1, newSet->totalSamples);
            newSet->levels[lvl].makeCopyOf(tempRaw);
        }
        currentWavetableSet = newSet;
        runBandlimitingTask(myJobId, newSet, tempRaw);
    }

    void runBandlimitingTask(int myJobId, WavetableSet::Ptr newSet, juce::AudioBuffer<float> tempRaw) {
        backgroundPool->addJob([this, myJobId, newSet, tempRaw]() {
            juce::dsp::FFT fft(11);
            juce::AudioBuffer<float> workBuf(1, 4096);
            for (int lvl = 1; lvl < NumLevels; ++lvl) {
                if (myJobId != loadJobId.load()) return;
                for (int f = 0; f < newSet->numFrames; ++f) {
                    if (myJobId != loadJobId.load()) return;
                    workBuf.clear();
                    auto* workPtr = workBuf.getWritePointer(0);
                    for (int i = 0; i < 2048; ++i) {
                        size_t srcIdx = static_cast<size_t>(f) * 2048 + static_cast<size_t>(i);
                        workPtr[i] = (srcIdx < static_cast<size_t>(newSet->totalSamples)) ? tempRaw.getSample(0, static_cast<int>(srcIdx)) : 0.0f;
                    }
                    fft.performRealOnlyForwardTransform(workPtr);
                    int harmonicLimit = std::max(2, 1024 >> lvl);
                    int transitionStart = (int)(harmonicLimit * 0.8f);
                    for (int k = transitionStart; k <= 1024; ++k) {
                        float multiplier = 0.0f;
                        if (k < harmonicLimit) {
                            float fraction = (float)(k - transitionStart) / (float)(harmonicLimit - transitionStart);
                            multiplier = 0.5f * (1.0f + std::cos(fraction * juce::MathConstants<float>::pi));
                        }
                        if (k == 1024) workPtr[1] *= multiplier;
                        else { workPtr[2 * k] *= multiplier; workPtr[2 * k + 1] *= multiplier; }
                    }
                    workPtr[0] = 0.0f;
                    fft.performRealOnlyInverseTransform(workPtr);
                    float peak = 1e-9f;
                    for (int i = 0; i < 2048; ++i) peak = std::max(peak, std::abs(workPtr[i]));
                    float normalizeScale = 1.0f / peak;
                    auto* destPtr = newSet->levels[lvl].getWritePointer(0);
                    for (int i = 0; i < 2048; ++i) {
                        size_t dstIdx = static_cast<size_t>(f) * 2048 + static_cast<size_t>(i);
                        if (dstIdx < static_cast<size_t>(newSet->totalSamples)) destPtr[dstIdx] = juce::jlimit(-1.0f, 1.0f, workPtr[i] * normalizeScale);
                    }
                }
            }
            });
    }

    void generateSingleCycle(std::array<float, 512>& displayBuffer) {
        WavetableSet::Ptr set = currentWavetableSet.get();
        if (set == nullptr || set->totalSamples == 0) { displayBuffer.fill(0.0f); return; }
        auto* ptr = set->levels[0].getReadPointer(0);
        float framePos = wtPosition * (float)std::max(0, set->numFrames - 1);
        int frameIdx = (int)framePos;
        float frameFrac = framePos - (float)frameIdx;

        size_t f0_base = static_cast<size_t>(frameIdx) * static_cast<size_t>(set->frameSize);
        size_t f1_base = static_cast<size_t>(std::min(frameIdx + 1, set->numFrames - 1)) * static_cast<size_t>(set->frameSize);

        for (int i = 0; i < 512; ++i) {
            float originalPhase = i / 512.0f;
            float phase = originalPhase;
            float mod = 0.0f;
            if (fmWaveform == 0) mod = std::sin(phase * juce::MathConstants<float>::twoPi);
            else if (fmWaveform == 1) mod = phase * 2.0f - 1.0f;
            else if (fmWaveform == 2) mod = phase < 0.5f ? 1.0f : -1.0f;
            else if (fmWaveform == 3) mod = 4.0f * std::abs(phase - 0.5f) - 1.0f;

            float tp = phase + mod * fmAmount * 0.1f;
            tp -= std::floor(tp); // ★修正: マイナス方向への確実なラップ

            float syncA = 1.0f, syncB = 1.0f, syncC = 1.0f;
            tp = applyPhaseWarp(tp, morphAMode, morphAAmount, morphAShift, syncA, originalPhase);
            tp = applyPhaseWarp(tp, morphBMode, morphBAmount, morphBShift, syncB, originalPhase);
            tp = applyPhaseWarp(tp, morphCMode, morphCAmount, morphCShift, syncC, originalPhase);

            tp -= std::floor(tp); // ★修正
            if (!std::isfinite(tp)) tp = 0.0f; // ★修正: NaNガード

            float floatPos = tp * set->frameSize;
            int pos = (int)floatPos;
            float frac = floatPos - (float)pos;
            float val = (getHermiteSample(ptr, f0_base, set->frameSize, pos, frac) * (1.0f - frameFrac) +
                getHermiteSample(ptr, f1_base, set->frameSize, pos, frac) * frameFrac);
            val *= (syncA * syncB * syncC);
            val = applyAmpWarp(val, morphAMode, morphAAmount, morphAShift, originalPhase);
            val = applyAmpWarp(val, morphBMode, morphBAmount, morphBShift, originalPhase);
            val = applyAmpWarp(val, morphCMode, morphCAmount, morphCShift, originalPhase);
            displayBuffer[i] = val;
        }
    }

    void setFrequency(float freqHz) { baseFreq = freqHz; recalculate(); }
    void setUnisonCount(int c) { unisonCount = juce::jlimit(1, MaxVoices, c); recalculate(); }
    void setUnisonDetune(float d) { detuneAmount = d; recalculate(); }
    void setStereoWidth(float w) { stereoWidth = juce::jlimit(0.0f, 1.0f, w); recalculate(); }
    void setWavetablePosition(float pos) { wtPosition = pos; }
    void setFMAmount(float amt) { fmAmount = amt; }
    void setFMWaveform(int shape) { fmWaveform = juce::jlimit(0, 3, shape); }
    void setDriftAmount(float amt) { driftAmount = amt; }
    void setMorphA(int mode, float amt, float shift) { morphAMode = mode; morphAAmount = amt; morphAShift = shift; }
    void setMorphB(int mode, float amt, float shift) { morphBMode = mode; morphBAmount = amt; morphBShift = shift; }
    void setMorphC(int mode, float amt, float shift) { morphCMode = mode; morphCAmount = amt; morphCShift = shift; }
    void setOscOn(bool on) { oscOn = on; }
    void setWavetableLevel(float level) { wtLevel = level; }
    void setWavetablePitchOffset(float semitones) { wtPitchOffset = semitones; }
    void setPitchDecay(float amount, float timeMs) {
        pitchDecayAmt = amount;
        pitchDecayCoef = (timeMs <= 0.001f) ? 0.0f : std::exp(-6.9077f / (timeMs * 0.001f * (float)sampleRate));
    }
    void setSubOn(bool on) { subOn = on; }
    void setSubWaveform(int shape) { subWaveform = juce::jlimit(0, 3, shape); }
    void setSubVolume(float vol) { subVolume = vol; }
    void setSubPitchOffset(float semitones) { subPitchOffset = semitones; }

    void resetPhase() {
        auto& random = juce::Random::getSystemRandom();
        for (int b = 0; b < MaxBlocks; ++b) {
            for (int i = 0; i < SimdWidth; ++i) phases[b].set(i, random.nextFloat());
        }
        subPhase = 0.0f; pitchEnvState = 1.0f;
        for (int i = 0; i < MaxVoices; ++i) driftPhase[i] = random.nextFloat();
    }

    void getSampleStereo(float& outL, float& outR, float& subL, float& subR) {
        outL = 0.0f; outR = 0.0f; subL = 0.0f; subR = 0.0f;
        WavetableSet::Ptr set = currentWavetableSet.get();
        if (set == nullptr || set->totalSamples == 0) return;

        pitchEnvState = (pitchEnvState < 0.0001f) ? 0.0f : pitchEnvState * pitchDecayCoef;
        float dynamicPitchMult = std::pow(2.0f, (wtPitchOffset + (pitchDecayAmt * pitchEnvState)) * 0.083333f);

        float framePos = wtPosition * (float)std::max(0, set->numFrames - 1);
        int frameIdx = (int)framePos;
        float frameFrac = framePos - (float)frameIdx;

        size_t f0_base = static_cast<size_t>(frameIdx) * static_cast<size_t>(set->frameSize);
        size_t f1_base = static_cast<size_t>(std::min(frameIdx + 1, set->numFrames - 1)) * static_cast<size_t>(set->frameSize);

        SIMDFloat outL_simd(0.0f), outR_simd(0.0f);
        int numBlocks = (unisonCount + SimdWidth - 1) / SimdWidth;

        for (int b = 0; b < numBlocks; ++b) {
            SIMDFloat p = phases[b];
            SIMDFloat modSignal(0.0f);

            for (int i = 0; i < SimdWidth; ++i) {
                float pVal = p.get(i);
                float mVal = 0.0f;
                if (fmWaveform == 0) {
                    float z = pVal - 0.5f;
                    mVal = -(z * 6.2831853f - (z * z * z) * 41.341702f + (z * z * z * z * z) * 81.605249f);
                }
                else if (fmWaveform == 1) { mVal = pVal * 2.0f - 1.0f; }
                else if (fmWaveform == 2) { mVal = pVal < 0.5f ? 1.0f : -1.0f; }
                else if (fmWaveform == 3) { mVal = 4.0f * std::abs(pVal - 0.5f) - 1.0f; }
                modSignal.set(i, mVal);
            }

            SIMDFloat tp_simd = p + modSignal * SIMDFloat(fmAmount * 0.1f);
            SIMDFloat vAmp(0.0f), nextP(0.0f);

            for (int i = 0; i < SimdWidth; ++i) {
                int vIdx = b * SimdWidth + i;
                if (vIdx >= unisonCount) { nextP.set(i, p.get(i)); continue; }

                float originalPhase = p.get(i);

                driftPhase[vIdx] += driftRate[vIdx] / (float)sampleRate;
                if (driftPhase[vIdx] >= 1.0f) driftPhase[vIdx] -= 1.0f;
                float drift = std::sin(driftPhase[vIdx] * juce::MathConstants<float>::twoPi) * driftAmount * 0.01f;
                float step = increments[b].get(i) * dynamicPitchMult * (1.0f + drift);
                float maxH = (float)sampleRate / (2.0f * std::max(1.0f, step * (float)sampleRate));
                int lvl0 = 0; float hL = 1024.0f;
                while (lvl0 < NumLevels - 2 && (hL * 0.5f) > maxH) { lvl0++; hL *= 0.5f; }
                float lvlFrac = (hL > maxH) ? juce::jlimit(0.0f, 1.0f, (hL - maxH) / (hL * 0.5f)) : 0.0f;

                float tp = tp_simd.get(i);
                tp -= std::floor(tp); // ★修正: マイナス方向へ確実にラップ

                float sA = 1.0f, sB = 1.0f, sC = 1.0f;

                tp = applyPhaseWarp(tp, morphAMode, morphAAmount, morphAShift, sA, originalPhase);
                tp = applyPhaseWarp(tp, morphBMode, morphBAmount, morphBShift, sB, originalPhase);
                tp = applyPhaseWarp(tp, morphCMode, morphCAmount, morphCShift, sC, originalPhase);

                tp -= std::floor(tp); // ★修正: 最終ラップ
                if (!std::isfinite(tp)) tp = 0.0f; // ★修正: NaNが伝播した場合はゼロクリア

                float fPos = tp * set->frameSize; int pos = (int)fPos; float frac = fPos - (float)pos;
                auto* p0 = set->levels[lvl0].getReadPointer(0); auto* p1 = set->levels[lvl0 + 1].getReadPointer(0);
                float v0 = getHermiteSample(p0, f0_base, set->frameSize, pos, frac) * (1.0f - frameFrac) + getHermiteSample(p0, f1_base, set->frameSize, pos, frac) * frameFrac;
                float v1 = getHermiteSample(p1, f0_base, set->frameSize, pos, frac) * (1.0f - frameFrac) + getHermiteSample(p1, f1_base, set->frameSize, pos, frac) * frameFrac;
                float val = (v0 * (1.0f - lvlFrac) + v1 * lvlFrac) * (sA * sB * sC);

                val = applyAmpWarp(val, morphAMode, morphAAmount, morphAShift, originalPhase);
                val = applyAmpWarp(val, morphBMode, morphBAmount, morphBShift, originalPhase);
                val = applyAmpWarp(val, morphCMode, morphCAmount, morphCShift, originalPhase);

                vAmp.set(i, oscOn ? val * amp[b].get(i) : 0.0f);
                float pNext = p.get(i) + step;
                pNext -= std::floor(pNext); // ★修正
                nextP.set(i, pNext);
            }
            outL_simd += vAmp * panL[b]; outR_simd += vAmp * panR[b]; phases[b] = nextP;
        }

        for (int i = 0; i < SimdWidth; ++i) { outL += outL_simd.get(i) * wtLevel; outR += outR_simd.get(i) * wtLevel; }

        if (subOn && subVolume > 0.001f) {
            subPhase += (baseFreq * std::pow(2.0f, subPitchOffset * 0.083333f)) / (float)sampleRate;
            subPhase -= std::floor(subPhase); // ★修正
            float rs = 0.0f;
            switch (subWaveform) {
            case 0: rs = std::sin(subPhase * juce::MathConstants<float>::twoPi); break;
            case 1: rs = 4.0f * std::abs(subPhase - 0.5f) - 1.0f; break;
            case 2: rs = subPhase < 0.5f ? 1.0f : -1.0f; break;
            case 3: rs = 2.0f * subPhase - 1.0f; break;
            }
            subLpfState += (juce::MathConstants<float>::twoPi * 250.0f / (float)sampleRate) * (rs - subLpfState);
            subL = subLpfState * subVolume;
            subR = subLpfState * subVolume;
        }
    }

private:
    double sampleRate = 44100.0;
    float baseFreq = 440.0f, detuneAmount = 0.0f, stereoWidth = 1.0f, wtPosition = 0.0f, fmAmount = 0.0f, driftAmount = 0.0f;
    int unisonCount = 1, fmWaveform = 0;
    bool oscOn = true, subOn = true;
    int morphAMode = 0, morphBMode = 0, morphCMode = 0;
    float morphAAmount = 0.0f, morphBAmount = 0.0f, morphCAmount = 0.0f;
    float morphAShift = 0.0f, morphBShift = 0.0f, morphCShift = 0.0f;
    float wtLevel = 1.0f, wtPitchOffset = 0.0f, pitchDecayAmt = 0.0f, pitchDecayCoef = 0.0f, pitchEnvState = 0.0f;
    int subWaveform = 0; float subVolume = 0.0f, subPitchOffset = -12.0f, subPhase = 0.0f, subLpfState = 0.0f;
    std::array<float, MaxVoices> driftPhase = { 0 }, driftRate = { 0 };
    juce::AudioFormatManager formatManager;

    juce::SharedResourcePointer<juce::ThreadPool> backgroundPool;

    std::atomic<int> loadJobId{ 0 };
    juce::ReferenceCountedObjectPtr<WavetableSet> currentWavetableSet;
    std::array<SIMDFloat, MaxBlocks> phases, increments, panL, panR, amp;

    inline float applyPhaseWarp(float p, int mode, float amt, float shift, float& sOut, float originalPhase) const {
        if (mode == 0) return p;

        float warped = p;
        if (mode == 1) {
            float sym = std::clamp(0.5f + shift * 0.49f, 0.01f, 0.99f);
            float b = std::exp(-std::clamp(amt, -0.99f, 0.99f) * 2.0f);
            if (p < sym) warped = sym * std::pow(p / sym, b);
            else warped = sym + (1.0f - sym) * (1.0f - std::pow((1.0f - p) / (1.0f - sym), b));
        }
        else if (mode == 2) {
            float c = std::clamp(0.5f + shift * 0.4f, 0.1f, 0.9f);
            float pw = std::clamp(0.01f + (amt * 0.5f + 0.5f) * 0.98f, 0.01f, 0.99f);
            float ps = p + (0.5f - c); ps -= std::floor(ps);
            float w = (ps < pw) ? (ps / pw * 0.5f) : (0.5f + (ps - pw) / (1.0f - pw) * 0.5f);
            warped = w + (c - 0.5f); warped -= std::floor(warped);
        }
        else if (mode == 3) {
            float ratio = 1.0f + std::abs(amt) * 7.0f;
            float res = (p + shift * 0.5f) * ratio; res -= std::floor(res);
            if (res > 0.985f) sOut *= (1.0f - res) / 0.015f; else if (res < 0.015f) sOut *= res / 0.015f;
            warped = res - shift * 0.5f; warped -= std::floor(warped);
        }
        else if (mode == 4) {
            float pt = std::clamp(0.5f + shift * 0.4f, 0.1f, 0.9f);
            float m = (p < pt) ? p * (0.5f / pt) : 1.0f - (p - pt) * (0.5f / (1.0f - pt));
            if (amt < 0.0f) m = (p > pt) ? (p - pt) * (0.5f / (1.0f - pt)) : 1.0f - p * (0.5f / pt);
            warped = p * (1.0f - std::abs(amt)) + m * std::abs(amt);
        }

        if (originalPhase < 0.005f) {
            float mix = originalPhase / 0.005f;
            return warped * mix + originalPhase * (1.0f - mix);
        }
        else if (originalPhase > 0.995f) {
            float mix = (1.0f - originalPhase) / 0.005f;
            return warped * mix + originalPhase * (1.0f - mix);
        }
        return warped;
    }

    inline float applyAmpWarp(float v, int mode, float amt, float shift, float originalPhase) const {
        if (mode < 5 || mode > 7) return v;

        float warped = v;
        if (mode == 5) {
            float d = 0.05f, out = 0.0f;
            if (v > shift + d) out = shift - (v - shift);
            else if (v < shift - d) out = shift + (shift - v);
            else { float t = (v - (shift - d)) / (2.0f * d); out = (shift + d) + (t * t * (3.0f - 2.0f * t)) * (-2.0f * d); }
            warped = v * (1.0f - std::abs(amt)) + out * std::abs(amt);
        }
        else if (mode == 6) {
            float st = std::pow(2.0f, 2.0f + (1.0f - std::abs(amt)) * 14.0f);
            float sc = (v + shift) * st, bs = std::floor(sc), fr = sc - bs, ed = 0.1f, sm = (fr > 1.0f - ed) ? (fr - (1.0f - ed)) / ed : 0.0f;
            warped = v * (1.0f - std::abs(amt)) + ((bs + (sm * sm * (3.0f - 2.0f * sm))) / st - shift) * std::abs(amt);
        }
        else if (mode == 7) {
            float dr = (v + shift * 0.5f) * (1.0f + std::abs(amt) * 4.0f);
            warped = (amt >= 0.0f ? std::tanh(dr) : std::sin(dr * 1.570796f)) - (shift * 0.5f);
        }

        if (originalPhase < 0.005f) return warped * (originalPhase / 0.005f);
        else if (originalPhase > 0.995f) return warped * ((1.0f - originalPhase) / 0.005f);

        return warped;
    }

    static inline float hermite(float t, float y0, float y1, float y2, float y3) {
        float c1 = 0.5f * (y2 - y0), c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3, c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * t + c2) * t + c1) * t + y1;
    }

    // ★修正: マイナスのインデックスを絶対に防ぐ安全なモジュロ演算へ書き換え
    inline float getHermiteSample(const float* ptr, size_t bOff, int fs, int pos, float t) const {
        int p0 = ((pos - 1) % fs + fs) % fs;
        int p1 = (pos % fs + fs) % fs;
        int p2 = ((pos + 1) % fs + fs) % fs;
        int p3 = ((pos + 2) % fs + fs) % fs;
        return hermite(t, ptr[bOff + static_cast<size_t>(p0)], ptr[bOff + static_cast<size_t>(p1)], ptr[bOff + static_cast<size_t>(p2)], ptr[bOff + static_cast<size_t>(p3)]);
    }

    void recalculate() {
        float norm = 1.0f / std::sqrt((float)unisonCount), cAng = 0.785398f;
        for (int b = 0; b < MaxBlocks; ++b) {
            SIMDFloat pL(0.0f), pR(0.0f), a(0.0f), inc(0.0f);
            for (int i = 0; i < SimdWidth; ++i) {
                int vIdx = b * SimdWidth + i;
                if (vIdx < unisonCount) {
                    float bias = (unisonCount == 1) ? 0.0f : (2.0f * vIdx / (float)(unisonCount - 1) - 1.0f);
                    inc.set(i, (float)((baseFreq * std::pow(2.0f, (bias * bias * bias * detuneAmount * 0.5f) * 0.083333f)) / sampleRate));
                    float ang = cAng + ((bias + 1.0f) * cAng - cAng) * (baseFreq < 150.0f ? stereoWidth * (baseFreq / 150.0f) : stereoWidth);
                    pL.set(i, std::cos(ang)); pR.set(i, std::sin(ang)); a.set(i, norm);
                }
            }
            panL[b] = pL; panR[b] = pR; amp[b] = a; increments[b] = inc;
        }
    }
};