// ==============================================================================
// Source/DSP/SpectralOscillator.h
// ==============================================================================
#pragma once
#include <JuceHeader.h>
#include <array>
#include "SpectralWavetable.h"

class SpectralVoice
{
public:
    using SIMDFloat = juce::dsp::SIMDRegister<float>;

    void process(const SpectralWavetable* wt, float framePos, int targetMipmap,
        SIMDFloat phase, SIMDFloat& outAmp)
    {
        if (wt == nullptr || wt->numFrames == 0) {
            outAmp = SIMDFloat(0.0f); return;
        }

        float fidx = framePos * (float)(wt->numFrames - 1);
        int f0 = (int)fidx;
        float ffrac = fidx - (float)f0;
        int f1 = std::min(f0 + 1, wt->numFrames - 1);

        // 各フレームから指定されたMipmapレベルを取得
        const auto& mip0 = wt->frames[(size_t)f0].mipmaps[(size_t)targetMipmap];
        const auto& mip1 = wt->frames[(size_t)f1].mipmaps[(size_t)targetMipmap];

        for (int i = 0; i < SIMDFloat::size(); ++i) {
            float sampleIdx = phase.get(i) * (float)kTableSize;
            int s0 = (int)sampleIdx;
            float sfrac = sampleIdx - (float)s0;

            // 線形補間（末尾の+1サンプルを使用してラップアラウンド）
            float samp0 = mip0[(size_t)s0] + sfrac * (mip0[(size_t)(s0 + 1)] - mip0[(size_t)s0]);
            float samp1 = mip1[(size_t)s0] + sfrac * (mip1[(size_t)(s0 + 1)] - mip1[(size_t)s0]);

            // フレーム間のクロスフェード
            outAmp.set(i, samp0 * (1.0f - ffrac) + samp1 * ffrac);
        }
    }
};

class SpectralUnisonOscillator
{
public:
    using SIMDFloat = juce::dsp::SIMDRegister<float>;
    static constexpr int MaxVoices = 12;
    static constexpr int SimdWidth = SIMDFloat::size();
    static constexpr int MaxBlocks = (MaxVoices + SimdWidth - 1) / SimdWidth;

    SpectralUnisonOscillator() {
        resetPhase();
    }

    void prepare(double sr) {
        sampleRate = (float)sr;
    }

    void setFrequency(float freqHz) { baseFreq = freqHz; recalculate(); }
    void setUnisonCount(int c) { unisonCount = juce::jlimit(1, MaxVoices, c); recalculate(); }
    void setUnisonDetune(float d) { detuneAmount = d; recalculate(); }
    void setStereoWidth(float w) { stereoWidth = juce::jlimit(0.0f, 1.0f, w); recalculate(); }
    void setWavetablePosition(float pos) { wtPosition = pos; }
    void setDriftAmount(float amt) { driftAmount = amt; }

    void setWavetableLevel(float level) { wtLevel = level; }
    void setWavetablePitchOffset(float semitones) { wtPitchOffset = semitones; }
    void setPitchDecay(float amount, float timeMs) {
        pitchDecayAmt = amount;
        pitchDecayCoef = timeMs <= 0.001f ? 0.0f : std::exp(-std::log(1000.0f) / (timeMs * 0.001f * sampleRate));
    }

    // --- Sub Oscillator Setters ---
    void setSubOn(bool on) { subOn = on; }
    void setSubWaveform(int shape) { subWaveform = juce::jlimit(0, 3, shape); }
    void setSubVolume(float vol) { subVolume = vol; }
    void setSubPitchOffset(float semitones) { subPitchOffset = semitones; }

    void resetPhase() {
        auto& random = juce::Random::getSystemRandom();
        for (int b = 0; b < MaxBlocks; ++b) {
            SIMDFloat p;
            for (int i = 0; i < SimdWidth; ++i) p.set(i, random.nextFloat());
            phases[(size_t)b] = p;
        }
        pitchEnvState = 1.0f;
        subPhase = 0.0f;

        for (int i = 0; i < MaxVoices; ++i) {
            driftPhase[(size_t)i] = random.nextFloat();
            // Drift速度をボイスごとにランダム化 (0.1Hz ～ 1.0Hz)
            driftRate[(size_t)i] = 0.1f + random.nextFloat() * 0.9f;
        }
    }

    void generateSingleCycle(std::array<float, 512>& displayBuffer, const SpectralWavetable* wt)
    {
        if (wt == nullptr || wt->frames.empty()) {
            displayBuffer.fill(0.0f);
            return;
        }

        float fidx = wtPosition * (float)(wt->numFrames - 1);
        int f0 = (int)fidx;
        float ffrac = fidx - (float)f0;
        int f1 = std::min(f0 + 1, wt->numFrames - 1);

        const auto& mip0_f0 = wt->frames[(size_t)f0].mipmaps[0];
        const auto& mip0_f1 = wt->frames[(size_t)f1].mipmaps[0];

        for (int i = 0; i < 512; ++i) {
            float phase = (float)i / 512.0f;
            float sampleIdx = phase * (float)kTableSize;
            int s0 = (int)sampleIdx;
            float sfrac = sampleIdx - (float)s0;

            float val0 = mip0_f0[(size_t)s0] + sfrac * (mip0_f0[(size_t)(s0 + 1)] - mip0_f0[(size_t)s0]);
            float val1 = mip0_f1[(size_t)s0] + sfrac * (mip0_f1[(size_t)(s0 + 1)] - mip0_f1[(size_t)s0]);

            displayBuffer[(size_t)i] = val0 * (1.0f - ffrac) + val1 * ffrac;
        }
    }

    void getSampleStereo(const SpectralWavetable* newestTable, float& outL, float& outR)
    {
        outL = 0.0f; outR = 0.0f;

        // テーブル更新検知とクロスフェード
        if (newestTable != currentTable && newestTable != nextTable) {
            if (currentTable == nullptr) {
                currentTable = newestTable;
            }
            else {
                nextTable = newestTable;
                fadeCounter = 0;
                isFading = true;
            }
        }

        if (currentTable == nullptr) return;

        float fadeRatio = 0.0f;
        if (isFading) {
            fadeRatio = (float)fadeCounter / 256.0f;
            fadeRatio = fadeRatio * fadeRatio * (3.0f - 2.0f * fadeRatio);
            if (++fadeCounter >= 256) {
                isFading = false;
                currentTable = nextTable;
            }
        }

        pitchEnvState *= pitchDecayCoef;
        if (pitchEnvState < 0.0001f) pitchEnvState = 0.0f;

        float currentWtPitch = wtPitchOffset + (pitchDecayAmt * pitchEnvState);
        float dynamicPitchMult = std::pow(2.0f, currentWtPitch / 12.0f);

        SIMDFloat outL_simd(0.0f), outR_simd(0.0f);
        int numBlocks = (unisonCount + SimdWidth - 1) / SimdWidth;

        for (int b = 0; b < numBlocks; ++b) {
            SIMDFloat p = phases[(size_t)b];
            SIMDFloat vAmp(0.0f), nextP(0.0f);

            // 代表ボイスの周波数から最適なMipmapレベルを決定
            float refStep = increments[(size_t)b].get(0) * dynamicPitchMult;
            float maxHarmonic = 0.5f / std::max(0.00001f, refStep);
            int targetMipmap = 0;
            float harmonicLimit = 1.0f;
            while (harmonicLimit < maxHarmonic && targetMipmap < kNumMipmaps - 1) {
                harmonicLimit *= 2.0f; ++targetMipmap;
            }
            targetMipmap = juce::jlimit(0, kNumMipmaps - 1, (kNumMipmaps - 1) - targetMipmap);

            if (isFading && nextTable != nullptr) {
                SIMDFloat ampOld(0.0f), ampNew(0.0f);
                voiceA.process(currentTable, wtPosition, targetMipmap, p, ampOld);
                voiceB.process(nextTable, wtPosition, targetMipmap, p, ampNew);
                for (int i = 0; i < SimdWidth; ++i) {
                    vAmp.set(i, ampOld.get(i) * (1.0f - fadeRatio) + ampNew.get(i) * fadeRatio);
                }
            }
            else {
                voiceA.process(currentTable, wtPosition, targetMipmap, p, vAmp);
            }

            for (int i = 0; i < SimdWidth; ++i) {
                int voiceIdx = b * SimdWidth + i;
                if (voiceIdx >= unisonCount) {
                    vAmp.set(i, 0.0f);
                    nextP.set(i, p.get(i));
                    continue;
                }

                // Driftの更新
                driftPhase[(size_t)voiceIdx] += driftRate[(size_t)voiceIdx] / sampleRate;
                if (driftPhase[(size_t)voiceIdx] >= 1.0f) driftPhase[(size_t)voiceIdx] -= 1.0f;
                float currentDriftMod = std::sin(driftPhase[(size_t)voiceIdx] * juce::MathConstants<float>::twoPi) * driftAmount * 0.01f;

                float step = increments[(size_t)b].get(i) * dynamicPitchMult * (1.0f + currentDriftMod);

                vAmp.set(i, vAmp.get(i) * amp[(size_t)b].get(i));
                float phaseInc = p.get(i) + step;
                if (phaseInc >= 1.0f) phaseInc -= 1.0f;
                nextP.set(i, phaseInc);
            }
            outL_simd += vAmp * panL[(size_t)b];
            outR_simd += vAmp * panR[(size_t)b];
            phases[(size_t)b] = nextP;
        }

        for (int i = 0; i < SimdWidth; ++i) {
            outL += outL_simd.get(i) * wtLevel;
            outR += outR_simd.get(i) * wtLevel;
        }

        // Sub Oscillator の処理
        if (subOn && subVolume > 0.001f) {
            float sFreq = baseFreq * std::pow(2.0f, subPitchOffset / 12.0f);
            subPhase += sFreq / sampleRate;
            if (subPhase >= 1.0f) subPhase -= 1.0f;

            float rawS = 0.0f;
            switch (subWaveform) {
            case 0: rawS = std::sin(subPhase * juce::MathConstants<float>::twoPi); break;
            case 1: rawS = 4.0f * std::abs(subPhase - 0.5f) - 1.0f; break;
            case 2: rawS = subPhase < 0.5f ? 1.0f : -1.0f; break;
            case 3: rawS = 2.0f * subPhase - 1.0f; break;
            }
            subLpfState += (juce::MathConstants<float>::twoPi * 250.0f / sampleRate) * (rawS - subLpfState);
            float s = subLpfState * subVolume;
            outL += s; outR += s;
        }
    }

private:
    float sampleRate = 44100.0f, baseFreq = 440.0f;
    float detuneAmount = 0.0f, stereoWidth = 1.0f, wtPosition = 0.0f, driftAmount = 0.0f;
    int unisonCount = 1;
    float wtLevel = 1.0f, wtPitchOffset = 0.0f, pitchDecayAmt = 0.0f, pitchDecayCoef = 0.0f, pitchEnvState = 0.0f;

    bool subOn = true;
    int subWaveform = 0;
    float subVolume = 0.0f;
    float subPitchOffset = -12.0f;
    float subPhase = 0.0f;
    float subLpfState = 0.0f;

    std::array<float, MaxVoices> driftPhase = { 0 }, driftRate = { 0 };
    std::array<SIMDFloat, MaxBlocks> phases, increments, panL, panR, amp;

    SpectralVoice voiceA, voiceB;
    const SpectralWavetable* currentTable = nullptr;
    const SpectralWavetable* nextTable = nullptr;
    int fadeCounter = 0;
    bool isFading = false;

    void recalculate() {
        float norm = 1.0f / std::sqrt((float)unisonCount);
        float centerAngle = 0.25f * juce::MathConstants<float>::pi;

        for (int b = 0; b < MaxBlocks; ++b) {
            SIMDFloat pL(0.0f), pR(0.0f), a(0.0f), inc(0.0f);
            for (int i = 0; i < SimdWidth; ++i) {
                int voiceIdx = b * SimdWidth + i;
                if (voiceIdx < unisonCount) {
                    float bias = (unisonCount == 1) ? 0.0f : (2.0f * (float)voiceIdx / (float)(unisonCount - 1) - 1.0f);
                    float pitchBias = bias * bias * bias;
                    float step = (baseFreq * std::pow(2.0f, (pitchBias * detuneAmount * 0.5f) / 12.0f)) / sampleRate;

                    float actualWidth = stereoWidth;
                    if (baseFreq < 150.0f) actualWidth *= juce::jlimit(0.0f, 1.0f, baseFreq / 150.0f);
                    float angle = centerAngle + ((bias * 0.7854f) * actualWidth);

                    pL.set(i, std::cos(angle)); pR.set(i, std::sin(angle)); a.set(i, norm); inc.set(i, step);
                }
            }
            panL[(size_t)b] = pL; panR[(size_t)b] = pR; amp[(size_t)b] = a; increments[(size_t)b] = inc;
        }
    }
};