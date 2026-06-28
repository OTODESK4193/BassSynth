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
        if (voices.size() == 0) return;
        voices[0].loadFactoryWavetable(index);
        auto sharedSet = voices[0].getWavetableSet();
        for (size_t i = 1; i < voices.size(); ++i) {
            voices[i].setWavetableSet(sharedSet);
        }
    }

    void loadCustomWavetable(const juce::File& file) {
        if (voices.size() == 0) return;
        voices[0].loadCustomWavetable(file);
        auto sharedSet = voices[0].getWavetableSet();
        for (size_t i = 1; i < voices.size(); ++i) {
            voices[i].setWavetableSet(sharedSet);
        }
    }

    void updateMsegStates(const MsegState& state0, const MsegState& state1) {
        for (auto& voice : voices) {
            voice.updateMsegStates(state0, state1);
        }
    }

    void noteOn(int noteNumber, float velocity, bool legatoEnabled) {
        // すでに同じノートが鳴っているボイスがあればリリースに移行させる（多重発音防止）
        for (auto& voice : voices) {
            if (voice.getIsActive() && voice.getNoteNumber() == noteNumber) {
                voice.noteOff(false);
            }
        }

        // --- レガート制御 ---
        // レガート有効（または発音数制限が1のモノフォニック時）で、すでにアクティブなボイスがあれば
        if ((legatoEnabled || maxVoices == 1) && hasActiveVoices()) {
            int activeVoiceIdx = -1;
            uint32_t latestTime = 0;
            // 最も新しく発音されたアクティブなボイスを探す
            for (int i = 0; i < maxVoices; ++i) {
                if (voices[i].getIsActive() && voices[i].getOnTime() > latestTime) {
                    latestTime = voices[i].getOnTime();
                    activeVoiceIdx = i;
                }
            }

            if (activeVoiceIdx != -1) {
                // 既存のボイスを isLegato = true にして再トリガー（ピッチ・エンベロープ等の滑らかな接続）
                voices[activeVoiceIdx].noteOn(noteNumber, velocity, true);
                return;
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

        for (int i = 0; i < maxVoices; ++i) {
            if (voices[i].getIsActive()) {
                voices[i].renderNextBlock(outputBuffer, bpm);
            }
        }

        // 固定ヘッドルームゲインの適用（ブロック境界のゲイン不連続クリックノイズを100%防止）
        outputBuffer.applyGain(0.25f);
    }

    void setUIParams(float pos, float fmAmt, int fmWave) {
        voices[0].setUIParams(pos, fmAmt, fmWave);
    }

    // ★ 追加: 表示用(voices[0])オシレーターへMorphを反映
    void setUIMorph(int mA, float aA, float sA, int mB, float aB, float sB, int mC, float aC, float sC) {
        voices[0].setUIMorph(mA, aA, sA, mB, aB, sB, mC, aC, sC);
    }

    float getLastModPos() const { return voices[0].getLastModPos(); }

    bool hasActiveVoices() const {
        for (const auto& voice : voices) {
            if (voice.getIsActive()) return true;
        }
        return false;
    }

    void getMorphValues(float& aA, float& sA, float& aB, float& sB, float& aC, float& sC) const {
        voices[0].getMorphValues(aA, sA, aB, sB, aC, sC);
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
