// ==============================================================================
// Source/PluginProcessor.cpp
// ==============================================================================
#include "PluginProcessor.h"
#include "PluginEditor.h"

LiquidDreamAudioProcessor::LiquidDreamAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    apvts(*this, nullptr, "PARAMS", createParameterLayout())
{
    outputScopeData.fill(0.0f); tempScopeBuffer.fill(0.0f);

    pOscOn = apvts.getRawParameterValue("osc_on"); pWave = apvts.getRawParameterValue("osc_wave"); pPos = apvts.getRawParameterValue("osc_pos");
    pOscLevel = apvts.getRawParameterValue("osc_level"); pOscPitch = apvts.getRawParameterValue("osc_pitch");
    pPDecayAmt = apvts.getRawParameterValue("osc_pdecay_amt"); pPDecayTime = apvts.getRawParameterValue("osc_pdecay_time");
    pFm = apvts.getRawParameterValue("osc_fm"); pFmWave = apvts.getRawParameterValue("osc_fm_wave");
    pMorphAMode = apvts.getRawParameterValue("osc_morph_a_mode"); pMorphAAmt = apvts.getRawParameterValue("osc_morph_a_amt"); pMorphAShift = apvts.getRawParameterValue("osc_morph_a_shift");
    pMorphBMode = apvts.getRawParameterValue("osc_morph_b_mode"); pMorphBAmt = apvts.getRawParameterValue("osc_morph_b_amt"); pMorphBShift = apvts.getRawParameterValue("osc_morph_b_shift");
    pMorphCMode = apvts.getRawParameterValue("osc_morph_c_mode"); pMorphCAmt = apvts.getRawParameterValue("osc_morph_c_amt"); pMorphCShift = apvts.getRawParameterValue("osc_morph_c_shift");
    pUni = apvts.getRawParameterValue("osc_uni"); pDetune = apvts.getRawParameterValue("osc_detune"); pWidth = apvts.getRawParameterValue("osc_width"); pDrift = apvts.getRawParameterValue("osc_drift");
    pSubOn = apvts.getRawParameterValue("sub_on"); pSubWave = apvts.getRawParameterValue("sub_wave"); pSubVol = apvts.getRawParameterValue("sub_vol"); pSubPitch = apvts.getRawParameterValue("sub_pitch");
    pCutoff = apvts.getRawParameterValue("flt_cutoff"); pReso = apvts.getRawParameterValue("flt_res"); pFltEnvAmt = apvts.getRawParameterValue("flt_env_amt");
    pDrive = apvts.getRawParameterValue("dist_drive"); pShpAmt = apvts.getRawParameterValue("shp_amt"); pShpRate = apvts.getRawParameterValue("shp_rate"); pShpBit = apvts.getRawParameterValue("shp_bit");
    pGain = apvts.getRawParameterValue("m_gain"); pGlide = apvts.getRawParameterValue("m_glide"); pLegato = apvts.getRawParameterValue("m_legato");
    pAAtk = apvts.getRawParameterValue("a_atk"); pADec = apvts.getRawParameterValue("a_dec"); pASus = apvts.getRawParameterValue("a_sus"); pARel = apvts.getRawParameterValue("a_rel");
    pFAtk = apvts.getRawParameterValue("f_atk"); pFDec = apvts.getRawParameterValue("f_dec"); pFSus = apvts.getRawParameterValue("f_sus"); pFRel = apvts.getRawParameterValue("f_rel");

    pColorOn = apvts.getRawParameterValue("color_on");
    pColorType = apvts.getRawParameterValue("color_type");
    pColorMix = apvts.getRawParameterValue("color_mix");
    pColorPreHp = apvts.getRawParameterValue("color_pre_hp");
    pColorPostHp = apvts.getRawParameterValue("color_post_hp");
    pColorAtk = apvts.getRawParameterValue("color_atk");
    pColorDec = apvts.getRawParameterValue("color_dec");

    pOttDepth = apvts.getRawParameterValue("ott_depth");
    pOttTime = apvts.getRawParameterValue("ott_time");
    pOttUp = apvts.getRawParameterValue("ott_up");
    pOttDown = apvts.getRawParameterValue("ott_down");
    pOttGain = apvts.getRawParameterValue("ott_gain");

    // ★ 変更
    pArpWave = apvts.getRawParameterValue("arp_wave");
    pArpMode = apvts.getRawParameterValue("arp_mode");
    pArpSpeed = apvts.getRawParameterValue("arp_speed");
    pArpPitch = apvts.getRawParameterValue("arp_pitch");
    pArpLevel = apvts.getRawParameterValue("arp_level");

    for (int i = 0; i < 3; ++i) {
        juce::String ms = "mod" + juce::String(i + 1) + "_";
        pModOn[i] = apvts.getRawParameterValue(ms + "on");
        pModAtk[i] = apvts.getRawParameterValue(ms + "atk"); pModDec[i] = apvts.getRawParameterValue(ms + "dec");
        pModSus[i] = apvts.getRawParameterValue(ms + "sus"); pModRel[i] = apvts.getRawParameterValue(ms + "rel");
        pModAmt[i] = apvts.getRawParameterValue(ms + "amt");

        juce::String ls = "lfo" + juce::String(i + 1) + "_";
        pLfoOn[i] = apvts.getRawParameterValue(ls + "on");
        pLfoWave[i] = apvts.getRawParameterValue(ls + "wave"); pLfoSync[i] = apvts.getRawParameterValue(ls + "sync");
        pLfoRate[i] = apvts.getRawParameterValue(ls + "rate"); pLfoBeat[i] = apvts.getRawParameterValue(ls + "beat");
        pLfoAmt[i] = apvts.getRawParameterValue(ls + "amt");
    }

    for (int src = 0; src < 6; ++src) {
        for (int slot = 0; slot < 3; ++slot) {
            pMatrixDest[src][slot] = apvts.getRawParameterValue("matrix_s" + juce::String(src) + "_d" + juce::String(slot));
            pMatrixAmt[src][slot] = apvts.getRawParameterValue("matrix_s" + juce::String(src) + "_a" + juce::String(slot));
        }
    }
}

LiquidDreamAudioProcessor::~LiquidDreamAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout LiquidDreamAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterBool>("osc_on", "Osc On", true));
    params.push_back(std::make_unique<juce::AudioParameterInt>("osc_wave", "Waveform", -1, EmbeddedWavetables::numTables - 1, -1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc_level", "WT Level", 0.0f, 1.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc_pos", "Position", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc_pitch", "WT Pitch", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc_pdecay_amt", "P.Decay", -24.0f, 24.0f, 0.0f));
    auto timeRange = juce::NormalisableRange<float>(1.0f, 2000.0f, 1.0f, 0.3f);
    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc_pdecay_time", "P.Time", timeRange, 100.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc_fm", "FM Amt", 0.0f, 3.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterInt>("osc_fm_wave", "FM Wave", 0, 3, 0));

    params.push_back(std::make_unique<juce::AudioParameterInt>("osc_morph_a_mode", "Morph A", 0, 13, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc_morph_a_amt", "Amount A", -1.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc_morph_a_shift", "Shift A", -1.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterInt>("osc_morph_b_mode", "Morph B", 0, 13, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc_morph_b_amt", "Amount B", -1.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc_morph_b_shift", "Shift B", -1.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterInt>("osc_morph_c_mode", "Morph C", 0, 13, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc_morph_c_amt", "Amount C", -1.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc_morph_c_shift", "Shift C", -1.0f, 1.0f, 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterInt>("osc_uni", "Unison", 1, 12, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc_detune", "Detune", 0.0f, 1.0f, 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc_width", "Width", 0.0f, 1.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc_drift", "Drift", 0.0f, 1.0f, 0.1f));

    params.push_back(std::make_unique<juce::AudioParameterBool>("sub_on", "Sub On", true));
    params.push_back(std::make_unique<juce::AudioParameterInt>("sub_wave", "Sub Wave", 0, 3, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("sub_vol", "Sub Vol", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("sub_pitch", "Sub Pitch", -24.0f, 0.0f, -12.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("dist_drive", "Drive", 1.0f, 10.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("shp_amt", "Sine Shaper", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("shp_bit", "Bit Depth", 1.0f, 24.0f, 24.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("shp_rate", "DS Rate", 1.0f, 20.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("flt_cutoff", "Cutoff", 20.0f, 20000.0f, 20000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("flt_res", "Reso", 0.0f, 0.95f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("flt_env_amt", "Env Amt", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("m_gain", "Gain", 0.0f, 1.0f, 0.5f));

    auto glideRange = juce::NormalisableRange<float>(0.0f, 1000.0f, 1.0f, 0.3f);
    params.push_back(std::make_unique<juce::AudioParameterFloat>("m_glide", "Glide", glideRange, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("m_legato", "Legato", false));
    params.push_back(std::make_unique<juce::AudioParameterInt>("m_pb", "PB Range", 0, 24, 12));

    auto attRange = juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f);
    params.push_back(std::make_unique<juce::AudioParameterFloat>("a_atk", "Amp Atk", attRange, 0.01f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("a_dec", "Amp Dec", attRange, 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("a_sus", "Amp Sus", 0.0f, 1.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("a_rel", "Amp Rel", attRange, 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("f_atk", "Flt Atk", attRange, 0.01f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("f_dec", "Flt Dec", attRange, 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("f_sus", "Flt Sus", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("f_rel", "Flt Rel", attRange, 0.3f));

    // ColorIR Params
    params.push_back(std::make_unique<juce::AudioParameterBool>("color_on", "Color On", false));
    params.push_back(std::make_unique<juce::AudioParameterInt>("color_type", "IR Type", 0, 3, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("color_mix", "Color Mix", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("color_pre_hp", "Pre HPF", 20.0f, 2000.0f, 150.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("color_post_hp", "Post HPF", 20.0f, 2000.0f, 150.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("color_atk", "IR Attack", 2.0f, 100.0f, 5.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("color_dec", "IR Decay", 10.0f, 500.0f, 100.0f));

    // True OTT Params
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ott_depth", "OTT Depth", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ott_time", "OTT Time", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ott_up", "Upward %", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ott_down", "Downward %", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ott_gain", "Out Gain", -12.0f, 24.0f, 0.0f));

    // ★ 変更：Rate (Int) を Speed (Hzの連続Float) に変更
    auto speedRange = juce::NormalisableRange<float>(1.0f, 100.0f, 0.1f, 0.3f);
    params.push_back(std::make_unique<juce::AudioParameterInt>("arp_wave", "Arp Wave", 0, 4, 2));
    params.push_back(std::make_unique<juce::AudioParameterInt>("arp_mode", "Arp Mode", 0, 3, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("arp_speed", "Arp Speed", speedRange, 20.0f));
    params.push_back(std::make_unique<juce::AudioParameterInt>("arp_pitch", "Arp Pitch", 0, 2, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("arp_level", "Arp Level", 0.0f, 1.0f, 0.0f));

    for (int i = 1; i <= 3; ++i) {
        juce::String pfx = "mod" + juce::String(i) + "_";
        juce::String nm = "Mod" + juce::String(i) + " ";
        params.push_back(std::make_unique<juce::AudioParameterBool>(pfx + "on", nm + "On", true));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(pfx + "atk", nm + "Atk", attRange, 0.01f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(pfx + "dec", nm + "Dec", attRange, 0.2f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(pfx + "sus", nm + "Sus", 0.0f, 1.0f, 0.5f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(pfx + "rel", nm + "Rel", attRange, 0.3f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(pfx + "amt", nm + "Amt", 0.0f, 1.0f, 1.0f));
    }

    auto lfoHzRange = juce::NormalisableRange<float>(0.01f, 50.0f, 0.01f, 0.3f);
    for (int i = 1; i <= 3; ++i) {
        juce::String pfx = "lfo" + juce::String(i) + "_";
        juce::String nm = "LFO" + juce::String(i) + " ";
        params.push_back(std::make_unique<juce::AudioParameterBool>(pfx + "on", nm + "On", true));
        params.push_back(std::make_unique<juce::AudioParameterInt>(pfx + "wave", nm + "Wave", 0, 3, 0));
        params.push_back(std::make_unique<juce::AudioParameterBool>(pfx + "sync", nm + "Sync", false));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(pfx + "rate", nm + "Rate", lfoHzRange, 1.0f));
        params.push_back(std::make_unique<juce::AudioParameterInt>(pfx + "beat", nm + "Beat", 0, 8, 2));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(pfx + "amt", nm + "Amt", 0.0f, 1.0f, 1.0f));
    }

    for (int src = 0; src < 6; ++src) {
        for (int slot = 0; slot < 3; ++slot) {
            juce::String destId = "matrix_s" + juce::String(src) + "_d" + juce::String(slot);
            juce::String amtId = "matrix_s" + juce::String(src) + "_a" + juce::String(slot);
            params.push_back(std::make_unique<juce::AudioParameterInt>(destId, "Dest", 0, 11, 0));
            params.push_back(std::make_unique<juce::AudioParameterFloat>(amtId, "Amt", -1.0f, 1.0f, 0.0f));
        }
    }

    return { params.begin(), params.end() };
}

void LiquidDreamAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    oscillator.prepare(sampleRate);
    spectralMorph.prepare(sampleRate, samplesPerBlock);
    filter.prepare(sampleRate);
    shaper.prepare(sampleRate);
    voiceManager.setSampleRate(sampleRate);
    colorEngine.prepare(sampleRate, samplesPerBlock);

    ampEnv.setSampleRate(sampleRate); filterEnv.setSampleRate(sampleRate);
    for (auto& env : modEnvs) env.setSampleRate(sampleRate);
    for (auto& lfo : lfos) lfo.setSampleRate(sampleRate);

    tempEnvBuffer.setSize(5, samplesPerBlock);
    tempSubBuffer.setSize(2, samplesPerBlock);
    tempWavetableBuffer.setSize(2, samplesPerBlock);

    double st = 0.02;
    smoothedWtLevel.reset(sampleRate, st); smoothedWtPitch.reset(sampleRate, st);
    smoothedPDecayAmt.reset(sampleRate, st); smoothedPDecayTime.reset(sampleRate, st);
    smoothedCutoff.reset(sampleRate, st); smoothedReso.reset(sampleRate, st);
    smoothedFltEnvAmt.reset(sampleRate, st); smoothedDrive.reset(sampleRate, st); smoothedShpAmt.reset(sampleRate, st);
    smoothedShpRate.reset(sampleRate, st); smoothedShpBit.reset(sampleRate, st); smoothedGain.reset(sampleRate, st);
    smoothedWtPos.reset(sampleRate, st); smoothedFm.reset(sampleRate, st); smoothedDrift.reset(sampleRate, st);
    smoothedSubVol.reset(sampleRate, st); smoothedSubPitch.reset(sampleRate, st); smoothedWidth.reset(sampleRate, st);
    smoothedMorphAAmt.reset(sampleRate, st); smoothedMorphAShift.reset(sampleRate, st);
    smoothedMorphBAmt.reset(sampleRate, st); smoothedMorphBShift.reset(sampleRate, st);
    smoothedMorphCAmt.reset(sampleRate, st); smoothedMorphCShift.reset(sampleRate, st);

    smoothedWtLevel.setCurrentAndTargetValue(pOscLevel->load(std::memory_order_relaxed));

    lastOscFreq = -1.0f;
    lastModeA = -1; lastModeB = -1; lastModeC = -1;
}

void LiquidDreamAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals; buffer.clear(); if (buffer.getNumChannels() < 2) return;

    double currentBpm = 120.0;

    auto updateSmoothTime = [this](std::atomic<float>* pMode, int& lastMode, juce::SmoothedValue<float>& sAmt, juce::SmoothedValue<float>& sShift) {
        int currentMode = (int)pMode->load(std::memory_order_relaxed);
        if (currentMode != lastMode) {
            double time = (currentMode >= 8) ? 0.025 : 0.005;
            sAmt.reset(getSampleRate(), time);
            sShift.reset(getSampleRate(), time);
            lastMode = currentMode;
        }
        return currentMode;
        };

    int currentModeA = updateSmoothTime(pMorphAMode, lastModeA, smoothedMorphAAmt, smoothedMorphAShift);
    int currentModeB = updateSmoothTime(pMorphBMode, lastModeB, smoothedMorphBAmt, smoothedMorphBShift);
    int currentModeC = updateSmoothTime(pMorphCMode, lastModeC, smoothedMorphCAmt, smoothedMorphCShift);

    smoothedWtPos.setTargetValue(pPos->load(std::memory_order_relaxed)); smoothedFm.setTargetValue(pFm->load(std::memory_order_relaxed));
    smoothedCutoff.setTargetValue(pCutoff->load(std::memory_order_relaxed)); smoothedReso.setTargetValue(pReso->load(std::memory_order_relaxed));
    smoothedGain.setTargetValue(pGain->load(std::memory_order_relaxed));
    smoothedMorphAAmt.setTargetValue(pMorphAAmt->load(std::memory_order_relaxed)); smoothedMorphAShift.setTargetValue(pMorphAShift->load(std::memory_order_relaxed));
    smoothedMorphBAmt.setTargetValue(pMorphBAmt->load(std::memory_order_relaxed)); smoothedMorphBShift.setTargetValue(pMorphBShift->load(std::memory_order_relaxed));
    smoothedMorphCAmt.setTargetValue(pMorphCAmt->load(std::memory_order_relaxed)); smoothedMorphCShift.setTargetValue(pMorphCShift->load(std::memory_order_relaxed));

    smoothedWtLevel.setTargetValue(pOscLevel->load(std::memory_order_relaxed));
    smoothedDrive.setTargetValue(pDrive->load(std::memory_order_relaxed)); smoothedShpAmt.setTargetValue(pShpAmt->load(std::memory_order_relaxed));
    smoothedShpRate.setTargetValue(pShpRate->load(std::memory_order_relaxed)); smoothedShpBit.setTargetValue(pShpBit->load(std::memory_order_relaxed));
    smoothedSubVol.setTargetValue(pSubVol->load(std::memory_order_relaxed));

    int waveIdx = (int)pWave->load(std::memory_order_relaxed);
    static int lastWaveIdx = -2;
    if (waveIdx != lastWaveIdx) {
        if (waveIdx >= 0 && waveIdx < EmbeddedWavetables::numTables) oscillator.loadWavetableFile(EmbeddedWavetables::allNames[waveIdx]);
        else if (waveIdx == -1) oscillator.createDefaultBasicShapes();
        lastWaveIdx = waveIdx;
    }

    voiceManager.setLegatoMode(pLegato->load(std::memory_order_relaxed) > 0.5f);

    for (const auto metadata : midiMessages) {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn()) {
            if (colorEngine.getLearnState() == ColorIREngine::LearnState::Learning) {
                colorEngine.addNote(msg.getNoteNumber());
            }
            if (std::find(activeMidiNotes.begin(), activeMidiNotes.end(), msg.getNoteNumber()) == activeMidiNotes.end()) {
                activeMidiNotes.push_back(msg.getNoteNumber());
            }

            if (voiceManager.noteOn(msg.getNoteNumber(), msg.getVelocity())) {
                bool isLegato = voiceManager.isLegatoTransition();
                ampEnv.noteOn(isLegato); filterEnv.noteOn(isLegato);
                for (auto& env : modEnvs) env.noteOn(isLegato);
                for (auto& lfo : lfos) lfo.noteOn(isLegato);
            }
        }
        else if (msg.isNoteOff()) {
            activeMidiNotes.erase(std::remove(activeMidiNotes.begin(), activeMidiNotes.end(), msg.getNoteNumber()), activeMidiNotes.end());

            if (voiceManager.noteOff(msg.getNoteNumber())) {
                ampEnv.noteOff(); filterEnv.noteOff();
                for (auto& env : modEnvs) env.noteOff();
            }
        }
    }

    oscillator.setOscOn(pOscOn->load(std::memory_order_relaxed) > 0.5f);
    oscillator.setSubOn(pSubOn->load(std::memory_order_relaxed) > 0.5f);
    oscillator.setSubWaveform((int)pSubWave->load(std::memory_order_relaxed));
    oscillator.setUnisonCount((int)pUni->load(std::memory_order_relaxed));
    oscillator.setUnisonDetune(pDetune->load(std::memory_order_relaxed));
    oscillator.setFMWaveform((int)pFmWave->load(std::memory_order_relaxed));

    voiceManager.setGlideTime(pGlide->load(std::memory_order_relaxed));
    ampEnv.setParameters(pAAtk->load(std::memory_order_relaxed), pADec->load(std::memory_order_relaxed), pASus->load(std::memory_order_relaxed), pARel->load(std::memory_order_relaxed));
    filterEnv.setParameters(pFAtk->load(std::memory_order_relaxed), pFDec->load(std::memory_order_relaxed), pFSus->load(std::memory_order_relaxed), pFRel->load(std::memory_order_relaxed));

    for (int i = 0; i < 3; ++i) {
        modEnvs[i].setParameters(pModAtk[i]->load(), pModDec[i]->load(), pModSus[i]->load(), pModRel[i]->load());
        lfos[i].setParameters((int)pLfoWave[i]->load(), pLfoSync[i]->load() > 0.5f, pLfoRate[i]->load(), (int)pLfoBeat[i]->load(), pLfoAmt[i]->load());
    }

    int numSamples = buffer.getNumSamples();
    if (tempEnvBuffer.getNumSamples() < numSamples) {
        tempEnvBuffer.setSize(5, numSamples, true, true, true);
        tempSubBuffer.setSize(2, numSamples, true, true, true);
        tempWavetableBuffer.setSize(2, numSamples, true, true, true);
    }

    auto* left = buffer.getWritePointer(0); auto* right = buffer.getWritePointer(1);
    auto* wtL = tempWavetableBuffer.getWritePointer(0); auto* wtR = tempWavetableBuffer.getWritePointer(1);
    float stft_aA = 0.0f, stft_sA = 0.0f, stft_aB = 0.0f, stft_sB = 0.0f, stft_aC = 0.0f, stft_sC = 0.0f;

    for (int i = 0; i < numSamples; ++i) {
        float modValues[6] = {
            (pModOn[0]->load(std::memory_order_relaxed) > 0.5f ? modEnvs[0].getNextSample() * pModAmt[0]->load(std::memory_order_relaxed) : 0.0f),
            (pModOn[1]->load(std::memory_order_relaxed) > 0.5f ? modEnvs[1].getNextSample() * pModAmt[1]->load(std::memory_order_relaxed) : 0.0f),
            (pModOn[2]->load(std::memory_order_relaxed) > 0.5f ? modEnvs[2].getNextSample() * pModAmt[2]->load(std::memory_order_relaxed) : 0.0f),
            (pLfoOn[0]->load(std::memory_order_relaxed) > 0.5f ? lfos[0].getNextSample(currentBpm) : 0.0f),
            (pLfoOn[1]->load(std::memory_order_relaxed) > 0.5f ? lfos[1].getNextSample(currentBpm) : 0.0f),
            (pLfoOn[2]->load(std::memory_order_relaxed) > 0.5f ? lfos[2].getNextSample(currentBpm) : 0.0f)
        };

        float destMods[12] = { 0 };
        for (int src = 0; src < 6; ++src) {
            for (int slot = 0; slot < 3; ++slot) {
                int dest = (int)pMatrixDest[src][slot]->load(std::memory_order_relaxed);
                if (dest > 0 && dest < 12) {
                    destMods[dest] += modValues[src] * pMatrixAmt[src][slot]->load(std::memory_order_relaxed);
                }
            }
        }

        float modPos = juce::jlimit(0.0f, 1.0f, smoothedWtPos.getNextValue() + destMods[1]);
        float modFm = juce::jlimit(0.0f, 3.0f, smoothedFm.getNextValue() + destMods[2] * 3.0f);

        float aA = juce::jlimit(-1.0f, 1.0f, smoothedMorphAAmt.getNextValue() + destMods[3]);
        float sA = juce::jlimit(-1.0f, 1.0f, smoothedMorphAShift.getNextValue() + destMods[4]);
        float aB = juce::jlimit(-1.0f, 1.0f, smoothedMorphBAmt.getNextValue() + destMods[5]);
        float sB = juce::jlimit(-1.0f, 1.0f, smoothedMorphBShift.getNextValue() + destMods[6]);
        float aC = juce::jlimit(-1.0f, 1.0f, smoothedMorphCAmt.getNextValue() + destMods[7]);
        float sC = juce::jlimit(-1.0f, 1.0f, smoothedMorphCShift.getNextValue() + destMods[8]);

        if (i == 0) { stft_aA = aA; stft_sA = sA; stft_aB = aB; stft_sB = sB; stft_aC = aC; stft_sC = sC; }

        oscillator.setWavetablePosition(modPos);
        oscillator.setFMAmount(modFm);
        oscillator.setDriftAmount(smoothedDrift.getNextValue());

        oscillator.setMorphA(currentModeA, aA, sA);
        oscillator.setMorphB(currentModeB, aB, sB);
        oscillator.setMorphC(currentModeC, aC, sC);

        oscillator.setWavetableLevel(smoothedWtLevel.getNextValue());
        oscillator.setSubVolume(smoothedSubVol.getNextValue());

        float cf = voiceManager.getCurrentFrequency();
        if (cf < 1.0f) cf = 1.0f;
        if (std::abs(cf - lastOscFreq) > 0.01f) { oscillator.setFrequency(cf); lastOscFreq = cf; }

        float aVal = ampEnv.getNextSample();
        float fVal = filterEnv.getNextSample();
        if (ampEnv.popJustReset()) oscillator.resetPhase();

        tempEnvBuffer.setSample(0, i, aVal);
        tempEnvBuffer.setSample(1, i, fVal);
        tempEnvBuffer.setSample(2, i, destMods[9]);
        tempEnvBuffer.setSample(3, i, destMods[10]);
        tempEnvBuffer.setSample(4, i, destMods[11]);

        float oL = 0.0f, oR = 0.0f, subL = 0.0f, subR = 0.0f;
        oscillator.getSampleStereo(oL, oR, subL, subR);

        wtL[i] = oL; wtR[i] = oR;
        tempSubBuffer.setSample(0, i, subL); tempSubBuffer.setSample(1, i, subR);
    }

    spectralMorph.process(tempWavetableBuffer, currentModeA, stft_aA, stft_sA, currentModeB, stft_aB, stft_sB, currentModeC, stft_aC, stft_sC);

    for (int i = 0; i < numSamples; ++i) {
        float cd = smoothedDrive.getNextValue(), csa = smoothedShpAmt.getNextValue(), csr = smoothedShpRate.getNextValue(), csb = smoothedShpBit.getNextValue();
        float sL = wtL[i]; float sR = wtR[i];
        shaper.processStereo(sL, sR, cd, csa, csr, csb, sL, sR);
        wtL[i] = sL; wtR[i] = sR;
    }

    if (pColorOn->load() > 0.5f) {
        colorEngine.setOttParameters(pOttDepth->load(), pOttTime->load(), pOttUp->load(), pOttDown->load(), pOttGain->load());
        colorEngine.setParameters(pColorPreHp->load(), pColorPostHp->load(), pColorMix->load());
        colorEngine.process(tempWavetableBuffer);
    }

    for (int i = 0; i < numSamples; ++i) {
        float cc = smoothedCutoff.getNextValue(), cr = smoothedReso.getNextValue(), ce = pFltEnvAmt->load(), cg = smoothedGain.getNextValue();
        float aVal = tempEnvBuffer.getSample(0, i);
        float fVal = tempEnvBuffer.getSample(1, i);
        float cutoffMod = tempEnvBuffer.getSample(2, i);
        float resoMod = tempEnvBuffer.getSample(3, i);
        float gainMod = tempEnvBuffer.getSample(4, i);

        float sL = wtL[i] + tempSubBuffer.getSample(0, i);
        float sR = wtR[i] + tempSubBuffer.getSample(1, i);

        float mc = cc * std::exp2(cutoffMod * 8.0f);
        mc += (fVal * ce * 10000.0f);
        filter.setParameters(juce::jlimit(20.0f, 20000.0f, mc), juce::jlimit(0.0f, 0.95f, cr + resoMod));

        sL = filter.processSample(sL); sR = filter.processSample(sR);

        float finalGain = juce::jlimit(0.0f, 1.0f, cg + gainMod) * aVal;
        left[i] = sL * finalGain; right[i] = sR * finalGain;
    }

    if (pColorOn->load() > 0.5f) {
        // ★ 変更：pArpSpeedの値を渡す
        colorEngine.setArpParameters((int)pArpWave->load(), (int)pArpMode->load(), pArpSpeed->load(), (int)pArpPitch->load(), pArpLevel->load());

        std::vector<int> currentNotes;
        {
            std::lock_guard<std::mutex> lock(midiNotesMutex);
            currentNotes = activeMidiNotes;
        }
        colorEngine.processArp(buffer, currentNotes);
    }

    for (int i = 0; i < numSamples; ++i) {
        tempScopeBuffer[(size_t)scopeWriteIndex] = (left[i] + right[i]) * 0.5f;
        scopeWriteIndex++;
        if (scopeWriteIndex >= 512) {
            outputScopeData = tempScopeBuffer;
            scopeWriteIndex = 0;
        }
    }
}

juce::AudioProcessorEditor* LiquidDreamAudioProcessor::createEditor() { return new LiquidDreamAudioProcessorEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new LiquidDreamAudioProcessor(); }