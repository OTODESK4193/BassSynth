#pragma once
#include <JuceHeader.h>
#include <array>
#include <cmath>

class MonoVoiceManager
{
public:
    enum class LegatoMode { Retrigger, Legato };

    MonoVoiceManager() { reset(); }

    void reset()
    {
        stack.fill(-1);
        activeNoteCount = 0;
        currentFrequency = 440.0f;
        targetFrequency = 440.0f;
    }

    void setSampleRate(double sr) { sampleRate = std::max(1.0, sr); }

    void setGlideTime(float seconds)
    {
        if (seconds <= 0.001f) {
            glideCoeff = 1.0f;
        }
        else {
            float tau = seconds * 0.25f;
            glideCoeff = 1.0f - std::exp(-1.0f / (float)(tau * sampleRate));
        }
    }

    void setLegatoMode(bool isLegato) { mode = isLegato ? LegatoMode::Legato : LegatoMode::Retrigger; }

    bool noteOn(int noteNumber, int velocity)
    {
        juce::ignoreUnused(velocity);
        bool wasStackEmpty = (activeNoteCount == 0);
        pushToStack(noteNumber);

        targetFrequency = (float)juce::MidiMessage::getMidiNoteInHertz(noteNumber, 440.0);

        if (wasStackEmpty) {
            currentFrequency = targetFrequency;
            return true; // Trigger envelopes
        }
        return mode != LegatoMode::Legato;
    }

    bool noteOff(int noteNumber)
    {
        removeFromStack(noteNumber);
        if (activeNoteCount > 0) {
            int returnNote = getTopNote();
            if (returnNote != -1) {
                targetFrequency = (float)juce::MidiMessage::getMidiNoteInHertz(returnNote, 440.0);
                return false; // Do not trigger release
            }
        }
        return true; // Trigger release
    }

    inline float getCurrentFrequency()
    {
        if (std::abs(targetFrequency - currentFrequency) > 0.001f) {
            currentFrequency += (targetFrequency - currentFrequency) * glideCoeff;
        }
        else {
            currentFrequency = targetFrequency;
        }
        return currentFrequency;
    }

private:
    static constexpr int MAX_STACK = 16;
    std::array<int, MAX_STACK> stack;
    int activeNoteCount = 0;

    double sampleRate = 44100.0;
    float currentFrequency = 440.0f, targetFrequency = 440.0f;
    float glideCoeff = 1.0f;
    LegatoMode mode = LegatoMode::Retrigger;

    void pushToStack(int note)
    {
        removeFromStack(note);
        if (activeNoteCount < MAX_STACK) {
            stack[activeNoteCount] = note;
            activeNoteCount++;
        }
    }

    void removeFromStack(int note)
    {
        for (int i = 0; i < activeNoteCount; ++i) {
            if (stack[i] == note) {
                for (int j = i; j < activeNoteCount - 1; ++j) stack[j] = stack[j + 1];
                activeNoteCount--;
                i--;
            }
        }
    }

    int getTopNote() const { return (activeNoteCount > 0) ? stack[activeNoteCount - 1] : -1; }
};


