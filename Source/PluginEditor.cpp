// ==============================================================================
// Source/PluginEditor.cpp
// ==============================================================================
#include "PluginProcessor.h"
#include "PluginEditor.h"

LiquidDreamAudioProcessorEditor::LiquidDreamAudioProcessorEditor(LiquidDreamAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
    dualScope(p.getOutputScopePtr()),
    browser(p.getAPVTS())
{
    setLookAndFeel(&abletonLnF);

    addAndMakeVisible(openBrowserButton);
    addAndMakeVisible(dualScope);

    auto setupS = [&](juce::Slider& s, juce::Label& l, const char* txt) {
        addAndMakeVisible(s);
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
        addAndMakeVisible(l);
        l.setText(txt, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setFont(12.0f);
        };

    oscGroup.setText("WAVETABLE"); addAndMakeVisible(oscGroup);
    subGroup.setText("SUB OSC"); addAndMakeVisible(subGroup);
    shaperGroup.setText("DISTORTION & SHAPER"); addAndMakeVisible(shaperGroup);
    filterGroup.setText("FILTER"); addAndMakeVisible(filterGroup);
    ampEnvGroup.setText("AMP ENVELOPE"); addAndMakeVisible(ampEnvGroup);
    modEnvGroup.setText("MOD ENVELOPE"); addAndMakeVisible(modEnvGroup);
    controlGroup.setText("PERFORMANCE"); addAndMakeVisible(controlGroup);

    // Osc Params Setup
    setupS(wtLevelSlider, wtLevelLabel, "Level");
    setupS(wtPosSlider, wtPosLabel, "Pos");
    setupS(oscPitchSlider, oscPitchLabel, "Pitch");

    setupS(uniCountSlider, uniCountLabel, "Unison");
    setupS(detuneSlider, detuneLabel, "Detune");
    setupS(widthSlider, widthLabel, "Width");
    setupS(driftSlider, driftLabel, "Drift");

    setupS(pitchDecayAmtSlider, pitchDecayAmtLabel, "P.Decay");
    setupS(pitchDecayTimeSlider, pitchDecayTimeLabel, "P.Time");
    setupS(fmAmtSlider, fmAmtLabel, "FM Amt");

    // ComboBox Setup Helper
    auto setupCombo = [&](juce::ComboBox& c, juce::Label& l, const char* txt, juce::StringArray items) {
        addAndMakeVisible(c);
        c.addItemList(items, 1);
        c.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(l);
        l.setText(txt, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setFont(12.0f);
        };

    setupCombo(fmWaveCombo, fmWaveLabel, "FM Mod", { "Sine", "Saw", "Pulse", "Triangle" });

    // Dual Morph Setup (14種類統合)
    juce::StringArray morphTypes = {
        "None", "Bend (+/-)", "PWM", "Sync", "Mirror", "Flip", "Quantize", "Remap",
        "Smear", "Vocode", "Stretch", "SpecCut", "Shepard", "Comb"
    };
    setupCombo(morphAModeCombo, morphAModeLabel, "Morph A", morphTypes);
    setupS(morphAAmtSlider, morphAAmtLabel, "Amt A");
    setupS(morphAShiftSlider, morphAShiftLabel, "Shift A");

    setupCombo(morphBModeCombo, morphBModeLabel, "Morph B", morphTypes);
    setupS(morphBAmtSlider, morphBAmtLabel, "Amt B");
    setupS(morphBShiftSlider, morphBShiftLabel, "Shift B");

    // Sub Osc Params
    addAndMakeVisible(subOnButton);
    setupCombo(subWaveCombo, subVolLabel /*dummy*/, "", { "Sine", "Triangle", "Pulse", "Saw" });
    subVolLabel.setText("Wave", juce::dontSendNotification);
    setupS(subVolSlider, subVolLabel, "Level");
    setupS(subPitchSlider, subPitchLabel, "Pitch");

    // Other Params
    setupS(distDriveSlider, distDriveLabel, "Drive"); setupS(shpAmtSlider, shpAmtLabel, "Shaper");
    setupS(bitSlider, bitLabel, "Bits"); setupS(rateSlider, rateLabel, "Rate");
    setupS(cutoffSlider, cutoffLabel, "Cutoff"); setupS(resSlider, resLabel, "Reso"); setupS(fltEnvAmtSlider, fltEnvAmtLabel, "EnvAmt");
    setupS(ampAtkSlider, ampAtkLabel, "A"); setupS(ampDecSlider, ampDecLabel, "D"); setupS(ampSusSlider, ampSusLabel, "S"); setupS(ampRelSlider, ampRelLabel, "R");
    setupS(modAtkSlider, modAtkLabel, "A"); setupS(modDecSlider, modDecLabel, "D"); setupS(modSusSlider, modSusLabel, "S"); setupS(modRelSlider, modRelLabel, "R");
    setupS(glideSlider, glideLabel, "Glide"); setupS(pitchSlider, pitchLabel, "Pitch"); setupS(gainSlider, gainLabel, "Gain");

    auto& apvts = audioProcessor.getAPVTS();
    auto att = [&](juce::Slider& s, const juce::String& id) {
        attachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, id, s));
        };

    // Attach Osc
    att(wtLevelSlider, "osc_level");
    att(wtPosSlider, "osc_pos");
    att(oscPitchSlider, "osc_pitch");

    att(uniCountSlider, "osc_uni");
    att(detuneSlider, "osc_detune");
    att(widthSlider, "osc_width");
    att(driftSlider, "osc_drift");

    att(pitchDecayAmtSlider, "osc_pdecay_amt");
    att(pitchDecayTimeSlider, "osc_pdecay_time");
    fmWaveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "osc_fm_wave", fmWaveCombo);
    att(fmAmtSlider, "osc_fm");

    morphAModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "osc_morph_a_mode", morphAModeCombo);
    att(morphAAmtSlider, "osc_morph_a_amt");
    att(morphAShiftSlider, "osc_morph_a_shift");

    morphBModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "osc_morph_b_mode", morphBModeCombo);
    att(morphBAmtSlider, "osc_morph_b_amt");
    att(morphBShiftSlider, "osc_morph_b_shift");

    // Attach Sub
    subOnAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "sub_on", subOnButton);
    subWaveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "sub_wave", subWaveCombo);
    att(subVolSlider, "sub_vol");
    att(subPitchSlider, "sub_pitch");

    // Attach Others
    att(distDriveSlider, "dist_drive"); att(shpAmtSlider, "shp_amt"); att(bitSlider, "shp_bit"); att(rateSlider, "shp_rate");
    att(cutoffSlider, "flt_cutoff"); att(resSlider, "flt_res"); att(fltEnvAmtSlider, "flt_env_amt");
    att(ampAtkSlider, "a_atk"); att(ampDecSlider, "a_dec"); att(ampSusSlider, "a_sus"); att(ampRelSlider, "a_rel");
    att(modAtkSlider, "f_atk"); att(modDecSlider, "f_dec"); att(modSusSlider, "f_sus"); att(modRelSlider, "f_rel");
    att(glideSlider, "m_glide"); att(pitchSlider, "m_pb"); att(gainSlider, "m_gain");

    addChildComponent(browser);
    browser.setVisible(false);

    openBrowserButton.onClick = [this] {
        browser.setVisible(!browser.isVisible());
        if (browser.isVisible()) browser.toFront(true);
        };

    startTimerHz(30);
    // 高さを拡張 (820 -> 900) してShiftノブのスペースを確保
    setSize(1000, 900);
}

LiquidDreamAudioProcessorEditor::~LiquidDreamAudioProcessorEditor() { setLookAndFeel(nullptr); }

void LiquidDreamAudioProcessorEditor::paint(juce::Graphics& g) { g.fillAll(juce::Colour::fromString("FF1E1E1E")); }

void LiquidDreamAudioProcessorEditor::timerCallback()
{
    std::array<float, 512> tempBuffer;
    audioProcessor.getStaticWaveform(tempBuffer);
    dualScope.updateStaticWave(tempBuffer.data(), 512);
}

void LiquidDreamAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(15);

    auto placeKnob = [](int x, int y, juce::Label& lbl, juce::Slider& sld) {
        lbl.setBounds(x, y, 70, 20);
        sld.setBounds(x, y + 20, 70, 50);
        };

    auto placeCombo = [](int x, int y, juce::Label& lbl, juce::ComboBox& cmb) {
        lbl.setBounds(x, y, 70, 20);
        cmb.setBounds(x, y + 30, 70, 24);
        };

    // --- 左側エリア ---
    auto leftArea = area.removeFromLeft(350);
    openBrowserButton.setBounds(leftArea.removeFromTop(35).reduced(2));
    leftArea.removeFromTop(5);
    dualScope.setBounds(leftArea.removeFromTop(370));
    leftArea.removeFromTop(10);

    auto ctrlRect = leftArea.removeFromTop(90);
    controlGroup.setBounds(ctrlRect);
    int cX = ctrlRect.getX(), cY = ctrlRect.getY() + 15;
    placeKnob(cX + 15, cY, glideLabel, glideSlider);
    placeKnob(cX + 95, cY, pitchLabel, pitchSlider);
    placeKnob(cX + 175, cY, gainLabel, gainSlider);

    leftArea.removeFromTop(10);
    auto modRect = leftArea.removeFromTop(90);
    modEnvGroup.setBounds(modRect);
    int mX = modRect.getX(), mY = modRect.getY() + 15;
    placeKnob(mX + 15, mY, modAtkLabel, modAtkSlider);
    placeKnob(mX + 95, mY, modDecLabel, modDecSlider);
    placeKnob(mX + 175, mY, modSusLabel, modSusSlider);
    placeKnob(mX + 255, mY, modRelLabel, modRelSlider);

    // --- 右側エリア ---
    area.removeFromLeft(15);
    auto rightArea = area;
    browser.setBounds(rightArea);

    // 1. Wavetable Osc (高さを拡張：300 -> 380)
    auto oscRect = rightArea.removeFromTop(380);
    oscGroup.setBounds(oscRect);
    int oX = oscRect.getX() + 10, oY = oscRect.getY() + 15;

    // Row 1: Basic & Pitch
    placeKnob(oX, oY, wtLevelLabel, wtLevelSlider);
    placeKnob(oX + 80, oY, wtPosLabel, wtPosSlider);
    placeKnob(oX + 160, oY, oscPitchLabel, oscPitchSlider);
    placeKnob(oX + 240, oY, driftLabel, driftSlider);

    // Row 2: Unison & FM Wave
    placeKnob(oX, oY + 70, uniCountLabel, uniCountSlider);
    placeKnob(oX + 80, oY + 70, detuneLabel, detuneSlider);
    placeKnob(oX + 160, oY + 70, widthLabel, widthSlider);
    placeCombo(oX + 240, oY + 70, fmWaveLabel, fmWaveCombo);

    // Row 3: FM Amt & Pitch Mod
    placeKnob(oX, oY + 140, fmAmtLabel, fmAmtSlider);
    placeKnob(oX + 80, oY + 140, pitchDecayAmtLabel, pitchDecayAmtSlider);
    placeKnob(oX + 160, oY + 140, pitchDecayTimeLabel, pitchDecayTimeSlider);

    // Row 4: Morph A (Mode, Amt, Shift)
    placeCombo(oX, oY + 210, morphAModeLabel, morphAModeCombo);
    placeKnob(oX + 80, oY + 210, morphAAmtLabel, morphAAmtSlider);
    placeKnob(oX + 160, oY + 210, morphAShiftLabel, morphAShiftSlider);

    // Row 5: Morph B (Mode, Amt, Shift)
    placeCombo(oX, oY + 280, morphBModeLabel, morphBModeCombo);
    placeKnob(oX + 80, oY + 280, morphBAmtLabel, morphBAmtSlider);
    placeKnob(oX + 160, oY + 280, morphBShiftLabel, morphBShiftSlider);

    rightArea.removeFromTop(10);

    // 2. Sub Osc
    auto subRect = rightArea.removeFromTop(90);
    subGroup.setBounds(subRect);
    int sbX = subRect.getX() + 10, sbY = subRect.getY() + 15;
    subOnButton.setBounds(sbX, sbY + 20, 50, 24);
    subWaveCombo.setBounds(sbX + 60, sbY + 25, 80, 24);
    placeKnob(sbX + 160, sbY, subVolLabel, subVolSlider);
    placeKnob(sbX + 240, sbY, subPitchLabel, subPitchSlider);

    rightArea.removeFromTop(10);

    // 3. Shaper
    auto shpRect = rightArea.removeFromTop(90);
    shaperGroup.setBounds(shpRect);
    int sX = shpRect.getX() + 10, sY = shpRect.getY() + 15;
    placeKnob(sX, sY, distDriveLabel, distDriveSlider);
    placeKnob(sX + 80, sY, shpAmtLabel, shpAmtSlider);
    placeKnob(sX + 160, sY, rateLabel, rateSlider);
    placeKnob(sX + 240, sY, bitLabel, bitSlider);

    rightArea.removeFromTop(10);

    // 4. Filter
    auto fltRect = rightArea.removeFromTop(90);
    filterGroup.setBounds(fltRect);
    int fX = fltRect.getX() + 10, fY = fltRect.getY() + 15;
    placeKnob(fX, fY, cutoffLabel, cutoffSlider);
    placeKnob(fX + 80, fY, resLabel, resSlider);
    placeKnob(fX + 160, fY, fltEnvAmtLabel, fltEnvAmtSlider);

    rightArea.removeFromTop(10);

    // 5. Amp Envelope
    ampEnvGroup.setBounds(rightArea);
    int eX = rightArea.getX() + 10, eY = rightArea.getY() + 15;
    placeKnob(eX, eY, ampAtkLabel, ampAtkSlider);
    placeKnob(eX + 80, eY, ampDecLabel, ampDecSlider);
    placeKnob(eX + 160, eY, ampSusLabel, ampSusSlider);
    placeKnob(eX + 240, eY, ampRelLabel, ampRelSlider);
}