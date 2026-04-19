// ==============================================================================
// Source/Logic/Lfo.h
// ==============================================================================
#pragma once
#include <JuceHeader.h>
#include <cmath>
#include <numbers>
#include <algorithm>

class Lfo
{
public:
    Lfo() {
        randomPhase = juce::Random::getSystemRandom().nextFloat();
        lastRandomVal = juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
        nextRandomVal = juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
    }

    void setSampleRate(double sr) {
        sampleRate = std::max(1.0, sr);
    }

    void setParameters(int wave, bool sync, float rate, int beat, float amt) {
        waveform = std::clamp(wave, 0, 3);
        isSync = sync;
        rateHz = rate;
        beatIdx = beat;
        depth = std::clamp(amt, 0.0f, 1.0f);
    }

    void noteOn(bool isLegato = false) {
        if (isLegato) return;
        phase = 0.0f;
        lastRandomVal = nextRandomVal;
        nextRandomVal = juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
    }

    // bpmはDAWから取得。取得できない場合は0が渡され、内部でデフォルトHzにフォールバックします
    inline float getNextSample(double bpm) {
        float freq = rateHz;

        if (isSync && bpm > 0.0) {
            // beatIdx: 0="1/1", 1="1/2", 2="1/4", 3="1/8", 4="1/16", 5="1/32", 6="1/4T", 7="1/8T", 8="1/16T"
            float beatsPerCycle = 1.0f;
            switch (beatIdx) {
            case 0: beatsPerCycle = 4.0f; break;
            case 1: beatsPerCycle = 2.0f; break;
            case 2: beatsPerCycle = 1.0f; break;
            case 3: beatsPerCycle = 0.5f; break;
            case 4: beatsPerCycle = 0.25f; break;
            case 5: beatsPerCycle = 0.125f; break;
            case 6: beatsPerCycle = 2.0f / 3.0f; break;
            case 7: beatsPerCycle = 1.0f / 3.0f; break;
            case 8: beatsPerCycle = 0.5f / 3.0f; break;
            }
            freq = (float)(bpm / 60.0) / beatsPerCycle;
        }

        // Phase Advance
        float inc = freq / (float)sampleRate;
        phase += inc;

        if (phase >= 1.0f) {
            phase -= std::floor(phase);
            lastRandomVal = nextRandomVal;
            nextRandomVal = juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
        }

        float out = 0.0f;
        switch (waveform) {
        case 0: // Sine
            out = std::sin(phase * 2.0f * std::numbers::pi_v<float>);
            break;
        case 1: // Saw (Down)
            out = 1.0f - 2.0f * phase;
            break;
        case 2: // Pulse (Square)
            out = phase < 0.5f ? 1.0f : -1.0f;
            break;
        case 3: // Random (S&H)
            out = lastRandomVal;
            break;
        }

        return out * depth;
    }

private:
    double sampleRate = 44100.0;
    float phase = 0.0f, randomPhase = 0.0f;
    float lastRandomVal = 0.0f, nextRandomVal = 0.0f;

    int waveform = 0;     // 0:Sine, 1:Saw, 2:Pulse, 3:Random
    bool isSync = false;
    float rateHz = 1.0f;
    int beatIdx = 2;      // 1/4
    float depth = 1.0f;
};