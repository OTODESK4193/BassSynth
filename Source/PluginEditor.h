// ==============================================================================
// Source/PluginEditor.h
// ==============================================================================
#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/AbletonLookAndFeel.h"
#include "UI/WavetableBrowser.h"

class DualScopeComponent : public juce::Component, public juce::Timer
{
public:
    DualScopeComponent(float* outDataPtr) : outputData(outDataPtr) {
        sourceWave.fill(0.0f);
        startTimerHz(60);
    }

    void updateStaticWave(const float* data, int size) {
        if (size <= 0) return;
        for (int i = 0; i < 512; ++i) {
            sourceWave[i] = data[juce::jlimit(0, size - 1, (i * size) / 512)];
        }
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour::fromString("FF121212"));
        auto area = getLocalBounds();

        auto top = area.removeFromTop(area.getHeight() / 2).reduced(5);
        drawWave(g, top, sourceWave.data(), 512, "SOURCE (Wavetable Shape)", juce::Colour::fromString("FF00FFCC"));

        auto bottom = area.reduced(5);
        drawWave(g, bottom, outputData, 512, "OUTPUT (Dynamic Stream)", juce::Colour::fromString("FFFF764D"));
    }

    void timerCallback() override { repaint(); }
private:
    void drawWave(juce::Graphics& g, juce::Rectangle<int> r, const float* data, int size, const char* label, juce::Colour col) {
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.fillRoundedRectangle(r.toFloat(), 4.0f);

        g.saveState();
        g.reduceClipRegion(r);

        g.setColour(col);
        juce::Path p;
        float xStep = (float)r.getWidth() / size;
        for (int i = 0; i < size; ++i) {
            float x = r.getX() + i * xStep;
            float val = juce::jlimit(-1.1f, 1.1f, data[i]);
            float y = r.getCentreY() - (val * r.getHeight() * 0.45f);
            if (i == 0) p.startNewSubPath(x, y);
            else p.lineTo(x, y);
        }
        g.strokePath(p, juce::PathStrokeType(1.5f));
        g.restoreState();

        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.setFont(10.0f);
        g.drawText(label, r.reduced(5), juce::Justification::topRight);
    }

    std::array<float, 512> sourceWave;
    float* outputData;
};

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

class MsegEditorComponent : public juce::Component {
public:
    // ★ 修正: MsegStateを「参照渡し」で受け取ることでプロセッサ本体のデータを直接編集する
    MsegEditorComponent(Mseg& engine, MsegState& linkedState);
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void loadState(const MsegState& newState);

private:
    Mseg& msegEngine;
    MsegState& state; // ★ 修正: 参照保持
    int draggingNodeIndex = -1;
    bool draggingCurve = false;

    float getXPos(float x) const;
    float getYPos(float y) const;
    float getXValue(float xPos) const;
    float getYValue(float yPos) const;
    int hitTestNode(const juce::Point<float>& pos) const;
    void updateDSP();
};

class MsegTab : public juce::Component {
public:
    MsegTab(LiquidDreamAudioProcessor& p, juce::AudioProcessorValueTreeState& vts);
    void paint(juce::Graphics& g) override;
    void resized() override;
    void updateFromProcessor();
private:
    LiquidDreamAudioProcessor& processor;
    juce::AudioProcessorValueTreeState& apvts;
    struct MsegUI {
        juce::ToggleButton onBtn{ "ON" };
        juce::ComboBox beat, trig;
        juce::ToggleButton sync{ "SYNC" };
        juce::Slider rate, amt;
        juce::Label beatLbl, trigLbl, rateLbl, amtLbl;
        std::unique_ptr<MsegEditorComponent> editor;
    };
    std::array<MsegUI, 2> msegs;
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
    struct SlotUI {
        juce::ComboBox src;
        juce::Slider amt;
        juce::ComboBox dest;
    };
    std::array<SlotUI, 10> slots;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAtts;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAtts;
};

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

    juce::TextButton openBrowserButton{ "BROWSE" };
    juce::TextButton prevWaveButton{ juce::String::fromUTF8("\xe2\x97\x80") };
    juce::TextButton nextWaveButton{ juce::String::fromUTF8("\xe2\x96\xb6") };
    juce::TextButton rndWaveButton{ "RND" };

    juce::ToggleButton colorOnBtn{ "ON" };
    juce::TextButton viewWaveBtn{ "SCOPE" };
    juce::TextButton viewColorBtn{ "COL" };

    ColorIrPanel colorPanel;
    juce::GroupComponent oscGroup, subGroup, shaperGroup, filterGroup, ampEnvGroup, controlGroup, presetGroup;

    juce::ToggleButton oscOnButton{ "ON" };
    juce::Slider wtLevelSlider, wtPosSlider, oscPitchSlider, uniCountSlider, detuneSlider, widthSlider, driftSlider;
    juce::Label  wtLevelLabel, wtPosLabel, oscPitchLabel, uniCountLabel, detuneLabel, widthLabel, driftLabel;
    juce::Slider pitchDecayAmtSlider, pitchDecayTimeSlider, fmAmtSlider;
    juce::Label  pitchDecayAmtLabel, pitchDecayTimeLabel, fmAmtLabel;
    juce::ComboBox fmWaveCombo; juce::Label fmWaveLabel;

    juce::ComboBox morphAModeCombo, morphBModeCombo, morphCModeCombo;
    juce::Label    morphAModeLabel, morphBModeLabel, morphCModeLabel;
    juce::Slider   morphAAmtSlider, morphBAmtSlider, morphCAmtSlider;
    juce::Label    morphAAmtLabel, morphBAmtLabel, morphCAmtLabel;
    juce::Slider   morphAShiftSlider, morphBShiftSlider, morphCShiftSlider;
    juce::Label    morphAShiftLabel, morphBShiftLabel, morphCShiftLabel;

    juce::ToggleButton subOnButton{ "ON" };
    juce::ComboBox subWaveCombo; juce::Label subVolLabel;
    juce::Slider subVolSlider, subPitchSlider; juce::Label subPitchLabel;

    juce::Slider distDriveSlider, shpAmtSlider, bitSlider, rateSlider;
    juce::Label  distDriveLabel, shpAmtLabel, bitLabel, rateLabel;

    juce::TextButton fltABtn{ "A" }, fltBBtn{ "B" };
    juce::ComboBox fltATypeCombo, fltBTypeCombo, fltRoutingCombo;
    juce::Slider fltACutoffSlider, fltBCutoffSlider, fltAResSlider, fltBResSlider;
    juce::Label  fltACutoffLabel, fltBCutoffLabel, fltAResLabel, fltBResLabel;
    juce::Slider fltMixSlider; juce::Label fltMixLabel;

    juce::Slider fltAEnvAmtSlider, fltBEnvAmtSlider;
    juce::Label  fltAEnvAmtLabel, fltBEnvAmtLabel;

    juce::Slider fltAAtkSlider, fltADecSlider, fltASusSlider, fltARelSlider;
    juce::Label  fltAAtkLabel, fltADecLabel, fltASusLabel, fltARelLabel;
    juce::Slider fltBAtkSlider, fltBDecSlider, fltBSusSlider, fltBRelSlider;
    juce::Label  fltBAtkLabel, fltBDecLabel, fltBSusLabel, fltBRelLabel;

    juce::Slider ampAtkSlider, ampDecSlider, ampSusSlider, ampRelSlider;
    juce::Label  ampAtkLabel, ampDecLabel, ampSusLabel, ampRelLabel;

    juce::ToggleButton legatoButton{ "LEGATO" };
    juce::Slider glideSlider, pitchSlider, gainSlider;
    juce::Label  glideLabel, pitchLabel, gainLabel;

    juce::ComboBox presetCombo;
    juce::TextButton savePresetBtn{ "Save" };
    juce::Array<juce::File> presetFiles;

    juce::TabbedComponent modTabs;
    LfoTab lfoTab;
    MsegTab msegTab;
    ModEnvTab modEnvTab;
    MatrixTab matrixTab;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> oscOnAtt, subOnAtt, legatoAtt, colorOnAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> subWaveAtt, fmWaveAtt, morphAAtt, morphBAtt, morphCAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> fltATypeAtt, fltBTypeAtt, fltRoutingAtt;

    int blinkCounter = 0;
    bool wtChanged = false;
    bool isColorPanelVisible = false;

    void updateFilterUI();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LiquidDreamAudioProcessorEditor)
};