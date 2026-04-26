// ==============================================================================
// Source/Logic/AdsrEnvelope.h
// ==============================================================================
#pragma once
#include <JuceHeader.h>
#include <algorithm>

class AdsrEnvelope
{
public:
    enum class State { Idle, Declick, Attack, Decay, Sustain, Release };

    AdsrEnvelope() = default;

    void setSampleRate(double sr) {
        sampleRate = std::max(1.0, sr);
    }

    void setParameters(float a, float d, float s, float r) {
        attackRate = 1.0f / (a * (float)sampleRate);
        decayRate = 1.0f / (d * (float)sampleRate);
        sustainLevel = juce::jlimit(0.0f, 1.0f, s);
        releaseRate = 1.0f / (r * (float)sampleRate);
    }

    void noteOn(bool isLegato = false) {
        if (isLegato) return; // Legato時はエンベロープを再トリガーしない

        if (state != State::Idle && currentValue > 0.001f) {
            // 前の音が鳴っている場合は、ノイズ防止のため2msの高速フェードアウトへ移行
            state = State::Declick;
            declickRate = currentValue / (0.002f * (float)sampleRate);
        }
        else {
            // 完全に無音の場合は即座にアタック開始
            state = State::Attack;
            currentValue = 0.0f;
            justResetFlag = true;
        }
    }

    void noteOff() {
        if (state != State::Idle && state != State::Declick) {
            state = State::Release;
        }
    }

    // オシレーターの位相リセットタイミングを通知するフラグ
    bool popJustReset() {
        bool result = justResetFlag;
        justResetFlag = false;
        return result;
    }

    float getNextSample() {
        switch (state) {
        case State::Declick:
            currentValue -= declickRate;
            if (currentValue <= 0.0f) {
                currentValue = 0.0f;
                state = State::Attack;
                justResetFlag = true; // 完全にゼロになった瞬間、位相リセットを指示
            }
            break;
        case State::Attack:
            currentValue += attackRate;
            if (currentValue >= 1.0f) {
                currentValue = 1.0f;
                state = State::Decay;
            }
            break;
        case State::Decay:
            currentValue -= decayRate;
            if (currentValue <= sustainLevel) {
                currentValue = sustainLevel;
                state = State::Sustain;
            }
            break;
        case State::Sustain:
            break;
        case State::Release:
            currentValue -= releaseRate;
            if (currentValue <= 0.0f) {
                currentValue = 0.0f;
                state = State::Idle;
            }
            break;
        case State::Idle:
            currentValue = 0.0f;
            break;
        }
        return currentValue;
    }

private:
    double sampleRate = 44100.0;
    State state = State::Idle;
    float currentValue = 0.0f;

    float attackRate = 0.0f;
    float decayRate = 0.0f;
    float sustainLevel = 1.0f;
    float releaseRate = 0.0f;
    float declickRate = 0.0f;

    bool justResetFlag = false;
};