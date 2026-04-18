#pragma once
#include <JuceHeader.h>
#include <cmath>
#include <numbers>
#include <array>

class SineShaper
{
public:
    SineShaper() { reset(); }

    void prepare(double sampleRate)
    {
        currentSampleRate = std::max(1.0, sampleRate);
        reset();
    }

    void reset()
    {
        edgeStateL = edgeStateR = 0.0f;
        downsampleCounter = 0.0f;
        heldSampleL = heldSampleR = 0.0f;
    }

    void setAmpType(int type) { ampType = type; }

    void setEdge(float edge01)
    {
        float minFreq = 800.0f;
        float maxFreq = 20000.0f;
        float cutoff = minFreq + std::pow(edge01, 2.0f) * (maxFreq - minFreq);
        float wc = 2.0f * std::numbers::pi_v<float> *cutoff / (float)currentSampleRate;
        edgeAlpha = std::min(1.0f, wc);
    }

    inline void processStereo(float inL, float inR, float drive, float rate, float bits, float& outL, float& outR)
    {
        // Edge Filter
        float pL = processOnePole(inL, edgeStateL, edgeAlpha);
        float pR = processOnePole(inR, edgeStateR, edgeAlpha);

        // Saturation & Wavefolding (Drive)
        float satL = applyDrive(pL, drive);
        float satR = applyDrive(pR, drive);

        // Bitcrush & Downsample
        if (rate > 1.0f || bits < 24.0f)
        {
            downsampleCounter += 1.0f;
            if (downsampleCounter >= rate)
            {
                downsampleCounter -= rate;
                heldSampleL = satL;
                heldSampleR = satR;
            }

            float steps = std::exp2(bits);
            outL = std::floor(heldSampleL * steps) / steps;
            outR = std::floor(heldSampleR * steps) / steps;
        }
        else
        {
            outL = satL;
            outR = satR;
        }
    }

private:
    double currentSampleRate = 44100.0;
    int ampType = 0;
    float edgeStateL = 0.0f, edgeStateR = 0.0f, edgeAlpha = 1.0f;
    float downsampleCounter = 0.0f, heldSampleL = 0.0f, heldSampleR = 0.0f;

    inline float processOnePole(float in, float& state, float alpha)
    {
        float out = state + alpha * (in - state);
        if (std::abs(out) < 1.0e-15f) out = 0.0f;
        state = out;
        return out;
    }

    inline float applyDrive(float in, float drive)
    {
        float soft = std::atan(in * 0.5f) * 2.0f;
        if (drive <= 1.0f) return soft * drive;

        float x = soft * drive;
        if (drive < 2.5f) return std::tanh(x);

        // Wavefolding for extreme settings
        float blend = std::min(1.0f, (drive - 2.5f) * 0.5f);
        return (1.0f - blend) * std::tanh(x) + blend * std::sin(x * 0.7f);
    }
};

