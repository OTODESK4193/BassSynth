// ==============================================================================
// Source/PluginEditor.h
// ==============================================================================
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
    juce::TextButton openBrowserButton{ "BROWSE" };

    juce::TextButton prevWaveButton{ juce::String::fromUTF8("\xe2\x97\x80") };
    juce::TextButton nextWaveButton{ juce::String::fromUTF8("\xe2\x96\xb6") };
    juce::TextButton rndWaveButton{ "RND" };

    juce::GroupComponent oscGroup, subGroup, shaperGroup, filterGroup, ampEnvGroup, modEnvGroup, controlGroup;

    juce::ToggleButton oscOnButton{ "ON" };
    juce::Slider wtLevelSlider, wtPosSlider, oscPitchSlider;
    juce::Label  wtLevelLabel, wtPosLabel, oscPitchLabel;

    juce::Slider uniCountSlider, detuneSlider, widthSlider, driftSlider;
    juce::Label  uniCountLabel, detuneLabel, widthLabel, driftLabel;

    juce::Slider pitchDecayAmtSlider, pitchDecayTimeSlider, fmAmtSlider;
    juce::Label  pitchDecayAmtLabel, pitchDecayTimeLabel, fmAmtLabel;

    juce::ComboBox fmWaveCombo;
    juce::Label    fmWaveLabel;

    juce::ComboBox morphAModeCombo, morphBModeCombo, morphCModeCombo;
    juce::Label    morphAModeLabel, morphBModeLabel, morphCModeLabel;
    juce::Slider   morphAAmtSlider, morphBAmtSlider, morphCAmtSlider;
    juce::Label    morphAAmtLabel, morphBAmtLabel, morphCAmtLabel;
    juce::Slider   morphAShiftSlider, morphBShiftSlider, morphCShiftSlider;
    juce::Label    morphAShiftLabel, morphBShiftLabel, morphCShiftLabel;

    juce::ToggleButton subOnButton{ "ON" };
    juce::ComboBox subWaveCombo;
    juce::Slider subVolSlider, subPitchSlider;
    juce::Label  subVolLabel, subPitchLabel;

    juce::Slider distDriveSlider, shpAmtSlider, bitSlider, rateSlider;
    juce::Label  distDriveLabel, shpAmtLabel, bitLabel, rateLabel;

    juce::Slider cutoffSlider, resSlider, fltEnvAmtSlider;
    juce::Label  cutoffLabel, resLabel, fltEnvAmtLabel;

    juce::Slider ampAtkSlider, ampDecSlider, ampSusSlider, ampRelSlider;
    juce::Label  ampAtkLabel, ampDecLabel, ampSusLabel, ampRelLabel;

    juce::Slider modAtkSlider, modDecSlider, modSusSlider, modRelSlider;
    juce::Label  modAtkLabel, modDecLabel, modSusLabel, modRelLabel;

    juce::ToggleButton legatoButton{ "LEGATO" };

    juce::Slider glideSlider, pitchSlider, gainSlider;
    juce::Label  glideLabel, pitchLabel, gainLabel;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> oscOnAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> subOnAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> legatoAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> subWaveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> fmWaveAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> morphAModeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> morphBModeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> morphCModeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LiquidDreamAudioProcessorEditor)
};