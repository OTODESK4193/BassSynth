// ==============================================================================
// Source/PluginEditor.cpp
// ==============================================================================
#include "PluginProcessor.h"
#include "PluginEditor.h"

// --- Helpers ---
static void setupS(juce::Slider& s, juce::Label& l, const char* txt, juce::Component* parent) {
    parent->addAndMakeVisible(s);
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    parent->addAndMakeVisible(l);
    l.setText(txt, juce::dontSendNotification);
    l.setJustificationType(juce::Justification::centred);
    l.setFont(12.0f);
}

static void placeKnob(int x, int y, juce::Label& lbl, juce::Slider& sld) {
    lbl.setBounds(x, y, 70, 20);
    sld.setBounds(x, y + 20, 70, 50);
}

static void setupCombo(juce::ComboBox& c, juce::Label& l, const char* txt, juce::StringArray items, juce::Component* parent) {
    parent->addAndMakeVisible(c);
    c.addItemList(items, 1);
    c.setJustificationType(juce::Justification::centred);
    if (txt != nullptr) {
        parent->addAndMakeVisible(l);
        l.setText(txt, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setFont(12.0f);
    }
}

// ==============================================================================
// LfoTab Implementation
// ==============================================================================
LfoTab::LfoTab(juce::AudioProcessorValueTreeState& vts) : apvts(vts) {
    juce::StringArray waves = { "Sine", "Saw", "Pulse", "Random" };
    juce::StringArray beats = { "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/4T", "1/8T", "1/16T" };

    for (int i = 0; i < 3; ++i) {
        addAndMakeVisible(lfos[i].onBtn);
        setupCombo(lfos[i].wave, lfos[i].waveLbl, "Wave", waves, this);
        setupCombo(lfos[i].beat, lfos[i].beatLbl, "Beat", beats, this);
        setupS(lfos[i].rate, lfos[i].rateLbl, "Rate(Hz)", this);
        setupS(lfos[i].amt, lfos[i].amtLbl, "Amount", this);
        addAndMakeVisible(lfos[i].sync);

        juce::String pfx = "lfo" + juce::String(i + 1) + "_";
        btnAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, pfx + "on", lfos[i].onBtn));
        comboAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, pfx + "wave", lfos[i].wave));
        comboAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, pfx + "beat", lfos[i].beat));
        sliderAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, pfx + "rate", lfos[i].rate));
        sliderAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, pfx + "amt", lfos[i].amt));
        btnAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, pfx + "sync", lfos[i].sync));
    }
}
void LfoTab::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour::fromString("FF1A1A1A"));
    for (int i = 0; i < 3; ++i) {
        g.setColour(juce::Colours::white.withAlpha(0.05f));
        g.fillRoundedRectangle(10, 10 + i * 130, getWidth() - 20, 115, 5.0f);
        g.setColour(juce::Colour::fromString("FFFF764D"));
        g.setFont(14.0f);
        g.drawText("LFO " + juce::String(i + 1), 20, 15 + i * 130, 50, 20, juce::Justification::centredLeft);
    }
}
void LfoTab::resized() {
    for (int i = 0; i < 3; ++i) {
        int y = 30 + i * 130;
        lfos[i].onBtn.setBounds(20, y + 15, 45, 24);
        lfos[i].waveLbl.setBounds(85, y + 5, 80, 20);
        lfos[i].wave.setBounds(85, y + 25, 80, 24);
        lfos[i].sync.setBounds(180, y + 25, 60, 24);
        placeKnob(245, y, lfos[i].rateLbl, lfos[i].rate);
        lfos[i].beatLbl.setBounds(330, y + 5, 70, 20);
        lfos[i].beat.setBounds(330, y + 25, 70, 24);
        placeKnob(415, y, lfos[i].amtLbl, lfos[i].amt);
    }
}

// ==============================================================================
// ModEnvTab Implementation
// ==============================================================================
ModEnvTab::ModEnvTab(juce::AudioProcessorValueTreeState& vts) : apvts(vts) {
    for (int i = 0; i < 3; ++i) {
        addAndMakeVisible(envs[i].onBtn);
        setupS(envs[i].a, envs[i].aL, "A", this);
        setupS(envs[i].d, envs[i].dL, "D", this);
        setupS(envs[i].s, envs[i].sL, "S", this);
        setupS(envs[i].r, envs[i].rL, "R", this);
        setupS(envs[i].amt, envs[i].amtL, "Amount", this);

        juce::String pfx = "mod" + juce::String(i + 1) + "_";
        btnAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, pfx + "on", envs[i].onBtn));
        sliderAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, pfx + "atk", envs[i].a));
        sliderAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, pfx + "dec", envs[i].d));
        sliderAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, pfx + "sus", envs[i].s));
        sliderAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, pfx + "rel", envs[i].r));
        sliderAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, pfx + "amt", envs[i].amt));
    }
}
void ModEnvTab::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour::fromString("FF1A1A1A"));
    for (int i = 0; i < 3; ++i) {
        g.setColour(juce::Colours::white.withAlpha(0.05f));
        g.fillRoundedRectangle(10, 10 + i * 130, getWidth() - 20, 115, 5.0f);
        g.setColour(juce::Colour::fromString("FFFF764D"));
        g.setFont(14.0f);
        g.drawText("ENV " + juce::String(i + 1), 20, 15 + i * 130, 50, 20, juce::Justification::centredLeft);
    }
}
void ModEnvTab::resized() {
    for (int i = 0; i < 3; ++i) {
        int y = 30 + i * 130;
        envs[i].onBtn.setBounds(20, y + 15, 45, 24);
        placeKnob(85, y, envs[i].aL, envs[i].a);
        placeKnob(160, y, envs[i].dL, envs[i].d);
        placeKnob(235, y, envs[i].sL, envs[i].s);
        placeKnob(310, y, envs[i].rL, envs[i].r);
        placeKnob(415, y, envs[i].amtL, envs[i].amt);
    }
}

// ==============================================================================
// MatrixTab Implementation
// ==============================================================================
MatrixTab::MatrixTab(juce::AudioProcessorValueTreeState& vts) : apvts(vts) {
    juce::StringArray dests = {
        "None", "WT: Position", "WT: FM Amt", "WT: MorphA Amt", "WT: MorphA Shf",
        "WT: MorphB Amt", "WT: MorphB Shf", "WT: MorphC Amt", "WT: MorphC Shf",
        "FLT: Cutoff", "FLT: Resonance", "PRF: Gain"
    };
    for (int src = 0; src < 6; ++src) {
        for (int slot = 0; slot < 3; ++slot) {
            setupCombo(rows[src].slots[slot].dest, rows[src].slots[slot].dummyLbl, nullptr, dests, this);
            addAndMakeVisible(rows[src].slots[slot].amt);
            rows[src].slots[slot].amt.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            rows[src].slots[slot].amt.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);

            juce::String dId = "matrix_s" + juce::String(src) + "_d" + juce::String(slot);
            juce::String aId = "matrix_s" + juce::String(src) + "_a" + juce::String(slot);
            comboAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, dId, rows[src].slots[slot].dest));
            sliderAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, aId, rows[src].slots[slot].amt));
        }
    }
}
void MatrixTab::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour::fromString("FF1A1A1A"));
    juce::StringArray srcNames = { "MOD 1", "MOD 2", "MOD 3", "LFO 1", "LFO 2", "LFO 3" };
    for (int i = 0; i < 6; ++i) {
        g.setColour(juce::Colours::white.withAlpha(i % 2 == 0 ? 0.03f : 0.00f));
        g.fillRect(0, i * 65, getWidth(), 65);
        g.setColour(juce::Colour::fromString("FFDDDDDD"));
        g.setFont(13.0f);
        g.drawText(srcNames[i], 10, i * 65, 55, 65, juce::Justification::centredLeft);
    }
}
void MatrixTab::resized() {
    for (int src = 0; src < 6; ++src) {
        int y = src * 65 + 18;
        for (int slot = 0; slot < 3; ++slot) {
            int x = 70 + slot * 140;
            rows[src].slots[slot].dest.setBounds(x, y + 3, 90, 24);
            rows[src].slots[slot].amt.setBounds(x + 95, y, 30, 30);
        }
    }
}

// ==============================================================================
// LiquidDreamAudioProcessorEditor Implementation
// ==============================================================================
LiquidDreamAudioProcessorEditor::LiquidDreamAudioProcessorEditor(LiquidDreamAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
    dualScope(p.getOutputScopePtr()), browser(p.getAPVTS()),
    modTabs(juce::TabbedButtonBar::TabsAtTop),
    lfoTab(p.getAPVTS()), modEnvTab(p.getAPVTS()), matrixTab(p.getAPVTS())
{
    setLookAndFeel(&abletonLnF);
    addAndMakeVisible(openBrowserButton); addAndMakeVisible(prevWaveButton);
    addAndMakeVisible(nextWaveButton); addAndMakeVisible(rndWaveButton);
    addAndMakeVisible(dualScope);

    oscGroup.setText("WAVETABLE"); addAndMakeVisible(oscGroup);
    subGroup.setText("SUB OSC"); addAndMakeVisible(subGroup);
    shaperGroup.setText("DISTORTION & SHAPER"); addAndMakeVisible(shaperGroup);
    filterGroup.setText("FILTER"); addAndMakeVisible(filterGroup);
    ampEnvGroup.setText("AMP ENVELOPE"); addAndMakeVisible(ampEnvGroup);
    filterEnvGroup.setText("FILTER ENVELOPE"); addAndMakeVisible(filterEnvGroup);
    controlGroup.setText("PERFORMANCE"); addAndMakeVisible(controlGroup);

    addAndMakeVisible(oscOnButton);
    setupS(wtLevelSlider, wtLevelLabel, "Level", this); setupS(wtPosSlider, wtPosLabel, "Pos", this);
    setupS(oscPitchSlider, oscPitchLabel, "Pitch", this); setupS(pitchDecayAmtSlider, pitchDecayAmtLabel, "P.Decay", this);
    setupS(pitchDecayTimeSlider, pitchDecayTimeLabel, "P.Time", this); setupS(driftSlider, driftLabel, "Drift", this);
    setupS(uniCountSlider, uniCountLabel, "Unison", this); setupS(detuneSlider, detuneLabel, "Detune", this);
    setupS(widthSlider, widthLabel, "Width", this); setupS(fmAmtSlider, fmAmtLabel, "FM Amt", this);
    setupCombo(fmWaveCombo, fmWaveLabel, "FM Mod", { "Sine", "Saw", "Pulse", "Triangle" }, this);

    juce::StringArray morphTypes = { "None", "Bend (+/-)", "PWM", "Sync", "Mirror", "Flip", "Quantize", "Remap", "Smear", "Vocode", "Stretch", "SpecCut", "Shepard", "Comb" };
    setupCombo(morphAModeCombo, morphAModeLabel, "Morph A", morphTypes, this); setupS(morphAAmtSlider, morphAAmtLabel, "Amt A", this); setupS(morphAShiftSlider, morphAShiftLabel, "Shift A", this);
    setupCombo(morphBModeCombo, morphBModeLabel, "Morph B", morphTypes, this); setupS(morphBAmtSlider, morphBAmtLabel, "Amt B", this); setupS(morphBShiftSlider, morphBShiftLabel, "Shift B", this);
    setupCombo(morphCModeCombo, morphCModeLabel, "Morph C", morphTypes, this); setupS(morphCAmtSlider, morphCAmtLabel, "Amt C", this); setupS(morphCShiftSlider, morphCShiftLabel, "Shift C", this);

    addAndMakeVisible(subOnButton);
    setupCombo(subWaveCombo, subVolLabel, "", { "Sine", "Triangle", "Pulse", "Saw" }, this);
    subVolLabel.setText("Wave", juce::dontSendNotification);
    setupS(subVolSlider, subVolLabel, "Level", this); setupS(subPitchSlider, subPitchLabel, "Pitch", this);

    setupS(distDriveSlider, distDriveLabel, "Drive", this); setupS(shpAmtSlider, shpAmtLabel, "Shaper", this);
    setupS(bitSlider, bitLabel, "Bits", this); setupS(rateSlider, rateLabel, "Rate", this);
    setupS(cutoffSlider, cutoffLabel, "Cutoff", this); setupS(resSlider, resLabel, "Reso", this); setupS(fltEnvAmtSlider, fltEnvAmtLabel, "EnvAmt", this);

    setupS(ampAtkSlider, ampAtkLabel, "A", this); setupS(ampDecSlider, ampDecLabel, "D", this); setupS(ampSusSlider, ampSusLabel, "S", this); setupS(ampRelSlider, ampRelLabel, "R", this);
    setupS(fltAtkSlider, fltAtkLabel, "A", this); setupS(fltDecSlider, fltDecLabel, "D", this); setupS(fltSusSlider, fltSusLabel, "S", this); setupS(fltRelSlider, fltRelLabel, "R", this);
    setupS(glideSlider, glideLabel, "Glide", this); setupS(pitchSlider, pitchLabel, "Pitch", this); setupS(gainSlider, gainLabel, "Gain", this);
    addAndMakeVisible(legatoButton);

    auto& apvts = audioProcessor.getAPVTS();
    auto att = [&](juce::Slider& s, const juce::String& id) { attachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, id, s)); };

    oscOnAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "osc_on", oscOnButton);
    att(wtLevelSlider, "osc_level"); att(wtPosSlider, "osc_pos"); att(oscPitchSlider, "osc_pitch");
    att(pitchDecayAmtSlider, "osc_pdecay_amt"); att(pitchDecayTimeSlider, "osc_pdecay_time");
    att(driftSlider, "osc_drift"); att(uniCountSlider, "osc_uni"); att(detuneSlider, "osc_detune");
    att(widthSlider, "osc_width"); att(fmAmtSlider, "osc_fm");
    fmWaveAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "osc_fm_wave", fmWaveCombo);

    morphAAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "osc_morph_a_mode", morphAModeCombo);
    att(morphAAmtSlider, "osc_morph_a_amt"); att(morphAShiftSlider, "osc_morph_a_shift");
    morphBAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "osc_morph_b_mode", morphBModeCombo);
    att(morphBAmtSlider, "osc_morph_b_amt"); att(morphBShiftSlider, "osc_morph_b_shift");
    morphCAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "osc_morph_c_mode", morphCModeCombo);
    att(morphCAmtSlider, "osc_morph_c_amt"); att(morphCShiftSlider, "osc_morph_c_shift");

    subOnAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "sub_on", subOnButton);
    subWaveAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "sub_wave", subWaveCombo);
    att(subVolSlider, "sub_vol"); att(subPitchSlider, "sub_pitch");
    legatoAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "m_legato", legatoButton);

    att(distDriveSlider, "dist_drive"); att(shpAmtSlider, "shp_amt"); att(bitSlider, "shp_bit"); att(rateSlider, "shp_rate");
    att(cutoffSlider, "flt_cutoff"); att(resSlider, "flt_res"); att(fltEnvAmtSlider, "flt_env_amt");
    att(ampAtkSlider, "a_atk"); att(ampDecSlider, "a_dec"); att(ampSusSlider, "a_sus"); att(ampRelSlider, "a_rel");
    att(fltAtkSlider, "f_atk"); att(fltDecSlider, "f_dec"); att(fltSusSlider, "f_sus"); att(fltRelSlider, "f_rel");
    att(glideSlider, "m_glide"); att(pitchSlider, "m_pb"); att(gainSlider, "m_gain");

    addAndMakeVisible(modTabs);
    modTabs.addTab("LFOs", juce::Colour::fromString("FF2A2A2A"), &lfoTab, false);
    modTabs.addTab("MOD ENVs", juce::Colour::fromString("FF2A2A2A"), &modEnvTab, false);
    modTabs.addTab("MATRIX", juce::Colour::fromString("FF2A2A2A"), &matrixTab, false);

    addChildComponent(browser); browser.setVisible(false);
    openBrowserButton.onClick = [this] { browser.setVisible(!browser.isVisible()); if (browser.isVisible()) browser.toFront(true); };
    prevWaveButton.onClick = [this] { browser.selectPrev(); }; nextWaveButton.onClick = [this] { browser.selectNext(); };
    rndWaveButton.onClick = [this] { browser.selectRandom(); };

    startTimerHz(30);
    setSize(1300, 700);
}

LiquidDreamAudioProcessorEditor::~LiquidDreamAudioProcessorEditor() { setLookAndFeel(nullptr); }

void LiquidDreamAudioProcessorEditor::paint(juce::Graphics& g) { g.fillAll(juce::Colour::fromString("FF1E1E1E")); }

void LiquidDreamAudioProcessorEditor::timerCallback() {
    // 1. スコープ波形の更新
    std::array<float, 512> tempBuffer;
    audioProcessor.getStaticWaveform(tempBuffer);
    dualScope.updateStaticWave(tempBuffer.data(), 512);

    // 2. モジュレーション・リング（ピンクArc）の更新ロジック
    float modDepths[12] = { 0.0f };
    auto& apvts = audioProcessor.getAPVTS();

    // マトリックス全探索によるモジュレーション深度の合算
    for (int src = 0; src < 6; ++src) {
        // ソースがOFFの場合は加算しない
        juce::String srcPrefix = (src < 3) ? "mod" + juce::String(src + 1) : "lfo" + juce::String(src - 2);
        if (apvts.getRawParameterValue(srcPrefix + "_on")->load() < 0.5f) continue;

        for (int slot = 0; slot < 3; ++slot) {
            int dest = (int)apvts.getRawParameterValue("matrix_s" + juce::String(src) + "_d" + juce::String(slot))->load();
            float amt = std::abs(apvts.getRawParameterValue("matrix_s" + juce::String(src) + "_a" + juce::String(slot))->load());
            if (dest > 0 && dest < 12) {
                modDepths[dest] += amt;
            }
        }
    }

    // スライダーへのプロパティ適用ヘルパー
    auto updateRing = [](juce::Slider& s, float depth) {
        if (depth > 0.001f) {
            s.getProperties().set("mod_active", true);
            auto range = s.getNormalisableRange();
            float pNorm = range.convertTo0to1((float)s.getValue());
            s.getProperties().set("mod_min", juce::jlimit(0.0f, 1.0f, pNorm - depth));
            s.getProperties().set("mod_max", juce::jlimit(0.0f, 1.0f, pNorm + depth));
        }
        else {
            s.getProperties().set("mod_active", false);
        }
        s.repaint();
        };

    // 各アサイン先スライダーへの適用
    updateRing(wtPosSlider, modDepths[1]);
    updateRing(fmAmtSlider, modDepths[2]);
    updateRing(morphAAmtSlider, modDepths[3]);
    updateRing(morphAShiftSlider, modDepths[4]);
    updateRing(morphBAmtSlider, modDepths[5]);
    updateRing(morphBShiftSlider, modDepths[6]);
    updateRing(morphCAmtSlider, modDepths[7]);
    updateRing(morphCShiftSlider, modDepths[8]);
    updateRing(cutoffSlider, modDepths[9]);
    updateRing(resSlider, modDepths[10]);
    updateRing(gainSlider, modDepths[11]);
}

void LiquidDreamAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(15);

    auto placeComboLocal = [](int x, int y, int w, juce::Label& lbl, juce::ComboBox& cmb) {
        lbl.setBounds(x, y, w, 20); cmb.setBounds(x, y + 30, w, 24);
        };

    // --- 左側エリア (350px) ---
    auto leftArea = area.removeFromLeft(350);
    auto navRect = leftArea.removeFromTop(35).reduced(2);
    openBrowserButton.setBounds(navRect.removeFromLeft(110)); navRect.removeFromLeft(5);
    prevWaveButton.setBounds(navRect.removeFromLeft(40)); navRect.removeFromLeft(5);
    nextWaveButton.setBounds(navRect.removeFromLeft(40)); navRect.removeFromLeft(5);
    rndWaveButton.setBounds(navRect.removeFromLeft(60));

    leftArea.removeFromTop(5);
    dualScope.setBounds(leftArea.removeFromTop(370));
    leftArea.removeFromTop(10);

    auto ctrlRect = leftArea.removeFromTop(100);
    controlGroup.setBounds(ctrlRect);
    int cX = ctrlRect.getX(), cY = ctrlRect.getY() + 15;
    legatoButton.setBounds(cX + 10, cY + 20, 75, 24);
    placeKnob(cX + 80, cY, glideLabel, glideSlider);
    placeKnob(cX + 160, cY, pitchLabel, pitchSlider);
    placeKnob(cX + 240, cY, gainLabel, gainSlider);

    leftArea.removeFromTop(10);
    auto subRect = leftArea.removeFromTop(120);
    subGroup.setBounds(subRect);
    int sbX = subRect.getX() + 10, sbY = subRect.getY() + 15;
    subOnButton.setBounds(sbX, sbY + 20, 50, 24);
    subWaveCombo.setBounds(sbX + 60, sbY + 25, 70, 24);
    placeKnob(sbX + 160, sbY, subVolLabel, subVolSlider);
    placeKnob(sbX + 240, sbY, subPitchLabel, subPitchSlider);

    // --- 右側エリア (920px) ---
    area.removeFromLeft(15);
    auto rightArea = area;
    browser.setBounds(rightArea);

    auto oscRect = rightArea.removeFromTop(240);
    oscGroup.setBounds(oscRect);
    int oX = oscRect.getX() + 10, oY = oscRect.getY() + 15;

    oscOnButton.setBounds(oX, oY + 20, 50, 24);
    int step = 73;
    placeKnob(oX + 60 + step * 0, oY, wtLevelLabel, wtLevelSlider);
    placeKnob(oX + 60 + step * 1, oY, wtPosLabel, wtPosSlider);
    placeKnob(oX + 60 + step * 2, oY, oscPitchLabel, oscPitchSlider);
    placeKnob(oX + 60 + step * 3, oY, pitchDecayAmtLabel, pitchDecayAmtSlider);
    placeKnob(oX + 60 + step * 4, oY, pitchDecayTimeLabel, pitchDecayTimeSlider);
    placeKnob(oX + 60 + step * 5, oY, driftLabel, driftSlider);
    placeKnob(oX + 60 + step * 6, oY, uniCountLabel, uniCountSlider);
    placeKnob(oX + 60 + step * 7, oY, detuneLabel, detuneSlider);
    placeKnob(oX + 60 + step * 8, oY, widthLabel, widthSlider);
    placeKnob(oX + 60 + step * 9, oY, fmAmtLabel, fmAmtSlider);
    placeComboLocal(oX + 60 + step * 10, oY, 80, fmWaveLabel, fmWaveCombo);

    int y2 = oY + 100, mWidth = 120;
    placeComboLocal(oX, y2, mWidth, morphAModeLabel, morphAModeCombo);
    placeKnob(oX + mWidth + 10, y2, morphAAmtLabel, morphAAmtSlider);
    placeKnob(oX + mWidth + 85, y2, morphAShiftLabel, morphAShiftSlider);

    int oX_B = oX + mWidth + 175;
    placeComboLocal(oX_B, y2, mWidth, morphBModeLabel, morphBModeCombo);
    placeKnob(oX_B + mWidth + 10, y2, morphBAmtLabel, morphBAmtSlider);
    placeKnob(oX_B + mWidth + 85, y2, morphBShiftLabel, morphBShiftSlider);

    int oX_C = oX_B + mWidth + 175;
    placeComboLocal(oX_C, y2, mWidth, morphCModeLabel, morphCModeCombo);
    placeKnob(oX_C + mWidth + 10, y2, morphCAmtLabel, morphCAmtSlider);
    placeKnob(oX_C + mWidth + 85, y2, morphCShiftLabel, morphCShiftSlider);

    rightArea.removeFromTop(10);

    // --- 下部レイアウト (幅920px) ---
    auto col1 = rightArea.removeFromLeft(200); // Shaper & Filter
    rightArea.removeFromLeft(10);
    auto col2 = rightArea.removeFromLeft(200); // Envelopes
    rightArea.removeFromLeft(10);
    auto col3 = rightArea;                     // Mod Tabs (500px)

    auto shpRect = col1.removeFromTop(205);
    shaperGroup.setBounds(shpRect);
    int sX = shpRect.getX() + 15, sY = shpRect.getY() + 15;
    placeKnob(sX, sY, distDriveLabel, distDriveSlider);
    placeKnob(sX + 80, sY, shpAmtLabel, shpAmtSlider);
    placeKnob(sX, sY + 80, rateLabel, rateSlider);
    placeKnob(sX + 80, sY + 80, bitLabel, bitSlider);

    col1.removeFromTop(10);
    filterGroup.setBounds(col1);
    int fX = col1.getX() + 15, fY = col1.getY() + 15;
    placeKnob(fX, fY, cutoffLabel, cutoffSlider);
    placeKnob(fX + 80, fY, resLabel, resSlider);
    placeKnob(fX + 40, fY + 80, fltEnvAmtLabel, fltEnvAmtSlider);

    auto aEnvRect = col2.removeFromTop(205);
    ampEnvGroup.setBounds(aEnvRect);
    int aX = aEnvRect.getX() + 15, aY = aEnvRect.getY() + 15;
    placeKnob(aX, aY, ampAtkLabel, ampAtkSlider);
    placeKnob(aX + 80, aY, ampDecLabel, ampDecSlider);
    placeKnob(aX, aY + 80, ampSusLabel, ampSusSlider);
    placeKnob(aX + 80, aY + 80, ampRelLabel, ampRelSlider);

    col2.removeFromTop(10);
    filterEnvGroup.setBounds(col2);
    int feX = col2.getX() + 15, feY = col2.getY() + 15;
    placeKnob(feX, feY, fltAtkLabel, fltAtkSlider);
    placeKnob(feX + 80, feY, fltDecLabel, fltDecSlider);
    placeKnob(feX, feY + 80, fltSusLabel, fltSusSlider);
    placeKnob(feX + 80, feY + 80, fltRelLabel, fltRelSlider);

    modTabs.setBounds(col3);
}