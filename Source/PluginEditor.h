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
    // widthSlider を追加
    juce::Slider wtPosSlider, fmAmtSlider, syncSlider, morphSlider, uniCountSlider, detuneSlider, widthSlider, oscPitchSlider, driftSlider;
    // widthLabel を追加
    juce::Label  wtPosLabel, fmAmtLabel, syncLabel, morphLabel, uniCountLabel, detuneLabel, widthLabel, oscPitchLabel, driftLabel;

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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LiquidDreamAudioProcessorEditor)
};