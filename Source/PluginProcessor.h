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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LiquidDreamAudioProcessor)
};