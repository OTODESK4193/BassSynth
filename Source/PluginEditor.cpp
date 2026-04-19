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
    addAndMakeVisible(prevWaveButton);
    addAndMakeVisible(nextWaveButton);
    addAndMakeVisible(rndWaveButton);
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
    addAndMakeVisible(oscOnButton);
    setupS(wtLevelSlider, wtLevelLabel, "Level");
    setupS(wtPosSlider, wtPosLabel, "Pos"); // Positionノブ復帰済み
    setupS(oscPitchSlider, oscPitchLabel, "Pitch");

    setupS(pitchDecayAmtSlider, pitchDecayAmtLabel, "P.Decay");
    setupS(pitchDecayTimeSlider, pitchDecayTimeLabel, "P.Time");

    setupS(driftSlider, driftLabel, "Drift");
    setupS(uniCountSlider, uniCountLabel, "Unison");
    setupS(detuneSlider, detuneLabel, "Detune");
    setupS(widthSlider, widthLabel, "Width");
    setupS(fmAmtSlider, fmAmtLabel, "FM Amt");

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

    // 3 Stage Morph Setup (14種類統合)
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

    setupCombo(morphCModeCombo, morphCModeLabel, "Morph C", morphTypes);
    setupS(morphCAmtSlider, morphCAmtLabel, "Amt C");
    setupS(morphCShiftSlider, morphCShiftLabel, "Shift C");

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
    oscOnAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "osc_on", oscOnButton);
    att(wtLevelSlider, "osc_level");
    att(wtPosSlider, "osc_pos");
    att(oscPitchSlider, "osc_pitch");
    att(pitchDecayAmtSlider, "osc_pdecay_amt");
    att(pitchDecayTimeSlider, "osc_pdecay_time");
    att(driftSlider, "osc_drift");
    att(uniCountSlider, "osc_uni");
    att(detuneSlider, "osc_detune");
    att(widthSlider, "osc_width");
    att(fmAmtSlider, "osc_fm");
    fmWaveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "osc_fm_wave", fmWaveCombo);

    // Attach Morph
    morphAModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "osc_morph_a_mode", morphAModeCombo);
    att(morphAAmtSlider, "osc_morph_a_amt");
    att(morphAShiftSlider, "osc_morph_a_shift");

    morphBModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "osc_morph_b_mode", morphBModeCombo);
    att(morphBAmtSlider, "osc_morph_b_amt");
    att(morphBShiftSlider, "osc_morph_b_shift");

    morphCModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "osc_morph_c_mode", morphCModeCombo);
    att(morphCAmtSlider, "osc_morph_c_amt");
    att(morphCShiftSlider, "osc_morph_c_shift");

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

    // ボタンのコールバック設定
    openBrowserButton.onClick = [this] {
        browser.setVisible(!browser.isVisible());
        if (browser.isVisible()) browser.toFront(true);
        };

    prevWaveButton.onClick = [this] { browser.selectPrev(); };
    nextWaveButton.onClick = [this] { browser.selectNext(); };
    rndWaveButton.onClick = [this] { browser.selectRandom(); };

    startTimerHz(30);

    setSize(1300, 700);
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

    auto placeCombo = [](int x, int y, int w, juce::Label& lbl, juce::ComboBox& cmb) {
        lbl.setBounds(x, y, w, 20);
        cmb.setBounds(x, y + 30, w, 24);
        };

    // --- 左側エリア (350px) ---
    auto leftArea = area.removeFromLeft(350);

    // 【NEW】ナビゲーションボタン群の配置
    auto navRect = leftArea.removeFromTop(35).reduced(2);
    openBrowserButton.setBounds(navRect.removeFromLeft(110));
    navRect.removeFromLeft(5);
    prevWaveButton.setBounds(navRect.removeFromLeft(40));
    navRect.removeFromLeft(5);
    nextWaveButton.setBounds(navRect.removeFromLeft(40));
    navRect.removeFromLeft(5);
    rndWaveButton.setBounds(navRect.removeFromLeft(60));

    leftArea.removeFromTop(5);
    dualScope.setBounds(leftArea.removeFromTop(370));
    leftArea.removeFromTop(10);

    auto ctrlRect = leftArea.removeFromTop(100);
    controlGroup.setBounds(ctrlRect);
    int cX = ctrlRect.getX(), cY = ctrlRect.getY() + 15;
    placeKnob(cX + 15, cY, glideLabel, glideSlider);
    placeKnob(cX + 95, cY, pitchLabel, pitchSlider);
    placeKnob(cX + 175, cY, gainLabel, gainSlider);

    leftArea.removeFromTop(10);
    auto modRect = leftArea.removeFromTop(100);
    modEnvGroup.setBounds(modRect);
    int mX = modRect.getX(), mY = modRect.getY() + 15;
    placeKnob(mX + 15, mY, modAtkLabel, modAtkSlider);
    placeKnob(mX + 95, mY, modDecLabel, modDecSlider);
    placeKnob(mX + 175, mY, modSusLabel, modSusSlider);
    placeKnob(mX + 255, mY, modRelLabel, modRelSlider);

    // --- 右側エリア (920px) ---
    area.removeFromLeft(15);
    auto rightArea = area;
    browser.setBounds(rightArea);

    // 1. Wavetable Osc (ワイド2段レイアウト)
    auto oscRect = rightArea.removeFromTop(240);
    oscGroup.setBounds(oscRect);
    int oX = oscRect.getX() + 10, oY = oscRect.getY() + 15;

    // --- 1段目 (Basic & FM) ---
    oscOnButton.setBounds(oX, oY + 20, 50, 24);
    int step = 73; // 11個並べるために間隔を微調整 (75 -> 73)
    placeKnob(oX + 60 + step * 0, oY, wtLevelLabel, wtLevelSlider);
    placeKnob(oX + 60 + step * 1, oY, wtPosLabel, wtPosSlider);       // Positionノブ
    placeKnob(oX + 60 + step * 2, oY, oscPitchLabel, oscPitchSlider);
    placeKnob(oX + 60 + step * 3, oY, pitchDecayAmtLabel, pitchDecayAmtSlider);
    placeKnob(oX + 60 + step * 4, oY, pitchDecayTimeLabel, pitchDecayTimeSlider);
    placeKnob(oX + 60 + step * 5, oY, driftLabel, driftSlider);
    placeKnob(oX + 60 + step * 6, oY, uniCountLabel, uniCountSlider);
    placeKnob(oX + 60 + step * 7, oY, detuneLabel, detuneSlider);
    placeKnob(oX + 60 + step * 8, oY, widthLabel, widthSlider);
    placeKnob(oX + 60 + step * 9, oY, fmAmtLabel, fmAmtSlider);
    placeCombo(oX + 60 + step * 10, oY, 80, fmWaveLabel, fmWaveCombo);

    // --- 2段目 (3 Stage Morph) ---
    int y2 = oY + 100;
    int mWidth = 120; // コンボボックスの幅を広げて見切れを防止

    // Morph A
    placeCombo(oX, y2, mWidth, morphAModeLabel, morphAModeCombo);
    placeKnob(oX + mWidth + 10, y2, morphAAmtLabel, morphAAmtSlider);
    placeKnob(oX + mWidth + 85, y2, morphAShiftLabel, morphAShiftSlider);

    // Morph B
    int oX_B = oX + mWidth + 175;
    placeCombo(oX_B, y2, mWidth, morphBModeLabel, morphBModeCombo);
    placeKnob(oX_B + mWidth + 10, y2, morphBAmtLabel, morphBAmtSlider);
    placeKnob(oX_B + mWidth + 85, y2, morphBShiftLabel, morphBShiftSlider);

    // Morph C
    int oX_C = oX_B + mWidth + 175;
    placeCombo(oX_C, y2, mWidth, morphCModeLabel, morphCModeCombo);
    placeKnob(oX_C + mWidth + 10, y2, morphCAmtLabel, morphCAmtSlider);
    placeKnob(oX_C + mWidth + 85, y2, morphCShiftLabel, morphCShiftSlider);

    rightArea.removeFromTop(10);

    // --- 下部: 横一列レイアウト ---
    int bottomGroupWidth = (rightArea.getWidth() - 30) / 4; // 4つのグループを均等幅で配置

    // 2. Sub Osc
    auto subRect = rightArea.removeFromLeft(bottomGroupWidth);
    subGroup.setBounds(subRect);
    int sbX = subRect.getX() + 10, sbY = subRect.getY() + 15;
    subOnButton.setBounds(sbX, sbY + 20, 50, 24);
    subWaveCombo.setBounds(sbX + 60, sbY + 25, 70, 24);
    placeKnob(sbX + 10, sbY + 60, subVolLabel, subVolSlider);
    placeKnob(sbX + 90, sbY + 60, subPitchLabel, subPitchSlider);

    rightArea.removeFromLeft(10);

    // 3. Shaper
    auto shpRect = rightArea.removeFromLeft(bottomGroupWidth);
    shaperGroup.setBounds(shpRect);
    int sX = shpRect.getX() + 10, sY = shpRect.getY() + 15;
    placeKnob(sX, sY, distDriveLabel, distDriveSlider);
    placeKnob(sX + 80, sY, shpAmtLabel, shpAmtSlider);
    placeKnob(sX, sY + 80, rateLabel, rateSlider);
    placeKnob(sX + 80, sY + 80, bitLabel, bitSlider);

    rightArea.removeFromLeft(10);

    // 4. Filter
    auto fltRect = rightArea.removeFromLeft(bottomGroupWidth);
    filterGroup.setBounds(fltRect);
    int fX = fltRect.getX() + 10, fY = fltRect.getY() + 15;
    placeKnob(fX, fY, cutoffLabel, cutoffSlider);
    placeKnob(fX + 80, fY, resLabel, resSlider);
    placeKnob(fX + 40, fY + 80, fltEnvAmtLabel, fltEnvAmtSlider);

    rightArea.removeFromLeft(10);

    // 5. Amp Envelope
    ampEnvGroup.setBounds(rightArea);
    int eX = rightArea.getX() + 10, eY = rightArea.getY() + 15;
    placeKnob(eX, eY, ampAtkLabel, ampAtkSlider);
    placeKnob(eX + 80, eY, ampDecLabel, ampDecSlider);
    placeKnob(eX, eY + 80, ampSusLabel, ampSusSlider);
    placeKnob(eX + 80, eY + 80, ampRelLabel, ampRelSlider);
}