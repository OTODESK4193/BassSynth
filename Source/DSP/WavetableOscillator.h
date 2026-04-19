// ==============================================================================
// Source/DSP/WavetableOscillator.h
// ==============================================================================
#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>
#include <complex>
#include <atomic>
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
            float speedHz = 0.1f + random.nextFloat() * 0.4f;
            driftRate[i] = speedHz;
        }
        resetPhase();
    }

    ~WavetableOscillator() {
        loadJobId++;
        backgroundPool.removeAllJobs(true, 1000);
    }

    void prepare(double sr) { sampleRate = std::max(1.0, sr); }

    void loadWavetableFile(juce::String fileName) {
        const int myJobId = ++loadJobId;

        // メインスレッド（または即座に）レベル0だけを読み込み、UIと音出しを最優先で可能にする
        juce::File file = juce::File(EmbeddedWavetables::wavetablesDir).getChildFile(fileName);
        if (!file.existsAsFile()) return;

        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
        if (reader == nullptr) return;

        WavetableSet::Ptr newSet = new WavetableSet();
        juce::AudioBuffer<float> tempRaw;
        tempRaw.setSize(1, (int)reader->lengthInSamples);
        reader->read(&tempRaw, 0, (int)reader->lengthInSamples, 0, true, false);

        newSet->totalSamples = tempRaw.getNumSamples();
        newSet->frameSize = (newSet->totalSamples >= 2048) ? 2048 : newSet->totalSamples;
        newSet->numFrames = std::max(1, newSet->totalSamples / newSet->frameSize);

        // レベル0（オリジナル波形）は全レベルに一旦コピーして即座に再生可能にする
        for (int lvl = 0; lvl < NumLevels; ++lvl) {
            newSet->levels[lvl].makeCopyOf(tempRaw);
        }

        // 即座に差し替え（ゼロレイテンシロード）
        currentWavetableSet = newSet;

        // バックグラウンドでエイリアシング防止用のMipmap（レベル1〜9）を計算する
        backgroundPool.addJob([this, myJobId, newSet, tempRaw]() {
            juce::dsp::FFT fft(11);
            juce::AudioBuffer<float> workBuf(1, 4096);

            for (int lvl = 1; lvl < NumLevels; ++lvl) {
                if (myJobId != loadJobId.load()) return; // 新しいリクエストが来たらキャンセル

                if (newSet->frameSize == 2048) {
                    for (int f = 0; f < newSet->numFrames; ++f) {
                        if (myJobId != loadJobId.load()) return;

                        workBuf.clear();
                        auto* workPtr = workBuf.getWritePointer(0);
                        for (int i = 0; i < 2048; ++i) {
                            int srcIdx = f * 2048 + i;
                            workPtr[i] = (srcIdx < newSet->totalSamples) ? tempRaw.getSample(0, srcIdx) : 0.0f;
                        }

                        fft.performRealOnlyForwardTransform(workPtr);

                        int harmonicLimit = 1024 >> lvl;
                        if (harmonicLimit < 2) harmonicLimit = 2;
                        int transitionStart = std::max(1, (int)(harmonicLimit * 0.8f));

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
                            int dstIdx = f * 2048 + i;
                            if (dstIdx < newSet->totalSamples) {
                                destPtr[dstIdx] = juce::jlimit(-1.0f, 1.0f, workPtr[i] * normalizeScale);
                            }
                        }
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
        int f0_base = frameIdx * set->frameSize;
        int f1_base = std::min(frameIdx + 1, set->numFrames - 1) * set->frameSize;

        for (int i = 0; i < 512; ++i) {
            float phase = i / 512.0f;
            float mod = 0.0f;

            if (fmWaveform == 0) mod = std::sin(phase * 2.0f * juce::MathConstants<float>::pi);
            else if (fmWaveform == 1) mod = phase * 2.0f - 1.0f;
            else if (fmWaveform == 2) mod = phase < 0.5f ? 1.0f : -1.0f;
            else if (fmWaveform == 3) mod = 4.0f * std::abs(phase - 0.5f) - 1.0f;

            float currentPhase = std::fmod(phase + mod * fmAmount * 0.1f, 1.0f);
            if (currentPhase < 0.0f) currentPhase += 1.0f;

            // Time Phase Morphing (0 to 7)
            currentPhase = applyPhaseWarp(currentPhase, morphAMode, morphAAmount);
            currentPhase = applyPhaseWarp(currentPhase, morphBMode, morphBAmount);

            float floatPos = currentPhase * set->frameSize;
            int pos = (int)floatPos;
            if (pos >= set->frameSize) pos -= set->frameSize;
            float frac = floatPos - (float)pos;
            float val0 = getHermiteSample(ptr, f0_base, set->frameSize, pos, frac);
            float val1 = getHermiteSample(ptr, f1_base, set->frameSize, pos, frac);
            float val = val0 * (1.0f - frameFrac) + val1 * frameFrac;

            // Time Amp Morphing (0 to 7)
            val = applyAmpWarp(val, morphAMode, morphAAmount);
            val = applyAmpWarp(val, morphBMode, morphBAmount);

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

    // Dual Morph System
    void setMorphA(int mode, float amt) { morphAMode = mode; morphAAmount = amt; }
    void setMorphB(int mode, float amt) { morphBMode = mode; morphBAmount = amt; }

    void setWavetableLevel(float level) { wtLevel = level; }
    void setWavetablePitchOffset(float semitones) { wtPitchOffset = semitones; }
    void setPitchDecay(float amount, float timeMs) {
        pitchDecayAmt = amount;
        if (timeMs <= 0.001f) pitchDecayCoef = 0.0f;
        else pitchDecayCoef = std::exp(-std::log(1000.0f) / (timeMs * 0.001f * (float)sampleRate));
    }

    void setSubOn(bool on) { subOn = on; }
    void setSubWaveform(int shape) { subWaveform = juce::jlimit(0, 3, shape); }
    void setSubVolume(float vol) { subVolume = vol; }
    void setSubPitchOffset(float semitones) { subPitchOffset = semitones; }

    void resetPhase() {
        auto& random = juce::Random::getSystemRandom();
        for (int b = 0; b < MaxBlocks; ++b) {
            SIMDFloat initialPhase;
            for (int i = 0; i < SimdWidth; ++i) initialPhase.set(i, random.nextFloat());
            phases[b] = initialPhase;
        }
        subPhase = 0.0f;
        pitchEnvState = 1.0f;
        for (int i = 0; i < MaxVoices; ++i) driftPhase[i] = random.nextFloat();
    }

    void getSampleStereo(float& outL, float& outR) {
        outL = 0.0f; outR = 0.0f;
        WavetableSet::Ptr set = currentWavetableSet.get();
        if (set == nullptr || set->totalSamples == 0) return;

        pitchEnvState *= pitchDecayCoef;
        if (pitchEnvState < 0.0001f) pitchEnvState = 0.0f;

        float currentWtPitch = wtPitchOffset + (pitchDecayAmt * pitchEnvState);
        float dynamicPitchMult = std::pow(2.0f, currentWtPitch / 12.0f);

        SIMDFloat fmScaled(fmAmount * 0.1f);
        float framePos = wtPosition * (float)std::max(0, set->numFrames - 1);
        int frameIdx = (int)framePos;
        float frameFrac = framePos - (float)frameIdx;
        int f0_base = frameIdx * set->frameSize;
        int f1_base = std::min(frameIdx + 1, set->numFrames - 1) * set->frameSize;

        SIMDFloat outL_simd(0.0f), outR_simd(0.0f);
        int numBlocks = (unisonCount + SimdWidth - 1) / SimdWidth;
        SIMDFloat half(0.5f), c1(6.2831853f), c2(41.341702f), c3(81.605249f), negOne(-1.0f);

        for (int b = 0; b < numBlocks; ++b) {
            SIMDFloat p = phases[b];
            SIMDFloat modSignal(0.0f);

            if (fmWaveform == 0) {
                SIMDFloat z = p - half;
                modSignal = (z * c1 - (z * z * z) * c2 + (z * z * z * z * z) * c3) * negOne;
            }
            else if (fmWaveform == 1) { modSignal = p * 2.0f - 1.0f; }
            else if (fmWaveform == 2) { for (int i = 0; i < SimdWidth; ++i) modSignal.set(i, p.get(i) < 0.5f ? 1.0f : -1.0f); }
            else if (fmWaveform == 3) { for (int i = 0; i < SimdWidth; ++i) modSignal.set(i, 4.0f * std::abs(p.get(i) - 0.5f) - 1.0f); }

            SIMDFloat totalPhase = p + modSignal * fmScaled;
            SIMDFloat vAmp(0.0f), nextP(0.0f);

            for (int i = 0; i < SimdWidth; ++i) {
                int voiceIdx = b * SimdWidth + i;
                if (voiceIdx >= unisonCount) {
                    vAmp.set(i, 0.0f); nextP.set(i, p.get(i)); continue;
                }
                driftPhase[voiceIdx] += driftRate[voiceIdx] / (float)sampleRate;
                if (driftPhase[voiceIdx] >= 1.0f) driftPhase[voiceIdx] -= 1.0f;
                float currentDriftMod = std::sin(driftPhase[voiceIdx] * juce::MathConstants<float>::twoPi) * driftAmount * 0.01f;

                float step = increments[b].get(i) * dynamicPitchMult * (1.0f + currentDriftMod);
                float voiceFreq = std::max(1.0f, step * (float)sampleRate);

                float maxH = (float)sampleRate / (2.0f * voiceFreq);
                int level0 = 0; float hLimit = 1024.0f;
                while (level0 < NumLevels - 2 && (hLimit * 0.5f) > maxH) { level0++; hLimit *= 0.5f; }
                float levelFrac = 0.0f; if (hLimit > maxH) {
                    levelFrac = juce::jlimit(0.0f, 1.0f, (hLimit - maxH) / (hLimit * 0.5f));
                }
                int level1 = level0 + 1;
                const float* ptr0 = set->levels[level0].getReadPointer(0);
                const float* ptr1 = set->levels[level1].getReadPointer(0);

                float tp = totalPhase.get(i);
                tp -= std::floor(tp); if (tp < 0.0f) tp += 1.0f;

                // --- 1. Phase Warping (Morph A -> Morph B) ---
                tp = applyPhaseWarp(tp, morphAMode, morphAAmount);
                tp = applyPhaseWarp(tp, morphBMode, morphBAmount);

                float floatPos = tp * set->frameSize; int pos = (int)floatPos;
                if (pos >= set->frameSize) pos -= set->frameSize; float frac = floatPos - (float)pos;

                float val0 = getHermiteSample(ptr0, f0_base, set->frameSize, pos, frac) * (1.0f - frameFrac) + getHermiteSample(ptr0, f1_base, set->frameSize, pos, frac) * frameFrac;
                float val1 = getHermiteSample(ptr1, f0_base, set->frameSize, pos, frac) * (1.0f - frameFrac) + getHermiteSample(ptr1, f1_base, set->frameSize, pos, frac) * frameFrac;
                float val = val0 * (1.0f - levelFrac) + val1 * levelFrac;

                // --- 2. Amplitude Shaping (Morph A -> Morph B) ---
                val = applyAmpWarp(val, morphAMode, morphAAmount);
                val = applyAmpWarp(val, morphBMode, morphBAmount);

                vAmp.set(i, val * amp[b].get(i));
                float phaseInc = p.get(i) + step; if (phaseInc >= 1.0f) phaseInc -= 1.0f; nextP.set(i, phaseInc);
            }
            outL_simd += vAmp * panL[b]; outR_simd += vAmp * panR[b]; phases[b] = nextP;
        }

        for (int i = 0; i < SimdWidth; ++i) {
            outL += outL_simd.get(i) * wtLevel;
            outR += outR_simd.get(i) * wtLevel;
        }

        if (subOn && subVolume > 0.001f) {
            float subFreq = std::max(1.0f, baseFreq * std::pow(2.0f, subPitchOffset / 12.0f));
            subPhase += subFreq / (float)sampleRate;
            if (subPhase >= 1.0f) subPhase -= 1.0f;
            float rawSub = 0.0f;
            switch (subWaveform) {
            case 0: rawSub = std::sin(subPhase * juce::MathConstants<float>::twoPi); break;
            case 1: rawSub = 4.0f * std::abs(subPhase - 0.5f) - 1.0f; break;
            case 2: rawSub = subPhase < 0.5f ? 1.0f : -1.0f; break;
            case 3: rawSub = 2.0f * subPhase - 1.0f; break;
            }
            float alpha = juce::MathConstants<float>::twoPi * 250.0f / (float)sampleRate;
            subLpfState += alpha * (rawSub - subLpfState);
            float s = subLpfState * subVolume; outL += s; outR += s;
        }
    }

private:
    double sampleRate = 44100.0;
    float baseFreq = 440.0f;
    float detuneAmount = 0.0f, stereoWidth = 1.0f, wtPosition = 0.0f, fmAmount = 0.0f, driftAmount = 0.0f;
    int unisonCount = 1;
    int fmWaveform = 0;

    int morphAMode = 0; float morphAAmount = 0.0f;
    int morphBMode = 0; float morphBAmount = 0.0f;

    float wtLevel = 1.0f, wtPitchOffset = 0.0f, pitchDecayAmt = 0.0f, pitchDecayCoef = 0.0f, pitchEnvState = 0.0f;
    bool subOn = true; int subWaveform = 0; float subVolume = 0.0f, subPitchOffset = -12.0f, subPhase = 0.0f, subLpfState = 0.0f;
    std::array<float, MaxVoices> driftPhase = { 0 }, driftRate = { 0 };
    juce::AudioFormatManager formatManager;
    juce::ThreadPool backgroundPool{ 1 };
    std::atomic<int> loadJobId{ 0 };
    juce::ReferenceCountedObjectPtr<WavetableSet> currentWavetableSet;
    std::array<SIMDFloat, MaxBlocks> phases, increments, panL, panR, amp;

    // --- Time Domain Morphing Algorithms (0 to 7) ---
    inline float applyPhaseWarp(float p, int mode, float amt) const {
        if (mode == 1) return std::pow(p, std::exp(-amt * 2.0f)); // Bend (+/-)
        if (mode == 2) { // PWM
            float pw = 0.01f + (amt * 0.5f + 0.5f) * 0.98f;
            return (p < pw) ? (p / pw * 0.5f) : (0.5f + (p - pw) / (1.0f - pw) * 0.5f);
        }
        if (mode == 3) return std::fmod(p * (1.0f + std::abs(amt) * 7.0f), 1.0f); // Sync
        if (mode == 4) { // Mirror
            float mirrored = (p < 0.5f) ? p * 2.0f : (1.0f - p) * 2.0f;
            if (amt < 0.0f) mirrored = (p > 0.5f) ? (p - 0.5f) * 2.0f : (0.5f - p) * 2.0f;
            return p * (1.0f - std::abs(amt)) + mirrored * std::abs(amt);
        }
        return p;
    }

    inline float applyAmpWarp(float v, int mode, float amt) const {
        if (mode == 5) { // Flip
            float flipped = amt > 0.0f ? std::abs(v) : -std::abs(v);
            return v * (1.0f - std::abs(amt)) + flipped * std::abs(amt);
        }
        if (mode == 6) { // Quantize
            float steps = std::pow(2.0f, 2.0f + (1.0f - std::abs(amt)) * 14.0f);
            return std::round(v * steps) / steps;
        }
        if (mode == 7) { // Remap (Saturation/Fold)
            return amt >= 0.0f ? std::tanh(v * (1.0f + amt * 4.0f)) : std::sin(v * (1.0f + std::abs(amt) * 3.0f) * juce::MathConstants<float>::halfPi);
        }
        return v;
    }

    static inline float hermite(float t, float y0, float y1, float y2, float y3) {
        float c0 = y1; float c1 = 0.5f * (y2 - y0); float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3; float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return c0 + t * (c1 + t * (c2 + t * c3));
    }
    inline float getHermiteSample(const float* ptr, int baseOffset, int fs, int pos, float t) const {
        int p0 = pos - 1; if (p0 < 0) p0 += fs;
        int p1 = pos;
        int p2 = pos + 1; if (p2 >= fs) p2 -= fs;
        int p3 = pos + 2; if (p3 >= fs) p3 -= fs;
        return hermite(t, ptr[baseOffset + p0], ptr[baseOffset + p1], ptr[baseOffset + p2], ptr[baseOffset + p3]);
    }

    void recalculate() {
        float norm = 1.0f / std::sqrt((float)unisonCount);
        float centerAngle = 0.25f * juce::MathConstants<float>::pi;

        for (int b = 0; b < MaxBlocks; ++b) {
            SIMDFloat pL(0.0f), pR(0.0f), a(0.0f), inc(0.0f);
            for (int i = 0; i < SimdWidth; ++i) {
                int voiceIdx = b * SimdWidth + i;
                if (voiceIdx < unisonCount) {
                    float bias = (unisonCount == 1) ? 0.0f : (2.0f * voiceIdx / (float)(unisonCount - 1) - 1.0f);
                    float pitchBias = bias * bias * bias;
                    float step = (float)((baseFreq * std::pow(2.0f, (pitchBias * detuneAmount * 0.5f) / 12.0f)) / sampleRate);

                    float targetAngle = (bias + 1.0f) * centerAngle;
                    float actualWidth = stereoWidth;
                    if (baseFreq < 150.0f) actualWidth *= juce::jlimit(0.0f, 1.0f, baseFreq / 150.0f);
                    float angle = centerAngle + (targetAngle - centerAngle) * actualWidth;

                    pL.set(i, std::cos(angle)); pR.set(i, std::sin(angle)); a.set(i, norm); inc.set(i, step);
                }
            }
            panL[b] = pL; panR[b] = pR; amp[b] = a; increments[b] = inc;
        }
    }
};