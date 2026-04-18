#pragma once
#include <JuceHeader.h>
#include <array>
#include "../Generated/WavetableData_Generated.h"

class WavetableOscillator
{
public:
    static constexpr int MaxVoices = 12;

    WavetableOscillator() {
        formatManager.registerBasicFormats();
        for (int i = 0; i < MaxVoices; ++i) phases[i] = 0.0f;
    }

    void prepare(double sr) { sampleRate = std::max(1.0, sr); }

    void loadWavetableFile(juce::String fileName) {
        juce::File file = juce::File(EmbeddedWavetables::wavetablesDir).getChildFile(fileName);
        if (!file.existsAsFile()) return;

        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
        if (reader != nullptr) {
            juce::AudioBuffer<float> tempBuffer;
            tempBuffer.setSize(1, (int)reader->lengthInSamples);
            reader->read(&tempBuffer, 0, (int)reader->lengthInSamples, 0, true, false);

            tableData.setSize(1, tempBuffer.getNumSamples());
            tableData.copyFrom(0, 0, tempBuffer, 0, 0, tempBuffer.getNumSamples());

            // FIX: Allow any WAV file length to produce sound (not just strict 2048 multiples)
            int totalSamples = tableData.getNumSamples();
            if (totalSamples >= 2048 && totalSamples % 2048 == 0) {
                frameSize = 2048;
                numFrames = totalSamples / 2048;
            }
            else {
                frameSize = std::max(1, totalSamples);
                numFrames = 1;
            }
        }
    }

    void generateSingleCycle(std::array<float, 512>& displayBuffer) {
        int totalSamples = tableData.getNumSamples();
        if (totalSamples == 0) { displayBuffer.fill(0.0f); return; }

        auto* ptr = tableData.getReadPointer(0);
        float framePos = wtPosition * (float)std::max(0, numFrames - 1);
        int frameIdx = (int)framePos;
        float frameFrac = framePos - (float)frameIdx;

        for (int i = 0; i < 512; ++i) {
            float phase = i / 512.0f;
            float mod = std::sin(phase * 2.0f * juce::MathConstants<float>::pi) * fmAmount * 0.1f;

            float syncPhase = std::fmod((phase + mod) * syncAmount, 1.0f);
            if (syncPhase < 0.0f) syncPhase += 1.0f;

            int samplePos = (int)(syncPhase * (frameSize - 1));
            int idx1 = juce::jlimit(0, totalSamples - 1, (frameIdx * frameSize) + samplePos);
            int idx2 = juce::jlimit(0, totalSamples - 1, ((frameIdx + 1) * frameSize) + samplePos);

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
    void resetPhase() { for (int i = 0; i < MaxVoices; ++i) phases[i] = 0.0f; }

    void getSampleStereo(float& outL, float& outR) {
        outL = 0.0f; outR = 0.0f;
        int totalSamples = tableData.getNumSamples();
        if (totalSamples == 0) return;

        auto* ptr = tableData.getReadPointer(0);

        for (int i = 0; i < unisonCount; ++i) {
            float mod = std::sin(phases[i] * 2.0f * juce::MathConstants<float>::pi) * fmAmount * 0.1f;
            float totalPhase = std::fmod((phases[i] + mod) * syncAmount, 1.0f);
            if (totalPhase < 0.0f) totalPhase += 1.0f;

            float framePos = wtPosition * (float)std::max(0, numFrames - 1);
            int frameIdx = (int)framePos;
            float frameFrac = framePos - (float)frameIdx;

            int samplePos = (int)(totalPhase * (frameSize - 1));
            int idx1 = juce::jlimit(0, totalSamples - 1, (frameIdx * frameSize) + samplePos);
            int idx2 = juce::jlimit(0, totalSamples - 1, ((frameIdx + 1) * frameSize) + samplePos);

            float val = ptr[idx1] * (1.0f - frameFrac) + ptr[idx2] * frameFrac;

            if (morphParam > 0.01f) {
                val *= (1.0f + morphParam * 4.0f);
                val = std::sin(val * juce::MathConstants<float>::halfPi);
            }

            phases[i] += increments[i];
            if (phases[i] >= 1.0f) phases[i] -= 1.0f;

            outL += val * panL[i] * amp[i];
            outR += val * panR[i] * amp[i];
        }
    }

private:
    double sampleRate = 44100.0;
    float baseFreq = 440.0f, detuneAmount = 0.0f, wtPosition = 0.0f;
    float fmAmount = 0.0f, syncAmount = 1.0f, morphParam = 0.0f;
    int unisonCount = 1, numFrames = 0, frameSize = 2048;

    juce::AudioBuffer<float> tableData;
    juce::AudioFormatManager formatManager;
    std::array<float, MaxVoices> phases, increments, panL, panR, amp;

    void recalculate() {
        float norm = 1.0f / std::sqrt((float)unisonCount);
        for (int i = 0; i < unisonCount; ++i) {
            float bias = (unisonCount == 1) ? 0.0f : (2.0f * i / (float)(unisonCount - 1) - 1.0f);
            increments[i] = (float)((baseFreq * std::pow(2.0f, (bias * detuneAmount * 0.5f) / 12.0f)) / sampleRate);

            float angle = (bias + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
            if (baseFreq < 150.0f) {
                float centerWeight = juce::jlimit(0.0f, 1.0f, baseFreq / 150.0f);
                angle = (0.25f * juce::MathConstants<float>::pi) + (angle - 0.25f * juce::MathConstants<float>::pi) * centerWeight;
            }
            panL[i] = std::cos(angle); panR[i] = std::sin(angle); amp[i] = norm;
        }
    }
};