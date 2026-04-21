// ==============================================================================
// Source/PluginEditor.h
// ==============================================================================
#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/AbletonLookAndFeel.h"
#include "UI/WavetableBrowser.h"
#include "UI/DualScopeComponent.h"

// --- Custom Tab Components for Modulation ---

class LfoTab : public juce::Component {
public:
    LfoTab(juce::AudioProcessorValueTreeState& vts);
    void paint(juce::Graphics& g) override;
    void resized() override;
private:
    juce::AudioProcessorValueTreeState& apvts;
    struct LfoUI {
        juce::ToggleButton onBtn{ "ON" };
        juce::ComboBox wave, beat;
        juce::ToggleButton sync{ "SYNC" };
        juce::Slider rate, amt;
        juce::Label waveLbl, beatLbl, rateLbl, amtLbl;
    };
    std::array<LfoUI, 3> lfos;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAtts;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAtts;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> btnAtts;
};

class ModEnvTab : public juce::Component {
public:
    ModEnvTab(juce::AudioProcessorValueTreeState& vts);
    void paint(juce::Graphics& g) override;
    void resized() override;
private:
    juce::AudioProcessorValueTreeState& apvts;
    struct EnvUI {
        juce::ToggleButton onBtn{ "ON" };
        juce::Slider a, d, s, r, amt;
        juce::Label aL, dL, sL, rL, amtL;
    };
    std::array<EnvUI, 3> envs;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAtts;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> btnAtts;
};

class MatrixTab : public juce::Component {
public:
    MatrixTab(juce::AudioProcessorValueTreeState& vts);
    void paint(juce::Graphics& g) override;
    void resized() override;
private:
    juce::AudioProcessorValueTreeState& apvts;
    struct SlotUI { juce::ComboBox dest; juce::Slider amt; juce::Label dummyLbl; };
    struct RowUI { std::array<SlotUI, 3> slots; };
    std::array<RowUI, 6> rows;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAtts;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAtts;
};

// --- ColorIR Vertical Rack Panel ---

class ColorIrPanel : public juce::Component {
public:
    ColorIrPanel(LiquidDreamAudioProcessor& p);
    void paint(juce::Graphics& g) override;
    void resized() override;

    void updateState(ColorIREngine::LearnState state, const juce::String& chordText, bool blinkFlag);

    juce::TextButton learnButton{ "LEARN CHORD" };
private:
    LiquidDreamAudioProcessor& processor;
    juce::AudioProcessorValueTreeState& apvts;
    juce::Label chordLabel;

    // Block 1: Generator
    juce::ComboBox typeCombo; juce::Label typeLabel;
    juce::Slider mixSlider, preHpSlider, postHpSlider, atkSlider, decSlider;
    juce::Label mixLabel, preHpLabel, postHpLabel, atkLabel, decLabel;

    // Block 2: True OTT
    juce::Slider ottDepthSlider, ottTimeSlider, ottUpSlider, ottDownSlider, ottGainSlider;
    juce::Label ottDepthLabel, ottTimeLabel, ottUpLabel, ottDownLabel, ottGainLabel;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> atts;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAtt;
};

// --- Main Editor Class ---

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

    // Nav
    juce::TextButton openBrowserButton{ "BROWSE" };
    juce::TextButton prevWaveButton{ juce::String::fromUTF8("\xe2\x97\x80") };
    juce::TextButton nextWaveButton{ juce::String::fromUTF8("\xe2\x96\xb6") };
    juce::TextButton rndWaveButton{ "RND" };
    juce::ToggleButton colorButton{ "COLOR" };

    // Overlays & Groups
    ColorIrPanel colorPanel;
    juce::GroupComponent oscGroup, subGroup, shaperGroup, filterGroup, ampEnvGroup, filterEnvGroup, controlGroup;

    // Osc Params
    juce::ToggleButton oscOnButton{ "ON" };
    juce::Slider wtLevelSlider, wtPosSlider, oscPitchSlider, uniCountSlider, detuneSlider, widthSlider, driftSlider;
    juce::Label  wtLevelLabel, wtPosLabel, oscPitchLabel, uniCountLabel, detuneLabel, widthLabel, driftLabel;
    juce::Slider pitchDecayAmtSlider, pitchDecayTimeSlider, fmAmtSlider;
    juce::Label  pitchDecayAmtLabel, pitchDecayTimeLabel, fmAmtLabel;
    juce::ComboBox fmWaveCombo; juce::Label fmWaveLabel;

    // Morph Params
    juce::ComboBox morphAModeCombo, morphBModeCombo, morphCModeCombo;
    juce::Label    morphAModeLabel, morphBModeLabel, morphCModeLabel;
    juce::Slider   morphAAmtSlider, morphBAmtSlider, morphCAmtSlider;
    juce::Label    morphAAmtLabel, morphBAmtLabel, morphCAmtLabel;
    juce::Slider   morphAShiftSlider, morphBShiftSlider, morphCShiftSlider;
    juce::Label    morphAShiftLabel, morphBShiftLabel, morphCShiftLabel;

    // Sub Osc
    juce::ToggleButton subOnButton{ "ON" };
    juce::ComboBox subWaveCombo; juce::Label subVolLabel;
    juce::Slider subVolSlider, subPitchSlider; juce::Label subPitchLabel;

    // Shaper & Filter
    juce::Slider distDriveSlider, shpAmtSlider, bitSlider, rateSlider;
    juce::Label  distDriveLabel, shpAmtLabel, bitLabel, rateLabel;
    juce::Slider cutoffSlider, resSlider, fltEnvAmtSlider;
    juce::Label  cutoffLabel, resLabel, fltEnvAmtLabel;

    // Core Envelopes
    juce::Slider ampAtkSlider, ampDecSlider, ampSusSlider, ampRelSlider;
    juce::Label  ampAtkLabel, ampDecLabel, ampSusLabel, ampRelLabel;
    juce::Slider fltAtkSlider, fltDecSlider, fltSusSlider, fltRelSlider;
    juce::Label  fltAtkLabel, fltDecLabel, fltSusLabel, fltRelLabel;

    // Performance
    juce::ToggleButton legatoButton{ "LEGATO" };
    juce::Slider glideSlider, pitchSlider, gainSlider;
    juce::Label  glideLabel, pitchLabel, gainLabel;

    // Modulation Tabs
    juce::TabbedComponent modTabs;
    LfoTab lfoTab;
    ModEnvTab modEnvTab;
    MatrixTab matrixTab;

    // Attachments
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> oscOnAtt, subOnAtt, legatoAtt, colorOnAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> subWaveAtt, fmWaveAtt, morphAAtt, morphBAtt, morphCAtt;

    int blinkCounter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LiquidDreamAudioProcessorEditor)
};