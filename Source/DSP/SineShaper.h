#pragma once
#include <JuceHeader.h>
#include <cmath>
#include <numbers>
#include <algorithm>

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

    inline void processStereo(float inL, float inR, float drive, float rate, float bits, float& outL, float& outR)
    {
        // Saturation & Wavefolding
        float satL = applyDrive(inL, drive);
        float satR = applyDrive(inR, drive);

        // 44.1kHz時の挙動を基準としたレート補正（SampleRate非依存化）
        float effectiveRate = rate * (float)(currentSampleRate / 44100.0);

        // Bitcrush & Downsample
        if (effectiveRate > 1.0f || bits < 24.0f)
        {
            downsampleCounter += 1.0f;
            if (downsampleCounter >= effectiveRate)
            {
                downsampleCounter -= effectiveRate;
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
    float edgeStateL = 0.0f, edgeStateR = 0.0f;
    float downsampleCounter = 0.0f, heldSampleL = 0.0f, heldSampleR = 0.0f;

    inline float applyDrive(float in, float drive)
    {
        float soft = std::atan(in * 0.5f) * 2.0f;
        if (drive <= 1.0f) return soft * drive;

        float x = soft * drive;
        if (drive < 2.5f) return std::tanh(x);

        float blend = std::min(1.0f, (drive - 2.5f) * 0.5f);
        return (1.0f - blend) * std::tanh(x) + blend * std::sin(x * 0.7f);
    }
};