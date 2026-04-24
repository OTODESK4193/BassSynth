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

    // ★ 変更: trigMode を追加
    void setParameters(int wave, bool sync, float rate, int beat, float amt, int tMode) {
        waveform = std::clamp(wave, 0, 3);
        isSync = sync;
        rateHz = rate;
        beatIdx = beat;
        depth = std::clamp(amt, 0.0f, 1.0f);
        trigMode = std::clamp(tMode, 0, 2);
    }

    void noteOn(bool isLegato = false) {
        if (isLegato) return;
        if (trigMode == 0) return; // Free モード時は位相リセットを無視

        phase = 0.0f;
        oneShotDone = false;
        lastRandomVal = nextRandomVal;
        nextRandomVal = juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
    }

    inline float getNextSample(double bpm) {
        if (trigMode == 2 && oneShotDone) {
            return getWaveformValue(1.0f) * depth; // One-Shot 完了時は終端の値を維持
        }

        float freq = rateHz;

        if (isSync && bpm > 0.0) {
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
            freq = static_cast<float>(bpm / 60.0) / beatsPerCycle;
        }

        float inc = freq / static_cast<float>(sampleRate);
        phase += inc;

        if (phase >= 1.0f) {
            if (trigMode == 2) {
                phase = 1.0f;
                oneShotDone = true; // One-Shot モード: 1周期で停止
            }
            else {
                phase -= std::floor(phase);
                lastRandomVal = nextRandomVal;
                nextRandomVal = juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
            }
        }

        return getWaveformValue(phase) * depth;
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
    int trigMode = 0;     // 0:Free, 1:Retrig, 2:One-Shot
    bool oneShotDone = false;

    inline float getWaveformValue(float p) const {
        float out = 0.0f;
        switch (waveform) {
        case 0: out = std::sin(p * 2.0f * std::numbers::pi_v<float>); break;
        case 1: out = 1.0f - 2.0f * p; break;
        case 2: out = p < 0.5f ? 1.0f : -1.0f; break;
        case 3: out = lastRandomVal; break;
        }
        return out;
    }
};