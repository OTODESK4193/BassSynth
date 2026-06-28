#pragma once
#include <JuceHeader.h>
#include <vector>
#include <array>
#include <algorithm>
#include "BassSynthVoice.h"

class PolyVoiceManager
{
public:
    PolyVoiceManager() = default;

    void init(const VoiceParams& params) {
        for (auto& voice : voices) {
            voice.init(params);
        }
    }

    void prepare(double sampleRate, int maxBlockSize) {
        for (auto& voice : voices) {
            voice.prepare(sampleRate, maxBlockSize);
        }
    }

    void setMaxVoices(int numVoices) {
        maxVoices = std::clamp(numVoices, 1, 32);
    }

    void loadFactoryWavetable(int index) {
        for (auto& voice : voices) {
            voice.loadFactoryWavetable(index);
        }
    }

    void loadCustomWavetable(const juce::File& file) {
        for (auto& voice : voices) {
            voice.loadCustomWavetable(file);
        }
    }

    void updateMsegStates(const MsegState& state0, const MsegState& state1) {
        for (auto& voice : voices) {
            voice.updateMsegStates(state0, state1);
        }
    }

    void noteOn(int noteNumber, float velocity) {
        // すでに同じノートが鳴っているボイスがあればリリースに移行させる（多重発音防止）
        for (auto& voice : voices) {
            if (voice.getIsActive() && voice.getNoteNumber() == noteNumber) {
                voice.noteOff(false);
            }
        }

        // 1. 空いている（非アクティブ）ボイスを探す
        for (int i = 0; i < maxVoices; ++i) {
            if (!voices[i].getIsActive()) {
                voices[i].noteOn(noteNumber, velocity, false);
                return;
            }
        }

        // 2. 空きがない場合、発音中のボイスの中から「最も古い」ボイスを奪う (Voice Stealing)
        int oldestVoiceIndex = -1;
        uint32_t oldestTime = std::numeric_limits<uint32_t>::max();

        for (int i = 0; i < maxVoices; ++i) {
            if (voices[i].getIsActive() && voices[i].getOnTime() < oldestTime) {
                oldestTime = voices[i].getOnTime();
                oldestVoiceIndex = i;
            }
        }

        if (oldestVoiceIndex != -1) {
            // 最も古いボイスを即座に再起動
            voices[oldestVoiceIndex].noteOn(noteNumber, velocity, false);
        }
    }

    void noteOff(int noteNumber) {
        for (auto& voice : voices) {
            if (voice.getIsActive() && voice.getNoteNumber() == noteNumber) {
                voice.noteOff(false);
            }
        }
    }

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, double bpm) {
        // 出力バッファをクリア（各ボイスがここへ加算描画します）
        outputBuffer.clear();

        int activeCount = 0;
        for (int i = 0; i < maxVoices; ++i) {
            if (voices[i].getIsActive()) {
                voices[i].renderNextBlock(outputBuffer, bpm);
                activeCount++;
            }
        }

        if (activeCount > 0) {
            outputBuffer.applyGain(0.4f);
        }
    }

    Mseg& getMsegEngine(int index) {
        return voices[0].getMsegEngine(index);
    }

    void generateSingleCycle(std::array<float, 512>& buffer) {
        // 表示用のシングルサイクル波形は、最初のボイスのオシレーターから取得します
        voices[0].generateSingleCycle(buffer);
    }

private:
    std::array<BassSynthVoice, 32> voices;
    int maxVoices = 12;
};
