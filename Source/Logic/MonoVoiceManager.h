// ==============================================================================
// Source/Logic/MonoVoiceManager.h
// ==============================================================================
#pragma once
#include <JuceHeader.h>
#include <vector>
#include <algorithm>

class MonoVoiceManager
{
public:
    MonoVoiceManager() = default;

    void setSampleRate(double sr) {
        sampleRate = std::max(1.0, sr);
    }

    void setGlideTime(float timeMs) {
        glideTimeMs = timeMs;
        if (timeMs <= 0.001f) {
            glideCoef = 0.0f;
        }
        else {
            glideCoef = std::exp(-std::log(1000.0f) / (timeMs * 0.001f * (float)sampleRate));
        }
    }

    void setLegatoMode(bool on) {
        legatoMode = on;
    }

    bool isLegatoTransition() const {
        return legatoTransition;
    }

    bool noteOn(int noteNumber, uint8_t velocity) {
        juce::ignoreUnused(velocity);

        // 既に押されているノートを削除して末尾に追加（最新優先）
        auto it = std::remove(heldNotes.begin(), heldNotes.end(), noteNumber);
        heldNotes.erase(it, heldNotes.end());
        heldNotes.push_back(noteNumber);

        targetFrequency = (float)juce::MidiMessage::getMidiNoteInHertz(noteNumber);

        if (legatoMode && heldNotes.size() > 1) {
            // ノートが重なっている場合はLegato発動
            legatoTransition = true;
        }
        else {
            // 最初のノート、またはLegatoオフの場合は通常発音
            legatoTransition = false;
            if (glideTimeMs <= 0.001f || heldNotes.size() == 1) {
                currentFrequency = targetFrequency;
            }
        }

        return true;
    }

    bool noteOff(int noteNumber) {
        auto it = std::remove(heldNotes.begin(), heldNotes.end(), noteNumber);
        heldNotes.erase(it, heldNotes.end());

        if (!heldNotes.empty()) {
            // 他のノートがまだ押されている場合は、その音程へ戻る
            targetFrequency = (float)juce::MidiMessage::getMidiNoteInHertz(heldNotes.back());
            if (legatoMode) legatoTransition = true;
            return false; // エンベロープはReleaseしない
        }

        return true; // 全てのキーが離されたのでReleaseする
    }

    float getCurrentFrequency() {
        if (glideCoef > 0.0f) {
            currentFrequency = targetFrequency + (currentFrequency - targetFrequency) * glideCoef;
        }
        else {
            currentFrequency = targetFrequency;
        }
        return currentFrequency;
    }

private:
    double sampleRate = 44100.0;
    float targetFrequency = 440.0f;
    float currentFrequency = 440.0f;
    float glideTimeMs = 0.0f;
    float glideCoef = 0.0f;

    bool legatoMode = false;
    bool legatoTransition = false;

    std::vector<int> heldNotes;
};