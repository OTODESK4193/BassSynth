#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>
#include <complex>
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
        resetPhase();
    }

    void prepare(double sr) { sampleRate = std::max(1.0, sr); }

    void loadWavetableFile(juce::String fileName) {
        juce::File file = juce::File(EmbeddedWavetables::wavetablesDir).getChildFile(fileName);
        if (!file.existsAsFile()) return;

        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
        if (reader != nullptr) {
            WavetableSet::Ptr newSet = new WavetableSet();
            juce::AudioBuffer<float> tempRaw;
            tempRaw.setSize(1, (int)reader->lengthInSamples);
            reader->read(&tempRaw, 0, (int)reader->lengthInSamples, 0, true, false);

            newSet->totalSamples = tempRaw.getNumSamples();
            newSet->frameSize = (newSet->totalSamples >= 2048) ? 2048 : newSet->totalSamples;
            newSet->numFrames = std::max(1, newSet->totalSamples / newSet->frameSize);

            juce::dsp::FFT fft(11); // 2048ポイント

            // 【解決策1】SIMDアライメントが完全に保証されたJUCE標準バッファを使用
            // Real-Only FFTは 2 * N のサイズを要求するため、2048 * 2 = 4096 を確保
            juce::AudioBuffer<float> workBuf(1, 4096);

            for (int lvl = 0; lvl < NumLevels; ++lvl) {
                newSet->levels[lvl].setSize(1, newSet->totalSamples);

                if (newSet->frameSize == 2048) {
                    for (int f = 0; f < newSet->numFrames; ++f) {
                        workBuf.clear();
                        auto* workPtr = workBuf.getWritePointer(0);

                        // 1. 元データをセット
                        for (int i = 0; i < 2048; ++i) {
                            int srcIdx = f * 2048 + i;
                            workPtr[i] = (srcIdx < newSet->totalSamples) ? tempRaw.getSample(0, srcIdx) : 0.0f;
                        }

                        // 2. 実数専用フォワードFFT（メモリ安全）
                        fft.performRealOnlyForwardTransform(workPtr);

                        // 3. 帯域制限（Nyquistと高周波のクリア）
                        if (lvl > 0) {
                            int harmonicLimit = 1024 >> lvl;
                            if (harmonicLimit < 2) harmonicLimit = 2;

                            // JUCE Real-Only FFTの仕様：[0]がDC, [1]がNyquist, 以降はReal/Imagのペア
                            for (int k = harmonicLimit; k <= 1024; ++k) {
                                if (k == 1024) {
                                    workPtr[1] = 0.0f; // Nyquist成分
                                }
                                else {
                                    workPtr[2 * k] = 0.0f;     // Real成分
                                    workPtr[2 * k + 1] = 0.0f; // Imag成分
                                }
                            }
                        }

                        // 【絶対防衛】DCオフセットの完全消去
                        workPtr[0] = 0.0f;

                        // 4. 実数専用インバースFFT
                        fft.performRealOnlyInverseTransform(workPtr);

                        // 5. 【解決策2】オート・ノーマライズ
                        // 逆FFT後の波形の最大ピークを探す
                        float peak = 1e-9f; // ゼロ除算防止
                        for (int i = 0; i < 2048; ++i) {
                            peak = std::max(peak, std::abs(workPtr[i]));
                        }

                        // ピークを1.0にするための倍率を計算
                        float normalizeScale = 1.0f / peak;

                        // 6. スケーリングして安全に最終バッファへ書き込み
                        auto* destPtr = newSet->levels[lvl].getWritePointer(0);
                        for (int i = 0; i < 2048; ++i) {
                            int dstIdx = f * 2048 + i;
                            if (dstIdx < newSet->totalSamples) {
                                // ここで必ず -1.0 〜 +1.0 の美しい波形に収まる
                                destPtr[dstIdx] = juce::jlimit(-1.0f, 1.0f, workPtr[i] * normalizeScale);
                            }
                        }
                    }
                }
                else {
                    newSet->levels[lvl].makeCopyOf(tempRaw);
                }
            }
            currentWavetableSet = newSet;
        }
    }

    void generateSingleCycle(std::array<float, 512>& displayBuffer) {
        WavetableSet::Ptr set = currentWavetableSet.get();
        if (set == nullptr || set->totalSamples == 0) { displayBuffer.fill(0.0f); return; }

        auto* ptr = set->levels[0].getReadPointer(0);
        float framePos = wtPosition * (float)std::max(0, set->numFrames - 1);
        int frameIdx = (int)framePos;
        float frameFrac = framePos - (float)frameIdx;

        for (int i = 0; i < 512; ++i) {
            float phase = i / 512.0f;
            float mod = std::sin(phase * 2.0f * juce::MathConstants<float>::pi) * fmAmount * 0.1f;
            float syncPhase = std::fmod((phase + mod) * syncAmount, 1.0f);
            if (syncPhase < 0.0f) syncPhase += 1.0f;

            int samplePos = (int)(syncPhase * (set->frameSize - 1));
            int idx1 = juce::jlimit(0, set->totalSamples - 1, (frameIdx * set->frameSize) + samplePos);
            int idx2 = juce::jlimit(0, set->totalSamples - 1, ((frameIdx + 1) * set->frameSize) + samplePos);

            float val = ptr[idx1] * (1.0f - frameFrac) + ptr[idx2] * frameFrac;
            if (morphParam > 0.01f) {
                val *= (1.0f + morphParam * 4.0f);
                val = std::sin(val * juce::MathConstants<float>::halfPi);
            }
            displayBuffer[i] = val;
        }
    }

    void setFrequency(float freqHz) { baseFreq = freqHz; recalculate(); }
    void setUnisonCount(int c) { unisonCount = juce::jlimit(1, MaxVoices, c); recalculate(); }
    void setUnisonDetune(float d) { detuneAmount = d; recalculate(); }
    void setWavetablePosition(float pos) { wtPosition = pos; }
    void setFMAmount(float amt) { fmAmount = amt; }
    void setSyncAmount(float amt) { syncAmount = std::max(1.0f, amt); }
    void setMorph(float m) { morphParam = m; }

    void resetPhase() {
        for (int b = 0; b < MaxBlocks; ++b) phases[b] = SIMDFloat(0.0f);
    }

    void getSampleStereo(float& outL, float& outR) {
        outL = 0.0f; outR = 0.0f;
        WavetableSet::Ptr set = currentWavetableSet.get();
        if (set == nullptr || set->totalSamples == 0) return;

        SIMDFloat fmScaled(fmAmount * 0.1f);
        SIMDFloat sync(syncAmount);
        float framePos = wtPosition * (float)std::max(0, set->numFrames - 1);
        int frameIdx = (int)framePos;
        float frameFrac = framePos - (float)frameIdx;

        SIMDFloat outL_simd(0.0f), outR_simd(0.0f);
        int numBlocks = (unisonCount + SimdWidth - 1) / SimdWidth;
        SIMDFloat half(0.5f), c1(6.2831853f), c2(41.341702f), c3(81.605249f), negOne(-1.0f);

        for (int b = 0; b < numBlocks; ++b) {
            SIMDFloat p = phases[b];
            SIMDFloat z = p - half;
            SIMDFloat fmSin = (z * c1 - (z * z * z) * c2 + (z * z * z * z * z) * c3) * negOne;
            SIMDFloat totalPhase = (p + fmSin * fmScaled) * sync;

            SIMDFloat vAmp(0.0f), nextP(0.0f);

            for (int i = 0; i < SimdWidth; ++i) {
                int voiceIdx = b * SimdWidth + i;
                if (voiceIdx >= unisonCount) {
                    vAmp.set(i, 0.0f);
                    nextP.set(i, p.get(i));
                    continue;
                }

                float step = increments[b].get(i);
                float voiceFreq = std::max(1.0f, step * (float)sampleRate);

                // --- 【STEP 4】Mipmapの滑らかなクロスフェード ---
                float maxH = (float)sampleRate / (2.0f * voiceFreq);
                int level0 = 0;
                float hLimit = 1024.0f;

                while (level0 < NumLevels - 2 && (hLimit * 0.5f) > maxH) {
                    level0++;
                    hLimit *= 0.5f;
                }

                // 境界での小数割合（フェード量）を計算
                float levelFrac = 0.0f;
                if (hLimit > maxH) {
                    levelFrac = (hLimit - maxH) / (hLimit * 0.5f);
                    levelFrac = juce::jlimit(0.0f, 1.0f, levelFrac);
                }
                int level1 = level0 + 1;

                const float* ptr0 = set->levels[level0].getReadPointer(0);
                const float* ptr1 = set->levels[level1].getReadPointer(0);

                float tp = totalPhase.get(i);
                tp -= std::floor(tp);
                if (tp < 0.0f) tp += 1.0f;

                int samplePos = (int)(tp * (set->frameSize - 1));
                int idx1 = juce::jlimit(0, set->totalSamples - 1, (frameIdx * set->frameSize) + samplePos);
                int idx2 = juce::jlimit(0, set->totalSamples - 1, ((frameIdx + 1) * set->frameSize) + samplePos);

                // 両方のレベルから値を読み出し
                float val0 = ptr0[idx1] * (1.0f - frameFrac) + ptr0[idx2] * frameFrac;
                float val1 = ptr1[idx1] * (1.0f - frameFrac) + ptr1[idx2] * frameFrac;

                // レベル間を滑らかにブレンド（クロスフェード）
                float val = val0 * (1.0f - levelFrac) + val1 * levelFrac;

                if (morphParam > 0.01f) {
                    val *= (1.0f + morphParam * 4.0f);
                    val = std::sin(val * juce::MathConstants<float>::halfPi);
                }

                vAmp.set(i, val * amp[b].get(i));
                float phaseInc = p.get(i) + step;
                if (phaseInc >= 1.0f) phaseInc -= 1.0f;
                nextP.set(i, phaseInc);
            }

            outL_simd += vAmp * panL[b];
            outR_simd += vAmp * panR[b];
            phases[b] = nextP;
        }

        for (int i = 0; i < SimdWidth; ++i) {
            outL += outL_simd.get(i);
            outR += outR_simd.get(i);
        }
    }

private:
    double sampleRate = 44100.0;
    float baseFreq = 440.0f, detuneAmount = 0.0f, wtPosition = 0.0f;
    float fmAmount = 0.0f, syncAmount = 1.0f, morphParam = 0.0f;
    int unisonCount = 1;

    juce::AudioFormatManager formatManager;
    juce::ReferenceCountedObjectPtr<WavetableSet> currentWavetableSet;
    std::array<SIMDFloat, MaxBlocks> phases, increments, panL, panR, amp;

    void recalculate() {
        float norm = 1.0f / std::sqrt((float)unisonCount);
        for (int b = 0; b < MaxBlocks; ++b) {
            SIMDFloat pL(0.0f), pR(0.0f), a(0.0f), inc(0.0f);
            for (int i = 0; i < SimdWidth; ++i) {
                int voiceIdx = b * SimdWidth + i;
                if (voiceIdx < unisonCount) {
                    float bias = (unisonCount == 1) ? 0.0f : (2.0f * voiceIdx / (float)(unisonCount - 1) - 1.0f);
                    float step = (float)((baseFreq * std::pow(2.0f, (bias * detuneAmount * 0.5f) / 12.0f)) / sampleRate);
                    float angle = (bias + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
                    if (baseFreq < 150.0f) {
                        float centerWeight = juce::jlimit(0.0f, 1.0f, baseFreq / 150.0f);
                        angle = (0.25f * juce::MathConstants<float>::pi) + (angle - 0.25f * juce::MathConstants<float>::pi) * centerWeight;
                    }
                    pL.set(i, std::cos(angle));
                    pR.set(i, std::sin(angle));
                    a.set(i, norm);
                    inc.set(i, step);
                }
            }
            panL[b] = pL; panR[b] = pR; amp[b] = a; increments[b] = inc;
        }
    }
};