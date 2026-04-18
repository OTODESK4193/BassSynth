#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/AbletonLookAndFeel.h"
#include "UI/WavetableBrowser.h"
#include "UI/DualScopeComponent.h"

class LiquidDreamAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    LiquidDreamAudioProcessorEditor(LiquidDreamAudioProcessor&);
    ~LiquidDreamAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    LiquidDreamAudioProcessor& audioProcessor;
    AbletonLookAndFeel abletonLnF;
    DualScopeComponent dualScope;

    WavetableBrowser browser;
    juce::TextButton openBrowserButton{ "BROWSE WAVETABLE" };

    // --- グループコンポーネント ---
    juce::GroupComponent oscGroup, subGroup, shaperGroup, filterGroup, ampEnvGroup, modEnvGroup, controlGroup;

    // --- Osc Parameters ---
    juce::Slider wtLevelSlider, wtPosSlider, syncSlider, oscPitchSlider;
    juce::Label  wtLevelLabel, wtPosLabel, syncLabel, oscPitchLabel;

    juce::Slider uniCountSlider, detuneSlider, widthSlider, driftSlider;
    juce::Label  uniCountLabel, detuneLabel, widthLabel, driftLabel;

    juce::Slider pitchDecayAmtSlider, pitchDecayTimeSlider, fmAmtSlider;
    juce::Label  pitchDecayAmtLabel, pitchDecayTimeLabel, fmAmtLabel;

    juce::ComboBox fmWaveCombo;
    juce::Label    fmWaveLabel;

    // --- Dual Morph Parameters ---
    juce::ComboBox morphAModeCombo, morphBModeCombo;
    juce::Label    morphAModeLabel, morphBModeLabel;
    juce::Slider   morphAAmtSlider, morphBAmtSlider;
    juce::Label    morphAAmtLabel, morphBAmtLabel;

    // --- Sub Osc Parameters ---
    juce::ToggleButton subOnButton{ "ON" };
    juce::ComboBox subWaveCombo;
    juce::Slider subVolSlider, subPitchSlider;
    juce::Label  subVolLabel, subPitchLabel;

    // --- Filter / Shaper / Perf ---
    juce::Slider distDriveSlider, shpAmtSlider, bitSlider, rateSlider;
    juce::Label  distDriveLabel, shpAmtLabel, bitLabel, rateLabel;

    juce::Slider cutoffSlider, resSlider, fltEnvAmtSlider;
    juce::Label  cutoffLabel, resLabel, fltEnvAmtLabel;

    juce::Slider ampAtkSlider, ampDecSlider, ampSusSlider, ampRelSlider;
    juce::Label  ampAtkLabel, ampDecLabel, ampSusLabel, ampRelLabel;

    juce::Slider modAtkSlider, modDecSlider, modSusSlider, modRelSlider;
    juce::Label  modAtkLabel, modDecLabel, modSusLabel, modRelLabel;

    juce::Slider glideSlider, pitchSlider, gainSlider;
    juce::Label  glideLabel, pitchLabel, gainLabel;

    // --- Attachments ---
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> subOnAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> subWaveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> fmWaveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> morphAModeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> morphBModeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LiquidDreamAudioProcessorEditor)
};