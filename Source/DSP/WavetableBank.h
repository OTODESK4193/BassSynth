// ==============================================================================
// Source/DSP/WavetableBank.h
// ==============================================================================
#pragma once

// CMake環境ではアングルブラケットでの指定が最も安定します
#include <JuceHeader.h> 
#include <atomic>
#include "SpectralWavetable.h"
#include "../Generated/WavetableData_Generated.h"

class WavetableBank
{
public:
    WavetableBank() : backgroundPool(1) {}

    ~WavetableBank() {
        backgroundPool.removeAllJobs(true, 2000);
        cleanupOnMessageThread();

        SpectralWavetable* cur = currentWavetable.exchange(nullptr);
        if (cur != nullptr) delete cur;

        SpectralWavetable* pen = pendingWavetable.exchange(nullptr);
        if (pen != nullptr) delete pen;
    }

    void requestBuild(int waveIdx, juce::AudioFormatManager& formatManager,
        int modeA, float amtA, int modeB, float amtB, int modWave)
    {
        const int myJobId = ++jobCounter;

        backgroundPool.addJob([this, waveIdx, &formatManager, modeA, amtA, modeB, amtB, modWave, myJobId]() {

            juce::File tableFile = juce::File(EmbeddedWavetables::wavetablesDir).getChildFile(EmbeddedWavetables::allNames[waveIdx]);

            std::vector<std::vector<float>> rawFrames;

            if (tableFile.existsAsFile())
            {
                std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(tableFile));
                if (reader != nullptr)
                {
                    juce::AudioBuffer<float> tempBuffer(1, (int)reader->lengthInSamples);
                    reader->read(&tempBuffer, 0, (int)reader->lengthInSamples, 0, true, false);

                    int numFrames = std::max(1, tempBuffer.getNumSamples() / kTableSize);
                    rawFrames.resize((size_t)numFrames, std::vector<float>((size_t)kTableSize));

                    for (int f = 0; f < numFrames; ++f) {
                        for (int i = 0; i < kTableSize; ++i) {
                            int srcIdx = f * kTableSize + i;
                            rawFrames[(size_t)f][(size_t)i] = (srcIdx < tempBuffer.getNumSamples()) ? tempBuffer.getSample(0, srcIdx) : 0.0f;
                        }
                    }
                }
            }

            // 【診断ロジック】読み込み失敗時はサイン波ではなく「ノコギリ波」を出す
            // これが鳴れば「コードは動いているがパスが通っていない」と断定できます
            if (rawFrames.empty())
            {
                rawFrames.resize(1, std::vector<float>((size_t)kTableSize));
                for (int i = 0; i < kTableSize; ++i) {
                    rawFrames[0][(size_t)i] = ((float)i / (float)kTableSize) * 2.0f - 1.0f;
                }
            }

            if (myJobId != jobCounter.load()) return;

            SpectralWavetableBuilder builder;
            auto newWt = builder.build(rawFrames, modeA, amtA, modeB, amtB, modWave);

            if (myJobId == jobCounter.load())
            {
                SpectralWavetable* oldPending = pendingWavetable.exchange(newWt.release(), std::memory_order_release);
                if (oldPending != nullptr) {
                    toDelete.push(oldPending);
                }
            }
            });
    }

    void pollForUpdate()
    {
        SpectralWavetable* pending = pendingWavetable.exchange(nullptr, std::memory_order_acq_rel);
        if (pending != nullptr)
        {
            SpectralWavetable* oldCurrent = currentWavetable.exchange(pending, std::memory_order_release);
            if (oldCurrent != nullptr) {
                toDelete.push(oldCurrent);
            }
        }
    }

    const SpectralWavetable* get() const
    {
        return currentWavetable.load(std::memory_order_acquire);
    }

    void cleanupOnMessageThread()
    {
        SpectralWavetable* old = nullptr;
        while (toDelete.pop(old))
        {
            if (old != nullptr) delete old;
        }
    }

private:
    juce::ThreadPool backgroundPool;
    std::atomic<int> jobCounter{ 0 };
    std::atomic<SpectralWavetable*> currentWavetable{ nullptr };
    std::atomic<SpectralWavetable*> pendingWavetable{ nullptr };

    struct SimpleQueue {
        static constexpr int Capacity = 64;
        std::array<SpectralWavetable*, Capacity> buf{};
        std::atomic<int> head{ 0 }, tail{ 0 }; // 0で初期化

        void push(SpectralWavetable* p)
        {
            if (p == nullptr) return;
            int t = tail.load(std::memory_order_relaxed);
            SpectralWavetable* existing = buf[(size_t)(t % Capacity)];
            if (existing != nullptr) delete existing;
            buf[(size_t)(t % Capacity)] = p;
            tail.store(t + 1, std::memory_order_release);
        }

        bool pop(SpectralWavetable*& p)
        {
            int h = head.load(std::memory_order_relaxed);
            if (h == tail.load(std::memory_order_acquire)) return false;
            p = buf[(size_t)(h % Capacity)];
            buf[(size_t)(h % Capacity)] = nullptr;
            head.store(h + 1, std::memory_order_release);
            return true;
        }
    } toDelete;
};