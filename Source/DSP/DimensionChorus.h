#pragma once
#include <JuceHeader.h>
#include <vector>
#include <cmath>
#include <algorithm>

//==============================================================================
// DimensionChorus (Cross-Coupled Phase Inversion) -- ported from LiquidDream
//==============================================================================
class DimensionChorus
{
public:
    DimensionChorus() = default;

    void prepare(double sampleRate)
    {
        currentSampleRate = std::max(1.0, sampleRate);
        int bufferLen = (int)(sampleRate * 0.1) + 100;

        delayBufferL.assign(bufferLen, 0.0f);
        delayBufferR.assign(bufferLen, 0.0f);

        bufferSize = bufferLen;
        writePos = 0;
        lfoPhase = 0.0f;
    }

    void setParameters(float mixValue, float depthValue, float speedValue)
    {
        mix = juce::jlimit(0.0f, 1.0f, mixValue);
        depth = juce::jlimit(0.0f, 0.02f, depthValue); // seconds (0..20ms)
        speed = juce::jlimit(0.0f, 8.0f, speedValue);   // Hz

        if (currentSampleRate > 0)
            lfoInc = speed / (float)currentSampleRate;
    }

    void processBlock(float* left, float* right, int numSamples)
    {
        if (mix <= 0.001f || bufferSize < 2) return;
        if (left == nullptr || right == nullptr) return;

        for (int i = 0; i < numSamples; ++i)
        {
            float inL = left[i];
            float inR = right[i];

            delayBufferL[writePos] = inL;
            delayBufferR[writePos] = inR;

            float lfo = (lfoPhase < 0.5f) ? (2.0f * lfoPhase) : (2.0f - 2.0f * lfoPhase);
            lfoPhase += lfoInc;
            if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;

            float baseDelay = 0.005f * (float)currentSampleRate; // 5ms
            float modSamples = depth * (float)currentSampleRate * lfo;

            float dTimeL = baseDelay + modSamples;
            float dTimeR = baseDelay - modSamples;

            float wetL = readInterpolated(delayBufferL, (float)writePos - dTimeL);
            float wetR = readInterpolated(delayBufferR, (float)writePos - dTimeR);

            float crossL = wetL - wetR;
            float crossR = wetR - wetL;

            left[i] = inL + (crossL * mix);
            right[i] = inR + (crossR * mix);

            writePos++;
            if (writePos >= bufferSize) writePos = 0;
        }
    }

private:
    double currentSampleRate = 44100.0;
    std::vector<float> delayBufferL, delayBufferR;
    int bufferSize = 0;
    int writePos = 0;

    float lfoPhase = 0.0f;
    float lfoInc = 0.0f;
    float mix = 0.0f;
    float depth = 0.002f;
    float speed = 0.5f;

    inline float readInterpolated(const std::vector<float>& buffer, float readPos)
    {
        while (readPos < 0.0f) readPos += (float)bufferSize;
        while (readPos >= (float)bufferSize) readPos -= (float)bufferSize;

        int idx0 = (int)readPos;
        int idx1 = idx0 + 1;
        if (idx1 >= bufferSize) idx1 = 0;
        float frac = readPos - idx0;

        return buffer[idx0] + frac * (buffer[idx1] - buffer[idx0]);
    }
};
