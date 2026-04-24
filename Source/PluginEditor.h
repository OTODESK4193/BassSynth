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
        juce::ComboBox wave, beat, trig;
        juce::ToggleButton sync{ "SYNC" };
        juce::Slider rate, amt;
        juce::Label waveLbl, beatLbl, trigLbl, rateLbl, amtLbl;
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

    juce::ComboBox typeCombo; juce::Label typeLabel;
    juce::Slider mixSlider, irVolSlider, preHpSlider, postHpSlider, atkSlider, decSlider;
    juce::Label mixLabel, irVolLabel, preHpLabel, postHpLabel, atkLabel, decLabel;

    juce::Slider ottDepthSlider, ottTimeSlider, ottUpSlider, ottDownSlider, ottGainSlider;
    juce::Label ottDepthLabel, ottTimeLabel, ottUpLabel, ottDownLabel, ottGainLabel;

    juce::Slider sootheSelSlider, sootheShpSlider, sootheFocSlider;
    juce::Label sootheSelLabel, sootheShpLabel, sootheFocLabel;

    juce::ComboBox arpWaveCombo, arpModeCombo, arpPitchCombo;
    juce::Label arpWaveLabel, arpModeLabel, arpPitchLabel;
    juce::Slider arpSpeedSlider, arpLevelSlider;
    juce::Label arpSpeedLabel, arpLevelLabel;

    juce::Slider masterGainSlider;
    juce::Label masterGainLabel;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> atts;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAtts;
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
    void scanPresets();

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
    juce::GroupComponent oscGroup, subGroup, shaperGroup, filterGroup, ampEnvGroup, controlGroup, presetGroup;

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

    // Shaper
    juce::Slider distDriveSlider, shpAmtSlider, bitSlider, rateSlider;
    juce::Label  distDriveLabel, shpAmtLabel, bitLabel, rateLabel;

    // Dual Filter UI
    juce::TextButton fltABtn{ "A" }, fltBBtn{ "B" };
    juce::ComboBox fltATypeCombo, fltBTypeCombo, fltRoutingCombo;
    juce::Slider fltACutoffSlider, fltBCutoffSlider, fltAResSlider, fltBResSlider;
    juce::Label  fltACutoffLabel, fltBCutoffLabel, fltAResLabel, fltBResLabel;
    juce::Slider fltMixSlider; juce::Label fltMixLabel;

    // A/B 独立 Envelope Amount
    juce::Slider fltAEnvAmtSlider, fltBEnvAmtSlider;
    juce::Label  fltAEnvAmtLabel, fltBEnvAmtLabel;

    // A/B 独立 ADSR
    juce::Slider fltAAtkSlider, fltADecSlider, fltASusSlider, fltARelSlider;
    juce::Label  fltAAtkLabel, fltADecLabel, fltASusLabel, fltARelLabel;
    juce::Slider fltBAtkSlider, fltBDecSlider, fltBSusSlider, fltBRelSlider;
    juce::Label  fltBAtkLabel, fltBDecLabel, fltBSusLabel, fltBRelLabel;

    // Amp Envelope
    juce::Slider ampAtkSlider, ampDecSlider, ampSusSlider, ampRelSlider;
    juce::Label  ampAtkLabel, ampDecLabel, ampSusLabel, ampRelLabel;

    // Performance
    juce::ToggleButton legatoButton{ "LEGATO" };
    juce::Slider glideSlider, pitchSlider, gainSlider;
    juce::Label  glideLabel, pitchLabel, gainLabel;

    // Preset UI
    juce::ComboBox presetCombo;
    juce::TextButton savePresetBtn{ "Save" };
    juce::TextButton loadPresetBtn{ "Load" };
    juce::Array<juce::File> presetFiles;

    // Modulation Tabs
    juce::TabbedComponent modTabs;
    LfoTab lfoTab;
    ModEnvTab modEnvTab;
    MatrixTab matrixTab;

    // Attachments
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> oscOnAtt, subOnAtt, legatoAtt, colorOnAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> subWaveAtt, fmWaveAtt, morphAAtt, morphBAtt, morphCAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> fltATypeAtt, fltBTypeAtt, fltRoutingAtt;

    int blinkCounter = 0;
    void updateFilterUI();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LiquidDreamAudioProcessorEditor)
};