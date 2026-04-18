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

    // Osc Params (12 Knobs total)
    setupS(wtLevelSlider, wtLevelLabel, "Level");
    setupS(wtPosSlider, wtPosLabel, "Pos");
    setupS(fmAmtSlider, fmAmtLabel, "FM");
    setupS(syncSlider, syncLabel, "Sync");

    setupS(morphSlider, morphLabel, "Warp");
    setupS(uniCountSlider, uniCountLabel, "Unison");
    setupS(detuneSlider, detuneLabel, "Detune");
    setupS(widthSlider, widthLabel, "Width");

    setupS(oscPitchSlider, oscPitchLabel, "Pitch");
    setupS(pitchDecayAmtSlider, pitchDecayAmtLabel, "P.Decay"); // 新規追加
    setupS(pitchDecayTimeSlider, pitchDecayTimeLabel, "P.Time"); // 新規追加
    setupS(driftSlider, driftLabel, "Drift");

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
    att(syncSlider, "osc_sync");
    att(morphSlider, "osc_morph");
    att(uniCountSlider, "osc_uni");
    att(detuneSlider, "osc_detune");
    att(widthSlider, "osc_width");
    att(oscPitchSlider, "osc_pitch");
    att(pitchDecayAmtSlider, "osc_pdecay_amt"); // 追加
    att(pitchDecayTimeSlider, "osc_pdecay_time"); // 追加
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
    // 高さを少し増やしてWavetableエリアの拡張分（+60px）を吸収
    setSize(1000, 760);
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

    // Scopeの高さはそのまま
    dualScope.setBounds(leftArea.removeFromTop(310));
    leftArea.removeFromTop(10);

    auto ctrlRect = leftArea.removeFromTop(120);
    controlGroup.setBounds(ctrlRect);
    int cX = ctrlRect.getX(), cY = ctrlRect.getY();
    glideLabel.setBounds(cX + 10, cY + 20, 70, 20); glideSlider.setBounds(cX + 10, cY + 40, 70, 65);
    pitchLabel.setBounds(cX + 90, cY + 20, 70, 20); pitchSlider.setBounds(cX + 90, cY + 40, 70, 65);
    gainLabel.setBounds(cX + 170, cY + 20, 70, 20); gainSlider.setBounds(cX + 170, cY + 40, 70, 65);

    auto modRect = leftArea.removeFromTop(120);
    modEnvGroup.setBounds(modRect);
    int mX = modRect.getX(), mY = modRect.getY();
    modAtkLabel.setBounds(mX + 10, mY + 20, 60, 20); modAtkSlider.setBounds(mX + 10, mY + 40, 60, 65);
    modDecLabel.setBounds(mX + 80, mY + 20, 60, 20); modDecSlider.setBounds(mX + 80, mY + 40, 60, 65);
    modSusLabel.setBounds(mX + 150, mY + 20, 60, 20); modSusSlider.setBounds(mX + 150, mY + 40, 60, 65);
    modRelLabel.setBounds(mX + 220, mY + 20, 60, 20); modRelSlider.setBounds(mX + 220, mY + 40, 60, 65);

    // --- 右側エリア (Osc, Sub, Shaper, Filter, AmpEnv) ---
    area.removeFromLeft(15);
    auto rightArea = area;

    browser.setBounds(rightArea);

    // 1. Wavetable Osc (260px に拡大して3段配置)
    auto oscRect = rightArea.removeFromTop(260);
    oscGroup.setBounds(oscRect);
    int oX = oscRect.getX(), oY = oscRect.getY();

    // 1段目：Level, Pos, FM, Sync
    wtLevelLabel.setBounds(oX + 10, oY + 20, 70, 20); wtLevelSlider.setBounds(oX + 10, oY + 40, 70, 60);
    wtPosLabel.setBounds(oX + 90, oY + 20, 70, 20);   wtPosSlider.setBounds(oX + 90, oY + 40, 70, 60);
    fmAmtLabel.setBounds(oX + 170, oY + 20, 70, 20);  fmAmtSlider.setBounds(oX + 170, oY + 40, 70, 60);
    syncLabel.setBounds(oX + 250, oY + 20, 70, 20);   syncSlider.setBounds(oX + 250, oY + 40, 70, 60);

    // 2段目：Warp, Unison, Detune, Width
    morphLabel.setBounds(oX + 10, oY + 100, 70, 20);    morphSlider.setBounds(oX + 10, oY + 120, 70, 60);
    uniCountLabel.setBounds(oX + 90, oY + 100, 70, 20); uniCountSlider.setBounds(oX + 90, oY + 120, 70, 60);
    detuneLabel.setBounds(oX + 170, oY + 100, 70, 20);  detuneSlider.setBounds(oX + 170, oY + 120, 70, 60);
    widthLabel.setBounds(oX + 250, oY + 100, 70, 20);   widthSlider.setBounds(oX + 250, oY + 120, 70, 60);

    // 3段目：Pitch, P.Decay, P.Time, Drift
    oscPitchLabel.setBounds(oX + 10, oY + 180, 70, 20);       oscPitchSlider.setBounds(oX + 10, oY + 200, 70, 60);
    pitchDecayAmtLabel.setBounds(oX + 90, oY + 180, 70, 20);  pitchDecayAmtSlider.setBounds(oX + 90, oY + 200, 70, 60);
    pitchDecayTimeLabel.setBounds(oX + 170, oY + 180, 70, 20); pitchDecayTimeSlider.setBounds(oX + 170, oY + 200, 70, 60);
    driftLabel.setBounds(oX + 250, oY + 180, 70, 20);         driftSlider.setBounds(oX + 250, oY + 200, 70, 60);

    rightArea.removeFromTop(10);

    // 2. Sub Osc (85px)
    auto subRect = rightArea.removeFromTop(85);
    subGroup.setBounds(subRect);
    int sbX = subRect.getX(), sbY = subRect.getY();
    subOnButton.setBounds(sbX + 15, sbY + 35, 50, 24);
    subWaveCombo.setBounds(sbX + 75, sbY + 35, 80, 24);
    subVolLabel.setBounds(sbX + 170, sbY + 15, 70, 20); subVolSlider.setBounds(sbX + 170, sbY + 30, 70, 50);
    subPitchLabel.setBounds(sbX + 250, sbY + 15, 70, 20); subPitchSlider.setBounds(sbX + 250, sbY + 30, 70, 50);

    rightArea.removeFromTop(10);

    // 3. Shaper (110px)
    auto shpRect = rightArea.removeFromTop(110);
    shaperGroup.setBounds(shpRect);
    int sX = shpRect.getX(), sY = shpRect.getY();
    shpAmtLabel.setBounds(sX + 10, sY + 20, 70, 20); shpAmtSlider.setBounds(sX + 10, sY + 40, 70, 60);
    distDriveLabel.setBounds(sX + 90, sY + 20, 70, 20); distDriveSlider.setBounds(sX + 90, sY + 40, 70, 60);
    bitLabel.setBounds(sX + 170, sY + 20, 70, 20); bitSlider.setBounds(sX + 170, sY + 40, 70, 60);
    rateLabel.setBounds(sX + 250, sY + 20, 70, 20); rateSlider.setBounds(sX + 250, sY + 40, 70, 60);

    rightArea.removeFromTop(10);

    // 4. Filter (110px)
    auto fltRect = rightArea.removeFromTop(110);
    filterGroup.setBounds(fltRect);
    int fX = fltRect.getX(), fY = fltRect.getY();
    cutoffLabel.setBounds(fX + 10, fY + 20, 70, 20); cutoffSlider.setBounds(fX + 10, fY + 40, 70, 60);
    resLabel.setBounds(fX + 90, fY + 20, 70, 20); resSlider.setBounds(fX + 90, fY + 40, 70, 60);
    fltEnvAmtLabel.setBounds(fX + 170, fY + 20, 70, 20); fltEnvAmtSlider.setBounds(fX + 170, fY + 40, 70, 60);

    rightArea.removeFromTop(10);

    // 5. Amp Envelope (Remaining Area)
    ampEnvGroup.setBounds(rightArea);
    int eX = rightArea.getX(), eY = rightArea.getY();
    ampAtkLabel.setBounds(eX + 10, eY + 20, 60, 20); ampAtkSlider.setBounds(eX + 10, eY + 40, 60, 60);
    ampDecLabel.setBounds(eX + 80, eY + 20, 60, 20); ampDecSlider.setBounds(eX + 80, eY + 40, 60, 60);
    ampSusLabel.setBounds(eX + 150, eY + 20, 60, 20); ampSusSlider.setBounds(eX + 150, eY + 40, 60, 60);
    ampRelLabel.setBounds(eX + 220, eY + 20, 60, 20); ampRelSlider.setBounds(eX + 220, eY + 40, 60, 60);
}