// ==============================================================================
// Source/PluginProcessor.h
// ==============================================================================
#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>
#include <mutex>
#include "DSP/WavetableOscillator.h"
#include "DSP/SpectralMorphProcessor.h"
#include "DSP/DualFilterEngine.h"
#include "DSP/SineShaper.h"
#include "DSP/ColorIREngine.h"
#include "Logic/MonoVoiceManager.h"
#include "Logic/AdsrEnvelope.h"
#include "Logic/Lfo.h"
#include "Logic/Mseg.h" // ★ 追加: MSEGコア
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

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    float* getOutputScopePtr() { return outputScopeData.data(); }

    void getStaticWaveform(std::array<float, 512>& buffer) {
        oscillator.generateSingleCycle(buffer);
        int mA = (int)pMorphAMode->load(std::memory_order_relaxed);
        float aA = pMorphAAmt->load(std::memory_order_relaxed);
        float sA = pMorphAShift->load(std::memory_order_relaxed);
        int mB = (int)pMorphBMode->load(std::memory_order_relaxed);
        float aB = pMorphBAmt->load(std::memory_order_relaxed);
        float sB = pMorphBShift->load(std::memory_order_relaxed);
        int mC = (int)pMorphCMode->load(std::memory_order_relaxed);
        float aC = pMorphCAmt->load(std::memory_order_relaxed);
        float sC = pMorphCShift->load(std::memory_order_relaxed);
        spectralMorph.processSingleCycleForDisplay(buffer, mA, aA, sA, mB, aB, sB, mC, aC, sC);
    }

    ColorIREngine& getColorEngine() { return colorEngine; }

    std::vector<int> getActiveMidiNotes() {
        std::lock_guard<std::mutex> lock(midiNotesMutex);
        return activeMidiNotes;
    }

    void loadCustomWavetable(const juce::File& file);
    void loadFactoryWavetable(int index);
    void setUserFolders(const juce::StringArray& folders);
    juce::StringArray getUserFolders() const { return userWavetableFolders; }

    // ★ 追加: GUI から MSEG データを取得・更新するための窓口
    MsegState msegStates[2];
    Mseg& getMsegEngine(int index) { return msegs[index]; }

private:
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    WavetableOscillator oscillator;
    SpectralMorphProcessor spectralMorph;
    DualFilterEngine dualFilter;
    SineShaper shaper;
    ColorIREngine colorEngine;
    MonoVoiceManager voiceManager;

    AdsrEnvelope ampEnv, filterEnvA, filterEnvB;
    std::array<AdsrEnvelope, 3> modEnvs;
    std::array<Lfo, 3> lfos;
    std::array<Mseg, 2> msegs; // ★ 追加: 2基の MSEG

    std::array<float, 512> outputScopeData;
    std::array<float, 512> tempScopeBuffer;
    int scopeWriteIndex = 0;

    juce::AudioBuffer<float> tempEnvBuffer;
    juce::AudioBuffer<float> tempSubBuffer;
    juce::AudioBuffer<float> tempWavetableBuffer;

    std::vector<int> activeMidiNotes;
    std::mutex midiNotesMutex;

    juce::String currentCustomWavetablePath;
    juce::StringArray userWavetableFolders;
    std::atomic<bool> customWavetableLoaded{ false };

    // Core Params
    std::atomic<float>* pOscOn = nullptr; std::atomic<float>* pWave = nullptr; std::atomic<float>* pPos = nullptr;
    std::atomic<float>* pOscLevel = nullptr; std::atomic<float>* pOscPitch = nullptr;
    std::atomic<float>* pPDecayAmt = nullptr; std::atomic<float>* pPDecayTime = nullptr;
    std::atomic<float>* pFm = nullptr; std::atomic<float>* pFmWave = nullptr;
    std::atomic<float>* pMorphAMode = nullptr; std::atomic<float>* pMorphAAmt = nullptr; std::atomic<float>* pMorphAShift = nullptr;
    std::atomic<float>* pMorphBMode = nullptr; std::atomic<float>* pMorphBAmt = nullptr; std::atomic<float>* pMorphBShift = nullptr;
    std::atomic<float>* pMorphCMode = nullptr; std::atomic<float>* pMorphCAmt = nullptr; std::atomic<float>* pMorphCShift = nullptr;
    std::atomic<float>* pUni = nullptr; std::atomic<float>* pDetune = nullptr; std::atomic<float>* pWidth = nullptr; std::atomic<float>* pDrift = nullptr;
    std::atomic<float>* pSubOn = nullptr; std::atomic<float>* pSubWave = nullptr; std::atomic<float>* pSubVol = nullptr; std::atomic<float>* pSubPitch = nullptr;

    std::atomic<float>* pFltAType = nullptr; std::atomic<float>* pFltACutoff = nullptr; std::atomic<float>* pFltAReso = nullptr;
    std::atomic<float>* pFltBType = nullptr; std::atomic<float>* pFltBCutoff = nullptr; std::atomic<float>* pFltBReso = nullptr;
    std::atomic<float>* pFltRouting = nullptr; std::atomic<float>* pFltMix = nullptr;
    std::atomic<float>* pFltAEnvAmt = nullptr; std::atomic<float>* pFltBEnvAmt = nullptr;

    std::atomic<float>* pDrive = nullptr; std::atomic<float>* pShpAmt = nullptr; std::atomic<float>* pShpRate = nullptr; std::atomic<float>* pShpBit = nullptr;
    std::atomic<float>* pGain = nullptr; std::atomic<float>* pGlide = nullptr; std::atomic<float>* pLegato = nullptr;
    std::atomic<float>* pAAtk = nullptr; std::atomic<float>* pADec = nullptr; std::atomic<float>* pASus = nullptr; std::atomic<float>* pARel = nullptr;
    std::atomic<float>* pFAtkA = nullptr; std::atomic<float>* pFDecA = nullptr; std::atomic<float>* pFSusA = nullptr; std::atomic<float>* pFRelA = nullptr;
    std::atomic<float>* pFAtkB = nullptr; std::atomic<float>* pFDecB = nullptr; std::atomic<float>* pFSusB = nullptr; std::atomic<float>* pFRelB = nullptr;

    std::atomic<float>* pColorOn = nullptr; std::atomic<float>* pColorType = nullptr;
    std::atomic<float>* pColorMix = nullptr; std::atomic<float>* pColorIrVol = nullptr;
    std::atomic<float>* pColorPreHp = nullptr; std::atomic<float>* pColorPostHp = nullptr;
    std::atomic<float>* pColorAtk = nullptr; std::atomic<float>* pColorDec = nullptr;
    std::atomic<float>* pOttDepth = nullptr; std::atomic<float>* pOttTime = nullptr;
    std::atomic<float>* pOttUp = nullptr; std::atomic<float>* pOttDown = nullptr;
    std::atomic<float>* pOttGain = nullptr;
    std::atomic<float>* pSootheSelectivity = nullptr; std::atomic<float>* pSootheSharpness = nullptr; std::atomic<float>* pSootheFocus = nullptr;
    std::atomic<float>* pArpWave = nullptr; std::atomic<float>* pArpMode = nullptr;
    std::atomic<float>* pArpSpeed = nullptr; std::atomic<float>* pArpPitch = nullptr;
    std::atomic<float>* pArpLevel = nullptr;

    std::array<std::atomic<float>*, 3> pModOn, pModAtk, pModDec, pModSus, pModRel, pModAmt;
    std::array<std::atomic<float>*, 3> pLfoOn, pLfoWave, pLfoSync, pLfoRate, pLfoBeat, pLfoAmt, pLfoTrig;

    // ★ 追加: MSEG 用パラメータ
    std::array<std::atomic<float>*, 2> pMsegOn, pMsegSync, pMsegRate, pMsegBeat, pMsegAmt, pMsegTrig;

    std::array<std::atomic<float>*, 10> pMatrixSrc;
    std::array<std::atomic<float>*, 10> pMatrixDest;
    std::array<std::atomic<float>*, 10> pMatrixAmt;

    juce::SmoothedValue<float> smoothedWtLevel, smoothedWtPitch, smoothedPDecayAmt, smoothedPDecayTime;
    juce::SmoothedValue<float> smoothedFltACutoff, smoothedFltAReso, smoothedFltBCutoff, smoothedFltBReso, smoothedFltMix;
    juce::SmoothedValue<float> smoothedDrive, smoothedShpAmt, smoothedShpRate, smoothedShpBit, smoothedGain;
    juce::SmoothedValue<float> smoothedWtPos, smoothedFm, smoothedDrift, smoothedSubVol, smoothedSubPitch, smoothedWidth;
    juce::SmoothedValue<float> smoothedMorphAAmt, smoothedMorphAShift, smoothedMorphBAmt, smoothedMorphBShift, smoothedMorphCAmt, smoothedMorphCShift;

    float lastOscFreq = -1.0f;
    int lastModeA = -1, lastModeB = -1, lastModeC = -1;

    // ★ 追加: MSEG シリアライズ / デシリアライズ用関数
    juce::String serializeMsegState(const MsegState& state) {
        juce::String result;
        for (int i = 0; i < state.numPoints; ++i) {
            result << state.points[i].x << "," << state.points[i].y << "," << state.points[i].curve;
            if (i < state.numPoints - 1) result << ";";
        }
        return result;
    }

    MsegState deserializeMsegState(const juce::String& str) {
        MsegState state;
        if (str.isEmpty()) return state;

        juce::StringArray parts;
        parts.addTokens(str, ";", "");
        state.numPoints = std::min(MAX_MSEG_POINTS, parts.size());
        for (int i = 0; i < state.numPoints; ++i) {
            juce::StringArray vals;
            vals.addTokens(parts[i], ",", "");
            if (vals.size() >= 3) {
                state.points[i].x = vals[0].getFloatValue();
                state.points[i].y = vals[1].getFloatValue();
                state.points[i].curve = vals[2].getFloatValue();
            }
        }
        return state;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LiquidDreamAudioProcessor)
};