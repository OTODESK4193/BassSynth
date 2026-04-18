#pragma once
#include <JuceHeader.h>
#include <cmath>
#include <numbers>
#include <span>

class LadderFilter
{
public:
    LadderFilter() = default;

    void prepare(double sampleRate)
    {
        currentSampleRate = std::max(1.0, sampleRate);
        reset();
    }

    void reset()
    {
        s1 = s2 = s3 = s4 = 0.0f;
        driftState = 20000.0f;
    }

    void setParameters(float cutoffHz, float resonance)
    {
        if (cutoffHz >= 19900.0f) {
            isBypassed = true;
            return;
        }
        isBypassed = false;

        // アナログコンデンサの遅れをシミュレート（ジッパーノイズ防止）
        driftState += 0.1f * (cutoffHz - driftState);
        float actualCutoff = driftState;

        float wc = 2.0f * std::numbers::pi_v<float> *actualCutoff / (float)currentSampleRate;
        wc = std::clamp(wc, 0.0f, std::numbers::pi_v<float> *0.99f);
        g = std::tan(wc * 0.5f);

        // レゾナンスのリミット
        k = 4.0f * std::clamp(resonance, 0.0f, 0.98f);

        // 低域補償 (Bass Compensation): レゾナンスを上げても低音が痩せないようにする
        bassComp = 1.0f + (k * 0.75f);
    }

    inline float processSample(float input)
    {
        if (isBypassed) return input;

        float x = input * bassComp;

        // ZDFソルバー
        float G = g / (1.0f + g);
        float S1 = s1 / (1.0f + g);
        float S2 = s2 / (1.0f + g);
        float S3 = s3 / (1.0f + g);
        float S4 = s4 / (1.0f + g);

        float S = G * G * G * S1 + G * G * S2 + G * S3 + S4;
        float D = 1.0f / (1.0f + k * G * G * G * G);

        float y4 = (G * G * G * G * x + S) * D;
        float u = x - k * std::tanh(y4); // Moog特有のサチュレーション

        float v1 = (g * u + s1) / (1.0f + g);
        float v2 = (g * v1 + s2) / (1.0f + g);
        float v3 = (g * v2 + s3) / (1.0f + g);
        float v4 = (g * v3 + s4) / (1.0f + g);

        s1 = 2.0f * v1 - s1;
        s2 = 2.0f * v2 - s2;
        s3 = 2.0f * v3 - s3;
        s4 = 2.0f * v4 - s4;

        return v4;
    }

private:
    float s1 = 0.0f, s2 = 0.0f, s3 = 0.0f, s4 = 0.0f;
    float driftState = 20000.0f;
    float g = 0.0f, k = 0.0f, bassComp = 1.0f;
    double currentSampleRate = 44100.0;
    bool isBypassed = false;
};


