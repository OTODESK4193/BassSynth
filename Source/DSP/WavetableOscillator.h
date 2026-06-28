// ==============================================================================
// Source/DSP/WavetableOscillator.h
// ==============================================================================
#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>
#include <complex>
#include <atomic>
#include <cmath>

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
        for (int i = 0; i < MaxVoices; ++i) driftRate[i] = 0.1f + random.nextFloat() * 0.4f;

        loadFactoryWavetable(0);
        resetPhase();
    }

    ~WavetableOscillator() {
        loadJobId++;
        backgroundPool.removeAllJobs(true, 1000); // ★ 修正
    }

    void prepare(double sr) { sampleRate = std::max(1.0, sr); lastCalcFreq = -1.0f; } // ★候補B: SR変更時に再計算を強制

    void loadFactoryWavetable(int index) {
        const int myJobId = ++loadJobId;
        WavetableSet::Ptr newSet = new WavetableSet();
        int numFrames = 64;
        newSet->totalSamples = 2048 * numFrames;
        newSet->frameSize = 2048;
        newSet->numFrames = numFrames;

        juce::AudioBuffer<float> tempRaw(1, newSet->totalSamples);
        auto* w = tempRaw.getWritePointer(0);

        for (int f = 0; f < numFrames; ++f) {
            float posMod = f / (float)(numFrames - 1);
            for (int i = 0; i < 2048; ++i) {
                float p = i / 2048.0f;
                float val = 0.0f;
                switch (index) {
                case 0:
                    if (posMod < 0.333f) {
                        float mix = posMod * 3.0f;
                        val = std::sin(p * 6.2831853f) * (1.0f - mix) + (4.0f * std::abs(p - 0.5f) - 1.0f) * mix;
                    }
                    else if (posMod < 0.666f) {
                        float mix = (posMod - 0.333f) * 3.0f;
                        val = (4.0f * std::abs(p - 0.5f) - 1.0f) * (1.0f - mix) + (1.0f - 2.0f * p) * mix;
                    }
                    else {
                        float mix = (posMod - 0.666f) * 3.0f;
                        val = (1.0f - 2.0f * p) * (1.0f - mix) + (p < 0.5f ? 1.0f : -1.0f) * mix;
                    }
                    break;
                case 1:
                    val = p < (0.5f - posMod * 0.45f) ? 1.0f : -1.0f;
                    break;
                case 2:
                    val = std::sin(std::fmod(p * (1.0f + posMod * 7.0f), 1.0f) * 6.2831853f) * (1.0f - p);
                    break;
                case 3:
                    for (int h = 1; h <= 1 + (int)(posMod * 15.0f); ++h) val += std::sin(p * h * 6.2831853f) / (float)h;
                    break;
                case 4:
                    val = std::sin(p * 6.2831853f + std::sin(p * 6.2831853f) * (posMod * 5.0f));
                    break;
                case 5:
                    val = 1.0f - 2.0f * std::fmod(p * (1.0f + posMod * 3.0f), 1.0f);
                    break;
                case 6:
                    for (int h = 1; h <= 12; ++h) {
                        float amp = std::exp(-std::pow(std::abs((float)h - (2.0f + posMod * 10.0f)), 2.0f) * 0.5f);
                        val += std::sin(p * h * 6.2831853f) * amp;
                    }
                    break;
                case 7:
                    val = std::sin(p * 6.2831853f) + posMod * std::sin(p * 3.14159265f);
                    break;
                case 8:
                    val = std::sin(p * 6.2831853f) + posMod * std::sin(p * 17.34f) + posMod * posMod * std::sin(p * 31.12f);
                    break;
                case 9:
                    val = std::sin(p * 6.2831853f) * (1.0f - posMod) + (juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f) * posMod;
                    break;
                }
                w[f * 2048 + i] = val;
            }
            float peak = 1e-9f;
            for (int i = 0; i < 2048; ++i) peak = std::max(peak, std::abs(w[f * 2048 + i]));
            for (int i = 0; i < 2048; ++i) w[f * 2048 + i] /= peak;
        }

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
        backgroundPool.removeAllJobs(true, 10); // ★ 古い処理の確実なキャンセル
        backgroundPool.addJob([this, myJobId, newSet, tempRaw]() { // ★ 専用プールを利用
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
            tp -= std::floor(tp);

            float syncA = 1.0f, syncB = 1.0f, syncC = 1.0f;
            float fadeWidth = juce::jlimit(0.001f, 0.03f, baseFreq * 0.00001f);
            tp = applyPhaseWarp(tp, morphAMode, warpA, syncA, originalPhase, fadeWidth);
            tp = applyPhaseWarp(tp, morphBMode, warpB, syncB, originalPhase, fadeWidth);
            tp = applyPhaseWarp(tp, morphCMode, warpC, syncC, originalPhase, fadeWidth);

            tp -= std::floor(tp);
            if (!std::isfinite(tp)) tp = 0.0f;

            float floatPos = tp * set->frameSize;
            int pos = (int)floatPos;
            float frac = floatPos - (float)pos;
            float val = (getHermiteSample(ptr, f0_base, set->frameSize, pos, frac) * (1.0f - frameFrac) +
                getHermiteSample(ptr, f1_base, set->frameSize, pos, frac) * frameFrac);
            val *= (syncA * syncB * syncC);
            val = applyAmpWarp(val, morphAMode, warpA, originalPhase);
            val = applyAmpWarp(val, morphBMode, warpB, originalPhase);
            val = applyAmpWarp(val, morphCMode, warpC, originalPhase);
            displayBuffer[i] = val;
        }
    }

    // ★候補B: 周波数が前回と同じならパン/増分テーブルの再計算をスキップ（グライド中以外で効く。結果は同一）
    void setFrequency(float freqHz) { if (freqHz == lastCalcFreq) return; baseFreq = freqHz; lastCalcFreq = freqHz; recalculate(); }
    void setUnisonCount(int c) { unisonCount = juce::jlimit(1, MaxVoices, c); recalculate(); }
    void setUnisonDetune(float d) { detuneAmount = d; recalculate(); }
    void setStereoWidth(float w) { stereoWidth = juce::jlimit(0.0f, 1.0f, w); recalculate(); }
    void setWavetablePosition(float pos) { wtPosition = pos; }
    float getWavetablePosition() const { return wtPosition; }
    WavetableSet::Ptr getWavetableSet() const { return currentWavetableSet; }
    void setWavetableSet(WavetableSet::Ptr newSet) { currentWavetableSet = newSet; }
    float getMorphAAmount() const { return morphAAmount; }
    float getMorphAShift() const { return morphAShift; }
    float getMorphBAmount() const { return morphBAmount; }
    float getMorphBShift() const { return morphBShift; }
    float getMorphCAmount() const { return morphCAmount; }
    float getMorphCShift() const { return morphCShift; }
    float getFMAmount() const { return fmAmount; }
    void setFMAmount(float amt) { fmAmount = amt; }
    void setFMWaveform(int shape) { fmWaveform = juce::jlimit(0, 3, shape); }
    void setDriftAmount(float amt) { driftAmount = amt; }
    // ★候補A: 値が変化したときだけ、しかも該当系統のみ前計算する（出力は従来と完全に同一）
    void setMorphA(int mode, float amt, float shift) {
        if (mode == morphAMode && amt == morphAAmount && shift == morphAShift) return;
        morphAMode = mode; morphAAmount = amt; morphAShift = shift;
        precomputeWarp(warpA, morphAMode, morphAAmount, morphAShift);
    }
    void setMorphB(int mode, float amt, float shift) {
        if (mode == morphBMode && amt == morphBAmount && shift == morphBShift) return;
        morphBMode = mode; morphBAmount = amt; morphBShift = shift;
        precomputeWarp(warpB, morphBMode, morphBAmount, morphBShift);
    }
    void setMorphC(int mode, float amt, float shift) {
        if (mode == morphCMode && amt == morphCAmount && shift == morphCShift) return;
        morphCMode = mode; morphCAmount = amt; morphCShift = shift;
        precomputeWarp(warpC, morphCMode, morphCAmount, morphCShift);
    }
    void setOscOn(bool on) { oscOn = on; }
    void setWavetableLevel(float level) { wtLevel = level; }
    void setWavetablePitchOffset(float semitones) { wtPitchOffset = semitones; }
    void setPitchDecay(float amount, float timeMs) { pitchDecayAmt = amount; pitchDecayCoef = (timeMs <= 0.001f) ? 0.0f : std::exp(-6.9077f / (timeMs * 0.001f * (float)sampleRate)); }
    void setSubOn(bool on) { subOn = on; }
    void setSubWaveform(int shape) { subWaveform = juce::jlimit(0, 3, shape); }
    void setSubVolume(float vol) { subVolume = vol; }
    void setSubPitchOffset(float semitones) { subPitchOffset = semitones; }

    void resetPhase() {
        auto& random = juce::Random::getSystemRandom();
        for (int b = 0; b < MaxBlocks; ++b) {
            for (int i = 0; i < SimdWidth; ++i) {
                int vIdx = b * SimdWidth + i;
                if (vIdx == 0) {
                    // センターボイスは常に位相0.0から開始し、安定した強力な打撃感（アタック）を確保
                    phases[b].set(i, 0.0f);
                }
                else {
                    // ステレオ広がり用のデチューンボイスのみ初期位相をランダム化し、コーラス感と広がりを確保
                    phases[b].set(i, random.nextFloat());
                }
            }
        }
        subPhase = 0.0f; pitchEnvState = 1.0f;
        for (int i = 0; i < MaxVoices; ++i) driftPhase[i] = random.nextFloat();
    }

    void getSampleStereo(float& outL, float& outR, float& subL, float& subR) {
        sampleCounter++;
        outL = 0.0f; outR = 0.0f; subL = 0.0f; subR = 0.0f;
        WavetableSet::Ptr set = currentWavetableSet.get();
        if (set == nullptr || set->totalSamples == 0) return;

        pitchEnvState = (pitchEnvState < 0.0001f) ? 0.0f : pitchEnvState * pitchDecayCoef;
        float dynamicPitchMult = std::exp2((wtPitchOffset + (pitchDecayAmt * pitchEnvState)) * 0.083333f); // ★候補C: pow(2,x)→exp2 (同値・高速)

        float framePos = wtPosition * (float)std::max(0, set->numFrames - 1);
        int frameIdx = (int)framePos;
        float frameFrac = framePos - (float)frameIdx;

        size_t f0_base = static_cast<size_t>(frameIdx) * static_cast<size_t>(set->frameSize);
        size_t f1_base = static_cast<size_t>(std::min(frameIdx + 1, set->numFrames - 1)) * static_cast<size_t>(set->frameSize);

        SIMDFloat outL_simd(0.0f), outR_simd(0.0f);
        int numBlocks = (unisonCount + SimdWidth - 1) / SimdWidth;

        float fadeWidth = juce::jlimit(0.001f, 0.03f, baseFreq * 0.00001f);
        bool hasMorph = (morphAMode != 0 || morphBMode != 0 || morphCMode != 0);

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

            if (hasMorph) {
                for (int i = 0; i < SimdWidth; ++i) {
                    int vIdx = b * SimdWidth + i;
                    if (vIdx >= unisonCount) { nextP.set(i, p.get(i)); continue; }

                    float originalPhase = p.get(i);
                    if ((sampleCounter & 31) == 0) {
                        driftPhase[vIdx] += (driftRate[vIdx] / (float)sampleRate) * 32.0f;
                        if (driftPhase[vIdx] >= 1.0f) driftPhase[vIdx] -= std::floor(driftPhase[vIdx]);
                        { float dz = driftPhase[vIdx] - 0.5f; // ★候補C: sinをFM変調器と同じ多項式近似に統一(低速ドリフトのため差は不可聴)
                          currentDrift[vIdx] = -(dz * 6.2831853f - (dz * dz * dz) * 41.341702f + (dz * dz * dz * dz * dz) * 81.605249f) * driftAmount * 0.01f; }
                    }
                    float step = increments[b].get(i) * dynamicPitchMult * (1.0f + currentDrift[vIdx]);
                    float maxH = (float)sampleRate / (2.0f * std::max(1.0f, step * (float)sampleRate));
                    int lvl0 = 0; float hL = 1024.0f;
                    while (lvl0 < NumLevels - 2 && (hL * 0.5f) > maxH) { lvl0++; hL *= 0.5f; }
                    float lvlFrac = (hL > maxH) ? juce::jlimit(0.0f, 1.0f, (hL - maxH) / (hL * 0.5f)) : 0.0f;

                    float tp = tp_simd.get(i);
                    tp -= std::floor(tp);

                    float sA = 1.0f, sB = 1.0f, sC = 1.0f;
                    tp = applyPhaseWarp(tp, morphAMode, warpA, sA, originalPhase, fadeWidth);
                    tp = applyPhaseWarp(tp, morphBMode, warpB, sB, originalPhase, fadeWidth);
                    tp = applyPhaseWarp(tp, morphCMode, warpC, sC, originalPhase, fadeWidth);

                    tp -= std::floor(tp);
                    if (!std::isfinite(tp)) tp = 0.0f;

                    float fPos = tp * set->frameSize; int pos = (int)fPos; float frac = fPos - (float)pos;
                    auto* p0 = set->levels[lvl0].getReadPointer(0); auto* p1 = set->levels[lvl0 + 1].getReadPointer(0);
                    float v0 = getHermiteSample(p0, f0_base, set->frameSize, pos, frac) * (1.0f - frameFrac) + getHermiteSample(p0, f1_base, set->frameSize, pos, frac) * frameFrac;
                    float v1 = getHermiteSample(p1, f0_base, set->frameSize, pos, frac) * (1.0f - frameFrac) + getHermiteSample(p1, f1_base, set->frameSize, pos, frac) * frameFrac;
                    float val = (v0 * (1.0f - lvlFrac) + v1 * lvlFrac) * (sA * sB * sC);

                    val = applyAmpWarp(val, morphAMode, warpA, originalPhase);
                    val = applyAmpWarp(val, morphBMode, warpB, originalPhase);
                    val = applyAmpWarp(val, morphCMode, warpC, originalPhase);

                    vAmp.set(i, oscOn ? val * amp[b].get(i) : 0.0f);
                    float pNext = p.get(i) + step;
                    pNext -= std::floor(pNext);
                    nextP.set(i, pNext);
                }
            } else {
                for (int i = 0; i < SimdWidth; ++i) {
                    int vIdx = b * SimdWidth + i;
                    if (vIdx >= unisonCount) { nextP.set(i, p.get(i)); continue; }

                    if ((sampleCounter & 31) == 0) {
                        driftPhase[vIdx] += (driftRate[vIdx] / (float)sampleRate) * 32.0f;
                        if (driftPhase[vIdx] >= 1.0f) driftPhase[vIdx] -= std::floor(driftPhase[vIdx]);
                        { float dz = driftPhase[vIdx] - 0.5f; // ★候補C: sinをFM変調器と同じ多項式近似に統一(低速ドリフトのため差は不可聴)
                          currentDrift[vIdx] = -(dz * 6.2831853f - (dz * dz * dz) * 41.341702f + (dz * dz * dz * dz * dz) * 81.605249f) * driftAmount * 0.01f; }
                    }
                    float step = increments[b].get(i) * dynamicPitchMult * (1.0f + currentDrift[vIdx]);
                    float maxH = (float)sampleRate / (2.0f * std::max(1.0f, step * (float)sampleRate));
                    int lvl0 = 0; float hL = 1024.0f;
                    while (lvl0 < NumLevels - 2 && (hL * 0.5f) > maxH) { lvl0++; hL *= 0.5f; }
                    float lvlFrac = (hL > maxH) ? juce::jlimit(0.0f, 1.0f, (hL - maxH) / (hL * 0.5f)) : 0.0f;

                    float tp = tp_simd.get(i);
                    tp -= std::floor(tp);
                    if (!std::isfinite(tp)) tp = 0.0f;

                    float fPos = tp * set->frameSize; int pos = (int)fPos; float frac = fPos - (float)pos;
                    auto* p0 = set->levels[lvl0].getReadPointer(0); auto* p1 = set->levels[lvl0 + 1].getReadPointer(0);
                    float v0 = getHermiteSample(p0, f0_base, set->frameSize, pos, frac) * (1.0f - frameFrac) + getHermiteSample(p0, f1_base, set->frameSize, pos, frac) * frameFrac;
                    float v1 = getHermiteSample(p1, f0_base, set->frameSize, pos, frac) * (1.0f - frameFrac) + getHermiteSample(p1, f1_base, set->frameSize, pos, frac) * frameFrac;
                    float val = v0 * (1.0f - lvlFrac) + v1 * lvlFrac;

                    vAmp.set(i, oscOn ? val * amp[b].get(i) : 0.0f);
                    float pNext = p.get(i) + step;
                    pNext -= std::floor(pNext);
                    nextP.set(i, pNext);
                }
            }
            outL_simd += vAmp * panL[b]; outR_simd += vAmp * panR[b]; phases[b] = nextP;
        }

        float finalL = 0.0f, finalR = 0.0f;
        for (int i = 0; i < SimdWidth; ++i) { 
            finalL += outL_simd.get(i) * wtLevel; 
            finalR += outR_simd.get(i) * wtLevel; 
        }
        
        float nextL = finalL - dcFilterL_x1 + 0.999f * dcFilterL_y1;
        dcFilterL_x1 = finalL; dcFilterL_y1 = nextL;
        
        float nextR = finalR - dcFilterR_x1 + 0.999f * dcFilterR_y1;
        dcFilterR_x1 = finalR; dcFilterR_y1 = nextR;
        
        outL = nextL; outR = nextR;

        if (subOn && subVolume > 0.001f) {
            subPhase += (baseFreq * std::exp2(subPitchOffset * 0.083333f)) / (float)sampleRate; // ★候補C: pow(2,x)→exp2
            subPhase -= std::floor(subPhase);
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
    float lastCalcFreq = -1.0f; // ★候補B: recalculate済みの周波数を記録（センチネル -1 で初回/SR変更時に必ず計算）
    int unisonCount = 1, fmWaveform = 0;
    bool oscOn = true, subOn = true;
    int morphAMode = 0, morphBMode = 0, morphCMode = 0;
    float morphAAmount = 0.0f, morphBAmount = 0.0f, morphCAmount = 0.0f;
    float morphAShift = 0.0f, morphBShift = 0.0f, morphCShift = 0.0f;
    float wtLevel = 1.0f, wtPitchOffset = 0.0f, pitchDecayAmt = 0.0f, pitchDecayCoef = 0.0f, pitchEnvState = 0.0f;
    int subWaveform = 0; float subVolume = 0.0f, subPitchOffset = -12.0f, subPhase = 0.0f, subLpfState = 0.0f;
    std::array<float, MaxVoices> driftPhase = { 0 }, driftRate = { 0 };
    std::array<float, MaxVoices> currentDrift = { 0 };
    int sampleCounter = 0;
    struct WarpPrecomputed {
        float bend_b = 1.0f;
        float bend_sym = 0.5f;
        float sync_c = 0.5f;
        float sync_pw = 0.5f;
        float sync_pw_recip = 2.0f;
        float sync_pw_inv_recip = 2.0f;
        float asym_pt = 0.5f;
        float asym_m_mul1 = 1.0f;
        float asym_m_mul2 = 1.0f;
        float asym_abs_amt = 0.0f;
        float asym_one_minus_amt = 1.0f;
        bool asym_amt_neg = false;
        
        float fold_shift_d = 0.05f;
        float fold_shift_sub_d = -0.05f;
        float fold_d_mul = 10.0f;
        float fold_d_div = -0.1f;
        float fold_abs_amt = 0.0f;
        float fold_one_minus_amt = 1.0f;
        
        float quant_st = 4.0f;
        float quant_st_recip = 0.25f;
        float quant_abs_amt = 0.0f;
        float quant_one_minus_amt = 1.0f;
        
        float sat_dr_mul = 1.0f;
        float sat_shift_half = 0.0f;
        bool sat_amt_pos = true;
    };
    
    WarpPrecomputed warpA, warpB, warpC;

    float dcFilterL_x1 = 0.0f, dcFilterL_y1 = 0.0f;
    float dcFilterR_x1 = 0.0f, dcFilterR_y1 = 0.0f;

    juce::AudioFormatManager formatManager;

    juce::ThreadPool backgroundPool{ 1 }; // ★ 修正: 共有プールから各インスタンス専用スレッドに変更

    std::atomic<int> loadJobId{ 0 };
    juce::ReferenceCountedObjectPtr<WavetableSet> currentWavetableSet;
    std::array<SIMDFloat, MaxBlocks> phases, increments, panL, panR, amp;

    void precomputeWarp(WarpPrecomputed& w, int mode, float amt, float shift) {
        if (mode == 0) return;
        if (mode == 1) {
            w.bend_sym = std::clamp(0.5f + shift * 0.49f, 0.01f, 0.99f);
            w.bend_b = std::exp(-std::clamp(amt, -0.99f, 0.99f) * 2.0f);
        }
        else if (mode == 2) {
            w.sync_c = std::clamp(0.5f + shift * 0.4f, 0.1f, 0.9f);
            w.sync_pw = std::clamp(0.01f + (amt * 0.5f + 0.5f) * 0.98f, 0.01f, 0.99f);
            w.sync_pw_recip = 0.5f / w.sync_pw;
            w.sync_pw_inv_recip = 0.5f / (1.0f - w.sync_pw);
        }
        else if (mode == 3) {
            w.quant_st = 1.0f + std::abs(amt) * 7.0f;
        }
        else if (mode == 4) {
            w.asym_pt = std::clamp(0.5f + shift * 0.4f, 0.1f, 0.9f);
            w.asym_m_mul1 = 0.5f / w.asym_pt;
            w.asym_m_mul2 = 0.5f / (1.0f - w.asym_pt);
            w.asym_abs_amt = std::abs(amt);
            w.asym_one_minus_amt = 1.0f - w.asym_abs_amt;
            w.asym_amt_neg = (amt < 0.0f);
        }
        else if (mode == 5) {
            float d = 0.05f;
            w.fold_shift_d = shift + d;
            w.fold_shift_sub_d = shift - d;
            w.fold_d_mul = 1.0f / (2.0f * d);
            w.fold_d_div = -2.0f * d;
            w.fold_abs_amt = std::abs(amt);
            w.fold_one_minus_amt = 1.0f - w.fold_abs_amt;
        }
        else if (mode == 6) {
            w.quant_st = std::pow(2.0f, 2.0f + (1.0f - std::abs(amt)) * 14.0f);
            w.quant_st_recip = 1.0f / w.quant_st;
            w.quant_abs_amt = std::abs(amt);
            w.quant_one_minus_amt = 1.0f - w.quant_abs_amt;
        }
        else if (mode == 7) {
            w.sat_dr_mul = 1.0f + std::abs(amt) * 4.0f;
            w.sat_shift_half = shift * 0.5f;
            w.sat_amt_pos = (amt >= 0.0f);
        }
    }

    inline float applyPhaseWarp(float p, int mode, const WarpPrecomputed& w, float& sOut, float originalPhase, float fadeWidth) const {
        if (mode == 0) return p;

        float warped = p;
        if (mode == 1) {
            if (p < w.bend_sym) warped = w.bend_sym * std::pow(p / w.bend_sym, w.bend_b);
            else warped = w.bend_sym + (1.0f - w.bend_sym) * (1.0f - std::pow((1.0f - p) / (1.0f - w.bend_sym), w.bend_b));
        }
        else if (mode == 2) {
            float ps = p + (0.5f - w.sync_c);
            if (ps >= 1.0f) ps -= 1.0f; else if (ps < 0.0f) ps += 1.0f;
            float wVal = (ps < w.sync_pw) ? (ps * w.sync_pw_recip) : (0.5f + (ps - w.sync_pw) * w.sync_pw_inv_recip);
            warped = wVal + (w.sync_c - 0.5f);
            if (warped >= 1.0f) warped -= 1.0f; else if (warped < 0.0f) warped += 1.0f;
        }
        else if (mode == 3) {
            float res = (p + w.sat_shift_half) * w.quant_st;
            res -= std::floor(res);
            if (res > 0.985f) sOut *= (1.0f - res) / 0.015f; else if (res < 0.015f) sOut *= res / 0.015f;
            warped = res - w.sat_shift_half;
            if (warped >= 1.0f) warped -= 1.0f; else if (warped < 0.0f) warped += 1.0f;
        }
        else if (mode == 4) {
            float m = (p < w.asym_pt) ? p * w.asym_m_mul1 : 1.0f - (p - w.asym_pt) * w.asym_m_mul2;
            if (w.asym_amt_neg) m = (p > w.asym_pt) ? (p - w.asym_pt) * w.asym_m_mul2 : 1.0f - p * w.asym_m_mul1;
            warped = p * w.asym_one_minus_amt + m * w.asym_abs_amt;
        }

        if (originalPhase < fadeWidth) {
            float mix = originalPhase / fadeWidth;
            return warped * mix + originalPhase * (1.0f - mix);
        }
        else if (originalPhase > 1.0f - fadeWidth) {
            float mix = (1.0f - originalPhase) / fadeWidth;
            return warped * mix + originalPhase * (1.0f - mix);
        }
        return warped;
    }

    inline float applyAmpWarp(float v, int mode, const WarpPrecomputed& w, float originalPhase) const {
        if (mode < 5 || mode > 7) return v;

        float warped = v;
        if (mode == 5) {
            float outVal = 0.0f;
            if (v > w.fold_shift_d) outVal = w.fold_shift_sub_d - (v - w.fold_shift_d);
            else if (v < w.fold_shift_sub_d) outVal = w.fold_shift_d + (w.fold_shift_sub_d - v);
            else { float t = (v - w.fold_shift_sub_d) * w.fold_d_mul; outVal = w.fold_shift_d + (t * t * (3.0f - 2.0f * t)) * w.fold_d_div; }
            warped = v * w.fold_one_minus_amt + outVal * w.fold_abs_amt;
        }
        else if (mode == 6) {
            float sc = (v + w.sat_shift_half * 2.0f) * w.quant_st, bs = std::floor(sc), fr = sc - bs, ed = 0.1f, sm = (fr > 1.0f - ed) ? (fr - (1.0f - ed)) / ed : 0.0f;
            warped = v * w.quant_one_minus_amt + ((bs + (sm * sm * (3.0f - 2.0f * sm))) * w.quant_st_recip - w.sat_shift_half * 2.0f) * w.quant_abs_amt;
        }
        else if (mode == 7) {
            float dr = (v + w.sat_shift_half) * w.sat_dr_mul;
            warped = (w.sat_amt_pos ? std::tanh(dr) : std::sin(dr * 1.570796f)) - w.sat_shift_half;
        }

        if (originalPhase < 0.005f) return warped * (originalPhase / 0.005f);
        else if (originalPhase > 0.995f) return warped * ((1.0f - originalPhase) / 0.005f);

        return warped;
    }

    static inline float hermite(float t, float y0, float y1, float y2, float y3) {
        float c1 = 0.5f * (y2 - y0), c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3, c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * t + c2) * t + c1) * t + y1;
    }

    inline float getHermiteSample(const float* ptr, size_t bOff, int fs, int pos, float t) const {
        int mask = fs - 1;
        int p0 = (pos - 1) & mask;
        int p1 = pos & mask;
        int p2 = (pos + 1) & mask;
        int p3 = (pos + 2) & mask;
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
                    inc.set(i, (float)((baseFreq * std::exp2((bias * bias * bias * detuneAmount * 0.5f) * 0.083333f)) / sampleRate)); // ★候補C: pow(2,x)→exp2
                    float ang = cAng + ((bias + 1.0f) * cAng - cAng) * (baseFreq < 150.0f ? stereoWidth * (baseFreq / 150.0f) : stereoWidth);
                    pL.set(i, std::cos(ang)); pR.set(i, std::sin(ang)); a.set(i, norm);
                }
            }
            panL[b] = pL; panR[b] = pR; amp[b] = a; increments[b] = inc;
        }
    }
};