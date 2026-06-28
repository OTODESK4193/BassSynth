#pragma once
#include <JuceHeader.h>
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>

//==============================================================================
// SimpleReverb (Diffusion Cloud)  -- ported from LiquidDream
// Series Allpass Filters to smear transients and create "liquid" space.
//==============================================================================
class SimpleReverb
{
public:
    SimpleReverb() = default;

    void prepare(double sampleRate)
    {
        currentSampleRate = std::max(1.0, sampleRate);
        const int sizesL[] = { 223, 337, 461, 593, 751, 919 };
        const int sizesR[] = { 227, 347, 463, 599, 757, 929 };

        numStages = 6;

        buffersL.resize(numStages);
        buffersR.resize(numStages);
        indicesL.assign(numStages, 0);
        indicesR.assign(numStages, 0);

        for (int i = 0; i < numStages; ++i)
        {
            buffersL[i].assign(sizesL[i], 0.0f);
            buffersR[i].assign(sizesR[i], 0.0f);
        }
    }

    void setParameters(float mixV, float size, float widthV)
    {
        this->mix = juce::jlimit(0.0f, 1.0f, mixV);
        this->feedback = 0.5f + (juce::jlimit(0.0f, 1.0f, size) * 0.2f); // 0.5 to 0.7
        this->width = juce::jlimit(0.0f, 1.0f, widthV);
    }

    void processBlock(float* left, float* right, int numSamples)
    {
        if (mix <= 0.001f) return;

        for (int n = 0; n < numSamples; ++n)
        {
            float inL = left[n];
            float inR = (right) ? right[n] : inL;

            float outL = inL;
            float outR = inR;

            float mono = (outL + outR) * 0.5f;
            float side = (outL - outR) * 0.5f;
            outL = mono + side * (1.0f + width * 0.5f);
            outR = mono - side * (1.0f + width * 0.5f);

            for (int i = 0; i < numStages; ++i)
            {
                outL = processAllpass(outL, buffersL[i], indicesL[i], feedback);
                outR = processAllpass(outR, buffersR[i], indicesR[i], feedback);
            }

            left[n] = inL * (1.0f - mix * 0.5f) + outL * mix;
            if (right)
                right[n] = inR * (1.0f - mix * 0.5f) + outR * mix;
        }
    }

private:
    double currentSampleRate = 44100.0;

    int numStages = 0;
    std::vector<std::vector<float>> buffersL;
    std::vector<std::vector<float>> buffersR;
    std::vector<int> indicesL;
    std::vector<int> indicesR;

    float mix = 0.0f;
    float feedback = 0.5f;
    float width = 0.5f;

    inline float processAllpass(float input, std::vector<float>& buffer, int& index, float fb)
    {
        int size = (int)buffer.size();
        float bufOut = buffer[index];

        float inVal = input + (bufOut * -fb);
        float outVal = bufOut + (inVal * fb);

        if (std::abs(inVal) < 1.0e-15f) inVal = 0.0f;

        buffer[index] = inVal;

        index++;
        if (index >= size) index = 0;

        return outVal;
    }
};
