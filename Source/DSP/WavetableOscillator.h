#pragma once
#include <JuceHeader.h>
#include <array>
#include "../Generated/WavetableData_Generated.h"

class WavetableOscillator
{
public:
    using SIMDFloat = juce::dsp::SIMDRegister<float>;
    static constexpr int MaxVoices = 12;
    // SIMDレジスタの幅（通常SSE/NEONで4、AVXで8）
    static constexpr int SimdWidth = SIMDFloat::size();
    static constexpr int MaxBlocks = (MaxVoices + SimdWidth - 1) / SimdWidth;

    // --- STEP 1: RCU (Read-Copy-Update) 管理用のデータ構造 ---
    // 波形データとそのメタデータをひとまとめにし、参照カウントで保護します。
    struct WavetableSet : public juce::ReferenceCountedObject {
        using Ptr = juce::ReferenceCountedObjectPtr<WavetableSet>;
        juce::AudioBuffer<float> tableData;
        int frameSize = 2048;
        int numFrames = 0;
        int totalSamples = 0;

        WavetableSet() = default;
    };

    WavetableOscillator() {
        formatManager.registerBasicFormats();

        // 起動時にNullアクセスを防ぐため、無音の安全なセットを初期化します
        auto* emptySet = new WavetableSet();
        emptySet->tableData.setSize(1, 2048);
        emptySet->tableData.clear();
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
            // --- Copy (裏で新しいデータを作る) ---
            WavetableSet::Ptr newSet = new WavetableSet();
            newSet->tableData.setSize(1, (int)reader->lengthInSamples);
            reader->read(&newSet->tableData, 0, (int)reader->lengthInSamples, 0, true, false);

            newSet->totalSamples = newSet->tableData.getNumSamples();
            if (newSet->totalSamples >= 2048 && newSet->totalSamples % 2048 == 0) {
                newSet->frameSize = 2048;
                newSet->numFrames = newSet->totalSamples / 2048;
            }
            else {
                newSet->frameSize = std::max(1, newSet->totalSamples);
                newSet->numFrames = 1;
            }

            // --- Update (完成したデータを一瞬で差し替える) ---
            // オーディオスレッドが古いデータを読んでいる最中でも、安全に切り替わります。
            currentWavetableSet = newSet;
        }
    }

    void generateSingleCycle(std::array<float, 512>& displayBuffer) {
        // --- Read (現在のデータの参照を確保する) ---
        WavetableSet::Ptr set = currentWavetableSet.get();
        if (set == nullptr || set->totalSamples == 0) { displayBuffer.fill(0.0f); return; }

        auto* ptr = set->tableData.getReadPointer(0);
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

        // --- Read (オーディオスレッドでの安全な参照確保) ---
        // 処理の途中でファイルが切り替わっても、このブロックが終わるまでメモリは消えません。
        WavetableSet::Ptr set = currentWavetableSet.get();
        if (set == nullptr || set->totalSamples == 0) return;

        auto* ptr = set->tableData.getReadPointer(0);

        SIMDFloat fmScaled(fmAmount * 0.1f);
        SIMDFloat sync(syncAmount);

        float framePos = wtPosition * (float)std::max(0, set->numFrames - 1);
        int frameIdx = (int)framePos;
        float frameFrac = framePos - (float)frameIdx;

        SIMDFloat outL_simd(0.0f);
        SIMDFloat outR_simd(0.0f);

        // 計算が必要なブロック数のみ処理
        int numBlocks = (unisonCount + SimdWidth - 1) / SimdWidth;

        // 定数のSIMD化（単項演算子のエラー回避用）
        SIMDFloat half(0.5f);
        SIMDFloat c1(6.2831853f), c2(41.341702f), c3(81.605249f), negOne(-1.0f);

        for (int b = 0; b < numBlocks; ++b) {
            SIMDFloat p = phases[b];

            // 1. 高速SIMDサイン近似 (-(z * c1 - z3 * c2 + z5 * c3) の代替として -1.0f を掛ける)
            SIMDFloat z = p - half;
            SIMDFloat z2 = z * z;
            SIMDFloat z3 = z2 * z;
            SIMDFloat z5 = z3 * z2;
            SIMDFloat fmSin = (z * c1 - z3 * c2 + z5 * c3) * negOne;

            SIMDFloat mod = fmSin * fmScaled;
            SIMDFloat totalPhase = (p + mod) * sync;

            // 2. メモリ・ギャザー（テーブル参照）のためのスカラー・アンパック（安全な.get()と.set()を使用）
            SIMDFloat vAmp(0.0f);
            SIMDFloat nextP(0.0f);

            for (int i = 0; i < SimdWidth; ++i) {
                int voiceIdx = b * SimdWidth + i;

                // 無効なボイスは計算をスキップし0で埋める
                if (voiceIdx >= unisonCount) {
                    vAmp.set(i, 0.0f);
                    nextP.set(i, p.get(i)); // 位相は維持
                    continue;
                }

                float tp = totalPhase.get(i);
                tp -= std::floor(tp);
                if (tp < 0.0f) tp += 1.0f;

                int samplePos = (int)(tp * (set->frameSize - 1));
                int idx1 = juce::jlimit(0, set->totalSamples - 1, (frameIdx * set->frameSize) + samplePos);
                int idx2 = juce::jlimit(0, set->totalSamples - 1, ((frameIdx + 1) * set->frameSize) + samplePos);

                float val = ptr[idx1] * (1.0f - frameFrac) + ptr[idx2] * frameFrac;

                if (morphParam > 0.01f) {
                    val *= (1.0f + morphParam * 4.0f);
                    val = std::sin(val * juce::MathConstants<float>::halfPi);
                }

                float finalAmp = val * amp[b].get(i);
                vAmp.set(i, finalAmp);

                // 位相のインクリメントとラップアラウンド
                float phaseInc = p.get(i) + increments[b].get(i);
                if (phaseInc >= 1.0f) phaseInc -= 1.0f;
                nextP.set(i, phaseInc);
            }

            // 3. リパックされたSIMDとパンニングの乗算（Accumulate）
            outL_simd += vAmp * panL[b];
            outR_simd += vAmp * panR[b];

            phases[b] = nextP;
        }

        // 全SIMDブロックの結果をステレオのスカラー変数に集約
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

    // --- 参照カウントによるポインタ管理 ---
    juce::ReferenceCountedObjectPtr<WavetableSet> currentWavetableSet;

    // 12ボイスをSIMDブロック（4または8）にパックして管理
    std::array<SIMDFloat, MaxBlocks> phases, increments, panL, panR, amp;

    void recalculate() {
        float norm = 1.0f / std::sqrt((float)unisonCount);

        // 安全にSIMDレジスタへパックする
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
            panL[b] = pL;
            panR[b] = pR;
            amp[b] = a;
            increments[b] = inc;
        }
    }
};