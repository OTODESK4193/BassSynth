#pragma once
#include <JuceHeader.h>
#include <cmath>
#include <algorithm>

class AdsrEnvelope
{
public:
    enum class State { Idle, Attack, Decay, Sustain, Release };

    AdsrEnvelope() = default;

    void setSampleRate(double sr) { sampleRate = std::max(1.0, sr); }

    void setParameters(float a, float d, float s, float r)
    {
        sustainLevel = std::clamp(s, 0.0f, 1.0f);
        attackInc = 1.0f / (std::max(0.001f, a) * (float)sampleRate);
        decayCoef = std::exp(-std::log(1000.0f) / (std::max(0.001f, d) * (float)sampleRate));
        releaseCoef = std::exp(-std::log(1000.0f) / (std::max(0.001f, r) * (float)sampleRate));
    }

    void noteOn() { state = State::Attack; }
    void noteOff() { if (state != State::Idle) state = State::Release; }

    inline float getNextSample()
    {
        switch (state) {
        case State::Idle: output = 0.0f; break;
        case State::Attack:
            output += attackInc;
            if (output >= 1.0f) { output = 1.0f; state = State::Decay; }
            break;
        case State::Decay:
            output = (output - sustainLevel) * decayCoef + sustainLevel;
            break;
        case State::Sustain:
            output = sustainLevel;
            break;
        case State::Release:
            output *= releaseCoef;
            if (output < 0.0001f) { output = 0.0f; state = State::Idle; }
            break;
        }
        return output;
    }

private:
    double sampleRate = 44100.0;
    State state = State::Idle;
    float output = 0.0f, sustainLevel = 1.0f;
    float attackInc = 0.0f, decayCoef = 0.0f, releaseCoef = 0.0f;
};


