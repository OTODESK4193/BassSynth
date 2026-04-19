// ==============================================================================
// Source/PluginProcessor.h
// ==============================================================================
#pragma once
#include <JuceHeader.h>
#include "DSP/WavetableOscillator.h"
#include "DSP/SpectralMorphProcessor.h"
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
        // 1. オシレーターから時間領域Morph適用済みの基本波形を取得
        oscillator.generateSingleCycle(buffer);

        // 2. 現在のSpectral Morphのパラメータを取得
        int mA = (int)pMorphAMode->load(std::memory_order_relaxed);
        float aA = pMorphAAmt->load(std::memory_order_relaxed);
        int mB = (int)pMorphBMode->load(std::memory_order_relaxed);
        float aB = pMorphBAmt->load(std::memory_order_relaxed);

        // 3. Spectral MorphingをGUI用の波形に適用
        spectralMorph.processSingleCycleForDisplay(buffer, mA, aA, mB, aB);
    }

private:
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    WavetableOscillator oscillator;
    SpectralMorphProcessor spectralMorph;
    LadderFilter filter;
    SineShaper shaper;
    MonoVoiceManager voiceManager;
    AdsrEnvelope ampEnv, filterEnv;

    std::array<float, 512> outputScopeData;
    int scopeWriteIndex = 0;

    // --- OSC Parameters ---
    std::atomic<float>* pWave = nullptr;
    std::atomic<float>* pPos = nullptr;
    std::atomic<float>* pOscLevel = nullptr;
    std::atomic<float>* pOscPitch = nullptr;
    std::atomic<float>* pPDecayAmt = nullptr;
    std::atomic<float>* pPDecayTime = nullptr;
    std::atomic<float>* pFm = nullptr;
    std::atomic<float>* pFmWave = nullptr;
    std::atomic<float>* pSync = nullptr;

    // Dual Morph Parameters
    std::atomic<float>* pMorphAMode = nullptr;
    std::atomic<float>* pMorphAAmt = nullptr;
    std::atomic<float>* pMorphBMode = nullptr;
    std::atomic<float>* pMorphBAmt = nullptr;

    std::atomic<float>* pUni = nullptr;
    std::atomic<float>* pDetune = nullptr;
    std::atomic<float>* pWidth = nullptr;
    std::atomic<float>* pDrift = nullptr;

    // --- Sub Parameters ---
    std::atomic<float>* pSubOn = nullptr;
    std::atomic<float>* pSubWave = nullptr;
    std::atomic<float>* pSubVol = nullptr;
    std::atomic<float>* pSubPitch = nullptr;

    // --- Filter / Drive / Envelope ---
    std::atomic<float>* pCutoff = nullptr;
    std::atomic<float>* pReso = nullptr;
    std::atomic<float>* pFltEnvAmt = nullptr;
    std::atomic<float>* pDrive = nullptr;
    std::atomic<float>* pShpAmt = nullptr;
    std::atomic<float>* pShpRate = nullptr;
    std::atomic<float>* pShpBit = nullptr;
    std::atomic<float>* pGain = nullptr;
    std::atomic<float>* pGlide = nullptr;

    std::atomic<float>* pAAtk = nullptr;
    std::atomic<float>* pADec = nullptr;
    std::atomic<float>* pASus = nullptr;
    std::atomic<float>* pARel = nullptr;
    std::atomic<float>* pFAtk = nullptr;
    std::atomic<float>* pFDec = nullptr;
    std::atomic<float>* pFSus = nullptr;
    std::atomic<float>* pFRel = nullptr;

    // --- Smoothed Values ---
    juce::SmoothedValue<float> smoothedWtLevel, smoothedWtPitch, smoothedPDecayAmt, smoothedPDecayTime;
    juce::SmoothedValue<float> smoothedCutoff, smoothedReso, smoothedFltEnvAmt, smoothedDrive, smoothedShpAmt, smoothedShpRate, smoothedShpBit, smoothedGain;
    juce::SmoothedValue<float> smoothedWtPos, smoothedFm, smoothedSync, smoothedDrift, smoothedSubVol, smoothedSubPitch, smoothedWidth;
    juce::SmoothedValue<float> smoothedMorphAAmt, smoothedMorphBAmt;

    float lastOscFreq = -1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LiquidDreamAudioProcessor)
};