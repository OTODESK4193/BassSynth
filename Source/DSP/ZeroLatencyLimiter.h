// ==============================================================================
// Source/DSP/ZeroLatencyLimiter.h
// ==============================================================================
#pragma once
#include <JuceHeader.h>
#include <cmath>
#include <algorithm>

class ZeroLatencyLimiter
{
public:
    void prepare(double sr) {
        sampleRate = std::max(1.0, sr);
        // リリース時間を50msに設定（自然なゲイン復帰のため）
        releaseCoef = std::exp(-1.0f / static_cast<float>(0.05 * sampleRate));
        currentGain = 1.0f;
    }

    void setCeiling(float ceilDb) {
        ceilingLinear = juce::Decibels::decibelsToGain(ceilDb);
    }

    void process(juce::AudioBuffer<float>& buffer) {
        int numSamples = buffer.getNumSamples();
        auto* l = buffer.getWritePointer(0);
        auto* r = buffer.getWritePointer(1);

        for (int i = 0; i < numSamples; ++i) {
            float peak = std::max(std::abs(l[i]), std::abs(r[i]));
            float targetGain = 1.0f;

            // ピークがCeilingを超えた場合、必要なゲインリダクション量を計算
            if (peak > ceilingLinear && peak > 0.00001f) {
                targetGain = ceilingLinear / peak;
            }

            // アタックは即時（ゼロレイテンシーでクリッピングを防止）、リリースは滑らかに
            if (targetGain < currentGain) {
                currentGain = targetGain;
            }
            else {
                currentGain = releaseCoef * currentGain + (1.0f - releaseCoef) * targetGain;
            }

            l[i] *= currentGain;
            r[i] *= currentGain;
        }
    }

private:
    double sampleRate = 44100.0;
    float ceilingLinear = 1.0f;
    float currentGain = 1.0f;
    float releaseCoef = 0.0f;
};