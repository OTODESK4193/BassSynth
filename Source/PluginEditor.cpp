// ==============================================================================
// Source/PluginEditor.cpp
// ==============================================================================
#include "PluginProcessor.h"
#include "PluginEditor.h"

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
// MSEG Editor Component (Canvas) Implementation
// ==============================================================================
MsegEditorComponent::MsegEditorComponent(Mseg& engine, MsegState& linkedState)
    : msegEngine(engine), state(linkedState) {
}

float MsegEditorComponent::getXPos(float x) const { return 10.0f + x * (getWidth() - 20.0f); }
float MsegEditorComponent::getYPos(float y) const { return getHeight() / 2.0f - y * (getHeight() / 2.0f - 10.0f); }
float MsegEditorComponent::getXValue(float xPos) const { return juce::jlimit(0.0f, 1.0f, (xPos - 10.0f) / (getWidth() - 20.0f)); }
float MsegEditorComponent::getYValue(float yPos) const { return juce::jlimit(-1.0f, 1.0f, (getHeight() / 2.0f - yPos) / (getHeight() / 2.0f - 10.0f)); }

int MsegEditorComponent::hitTestNode(const juce::Point<float>& pos) const {
    for (int i = 0; i < state.numPoints; ++i) {
        float nx = getXPos(state.points[i].x);
        float ny = getYPos(state.points[i].y);
        if (pos.getDistanceFrom(juce::Point<float>(nx, ny)) < 8.0f) return i;
    }
    return -1;
}

void MsegEditorComponent::loadState(const MsegState& newState) {
    state = newState;
    repaint();
}

void MsegEditorComponent::updateDSP() {
    msegEngine.pushNewState(state);
    repaint();
}

void MsegEditorComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour::fromString("FF0A0A0A"));
    g.setColour(juce::Colours::white.withAlpha(0.2f));
    g.drawLine(0.0f, getHeight() / 2.0f, static_cast<float>(getWidth()), getHeight() / 2.0f, 1.0f);

    if (state.numPoints == 0) return;

    juce::Path p;
    p.startNewSubPath(getXPos(state.points[0].x), getYPos(state.points[0].y));

    for (int i = 0; i < state.numPoints - 1; ++i) {
        float x0 = getXPos(state.points[i].x); float y0 = getYPos(state.points[i].y);
        float x1 = getXPos(state.points[i + 1].x); float y1 = getYPos(state.points[i + 1].y);

        if (std::abs(state.points[i].curve) < 0.01f) {
            p.lineTo(x1, y1);
        }
        else {
            float cx = (x0 + x1) * 0.5f;
            float cy = (y0 + y1) * 0.5f - state.points[i].curve * std::abs(y1 - y0) * 0.5f;
            p.quadraticTo(cx, cy, x1, y1);
        }
    }

    g.setColour(juce::Colour::fromString("FF00FFCC"));
    g.strokePath(p, juce::PathStrokeType(2.0f));

    for (int i = 0; i < state.numPoints; ++i) {
        float nx = getXPos(state.points[i].x); float ny = getYPos(state.points[i].y);
        g.setColour((i == draggingNodeIndex) ? juce::Colours::orange : juce::Colours::white);
        g.fillEllipse(nx - 4.0f, ny - 4.0f, 8.0f, 8.0f);
    }
}

void MsegEditorComponent::mouseDown(const juce::MouseEvent& e) {
    draggingCurve = e.mods.isAltDown();
    draggingNodeIndex = hitTestNode(e.position);
}

void MsegEditorComponent::mouseDrag(const juce::MouseEvent& e) {
    if (draggingNodeIndex >= 0 && draggingNodeIndex < state.numPoints) {
        if (draggingCurve && draggingNodeIndex < state.numPoints - 1) {
            float deltaY = (e.position.y - e.getMouseDownY()) / 50.0f;
            state.points[draggingNodeIndex].curve = juce::jlimit(-1.0f, 1.0f, state.points[draggingNodeIndex].curve + deltaY);
        }
        else {
            float newX = getXValue(e.position.x);
            float newY = getYValue(e.position.y);

            float minX = (draggingNodeIndex > 0) ? state.points[draggingNodeIndex - 1].x + 0.01f : 0.0f;
            float maxX = (draggingNodeIndex < state.numPoints - 1) ? state.points[draggingNodeIndex + 1].x - 0.01f : 1.0f;
            if (draggingNodeIndex == 0) newX = 0.0f;
            if (draggingNodeIndex == state.numPoints - 1) newX = 1.0f;

            state.points[draggingNodeIndex].x = juce::jlimit(minX, maxX, newX);
            state.points[draggingNodeIndex].y = newY;
        }
        updateDSP();
    }
}

void MsegEditorComponent::mouseUp(const juce::MouseEvent&) {
    draggingNodeIndex = -1;
    draggingCurve = false;
    updateDSP();
}

void MsegEditorComponent::mouseDoubleClick(const juce::MouseEvent& e) {
    int hit = hitTestNode(e.position);
    if (hit > 0 && hit < state.numPoints - 1 && state.numPoints > 2) {
        for (int i = hit; i < state.numPoints - 1; ++i) state.points[i] = state.points[i + 1];
        state.numPoints--;
    }
    else if (hit == -1 && state.numPoints < MAX_MSEG_POINTS) {
        float newX = getXValue(e.position.x);
        float newY = getYValue(e.position.y);
        int insertIdx = 1;
        while (insertIdx < state.numPoints && state.points[insertIdx].x < newX) insertIdx++;

        for (int i = state.numPoints; i > insertIdx; --i) state.points[i] = state.points[i - 1];
        state.points[insertIdx] = { newX, newY, 0.0f };
        state.numPoints++;
    }
    updateDSP();
}

// ==============================================================================
// MsegTab Implementation
// ==============================================================================
MsegTab::MsegTab(LiquidDreamAudioProcessor& p, juce::AudioProcessorValueTreeState& vts)
    : processor(p), apvts(vts) {

    juce::StringArray beats = { "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/4T", "1/8T", "1/16T" };
    juce::StringArray trigs = { "Free", "Retrig", "OneShot" };

    for (int i = 0; i < 2; ++i) {
        addAndMakeVisible(msegs[i].onBtn);
        setupCombo(msegs[i].trig, msegs[i].trigLbl, "Trig", trigs, this);
        setupCombo(msegs[i].beat, msegs[i].beatLbl, "Beat", beats, this);
        setupS(msegs[i].rate, msegs[i].rateLbl, "Rate(Hz)", this);
        setupS(msegs[i].amt, msegs[i].amtLbl, "Amount", this);
        addAndMakeVisible(msegs[i].sync);

        msegs[i].editor = std::make_unique<MsegEditorComponent>(processor.getMsegEngine(i), processor.msegStates[i]);
        addAndMakeVisible(*msegs[i].editor);

        juce::String pfx = "mseg" + juce::String(i + 1) + "_";
        btnAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, pfx + "on", msegs[i].onBtn));
        comboAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, pfx + "trig", msegs[i].trig));
        comboAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, pfx + "beat", msegs[i].beat));
        sliderAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, pfx + "rate", msegs[i].rate));
        sliderAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, pfx + "amt", msegs[i].amt));
        btnAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, pfx + "sync", msegs[i].sync));
    }
}

void MsegTab::updateFromProcessor() {
    msegs[0].editor->loadState(processor.msegStates[0]);
    msegs[1].editor->loadState(processor.msegStates[1]);
}

void MsegTab::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour::fromString("FF1A1A1A"));
    for (int i = 0; i < 2; ++i) {
        g.setColour(juce::Colours::white.withAlpha(0.05f));
        g.fillRoundedRectangle(10, 10 + i * 190, getWidth() - 20, 180, 5.0f);
        g.setColour(juce::Colour::fromString("FFFF764D"));
        g.setFont(14.0f);
        g.drawText("MSEG " + juce::String(i + 1), 20, 15 + i * 190, 60, 20, juce::Justification::centredLeft);
    }
}

void MsegTab::resized() {
    for (int i = 0; i < 2; ++i) {
        int boxY = 10 + i * 190;
        msegs[i].editor->setBounds(15, boxY + 40, 260, 130);
        msegs[i].onBtn.setBounds(290, boxY + 40, 55, 22);
        msegs[i].trigLbl.setBounds(290, boxY + 65, 85, 15);
        msegs[i].trig.setBounds(290, boxY + 80, 85, 22);
        msegs[i].sync.setBounds(290, boxY + 110, 55, 22);
        msegs[i].beatLbl.setBounds(290, boxY + 135, 85, 15);
        msegs[i].beat.setBounds(290, boxY + 150, 85, 22);
        placeKnob(400, boxY + 35, msegs[i].rateLbl, msegs[i].rate);
        placeKnob(400, boxY + 105, msegs[i].amtLbl, msegs[i].amt);
    }
}

// ==============================================================================
// LfoTab Implementation
// ==============================================================================
LfoTab::LfoTab(juce::AudioProcessorValueTreeState& vts) : apvts(vts) {
    juce::StringArray waves = { "Sine", "Saw", "Pulse", "Random" };
    juce::StringArray beats = { "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/4T", "1/8T", "1/16T" };
    juce::StringArray trigs = { "Free", "Retrig", "OneShot" };

    for (int i = 0; i < 3; ++i) {
        addAndMakeVisible(lfos[i].onBtn);
        setupCombo(lfos[i].wave, lfos[i].waveLbl, "Wave", waves, this);
        setupCombo(lfos[i].trig, lfos[i].trigLbl, "Trig", trigs, this);
        setupCombo(lfos[i].beat, lfos[i].beatLbl, "Beat", beats, this);
        setupS(lfos[i].rate, lfos[i].rateLbl, "Rate(Hz)", this);
        setupS(lfos[i].amt, lfos[i].amtLbl, "Amount", this);
        addAndMakeVisible(lfos[i].sync);

        juce::String pfx = "lfo" + juce::String(i + 1) + "_";
        btnAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, pfx + "on", lfos[i].onBtn));
        comboAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, pfx + "wave", lfos[i].wave));
        comboAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, pfx + "trig", lfos[i].trig));
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
        int boxY = 10 + i * 130;
        lfos[i].onBtn.setBounds(15, boxY + 45, 45, 24);
        lfos[i].waveLbl.setBounds(75, boxY + 10, 65, 16);  lfos[i].wave.setBounds(75, boxY + 26, 65, 22);
        lfos[i].trigLbl.setBounds(75, boxY + 54, 65, 16);  lfos[i].trig.setBounds(75, boxY + 70, 65, 22);
        lfos[i].sync.setBounds(155, boxY + 45, 50, 24);
        placeKnob(220, boxY + 22, lfos[i].rateLbl, lfos[i].rate);
        lfos[i].beatLbl.setBounds(305, boxY + 30, 65, 20); lfos[i].beat.setBounds(305, boxY + 50, 65, 24);
        placeKnob(385, boxY + 22, lfos[i].amtLbl, lfos[i].amt);
    }
}

// ==============================================================================
// ModEnvTab Implementation
// ==============================================================================
ModEnvTab::ModEnvTab(juce::AudioProcessorValueTreeState& vts) : apvts(vts) {
    for (int i = 0; i < 3; ++i) {
        addAndMakeVisible(envs[i].onBtn);
        setupS(envs[i].a, envs[i].aL, "A", this); setupS(envs[i].d, envs[i].dL, "D", this);
        setupS(envs[i].s, envs[i].sL, "S", this); setupS(envs[i].r, envs[i].rL, "R", this);
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
        envs[i].onBtn.setBounds(15, y + 15, 45, 24);
        placeKnob(75, y, envs[i].aL, envs[i].a); placeKnob(150, y, envs[i].dL, envs[i].d);
        placeKnob(225, y, envs[i].sL, envs[i].s); placeKnob(300, y, envs[i].rL, envs[i].r);
        placeKnob(385, y, envs[i].amtL, envs[i].amt);
    }
}

// ==============================================================================
// MatrixTab Implementation
// ==============================================================================
MatrixTab::MatrixTab(juce::AudioProcessorValueTreeState& vts) : apvts(vts) {
    juce::StringArray sources = {
        "None", "MOD 1", "MOD 2", "MOD 3", "LFO 1", "LFO 2", "LFO 3", "MSEG 1", "MSEG 2"
    };
    juce::StringArray dests = {
        "None", "WT: Position", "WT: FM Amt", "WT: MorphA Amt", "WT: MorphA Shf",
        "WT: MorphB Amt", "WT: MorphB Shf", "WT: MorphC Amt", "WT: MorphC Shf",
        "FLT A: Cutoff", "FLT A: Reso", "FLT B: Cutoff", "FLT B: Reso", "PRF: Gain"
    };

    for (int i = 0; i < 10; ++i) {
        addAndMakeVisible(slots[i].src);
        slots[i].src.addItemList(sources, 1);
        slots[i].src.setJustificationType(juce::Justification::centred);

        addAndMakeVisible(slots[i].amt);
        slots[i].amt.setSliderStyle(juce::Slider::LinearHorizontal);
        slots[i].amt.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slots[i].amt.setPopupDisplayEnabled(true, false, this);
        slots[i].amt.setColour(juce::Slider::trackColourId, juce::Colour::fromString("FFFF764D"));

        addAndMakeVisible(slots[i].dest);
        slots[i].dest.addItemList(dests, 1);
        slots[i].dest.setJustificationType(juce::Justification::centred);

        juce::String idxStr = juce::String(i);
        comboAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "matrix_src_" + idxStr, slots[i].src));
        sliderAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "matrix_amt_" + idxStr, slots[i].amt));
        comboAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "matrix_dest_" + idxStr, slots[i].dest));
    }
}

void MatrixTab::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour::fromString("FF1A1A1A"));
    g.setColour(juce::Colour::fromString("FFDDDDDD"));
    g.setFont(12.0f);
    g.drawText("SOURCE", 10, 5, 110, 20, juce::Justification::centred);
    g.drawText("AMOUNT", 130, 5, 130, 20, juce::Justification::centred);
    g.drawText("DESTINATION", 270, 5, 130, 20, juce::Justification::centred);

    for (int i = 0; i < 10; ++i) {
        g.setColour(juce::Colours::white.withAlpha(i % 2 == 0 ? 0.03f : 0.00f));
        g.fillRect(0, 25 + i * 36, getWidth(), 36);
    }
}

void MatrixTab::resized() {
    int startY = 25;
    int rowH = 36;
    for (int i = 0; i < 10; ++i) {
        int y = startY + i * rowH + 6;
        slots[i].src.setBounds(15, y, 115, 24);
        slots[i].amt.setBounds(140, y + 2, 135, 20);
        slots[i].dest.setBounds(285, y, 180, 24);
    }
}

// ==============================================================================
// ColorIrPanel Implementation
// ==============================================================================
ColorIrPanel::ColorIrPanel(LiquidDreamAudioProcessor& p) : processor(p), apvts(p.getAPVTS()) {
    addAndMakeVisible(learnButton);
    learnButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
    learnButton.onClick = [this] {
        auto state = processor.getColorEngine().getLearnState();
        if (state == ColorIREngine::LearnState::Idle || state == ColorIREngine::LearnState::Active) {
            processor.getColorEngine().setLearnState(ColorIREngine::LearnState::Learning);
        }
        else if (state == ColorIREngine::LearnState::Learning) {
            processor.getColorEngine().finishLearningAndGenerate(atkSlider.getValue(), decSlider.getValue(), typeCombo.getSelectedId() - 1);
        }
        };

    addAndMakeVisible(chordLabel);
    chordLabel.setJustificationType(juce::Justification::centred);
    chordLabel.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    chordLabel.setColour(juce::Label::textColourId, juce::Colour::fromString("FF00FFCC"));

    setupCombo(typeCombo, typeLabel, "IR Type", { "Crystal Saw", "Shimmer PWM", "Harmonic Bell", "Stacked Shimmer", "Crystal Pluck" }, this);
    comboAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "color_type", typeCombo));

    setupS(mixSlider, mixLabel, "Mix", this);
    setupS(irVolSlider, irVolLabel, "IR Vol", this);
    setupS(atkSlider, atkLabel, "Attack", this);
    setupS(decSlider, decLabel, "Decay", this);
    setupS(preHpSlider, preHpLabel, "Pre HPF", this);
    setupS(postHpSlider, postHpLabel, "Post HPF", this);

    atts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "color_mix", mixSlider));
    atts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "color_ir_vol", irVolSlider));
    atts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "color_atk", atkSlider));
    atts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "color_dec", decSlider));
    atts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "color_pre_hp", preHpSlider));
    atts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "color_post_hp", postHpSlider));

    setupS(ottDepthSlider, ottDepthLabel, "Depth", this);
    setupS(ottTimeSlider, ottTimeLabel, "Speed", this);
    setupS(ottUpSlider, ottUpLabel, "Upward %", this);
    setupS(ottDownSlider, ottDownLabel, "Down %", this);
    setupS(ottGainSlider, ottGainLabel, "Out Gain", this);

    setupS(sootheSelSlider, sootheSelLabel, "Select", this);
    setupS(sootheShpSlider, sootheShpLabel, "Sharp", this);
    setupS(sootheFocSlider, sootheFocLabel, "Focus", this);

    atts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "ott_depth", ottDepthSlider));
    atts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "ott_time", ottTimeSlider));
    atts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "ott_up", ottUpSlider));
    atts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "ott_down", ottDownSlider));
    atts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "ott_gain", ottGainSlider));
    atts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "soothe_sel", sootheSelSlider));
    atts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "soothe_shp", sootheShpSlider));
    atts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "soothe_foc", sootheFocSlider));

    setupCombo(arpWaveCombo, arpWaveLabel, "Arp Wave", { "Sine", "Saw", "Square", "Pulse 25%", "Pulse 12.5%" }, this);
    setupCombo(arpModeCombo, arpModeLabel, "Mode", { "Up", "Down", "Up/Down", "Random" }, this);
    setupCombo(arpPitchCombo, arpPitchLabel, "Octave", { "+2 Oct", "+3 Oct", "+4 Oct" }, this);

    setupS(arpSpeedSlider, arpSpeedLabel, "Speed(Hz)", this);
    setupS(arpLevelSlider, arpLevelLabel, "Level", this);
    setupS(masterGainSlider, masterGainLabel, "Master Vol", this);
    atts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "m_gain", masterGainSlider));

    comboAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "arp_wave", arpWaveCombo));
    comboAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "arp_mode", arpModeCombo));
    comboAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "arp_pitch", arpPitchCombo));
    atts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "arp_speed", arpSpeedSlider));
    atts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "arp_level", arpLevelSlider));

    auto regenIR = [this]() {
        if (processor.getColorEngine().getLearnState() == ColorIREngine::LearnState::Active) {
            processor.getColorEngine().finishLearningAndGenerate(atkSlider.getValue(), decSlider.getValue(), typeCombo.getSelectedId() - 1);
        }
        };
    atkSlider.onValueChange = regenIR;
    decSlider.onValueChange = regenIR;
    typeCombo.onChange = regenIR;
}

void ColorIrPanel::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour::fromString("FF121212"));

    auto drawBlock = [&](int y, int height, const juce::String& title, juce::Colour color) {
        g.setColour(juce::Colours::white.withAlpha(0.04f));
        g.fillRoundedRectangle(5, y, getWidth() - 10, height, 5.0f);
        g.setColour(color);
        g.setFont(14.0f);
        g.drawText(title, 15, y + 5, 250, 20, juce::Justification::left);
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.drawLine(15, y + 25, getWidth() - 15, y + 25, 1.0f);
        };

    drawBlock(5, 255, "1. COLOR IR GENERATOR", juce::Colour::fromString("FFFF764D"));
    drawBlock(265, 175, "2. DYNAMICS (OTT & SOOTHE)", juce::Colour::fromString("FF00FFCC"));
    drawBlock(445, 185, "3. SPARKLE ARP & MASTER", juce::Colour::fromString("FFFFD700"));
}

void ColorIrPanel::resized() {
    learnButton.setBounds(20, 35, 310, 30);
    chordLabel.setBounds(20, 70, 310, 30);

    typeLabel.setBounds(20, 110, 120, 20); typeCombo.setBounds(20, 130, 120, 24);
    placeKnob(160, 110, atkLabel, atkSlider);
    placeKnob(250, 110, decLabel, decSlider);

    placeKnob(15, 180, mixLabel, mixSlider);
    placeKnob(90, 180, irVolLabel, irVolSlider);
    placeKnob(165, 180, preHpLabel, preHpSlider);
    placeKnob(240, 180, postHpLabel, postHpSlider);

    int b2y = 295;
    placeKnob(15, b2y, ottDepthLabel, ottDepthSlider);
    placeKnob(90, b2y, ottTimeLabel, ottTimeSlider);
    placeKnob(165, b2y, ottUpLabel, ottUpSlider);
    placeKnob(240, b2y, ottDownLabel, ottDownSlider);

    int b2y2 = 365;
    placeKnob(15, b2y2, sootheSelLabel, sootheSelSlider);
    placeKnob(90, b2y2, sootheShpLabel, sootheShpSlider);
    placeKnob(165, b2y2, sootheFocLabel, sootheFocSlider);
    placeKnob(240, b2y2, ottGainLabel, ottGainSlider);

    int b3y = 480;
    arpWaveLabel.setBounds(20, b3y, 90, 20); arpWaveCombo.setBounds(20, b3y + 20, 90, 24);
    arpModeLabel.setBounds(120, b3y, 90, 20); arpModeCombo.setBounds(120, b3y + 20, 90, 24);
    arpPitchLabel.setBounds(220, b3y, 90, 20); arpPitchCombo.setBounds(220, b3y + 20, 90, 24);

    placeKnob(20, b3y + 60, arpSpeedLabel, arpSpeedSlider);
    placeKnob(110, b3y + 60, arpLevelLabel, arpLevelSlider);
    placeKnob(240, b3y + 60, masterGainLabel, masterGainSlider);
}

void ColorIrPanel::updateState(ColorIREngine::LearnState state, const juce::String& chordText, bool blinkFlag) {
    chordLabel.setText(chordText, juce::dontSendNotification);

    if (state == ColorIREngine::LearnState::Learning) {
        learnButton.setButtonText(blinkFlag ? "WAITING FOR INPUT..." : "");
        learnButton.setColour(juce::TextButton::buttonColourId, blinkFlag ? juce::Colour::fromString("FFB22222") : juce::Colours::darkgrey);
    }
    else if (state == ColorIREngine::LearnState::Active) {
        learnButton.setButtonText("LEARNED");
        learnButton.setColour(juce::TextButton::buttonColourId, juce::Colour::fromString("FFFF764D"));
    }
    else {
        learnButton.setButtonText("LEARN CHORD");
        learnButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
    }
}

// ==============================================================================
// LiquidDreamAudioProcessorEditor Implementation
// ==============================================================================
LiquidDreamAudioProcessorEditor::LiquidDreamAudioProcessorEditor(LiquidDreamAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
    dualScope(p.getOutputScopePtr()), browser(p.getAPVTS()),
    colorPanel(p), modTabs(juce::TabbedButtonBar::TabsAtTop),
    lfoTab(p.getAPVTS()), msegTab(p, p.getAPVTS()), modEnvTab(p.getAPVTS()), matrixTab(p.getAPVTS())
{
    setLookAndFeel(&abletonLnF);
    addAndMakeVisible(openBrowserButton); addAndMakeVisible(prevWaveButton);
    addAndMakeVisible(nextWaveButton); addAndMakeVisible(rndWaveButton);

    addAndMakeVisible(viewWaveBtn); addAndMakeVisible(viewColorBtn); addAndMakeVisible(colorOnBtn);
    viewWaveBtn.onClick = [this] { isColorPanelVisible = false; resized(); };
    viewColorBtn.onClick = [this] { isColorPanelVisible = true; resized(); };

    addAndMakeVisible(dualScope);
    addAndMakeVisible(colorPanel);
    colorPanel.setVisible(false);

    oscGroup.setText("WAVETABLE"); addAndMakeVisible(oscGroup);
    subGroup.setText("SUB OSC"); addAndMakeVisible(subGroup);
    shaperGroup.setText("DISTORTION & SHAPER"); addAndMakeVisible(shaperGroup);
    ampEnvGroup.setText("AMP ENVELOPE"); addAndMakeVisible(ampEnvGroup);
    filterGroup.setText("DUAL FILTER & ENV"); addAndMakeVisible(filterGroup);
    addChildComponent(presetGroup);
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

    addAndMakeVisible(fltABtn); addAndMakeVisible(fltBBtn);
    fltABtn.setRadioGroupId(1); fltBBtn.setRadioGroupId(1);
    fltABtn.setClickingTogglesState(true); fltBBtn.setClickingTogglesState(true);
    fltABtn.setToggleState(true, juce::dontSendNotification);

    fltABtn.setColour(juce::TextButton::buttonColourId, juce::Colour::fromString("FF333333"));
    fltABtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour::fromString("FFFF764D"));
    fltBBtn.setColour(juce::TextButton::buttonColourId, juce::Colour::fromString("FF333333"));
    fltBBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour::fromString("FFFF764D"));

    juce::StringArray fTypes = { "LPF", "HPF", "BPF", "Notch", "Comb", "Analog LPF" };
    setupCombo(fltATypeCombo, fltACutoffLabel, nullptr, fTypes, this);
    setupCombo(fltBTypeCombo, fltBCutoffLabel, nullptr, fTypes, this);
    setupCombo(fltRoutingCombo, fltMixLabel, nullptr, { "Serial", "Parallel" }, this);

    setupS(fltACutoffSlider, fltACutoffLabel, "Cutoff", this); setupS(fltBCutoffSlider, fltBCutoffLabel, "Cutoff", this);
    setupS(fltAResSlider, fltAResLabel, "Reso", this); setupS(fltBResSlider, fltBResLabel, "Reso", this);
    setupS(fltMixSlider, fltMixLabel, "Mix (Wet/B)", this);
    setupS(fltAEnvAmtSlider, fltAEnvAmtLabel, "EnvAmt", this); setupS(fltBEnvAmtSlider, fltBEnvAmtLabel, "EnvAmt", this);

    fltABtn.onClick = [this] { updateFilterUI(); }; fltBBtn.onClick = [this] { updateFilterUI(); };

    setupS(ampAtkSlider, ampAtkLabel, "A", this); setupS(ampDecSlider, ampDecLabel, "D", this); setupS(ampSusSlider, ampSusLabel, "S", this); setupS(ampRelSlider, ampRelLabel, "R", this);
    setupS(fltAAtkSlider, fltAAtkLabel, "A", this); setupS(fltADecSlider, fltADecLabel, "D", this); setupS(fltASusSlider, fltASusLabel, "S", this); setupS(fltARelSlider, fltARelLabel, "R", this);
    setupS(fltBAtkSlider, fltBAtkLabel, "A", this); setupS(fltBDecSlider, fltBDecLabel, "D", this); setupS(fltBSusSlider, fltBSusLabel, "S", this); setupS(fltBRelSlider, fltBRelLabel, "R", this);

    setupS(glideSlider, glideLabel, "Glide", this); setupS(pitchSlider, pitchLabel, "Pitch", this); setupS(gainSlider, gainLabel, "Gain", this);
    addAndMakeVisible(legatoButton);

    addAndMakeVisible(presetCombo); presetCombo.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(savePresetBtn);
    savePresetBtn.setColour(juce::TextButton::buttonColourId, juce::Colour::fromString("FF2A2A2A"));

    scanPresets();
    presetCombo.onChange = [this]() {
        int id = presetCombo.getSelectedId();
        if (id == 1) {
            for (auto* p : audioProcessor.getParameters()) {
                if (auto* floatParam = dynamic_cast<juce::AudioProcessorParameterWithID*>(p)) {
                    floatParam->setValueNotifyingHost(floatParam->getDefaultValue());
                }
            }
            audioProcessor.loadFactoryWavetable(0);
        }
        else if (id > 1) {
            int index = id - 2;
            if (juce::isPositiveAndBelow(index, presetFiles.size())) {
                juce::MemoryBlock mb; presetFiles[index].loadFileAsData(mb);
                audioProcessor.setStateInformation(mb.getData(), (int)mb.getSize());
            }
        }
        };

    savePresetBtn.onClick = [this] {
        auto presetDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("BassSynthPresets");
        if (!presetDir.exists()) presetDir.createDirectory();
        auto chooser = std::make_shared<juce::FileChooser>("Save Preset", presetDir, "*.xml");
        auto flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting;

        chooser->launchAsync(flags, [this, chooser](const juce::FileChooser& fc) {
            auto file = fc.getResult();
            if (file != juce::File{}) {
                if (!file.hasFileExtension("xml")) file = file.withFileExtension("xml");
                juce::MemoryBlock mb; audioProcessor.getStateInformation(mb);
                file.replaceWithData(mb.getData(), mb.getSize()); scanPresets();
                for (int i = 0; i < presetFiles.size(); ++i) {
                    if (presetFiles[i] == file) { presetCombo.setSelectedId(i + 2, juce::dontSendNotification); break; }
                }
            }
            });
        };

    auto& apvts = audioProcessor.getAPVTS();
    auto att = [&](juce::Slider& s, const juce::String& id) { attachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, id, s)); };

    oscOnAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "osc_on", oscOnButton);
    colorOnAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "color_on", colorOnBtn);
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
    fltATypeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "flt_a_type", fltATypeCombo);
    fltBTypeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "flt_b_type", fltBTypeCombo);
    fltRoutingAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "flt_routing", fltRoutingCombo);
    att(fltACutoffSlider, "flt_a_cutoff"); att(fltAResSlider, "flt_a_res"); att(fltBCutoffSlider, "flt_b_cutoff"); att(fltBResSlider, "flt_b_res");
    att(fltMixSlider, "flt_mix"); att(fltAEnvAmtSlider, "flt_a_env_amt"); att(fltBEnvAmtSlider, "flt_b_env_amt");
    att(ampAtkSlider, "a_atk"); att(ampDecSlider, "a_dec"); att(ampSusSlider, "a_sus"); att(ampRelSlider, "a_rel");
    att(fltAAtkSlider, "f_a_atk"); att(fltADecSlider, "f_a_dec"); att(fltASusSlider, "f_a_sus"); att(fltARelSlider, "f_a_rel");
    att(fltBAtkSlider, "f_b_atk"); att(fltBDecSlider, "f_b_dec"); att(fltBSusSlider, "f_b_sus"); att(fltBRelSlider, "f_b_rel");
    att(glideSlider, "m_glide"); att(pitchSlider, "m_pb"); att(gainSlider, "m_gain");

    addAndMakeVisible(modTabs);
    modTabs.addTab("LFOs", juce::Colour::fromString("FF2A2A2A"), &lfoTab, false);
    modTabs.addTab("MSEGs", juce::Colour::fromString("FF2A2A2A"), &msegTab, false);
    modTabs.addTab("MOD ENVs", juce::Colour::fromString("FF2A2A2A"), &modEnvTab, false);
    modTabs.addTab("MATRIX", juce::Colour::fromString("FF2A2A2A"), &matrixTab, false);

    addChildComponent(browser); browser.setVisible(false);
    openBrowserButton.onClick = [this] { browser.setVisible(!browser.isVisible()); if (browser.isVisible()) browser.toFront(true); };
    prevWaveButton.onClick = [this] { browser.selectPrev(); }; nextWaveButton.onClick = [this] { browser.selectNext(); };
    rndWaveButton.onClick = [this] { browser.selectRandom(); };

    browser.onCustomFileSelected = [this](const juce::File& f) { audioProcessor.loadCustomWavetable(f); };
    browser.onFactoryIndexSelected = [this](int idx) { audioProcessor.loadFactoryWavetable(idx); };
    browser.onUserFoldersChanged = [this](const juce::StringArray& folders) { audioProcessor.setUserFolders(folders); };
    browser.onFavoritesChanged = [this](const juce::StringArray& favs) { audioProcessor.setFavorites(favs); };

    browser.setFavorites(audioProcessor.getFavorites());
    browser.loadUserFolders(audioProcessor.getUserFolders());
    browser.onCloseRequested = [this] { browser.setVisible(false); };

    updateFilterUI();
    startTimerHz(20);
    setSize(1300, 700);
}

LiquidDreamAudioProcessorEditor::~LiquidDreamAudioProcessorEditor() { setLookAndFeel(nullptr); }

void LiquidDreamAudioProcessorEditor::updateFilterUI() {
    bool isA = fltABtn.getToggleState();
    fltATypeCombo.setVisible(isA); fltACutoffSlider.setVisible(isA); fltACutoffLabel.setVisible(isA);
    fltAResSlider.setVisible(isA); fltAResLabel.setVisible(isA); fltAEnvAmtSlider.setVisible(isA); fltAEnvAmtLabel.setVisible(isA);
    fltAAtkSlider.setVisible(isA); fltAAtkLabel.setVisible(isA); fltADecSlider.setVisible(isA); fltADecLabel.setVisible(isA);
    fltASusSlider.setVisible(isA); fltASusLabel.setVisible(isA); fltARelSlider.setVisible(isA); fltARelLabel.setVisible(isA);
    fltBTypeCombo.setVisible(!isA); fltBCutoffSlider.setVisible(!isA); fltBCutoffLabel.setVisible(!isA);
    fltBResSlider.setVisible(!isA); fltBResLabel.setVisible(!isA); fltBEnvAmtSlider.setVisible(!isA); fltBEnvAmtLabel.setVisible(!isA);
    fltBAtkSlider.setVisible(!isA); fltBAtkLabel.setVisible(!isA); fltBDecSlider.setVisible(!isA); fltBDecLabel.setVisible(!isA);
    fltBSusSlider.setVisible(!isA); fltBSusLabel.setVisible(!isA); fltBRelSlider.setVisible(!isA); fltBRelLabel.setVisible(!isA);
    if (isA) {
        fltACutoffSlider.toFront(false); fltAResSlider.toFront(false); fltATypeCombo.toFront(false); fltAEnvAmtSlider.toFront(false);
        fltAAtkSlider.toFront(false); fltADecSlider.toFront(false); fltASusSlider.toFront(false); fltARelSlider.toFront(false);
    }
    else {
        fltBCutoffSlider.toFront(false); fltBResSlider.toFront(false); fltBTypeCombo.toFront(false); fltBEnvAmtSlider.toFront(false);
        fltBAtkSlider.toFront(false); fltBDecSlider.toFront(false); fltBSusSlider.toFront(false); fltBRelSlider.toFront(false);
    }
}

void LiquidDreamAudioProcessorEditor::scanPresets() {
    presetCombo.clear(); presetCombo.addItem("Init", 1);
    auto presetDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("BassSynthPresets");
    if (!presetDir.exists()) presetDir.createDirectory();
    presetFiles = presetDir.findChildFiles(juce::File::findFiles, false, "*.xml");
    int id = 2; for (auto& f : presetFiles) { presetCombo.addItem(f.getFileNameWithoutExtension(), id++); }
    if (presetCombo.getSelectedId() == 0) { presetCombo.setSelectedId(1, juce::dontSendNotification); }
}

void LiquidDreamAudioProcessorEditor::paint(juce::Graphics& g) { g.fillAll(juce::Colour::fromString("FF1E1E1E")); }

void LiquidDreamAudioProcessorEditor::timerCallback() {
    blinkCounter = (blinkCounter + 1) % 20;
    auto& apvts = audioProcessor.getAPVTS();

    if (audioProcessor.presetLoadedFlag.exchange(false)) {
        msegTab.updateFromProcessor();
        browser.setFavorites(audioProcessor.getFavorites());
        browser.loadUserFolders(audioProcessor.getUserFolders());

        if (audioProcessor.isCustomWavetableLoaded()) {
            browser.onCustomFileSelected = nullptr;
            browser.onCustomFileSelected = [this](const juce::File& f) { audioProcessor.loadCustomWavetable(f); };
        }
    }

    if (audioProcessor.forceScopeUpdate.exchange(false)) {
        wtChanged = true;
    }

    float curWave = apvts.getRawParameterValue("osc_wave")->load();
    float curPos = apvts.getRawParameterValue("osc_pos")->load();
    float curFm = apvts.getRawParameterValue("osc_fm")->load();
    float curModeA = apvts.getRawParameterValue("osc_morph_a_mode")->load();
    float curModeB = apvts.getRawParameterValue("osc_morph_b_mode")->load();
    float curModeC = apvts.getRawParameterValue("osc_morph_c_mode")->load();

    float curMa = (curModeA > 0.5f) ? apvts.getRawParameterValue("osc_morph_a_amt")->load() : 0.0f;
    float curSa = (curModeA > 0.5f) ? apvts.getRawParameterValue("osc_morph_a_shift")->load() : 0.0f;
    float curMb = (curModeB > 0.5f) ? apvts.getRawParameterValue("osc_morph_b_amt")->load() : 0.0f;
    float curSb = (curModeB > 0.5f) ? apvts.getRawParameterValue("osc_morph_b_shift")->load() : 0.0f;
    float curMc = (curModeC > 0.5f) ? apvts.getRawParameterValue("osc_morph_c_amt")->load() : 0.0f;
    float curSc = (curModeC > 0.5f) ? apvts.getRawParameterValue("osc_morph_c_shift")->load() : 0.0f;

    if (curWave != lastWaveVal || curPos != lastPosVal || curFm != lastFmVal ||
        curMa != lastMaVal || curMb != lastMbVal || curMc != lastMcVal ||
        curSa != lastSaVal || curSb != lastSbVal || curSc != lastScVal) {

        wtChanged = true;
        lastWaveVal = curWave; lastPosVal = curPos; lastFmVal = curFm;
        lastMaVal = curMa; lastMbVal = curMb; lastMcVal = curMc;
        lastSaVal = curSa; lastSbVal = curSb; lastScVal = curSc;
    }

    if (colorOnBtn.getToggleState()) {
        auto state = audioProcessor.getColorEngine().getLearnState();
        auto text = audioProcessor.getColorEngine().getLearnedChordNames();
        colorPanel.updateState(state, text, blinkCounter < 10);
    }

    float modDepths[14] = { 0.0f };
    for (int slot = 0; slot < 10; ++slot) {
        int srcIdx = (int)apvts.getRawParameterValue("matrix_src_" + juce::String(slot))->load();
        int destIdx = (int)apvts.getRawParameterValue("matrix_dest_" + juce::String(slot))->load();
        float amt = std::abs(apvts.getRawParameterValue("matrix_amt_" + juce::String(slot))->load());
        if (srcIdx > 0 && destIdx > 0 && destIdx < 14) { modDepths[destIdx] += amt; }
    }

    bool isModulated = (modDepths[1] > 0.001f || modDepths[2] > 0.001f || modDepths[3] > 0.001f || modDepths[4] > 0.001f ||
        modDepths[5] > 0.001f || modDepths[6] > 0.001f || modDepths[7] > 0.001f || modDepths[8] > 0.001f);

    if (wtChanged || isModulated) {
        std::array<float, 512> tempBuffer;
        audioProcessor.getStaticWaveform(tempBuffer);
        dualScope.updateStaticWave(tempBuffer.data(), 512);
        wtChanged = false;
    }

    auto updateRing = [](juce::Slider& s, float depth) {
        bool changed = false; bool currentlyActive = s.getProperties().getWithDefault("mod_active", false); bool shouldBeActive = (depth > 0.001f);
        if (currentlyActive != shouldBeActive) changed = true;
        if (shouldBeActive) {
            auto range = s.getNormalisableRange(); float pNorm = range.convertTo0to1((float)s.getValue());
            float newMin = juce::jlimit(0.0f, 1.0f, pNorm - depth); float newMax = juce::jlimit(0.0f, 1.0f, pNorm + depth);
            float oldMin = s.getProperties().getWithDefault("mod_min", 0.0f); float oldMax = s.getProperties().getWithDefault("mod_max", 1.0f);
            if (std::abs(newMin - oldMin) > 0.001f || std::abs(newMax - oldMax) > 0.001f) {
                s.getProperties().set("mod_min", newMin); s.getProperties().set("mod_max", newMax); changed = true;
            }
            s.getProperties().set("mod_active", true);
        }
        else { s.getProperties().set("mod_active", false); }
        if (changed) s.repaint();
        };

    updateRing(wtPosSlider, modDepths[1]); updateRing(fmAmtSlider, modDepths[2]); updateRing(morphAAmtSlider, modDepths[3]); updateRing(morphAShiftSlider, modDepths[4]);
    updateRing(morphBAmtSlider, modDepths[5]); updateRing(morphBShiftSlider, modDepths[6]); updateRing(morphCAmtSlider, modDepths[7]); updateRing(morphCShiftSlider, modDepths[8]);
    updateRing(fltACutoffSlider, modDepths[9]); updateRing(fltAResSlider, modDepths[10]); updateRing(fltBCutoffSlider, modDepths[11]); updateRing(fltBResSlider, modDepths[12]);
    updateRing(gainSlider, modDepths[13]);
}

void LiquidDreamAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(15);
    auto placeComboLocal = [](int x, int y, int w, juce::Label& lbl, juce::ComboBox& cmb) { lbl.setBounds(x, y, w, 20); cmb.setBounds(x, y + 30, w, 24); };
    auto leftArea = area.removeFromLeft(350); auto navRect = leftArea.removeFromTop(35).reduced(2);

    openBrowserButton.setBounds(navRect.removeFromLeft(80)); navRect.removeFromLeft(5);
    prevWaveButton.setBounds(navRect.removeFromLeft(25)); navRect.removeFromLeft(5);
    nextWaveButton.setBounds(navRect.removeFromLeft(25)); navRect.removeFromLeft(5);
    rndWaveButton.setBounds(navRect.removeFromLeft(40)); navRect.removeFromLeft(5);

    viewWaveBtn.setBounds(navRect.removeFromLeft(50)); navRect.removeFromLeft(5);
    viewColorBtn.setBounds(navRect.removeFromLeft(40)); navRect.removeFromLeft(5);
    colorOnBtn.setBounds(navRect.removeFromLeft(35));
    leftArea.removeFromTop(5);

    dualScope.setVisible(!isColorPanelVisible); controlGroup.setVisible(!isColorPanelVisible); legatoButton.setVisible(!isColorPanelVisible);
    glideSlider.setVisible(!isColorPanelVisible); glideLabel.setVisible(!isColorPanelVisible); pitchSlider.setVisible(!isColorPanelVisible); pitchLabel.setVisible(!isColorPanelVisible);
    gainSlider.setVisible(!isColorPanelVisible); gainLabel.setVisible(!isColorPanelVisible); subGroup.setVisible(!isColorPanelVisible); subOnButton.setVisible(!isColorPanelVisible);
    subWaveCombo.setVisible(!isColorPanelVisible); subVolSlider.setVisible(!isColorPanelVisible); subVolLabel.setVisible(!isColorPanelVisible); subPitchSlider.setVisible(!isColorPanelVisible); subPitchLabel.setVisible(!isColorPanelVisible);
    colorPanel.setVisible(isColorPanelVisible);

    if (isColorPanelVisible) { colorPanel.setBounds(leftArea); }
    else {
        dualScope.setBounds(leftArea.removeFromTop(370)); leftArea.removeFromTop(10); auto ctrlRect = leftArea.removeFromTop(100); controlGroup.setBounds(ctrlRect);
        int cX = ctrlRect.getX(), cY = ctrlRect.getY() + 15; legatoButton.setBounds(cX + 10, cY + 20, 75, 24);
        placeKnob(cX + 80, cY, glideLabel, glideSlider); placeKnob(cX + 160, cY, pitchLabel, pitchSlider); placeKnob(cX + 240, cY, gainLabel, gainSlider);
        leftArea.removeFromTop(10); auto subRect = leftArea.removeFromTop(120); subGroup.setBounds(subRect); int sbX = subRect.getX() + 10, sbY = subRect.getY() + 15;
        subOnButton.setBounds(sbX, sbY + 20, 50, 24); subWaveCombo.setBounds(sbX + 60, sbY + 25, 70, 24); placeKnob(sbX + 160, sbY, subVolLabel, subVolSlider); placeKnob(sbX + 240, sbY, subPitchLabel, subPitchSlider);
    }

    area.removeFromLeft(15); auto rightArea = area; browser.setBounds(rightArea);
    auto oscRect = rightArea.removeFromTop(210); oscGroup.setBounds(oscRect); int oX = oscRect.getX() + 10, oY = oscRect.getY() + 18;
    oscOnButton.setBounds(oX, oY + 20, 50, 24); int step = 73;
    placeKnob(oX + 60 + step * 0, oY, wtLevelLabel, wtLevelSlider); placeKnob(oX + 60 + step * 1, oY, wtPosLabel, wtPosSlider);
    placeKnob(oX + 60 + step * 2, oY, oscPitchLabel, oscPitchSlider); placeKnob(oX + 60 + step * 3, oY, pitchDecayAmtLabel, pitchDecayAmtSlider);
    placeKnob(oX + 60 + step * 4, oY, pitchDecayTimeLabel, pitchDecayTimeSlider); placeKnob(oX + 60 + step * 5, oY, driftLabel, driftSlider);
    placeKnob(oX + 60 + step * 6, oY, uniCountLabel, uniCountSlider); placeKnob(oX + 60 + step * 7, oY, detuneLabel, detuneSlider);
    placeKnob(oX + 60 + step * 8, oY, widthLabel, widthSlider); placeKnob(oX + 60 + step * 9, oY, fmAmtLabel, fmAmtSlider);
    placeComboLocal(oX + 60 + step * 10, oY, 80, fmWaveLabel, fmWaveCombo);
    int y2 = oY + 85, mWidth = 120; placeComboLocal(oX, y2, mWidth, morphAModeLabel, morphAModeCombo); placeKnob(oX + mWidth + 10, y2, morphAAmtLabel, morphAAmtSlider); placeKnob(oX + mWidth + 85, y2, morphAShiftLabel, morphAShiftSlider);
    int oX_B = oX + mWidth + 175; placeComboLocal(oX_B, y2, mWidth, morphBModeLabel, morphBModeCombo); placeKnob(oX_B + mWidth + 10, y2, morphBAmtLabel, morphBAmtSlider); placeKnob(oX_B + mWidth + 85, y2, morphBShiftLabel, morphBShiftSlider);
    int oX_C = oX_B + mWidth + 175; placeComboLocal(oX_C, y2, mWidth, morphCModeLabel, morphCModeCombo); placeKnob(oX_C + mWidth + 10, y2, morphCAmtLabel, morphCAmtSlider); placeKnob(oX_C + mWidth + 85, y2, morphCShiftLabel, morphCShiftSlider);
    rightArea.removeFromTop(10); auto centerCol = rightArea.removeFromLeft(410); rightArea.removeFromLeft(10); auto modCol = rightArea;
    auto shpRect = centerCol.removeFromTop(100); shaperGroup.setBounds(shpRect); int sX = shpRect.getX(), sY = shpRect.getY() + 15;
    placeKnob(sX + 15, sY, distDriveLabel, distDriveSlider); placeKnob(sX + 115, sY, shpAmtLabel, shpAmtSlider); placeKnob(sX + 215, sY, rateLabel, rateSlider); placeKnob(sX + 315, sY, bitLabel, bitSlider);
    auto aEnvRect = centerCol.removeFromTop(100); ampEnvGroup.setBounds(aEnvRect); int aX = aEnvRect.getX(), aY = aEnvRect.getY() + 15;
    placeKnob(aX + 15, aY, ampAtkLabel, ampAtkSlider); placeKnob(aX + 115, aY, ampDecLabel, ampDecSlider); placeKnob(aX + 215, aY, ampSusLabel, ampSusSlider); placeKnob(aX + 315, aY, ampRelLabel, ampRelSlider);

    auto presetRect = centerCol.removeFromBottom(40); int pX = presetRect.getX() + 10, pY = presetRect.getY() + 5;
    presetCombo.setBounds(pX, pY, 290, 24);
    savePresetBtn.setBounds(pX + 300, pY, 80, 24);

    auto filterRect = centerCol; filterGroup.setBounds(filterRect.reduced(0, 5)); int fX = filterRect.getX() + 10, fY = filterRect.getY() + 20;
    fltABtn.setBounds(fX, fY, 35, 24); fltBBtn.setBounds(fX + 35, fY, 35, 24); fltATypeCombo.setBounds(fX + 80, fY, 80, 24); fltBTypeCombo.setBounds(fX + 80, fY, 80, 24); fltRoutingCombo.setBounds(fX + 170, fY, 80, 24);
    int r2Y = fY + 35, spacing = 80;
    placeKnob(fX, r2Y, fltACutoffLabel, fltACutoffSlider); placeKnob(fX, r2Y, fltBCutoffLabel, fltBCutoffSlider);
    placeKnob(fX + spacing, r2Y, fltAResLabel, fltAResSlider); placeKnob(fX + spacing, r2Y, fltBResLabel, fltBResSlider);
    placeKnob(fX + spacing * 2, r2Y, fltMixLabel, fltMixSlider); placeKnob(fX + spacing * 3, r2Y, fltAEnvAmtLabel, fltAEnvAmtSlider); placeKnob(fX + spacing * 3, r2Y, fltBEnvAmtLabel, fltBEnvAmtSlider);
    int r3Y = r2Y + 70;
    placeKnob(fX, r3Y, fltAAtkLabel, fltAAtkSlider); placeKnob(fX, r3Y, fltBAtkLabel, fltBAtkSlider);
    placeKnob(fX + spacing, r3Y, fltADecLabel, fltADecSlider); placeKnob(fX + spacing, r3Y, fltBDecLabel, fltBDecSlider);
    placeKnob(fX + spacing * 2, r3Y, fltASusLabel, fltASusSlider); placeKnob(fX + spacing * 2, r3Y, fltBSusLabel, fltBSusSlider);
    placeKnob(fX + spacing * 3, r3Y, fltARelLabel, fltARelSlider); placeKnob(fX + spacing * 3, r3Y, fltBRelLabel, fltBRelSlider);
    modTabs.setBounds(modCol);
}