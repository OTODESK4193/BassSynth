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
    subGroup.setText("SUB OSC"); addAndMakeVisible(subGroup); // Sub Oscグループを追加
    shaperGroup.setText("DISTORTION & SHAPER"); addAndMakeVisible(shaperGroup);
    filterGroup.setText("FILTER"); addAndMakeVisible(filterGroup);
    ampEnvGroup.setText("AMP ENVELOPE"); addAndMakeVisible(ampEnvGroup);
    modEnvGroup.setText("MOD ENVELOPE"); addAndMakeVisible(modEnvGroup);
    controlGroup.setText("PERFORMANCE"); addAndMakeVisible(controlGroup);

    // Osc Params
    setupS(wtPosSlider, wtPosLabel, "Pos"); setupS(fmAmtSlider, fmAmtLabel, "FM");
    setupS(syncSlider, syncLabel, "Sync"); setupS(morphSlider, morphLabel, "Warp");
    setupS(uniCountSlider, uniCountLabel, "Unison"); setupS(detuneSlider, detuneLabel, "Detune");
    setupS(oscPitchSlider, oscPitchLabel, "Pitch");
    setupS(driftSlider, driftLabel, "Drift"); // Driftノブ追加

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
    att(wtPosSlider, "osc_pos"); att(fmAmtSlider, "osc_fm"); att(syncSlider, "osc_sync"); att(morphSlider, "osc_morph");
    att(uniCountSlider, "osc_uni"); att(detuneSlider, "osc_detune"); att(oscPitchSlider, "osc_pitch");
    att(driftSlider, "osc_drift"); // Driftバインド

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
    setSize(1000, 700);
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

    // --- 左側エリア (Scope & Performance/Env) ---
    auto leftArea = area.removeFromLeft(350);

    openBrowserButton.setBounds(leftArea.removeFromTop(35).reduced(2));
    leftArea.removeFromTop(5);

    dualScope.setBounds(leftArea.removeFromTop(310));
    leftArea.removeFromTop(10);

    auto leftBottom = leftArea;
    controlGroup.setBounds(leftBottom.removeFromTop(120));
    glideLabel.setBounds(leftBottom.getX() + 10, leftBottom.getY() - 95, 70, 20); glideSlider.setBounds(leftBottom.getX() + 10, leftBottom.getY() - 75, 70, 65);
    pitchLabel.setBounds(leftBottom.getX() + 90, leftBottom.getY() - 95, 70, 20); pitchSlider.setBounds(leftBottom.getX() + 90, leftBottom.getY() - 75, 70, 65);
    gainLabel.setBounds(leftBottom.getX() + 170, leftBottom.getY() - 95, 70, 20); gainSlider.setBounds(leftBottom.getX() + 170, leftBottom.getY() - 75, 70, 65);

    modEnvGroup.setBounds(leftBottom.removeFromTop(120));
    modAtkLabel.setBounds(leftBottom.getX() + 10, leftBottom.getY() - 95, 60, 20); modAtkSlider.setBounds(leftBottom.getX() + 10, leftBottom.getY() - 75, 60, 65);
    modDecLabel.setBounds(leftBottom.getX() + 80, leftBottom.getY() - 95, 60, 20); modDecSlider.setBounds(leftBottom.getX() + 80, leftBottom.getY() - 75, 60, 65);
    modSusLabel.setBounds(leftBottom.getX() + 150, leftBottom.getY() - 95, 60, 20); modSusSlider.setBounds(leftBottom.getX() + 150, leftBottom.getY() - 75, 60, 65);
    modRelLabel.setBounds(leftBottom.getX() + 220, leftBottom.getY() - 95, 60, 20); modRelSlider.setBounds(leftBottom.getX() + 220, leftBottom.getY() - 75, 60, 65);

    // --- 右側エリア (Osc, Sub, Shaper, Filter, AmpEnv) ---
    area.removeFromLeft(15);
    auto rightArea = area;

    browser.setBounds(rightArea);

    // 1. Wavetable Osc
    auto oscRect = rightArea.removeFromTop(200);
    oscGroup.setBounds(oscRect);
    int oX = oscRect.getX(), oY = oscRect.getY();
    wtPosLabel.setBounds(oX + 10, oY + 35, 70, 20); wtPosSlider.setBounds(oX + 10, oY + 55, 70, 60);
    fmAmtLabel.setBounds(oX + 90, oY + 35, 70, 20); fmAmtSlider.setBounds(oX + 90, oY + 55, 70, 60);
    syncLabel.setBounds(oX + 170, oY + 35, 70, 20); syncSlider.setBounds(oX + 170, oY + 55, 70, 60);
    morphLabel.setBounds(oX + 250, oY + 35, 70, 20); morphSlider.setBounds(oX + 250, oY + 55, 70, 60);

    uniCountLabel.setBounds(oX + 10, oY + 120, 70, 20); uniCountSlider.setBounds(oX + 10, oY + 140, 70, 60);
    detuneLabel.setBounds(oX + 90, oY + 120, 70, 20); detuneSlider.setBounds(oX + 90, oY + 140, 70, 60);
    oscPitchLabel.setBounds(oX + 170, oY + 120, 70, 20); oscPitchSlider.setBounds(oX + 170, oY + 140, 70, 60);
    driftLabel.setBounds(oX + 250, oY + 120, 70, 20); driftSlider.setBounds(oX + 250, oY + 140, 70, 60); // Driftノブ

    rightArea.removeFromTop(10);

    // 2. Sub Osc
    auto subRect = rightArea.removeFromTop(90); // Sub Osc用のスペース
    subGroup.setBounds(subRect);
    int sbX = subRect.getX(), sbY = subRect.getY();
    subOnButton.setBounds(sbX + 15, sbY + 40, 50, 24);
    subWaveCombo.setBounds(sbX + 75, sbY + 40, 80, 24);
    subVolLabel.setBounds(sbX + 170, sbY + 20, 70, 20); subVolSlider.setBounds(sbX + 170, sbY + 35, 70, 50);
    subPitchLabel.setBounds(sbX + 250, sbY + 20, 70, 20); subPitchSlider.setBounds(sbX + 250, sbY + 35, 70, 50);

    rightArea.removeFromTop(10);

    // 3. Shaper
    auto shpRect = rightArea.removeFromTop(110);
    shaperGroup.setBounds(shpRect);
    int sX = shpRect.getX(), sY = shpRect.getY();
    distDriveLabel.setBounds(sX + 10, sY + 25, 70, 20); distDriveSlider.setBounds(sX + 10, sY + 45, 70, 60);
    shpAmtLabel.setBounds(sX + 90, sY + 25, 70, 20); shpAmtSlider.setBounds(sX + 90, sY + 45, 70, 60);
    bitLabel.setBounds(sX + 170, sY + 25, 70, 20); bitSlider.setBounds(sX + 170, sY + 45, 70, 60);
    rateLabel.setBounds(sX + 250, sY + 25, 70, 20); rateSlider.setBounds(sX + 250, sY + 45, 70, 60);

    rightArea.removeFromTop(10);

    // 4. Filter
    auto fltRect = rightArea.removeFromTop(110);
    filterGroup.setBounds(fltRect);
    int fX = fltRect.getX(), fY = fltRect.getY();
    cutoffLabel.setBounds(fX + 10, fY + 25, 70, 20); cutoffSlider.setBounds(fX + 10, fY + 45, 70, 60);
    resLabel.setBounds(fX + 90, fY + 25, 70, 20); resSlider.setBounds(fX + 90, fY + 45, 70, 60);
    fltEnvAmtLabel.setBounds(fX + 170, fY + 25, 70, 20); fltEnvAmtSlider.setBounds(fX + 170, fY + 45, 70, 60);

    rightArea.removeFromTop(10);

    // 5. Amp Envelope
    ampEnvGroup.setBounds(rightArea); // 残りのスペースを使用
    int eX = rightArea.getX(), eY = rightArea.getY();
    ampAtkLabel.setBounds(eX + 10, eY + 25, 60, 20); ampAtkSlider.setBounds(eX + 10, eY + 45, 60, 60);
    ampDecLabel.setBounds(eX + 80, eY + 25, 60, 20); ampDecSlider.setBounds(eX + 80, eY + 45, 60, 60);
    ampSusLabel.setBounds(eX + 150, eY + 25, 60, 20); ampSusSlider.setBounds(eX + 150, eY + 45, 60, 60);
    ampRelLabel.setBounds(eX + 220, eY + 25, 60, 20); ampRelSlider.setBounds(eX + 220, eY + 45, 60, 60);
}