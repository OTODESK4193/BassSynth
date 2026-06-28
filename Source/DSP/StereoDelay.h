#pragma once
#include <JuceHeader.h>
#include <vector>
#include <cmath>
#include <algorithm>

//==============================================================================
// StereoDelay (Smart Ducking) -- ported from LiquidDream
//==============================================================================
class StereoDelay
{
public:
    StereoDelay() = default;

    void prepare(double sampleRate)
    {
        currentSampleRate = std::max(1.0, sampleRate);
        int maxDelaySamples = (int)(sampleRate * 2.0) + 100;

        delayBufferL.assign(maxDelaySamples, 0.0f);
        delayBufferR.assign(maxDelaySamples, 0.0f);

        bufferSize = maxDelaySamples;
        writePos = 0;
        lpfStateL = 0.0f; lpfStateR = 0.0f;
        envelopeFollower = 0.0f;
    }

    void setParameters(float time, float fb, float mixV, float damping, bool pingPong)
    {
        delayTimeSamples = time * (float)currentSampleRate;
        this->feedback = juce::jlimit(0.0f, 0.95f, fb);
        this->mix = juce::jlimit(0.0f, 1.0f, mixV);
        dampingCoeff = 1.0f - (juce::jlimit(0.0f, 1.0f, damping) * 0.9f);
        isPingPong = pingPong;
    }

    void processBlock(float* leftChannel, float* rightChannel, int numSamples)
    {
        if (bufferSize < 1) return;
        if (leftChannel == nullptr) return;
        if (mix <= 0.001f) return;

        constexpr float duckAttack = 0.99f;
        constexpr float duckRelease = 0.995f;
        constexpr float duckAmount = 0.6f;

        for (int i = 0; i < numSamples; ++i)
        {
            float inL = leftChannel[i];
            float inR = (rightChannel) ? rightChannel[i] : inL;

            float inputMag = std::max(std::abs(inL), std::abs(inR));
            if (inputMag > envelopeFollower)
                envelopeFollower = envelopeFollower * duckAttack + inputMag * (1.0f - duckAttack);
            else
                envelopeFollower = envelopeFollower * duckRelease + inputMag * (1.0f - duckRelease);

            float duckGain = 1.0f - (std::min(envelopeFollower, 1.0f) * duckAmount);
            float currentMix = mix * duckGain;

            float readPos = (float)writePos - delayTimeSamples;
            while (readPos < 0.0f) readPos += (float)bufferSize;
            while (readPos >= (float)bufferSize) readPos -= (float)bufferSize;

            int rIndex = (int)readPos;
            int rNext = (rIndex + 1);
            if (rNext >= bufferSize) rNext = 0;
            float frac = readPos - rIndex;

            float delayL = 0.0f, delayR = 0.0f;
            if (rIndex >= 0 && rIndex < bufferSize)
            {
                delayL = delayBufferL[rIndex] + frac * (delayBufferL[rNext] - delayBufferL[rIndex]);
                delayR = delayBufferR[rIndex] + frac * (delayBufferR[rNext] - delayBufferR[rIndex]);
            }

            leftChannel[i] = inL * (1.0f - currentMix * 0.5f) + delayL * currentMix;
            if (rightChannel)
                rightChannel[i] = inR * (1.0f - currentMix * 0.5f) + delayR * currentMix;

            lpfStateL = (delayL * dampingCoeff) + (lpfStateL * (1.0f - dampingCoeff));
            lpfStateR = (delayR * dampingCoeff) + (lpfStateR * (1.0f - dampingCoeff));

            if (std::abs(lpfStateL) < 1.0e-15f) lpfStateL = 0.0f;
            if (std::abs(lpfStateR) < 1.0e-15f) lpfStateR = 0.0f;

            float fbInL, fbInR;
            if (isPingPong)
            {
                fbInL = inL + lpfStateR * feedback;
                fbInR = inR + lpfStateL * feedback;
            }
            else
            {
                fbInL = inL + lpfStateL * feedback;
                fbInR = inR + lpfStateR * feedback;
            }

            if (writePos >= 0 && writePos < bufferSize)
            {
                delayBufferL[writePos] = fbInL;
                delayBufferR[writePos] = fbInR;
            }

            writePos++;
            if (writePos >= bufferSize) writePos = 0;
        }
    }

private:
    double currentSampleRate = 44100.0;
    std::vector<float> delayBufferL;
    std::vector<float> delayBufferR;
    int bufferSize = 0;
    int writePos = 0;

    float delayTimeSamples = 0.0f;
    float feedback = 0.0f;
    float mix = 0.0f;
    float lpfStateL = 0.0f;
    float lpfStateR = 0.0f;
    float dampingCoeff = 1.0f;
    bool isPingPong = false;

    float envelopeFollower = 0.0f;
};
