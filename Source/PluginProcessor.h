#pragma once
#include <JuceHeader.h>
#include "DSP/WavetableOscillator.h"
#include "DSP/LadderFilter.h"
#include "DSP/SineShaper.h"
#include "Logic/MonoVoiceManager.h"
#include "Logic/AdsrEnvelope.h"
#include "Generated/WavetableData_Generated.h"

class LiquidDreamAudioProcessor : public juce::AudioProcessor
{
public:
    LiquidDreamAudioProcessor();
    ~LiquidDreamAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "BassSynth"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override {}
    const juce::String getProgramName(int index) override { return {}; }
    void changeProgramName(int index, const juce::String& newName) override {}
    void getStateInformation(juce::MemoryBlock& destData) override {}
    void setStateInformation(const void* data, int sizeInBytes) override {}

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    float* getOutputScopePtr() { return outputScopeData.data(); }

    void getStaticWaveform(std::array<float, 512>& buffer) {
        oscillator.generateSingleCycle(buffer);
    }

private:
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    WavetableOscillator oscillator;
    LadderFilter filter;
    SineShaper shaper;
    MonoVoiceManager voiceManager;
    AdsrEnvelope ampEnv, filterEnv;

    std::array<float, 512> outputScopeData;
    int scopeWriteIndex = 0;

    std::atomic<float>* pWave = nullptr;
    std::atomic<float>* pPos = nullptr;
    std::atomic<float>* pFm = nullptr;
    std::atomic<float>* pSync = nullptr;
    std::atomic<float>* pMorph = nullptr;
    std::atomic<float>* pUni = nullptr;
    std::atomic<float>* pDetune = nullptr;
    std::atomic<float>* pOscPitch = nullptr;

    std::atomic<float>* pGlide = nullptr;
    std::atomic<float>* pAAtk = nullptr;
    std::atomic<float>* pADec = nullptr;
    std::atomic<float>* pASus = nullptr;
    std::atomic<float>* pARel = nullptr;
    std::atomic<float>* pFAtk = nullptr;
    std::atomic<float>* pFDec = nullptr;
    std::atomic<float>* pFSus = nullptr;
    std::atomic<float>* pFRel = nullptr;

    std::atomic<float>* pCutoff = nullptr;
    std::atomic<float>* pReso = nullptr;
    std::atomic<float>* pFltEnvAmt = nullptr;
    std::atomic<float>* pDrive = nullptr;
    std::atomic<float>* pShpAmt = nullptr;
    std::atomic<float>* pShpRate = nullptr;
    std::atomic<float>* pShpBit = nullptr;
    std::atomic<float>* pGain = nullptr;

    juce::SmoothedValue<float> smoothedPitchMult;
    juce::SmoothedValue<float> smoothedCutoff;
    juce::SmoothedValue<float> smoothedReso;
    juce::SmoothedValue<float> smoothedFltEnvAmt;
    juce::SmoothedValue<float> smoothedDrive;
    juce::SmoothedValue<float> smoothedShpAmt;
    juce::SmoothedValue<float> smoothedShpRate;
    juce::SmoothedValue<float> smoothedShpBit;
    juce::SmoothedValue<float> smoothedGain;

    // --- 追加：Wavetable系パラメータのスムージング用 ---
    juce::SmoothedValue<float> smoothedWtPos;
    juce::SmoothedValue<float> smoothedFm;
    juce::SmoothedValue<float> smoothedSync;
    juce::SmoothedValue<float> smoothedMorph;

    // --- 追加：CPU負荷対策のためのキャッシュ ---
    float lastOscFreq = -1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LiquidDreamAudioProcessor)
};