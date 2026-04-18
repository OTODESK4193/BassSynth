#pragma once
#include <JuceHeader.h>
#include <cmath>
#include <numbers>
#include <algorithm>

class LadderFilter
{
public:
    LadderFilter() = default;

    void prepare(double sampleRate)
    {
        currentSampleRate = std::max(1.0, sampleRate);
        // 10msの時定数で正確にスムージングするための係数を計算（SampleRate非依存）
        driftAlpha = 1.0f - std::exp(-1.0f / (0.01f * (float)currentSampleRate));
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

        // サンプルレートに依存しない滑らかな追従
        driftState += driftAlpha * (cutoffHz - driftState);
        float actualCutoff = driftState;

        float wc = 2.0f * std::numbers::pi_v<float> *actualCutoff / (float)currentSampleRate;
        wc = std::clamp(wc, 0.0f, std::numbers::pi_v<float> *0.99f);
        g = std::tan(wc * 0.5f);

        k = 4.0f * std::clamp(resonance, 0.0f, 0.98f);
        bassComp = 1.0f + (k * 0.75f);
    }

    inline float processSample(float input)
    {
        if (isBypassed) return input;

        float x = input * bassComp;

        float G = g / (1.0f + g);
        float S1 = s1 / (1.0f + g);
        float S2 = s2 / (1.0f + g);
        float S3 = s3 / (1.0f + g);
        float S4 = s4 / (1.0f + g);

        float S = G * G * G * S1 + G * G * S2 + G * S3 + S4;
        float D = 1.0f / (1.0f + k * G * G * G * G);

        float y4 = (G * G * G * G * x + S) * D;
        float u = x - k * std::tanh(y4);

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
    float driftState = 20000.0f, driftAlpha = 0.1f;
    float g = 0.0f, k = 0.0f, bassComp = 1.0f;
    double currentSampleRate = 44100.0;
    bool isBypassed = false;
};