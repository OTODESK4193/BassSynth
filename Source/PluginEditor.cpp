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

    // Osc Params
    setupS(wtLevelSlider, wtLevelLabel, "Level");
    setupS(wtPosSlider, wtPosLabel, "Pos");
    setupS(fmAmtSlider, fmAmtLabel, "FM Amt");
    setupS(syncSlider, syncLabel, "Sync");

    setupS(morphSlider, morphLabel, "Warp");
    setupS(uniCountSlider, uniCountLabel, "Unison");
    setupS(detuneSlider, detuneLabel, "Detune");
    setupS(widthSlider, widthLabel, "Width");

    setupS(oscPitchSlider, oscPitchLabel, "Pitch");
    setupS(pitchDecayAmtSlider, pitchDecayAmtLabel, "P.Decay");
    setupS(pitchDecayTimeSlider, pitchDecayTimeLabel, "P.Time");
    setupS(driftSlider, driftLabel, "Drift");

    // FM Wave Combo
    addAndMakeVisible(fmWaveCombo);
    fmWaveCombo.addItem("Sine", 1);
    fmWaveCombo.addItem("Saw", 2);
    fmWaveCombo.addItem("Pulse", 3);
    fmWaveCombo.addItem("Noise", 4);
    fmWaveCombo.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(fmWaveLabel);
    fmWaveLabel.setText("FM Mod", juce::dontSendNotification);
    fmWaveLabel.setJustificationType(juce::Justification::centred);
    fmWaveLabel.setFont(12.0f);

    // Sub Osc Params
    addAndMakeVisible(subOnButton);
    addAndMakeVisible(subWaveCombo);
    subWaveCombo.addItem("Sine", 1);
    subWaveCombo.addItem("Triangle", 2);
    subWaveCombo.addItem("Pulse", 3);
    subWaveCombo.addItem("Saw", 4);
    subWaveCombo.setJustificationType(juce::Justification::centred);
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
    att(fmAmtSlider, "osc_fm");
    fmWaveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "osc_fm_wave", fmWaveCombo);
    att(syncSlider, "osc_sync");
    att(morphSlider, "osc_morph");
    att(uniCountSlider, "osc_uni");
    att(detuneSlider, "osc_detune");
    att(widthSlider, "osc_width");
    att(oscPitchSlider, "osc_pitch");
    att(pitchDecayAmtSlider, "osc_pdecay_amt");
    att(pitchDecayTimeSlider, "osc_pdecay_time");
    att(driftSlider, "osc_drift");

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
    // 高さを拡大してフル4段レイアウトを吸収 (820px)
    setSize(1000, 820);
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

    // --- ノブ配置の標準化ラムダ (幅70px, 高さ70px [Label 20px + Slider 50px]) ---
    auto placeKnob = [](int x, int y, juce::Label& lbl, juce::Slider& sld) {
        lbl.setBounds(x, y, 70, 20);
        sld.setBounds(x, y + 20, 70, 50);
        };

    // --- 左側エリア ---
    auto leftArea = area.removeFromLeft(350);

    openBrowserButton.setBounds(leftArea.removeFromTop(35).reduced(2));
    leftArea.removeFromTop(5);

    dualScope.setBounds(leftArea.removeFromTop(310));
    leftArea.removeFromTop(10);

    auto ctrlRect = leftArea.removeFromTop(120);
    controlGroup.setBounds(ctrlRect);
    int cX = ctrlRect.getX(), cY = ctrlRect.getY() + 25;
    placeKnob(cX + 15, cY, glideLabel, glideSlider);
    placeKnob(cX + 95, cY, pitchLabel, pitchSlider);
    placeKnob(cX + 175, cY, gainLabel, gainSlider);

    auto modRect = leftArea.removeFromTop(120);
    modEnvGroup.setBounds(modRect);
    int mX = modRect.getX(), mY = modRect.getY() + 25;
    placeKnob(mX + 15, mY, modAtkLabel, modAtkSlider);
    placeKnob(mX + 95, mY, modDecLabel, modDecSlider);
    placeKnob(mX + 175, mY, modSusLabel, modSusSlider);
    placeKnob(mX + 255, mY, modRelLabel, modRelSlider);

    // --- 右側エリア ---
    area.removeFromLeft(15);
    auto rightArea = area;

    browser.setBounds(rightArea);

    // 1. Wavetable Osc (340px - 4段 x 4列 配置)
    auto oscRect = rightArea.removeFromTop(340);
    oscGroup.setBounds(oscRect);
    int oX = oscRect.getX() + 10, oY = oscRect.getY() + 25;

    // Row 1
    placeKnob(oX, oY, wtLevelLabel, wtLevelSlider);
    placeKnob(oX + 80, oY, wtPosLabel, wtPosSlider);
    placeKnob(oX + 160, oY, morphLabel, morphSlider);
    placeKnob(oX + 240, oY, syncLabel, syncSlider);

    // Row 2
    placeKnob(oX, oY + 80, uniCountLabel, uniCountSlider);
    placeKnob(oX + 80, oY + 80, detuneLabel, detuneSlider);
    placeKnob(oX + 160, oY + 80, widthLabel, widthSlider);
    placeKnob(oX + 240, oY + 80, driftLabel, driftSlider);

    // Row 3
    placeKnob(oX, oY + 160, oscPitchLabel, oscPitchSlider);
    placeKnob(oX + 80, oY + 160, pitchDecayAmtLabel, pitchDecayAmtSlider);
    placeKnob(oX + 160, oY + 160, pitchDecayTimeLabel, pitchDecayTimeSlider);

    // Row 4 (FM Section)
    fmWaveLabel.setBounds(oX, oY + 240, 70, 20);
    fmWaveCombo.setBounds(oX, oY + 260, 70, 24);
    placeKnob(oX + 80, oY + 240, fmAmtLabel, fmAmtSlider);

    rightArea.removeFromTop(10);

    // 2. Sub Osc (100px)
    auto subRect = rightArea.removeFromTop(100);
    subGroup.setBounds(subRect);
    int sbX = subRect.getX() + 10, sbY = subRect.getY() + 25;

    subOnButton.setBounds(sbX, sbY + 20, 50, 24);
    subWaveCombo.setBounds(sbX + 60, sbY + 20, 80, 24);
    placeKnob(sbX + 160, sbY, subVolLabel, subVolSlider);
    placeKnob(sbX + 240, sbY, subPitchLabel, subPitchSlider);

    rightArea.removeFromTop(10);

    // 3. Shaper (100px)
    auto shpRect = rightArea.removeFromTop(100);
    shaperGroup.setBounds(shpRect);
    int sX = shpRect.getX() + 10, sY = shpRect.getY() + 25;
    placeKnob(sX, sY, distDriveLabel, distDriveSlider);
    placeKnob(sX + 80, sY, shpAmtLabel, shpAmtSlider);
    placeKnob(sX + 160, sY, rateLabel, rateSlider);
    placeKnob(sX + 240, sY, bitLabel, bitSlider);

    rightArea.removeFromTop(10);

    // 4. Filter (100px)
    auto fltRect = rightArea.removeFromTop(100);
    filterGroup.setBounds(fltRect);
    int fX = fltRect.getX() + 10, fY = fltRect.getY() + 25;
    placeKnob(fX, fY, cutoffLabel, cutoffSlider);
    placeKnob(fX + 80, fY, resLabel, resSlider);
    placeKnob(fX + 160, fY, fltEnvAmtLabel, fltEnvAmtSlider);

    rightArea.removeFromTop(10);

    // 5. Amp Envelope
    ampEnvGroup.setBounds(rightArea);
    int eX = rightArea.getX() + 10, eY = rightArea.getY() + 25;
    placeKnob(eX, eY, ampAtkLabel, ampAtkSlider);
    placeKnob(eX + 80, eY, ampDecLabel, ampDecSlider);
    placeKnob(eX + 160, eY, ampSusLabel, ampSusSlider);
    placeKnob(eX + 240, eY, ampRelLabel, ampRelSlider);
}