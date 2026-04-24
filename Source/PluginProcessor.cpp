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

    pFltAType = apvts.getRawParameterValue("flt_a_type"); pFltACutoff = apvts.getRawParameterValue("flt_a_cutoff"); pFltAReso = apvts.getRawParameterValue("flt_a_res");
    pFltBType = apvts.getRawParameterValue("flt_b_type"); pFltBCutoff = apvts.getRawParameterValue("flt_b_cutoff"); pFltBReso = apvts.getRawParameterValue("flt_b_res");
    pFltRouting = apvts.getRawParameterValue("flt_routing"); pFltMix = apvts.getRawParameterValue("flt_mix");
    pFltAEnvAmt = apvts.getRawParameterValue("flt_a_env_amt"); pFltBEnvAmt = apvts.getRawParameterValue("flt_b_env_amt");

    pDrive = apvts.getRawParameterValue("dist_drive"); pShpAmt = apvts.getRawParameterValue("shp_amt"); pShpRate = apvts.getRawParameterValue("shp_rate"); pShpBit = apvts.getRawParameterValue("shp_bit");
    pGain = apvts.getRawParameterValue("m_gain"); pGlide = apvts.getRawParameterValue("m_glide"); pLegato = apvts.getRawParameterValue("m_legato");
    pAAtk = apvts.getRawParameterValue("a_atk"); pADec = apvts.getRawParameterValue("a_dec"); pASus = apvts.getRawParameterValue("a_sus"); pARel = apvts.getRawParameterValue("a_rel");
    pFAtkA = apvts.getRawParameterValue("f_a_atk"); pFDecA = apvts.getRawParameterValue("f_a_dec"); pFSusA = apvts.getRawParameterValue("f_a_sus"); pFRelA = apvts.getRawParameterValue("f_a_rel");
    pFAtkB = apvts.getRawParameterValue("f_b_atk"); pFDecB = apvts.getRawParameterValue("f_b_dec"); pFSusB = apvts.getRawParameterValue("f_b_sus"); pFRelB = apvts.getRawParameterValue("f_b_rel");

    pColorOn = apvts.getRawParameterValue("color_on"); pColorType = apvts.getRawParameterValue("color_type");
    pColorMix = apvts.getRawParameterValue("color_mix"); pColorIrVol = apvts.getRawParameterValue("color_ir_vol");
    pColorPreHp = apvts.getRawParameterValue("color_pre_hp"); pColorPostHp = apvts.getRawParameterValue("color_post_hp");
    pColorAtk = apvts.getRawParameterValue("color_atk"); pColorDec = apvts.getRawParameterValue("color_dec");

    pOttDepth = apvts.getRawParameterValue("ott_depth"); pOttTime = apvts.getRawParameterValue("ott_time");
    pOttUp = apvts.getRawParameterValue("ott_up"); pOttDown = apvts.getRawParameterValue("ott_down"); pOttGain = apvts.getRawParameterValue("ott_gain");
    pSootheSelectivity = apvts.getRawParameterValue("soothe_sel"); pSootheSharpness = apvts.getRawParameterValue("soothe_shp"); pSootheFocus = apvts.getRawParameterValue("soothe_foc");

    pArpWave = apvts.getRawParameterValue("arp_wave"); pArpMode = apvts.getRawParameterValue("arp_mode");
    pArpSpeed = apvts.getRawParameterValue("arp_speed"); pArpPitch = apvts.getRawParameterValue("arp_pitch"); pArpLevel = apvts.getRawParameterValue("arp_level");

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
        pLfoAmt[i] = apvts.getRawParameterValue(ls + "amt"); pLfoTrig[i] = apvts.getRawParameterValue(ls + "trig");
    }

    // ★ 変更: 8つの動的マトリックススロットをバインド
    for (int i = 0; i < 8; ++i) {
        pMatrixSrc[i] = apvts.getRawParameterValue("matrix_src_" + juce::String(i));
        pMatrixDest[i] = apvts.getRawParameterValue("matrix_dest_" + juce::String(i));
        pMatrixAmt[i] = apvts.getRawParameterValue("matrix_amt_" + juce::String(i));
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
    params.push_back(std::make_unique<juce::AudioParameterInt>("flt_a_type", "Flt A Type", 0, 5, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("flt_a_cutoff", "Flt A Cutoff", 20.0f, 20000.0f, 20000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("flt_a_res", "Flt A Reso", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterInt>("flt_b_type", "Flt B Type", 0, 5, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("flt_b_cutoff", "Flt B Cutoff", 20.0f, 20000.0f, 20000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("flt_b_res", "Flt B Reso", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterInt>("flt_routing", "Routing", 0, 1, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("flt_mix", "Flt Mix", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("flt_a_env_amt", "FltA Env Amt", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("flt_b_env_amt", "FltB Env Amt", 0.0f, 1.0f, 0.0f));
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
    params.push_back(std::make_unique<juce::AudioParameterFloat>("f_a_atk", "Flt A Atk", attRange, 0.01f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("f_a_dec", "Flt A Dec", attRange, 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("f_a_sus", "Flt A Sus", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("f_a_rel", "Flt A Rel", attRange, 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("f_b_atk", "Flt B Atk", attRange, 0.01f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("f_b_dec", "Flt B Dec", attRange, 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("f_b_sus", "Flt B Sus", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("f_b_rel", "Flt B Rel", attRange, 0.3f));

    params.push_back(std::make_unique<juce::AudioParameterBool>("color_on", "Color On", false));
    params.push_back(std::make_unique<juce::AudioParameterInt>("color_type", "IR Type", 0, 3, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("color_mix", "Color Mix", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("color_ir_vol", "IR Vol", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("color_pre_hp", "Pre HPF", 20.0f, 2000.0f, 150.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("color_post_hp", "Post HPF", 20.0f, 2000.0f, 150.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("color_atk", "IR Attack", 2.0f, 100.0f, 5.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("color_dec", "IR Decay", 10.0f, 500.0f, 100.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("ott_depth", "OTT Depth", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ott_time", "OTT Time", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ott_up", "Upward %", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ott_down", "Downward %", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ott_gain", "Out Gain", -12.0f, 24.0f, 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("soothe_sel", "Selectivity", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("soothe_shp", "Sharpness", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("soothe_foc", "Focus", -1.0f, 1.0f, 0.0f));

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
        params.push_back(std::make_unique<juce::AudioParameterInt>(pfx + "trig", nm + "Trig", 0, 2, 0));
    }

    // ★ 変更: ダイナミックマトリックスの8スロット (Source 0~6, Dest 0~13)
    for (int i = 0; i < 8; ++i) {
        juce::String sIdx = juce::String(i);
        params.push_back(std::make_unique<juce::AudioParameterInt>("matrix_src_" + sIdx, "Src", 0, 6, 0));
        params.push_back(std::make_unique<juce::AudioParameterInt>("matrix_dest_" + sIdx, "Dest", 0, 13, 0));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("matrix_amt_" + sIdx, "Amt", -1.0f, 1.0f, 0.0f));
    }

    return { params.begin(), params.end() };
}

void LiquidDreamAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    if (xml != nullptr) {
        xml->setAttribute("CustomWavePath", currentCustomWavetablePath);
        xml->setAttribute("UserFolders", userWavetableFolders.joinIntoString("|"));
        copyXmlToBinary(*xml, destData);
    }
}

void LiquidDreamAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr) {
        if (xmlState->hasTagName(apvts.state.getType())) {
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
        }
        userWavetableFolders.clear();
        juce::String foldersStr = xmlState->getStringAttribute("UserFolders", "");
        if (foldersStr.isNotEmpty()) userWavetableFolders.addTokens(foldersStr, "|", "");

        juce::String customPath = xmlState->getStringAttribute("CustomWavePath", "");
        if (customPath.isNotEmpty() && juce::File(customPath).existsAsFile()) {
            loadCustomWavetable(juce::File(customPath));
        }
    }
}

void LiquidDreamAudioProcessor::loadCustomWavetable(const juce::File& file) {
    currentCustomWavetablePath = file.getFullPathName();
    customWavetableLoaded.store(true);
    oscillator.loadCustomWavetableFile(file);
}

void LiquidDreamAudioProcessor::loadFactoryWavetable(int index) {
    currentCustomWavetablePath = "";
    customWavetableLoaded.store(false);
    if (index >= 0 && index < EmbeddedWavetables::numTables) oscillator.loadWavetableFile(EmbeddedWavetables::allNames[index]);
    else oscillator.createDefaultBasicShapes();
}

void LiquidDreamAudioProcessor::setUserFolders(const juce::StringArray& folders) {
    userWavetableFolders = folders;
}

void LiquidDreamAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    oscillator.prepare(sampleRate);
    spectralMorph.prepare(sampleRate, samplesPerBlock);
    dualFilter.prepare(sampleRate, samplesPerBlock);
    shaper.prepare(sampleRate);
    voiceManager.setSampleRate(sampleRate);
    colorEngine.prepare(sampleRate, samplesPerBlock);

    ampEnv.setSampleRate(sampleRate);
    filterEnvA.setSampleRate(sampleRate); filterEnvB.setSampleRate(sampleRate);
    for (auto& env : modEnvs) env.setSampleRate(sampleRate);
    for (auto& lfo : lfos) lfo.setSampleRate(sampleRate);

    tempEnvBuffer.setSize(8, samplesPerBlock);
    tempSubBuffer.setSize(2, samplesPerBlock);
    tempWavetableBuffer.setSize(2, samplesPerBlock);

    double st = 0.02;
    smoothedWtLevel.reset(sampleRate, st); smoothedWtPitch.reset(sampleRate, st);
    smoothedPDecayAmt.reset(sampleRate, st); smoothedPDecayTime.reset(sampleRate, st);
    smoothedFltACutoff.reset(sampleRate, st); smoothedFltAReso.reset(sampleRate, st);
    smoothedFltBCutoff.reset(sampleRate, st); smoothedFltBReso.reset(sampleRate, st);
    smoothedFltMix.reset(sampleRate, st);
    smoothedDrive.reset(sampleRate, st); smoothedShpAmt.reset(sampleRate, st);
    smoothedShpRate.reset(sampleRate, st); smoothedShpBit.reset(sampleRate, st); smoothedGain.reset(sampleRate, st);
    smoothedWtPos.reset(sampleRate, st); smoothedFm.reset(sampleRate, st); smoothedDrift.reset(sampleRate, st);
    smoothedSubVol.reset(sampleRate, st); smoothedSubPitch.reset(sampleRate, st); smoothedWidth.reset(sampleRate, st);
    smoothedMorphAAmt.reset(sampleRate, st); smoothedMorphAShift.reset(sampleRate, st);
    smoothedMorphBAmt.reset(sampleRate, st); smoothedMorphBShift.reset(sampleRate, st);
    smoothedMorphCAmt.reset(sampleRate, st); smoothedMorphCShift.reset(sampleRate, st);

    smoothedWtLevel.setCurrentAndTargetValue(pOscLevel->load(std::memory_order_relaxed));
    smoothedWtPitch.setCurrentAndTargetValue(pOscPitch->load(std::memory_order_relaxed));
    smoothedPDecayAmt.setCurrentAndTargetValue(pPDecayAmt->load(std::memory_order_relaxed));
    smoothedPDecayTime.setCurrentAndTargetValue(pPDecayTime->load(std::memory_order_relaxed));
    smoothedDrift.setCurrentAndTargetValue(pDrift->load(std::memory_order_relaxed));

    lastOscFreq = -1.0f;
    lastModeA = -1; lastModeB = -1; lastModeC = -1;
}

void LiquidDreamAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals; buffer.clear(); if (buffer.getNumChannels() < 2) return;

    double currentBpm = 120.0;
    if (auto* playHead = getPlayHead()) {
        if (auto posInfo = playHead->getPosition()) {
            if (posInfo->getBpm().hasValue()) {
                currentBpm = *posInfo->getBpm();
            }
        }
    }

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
    smoothedWtPitch.setTargetValue(pOscPitch->load(std::memory_order_relaxed));
    smoothedPDecayAmt.setTargetValue(pPDecayAmt->load(std::memory_order_relaxed));
    smoothedPDecayTime.setTargetValue(pPDecayTime->load(std::memory_order_relaxed));
    smoothedDrift.setTargetValue(pDrift->load(std::memory_order_relaxed));
    smoothedFltACutoff.setTargetValue(pFltACutoff->load(std::memory_order_relaxed));
    smoothedFltAReso.setTargetValue(pFltAReso->load(std::memory_order_relaxed));
    smoothedFltBCutoff.setTargetValue(pFltBCutoff->load(std::memory_order_relaxed));
    smoothedFltBReso.setTargetValue(pFltBReso->load(std::memory_order_relaxed));
    smoothedFltMix.setTargetValue(pFltMix->load(std::memory_order_relaxed));
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
        if (lastWaveIdx == -2 && customWavetableLoaded.load()) {
            lastWaveIdx = waveIdx;
        }
        else {
            lastWaveIdx = waveIdx;
            customWavetableLoaded.store(false);
            currentCustomWavetablePath = "";
            if (waveIdx >= 0 && waveIdx < EmbeddedWavetables::numTables) oscillator.loadWavetableFile(EmbeddedWavetables::allNames[waveIdx]);
            else if (waveIdx == -1) oscillator.createDefaultBasicShapes();
        }
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
                ampEnv.noteOn(isLegato);
                filterEnvA.noteOn(isLegato); filterEnvB.noteOn(isLegato);
                for (auto& env : modEnvs) env.noteOn(isLegato);
                for (auto& lfo : lfos) lfo.noteOn(isLegato);
            }
        }
        else if (msg.isNoteOff()) {
            activeMidiNotes.erase(std::remove(activeMidiNotes.begin(), activeMidiNotes.end(), msg.getNoteNumber()), activeMidiNotes.end());
            if (voiceManager.noteOff(msg.getNoteNumber())) {
                ampEnv.noteOff();
                filterEnvA.noteOff(); filterEnvB.noteOff();
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
    filterEnvA.setParameters(pFAtkA->load(std::memory_order_relaxed), pFDecA->load(std::memory_order_relaxed), pFSusA->load(std::memory_order_relaxed), pFRelA->load(std::memory_order_relaxed));
    filterEnvB.setParameters(pFAtkB->load(std::memory_order_relaxed), pFDecB->load(std::memory_order_relaxed), pFSusB->load(std::memory_order_relaxed), pFRelB->load(std::memory_order_relaxed));

    for (int i = 0; i < 3; ++i) {
        modEnvs[i].setParameters(pModAtk[i]->load(), pModDec[i]->load(), pModSus[i]->load(), pModRel[i]->load());
        lfos[i].setParameters((int)pLfoWave[i]->load(), pLfoSync[i]->load() > 0.5f, pLfoRate[i]->load(), (int)pLfoBeat[i]->load(), pLfoAmt[i]->load(), (int)pLfoTrig[i]->load());
    }

    int numSamples = buffer.getNumSamples();
    if (tempEnvBuffer.getNumSamples() < numSamples) {
        tempEnvBuffer.setSize(8, numSamples, true, true, true);
        tempSubBuffer.setSize(2, numSamples, true, true, true);
        tempWavetableBuffer.setSize(2, numSamples, true, true, true);
    }

    auto* left = buffer.getWritePointer(0); auto* right = buffer.getWritePointer(1);
    auto* wtL = tempWavetableBuffer.getWritePointer(0); auto* wtR = tempWavetableBuffer.getWritePointer(1);
    float stft_aA = 0.0f, stft_sA = 0.0f, stft_aB = 0.0f, stft_sB = 0.0f, stft_aC = 0.0f, stft_sC = 0.0f;

    // --- STEP 1: Modulations & Envelopes (Dynamic Matrix) ---
    for (int i = 0; i < numSamples; ++i) {

        // ★ 変更: 全モジュレーションソースの現在値を配列に確保
        float sources[7] = {
            0.0f, // 0: None
            (pModOn[0]->load(std::memory_order_relaxed) > 0.5f ? modEnvs[0].getNextSample() : 0.0f),
            (pModOn[1]->load(std::memory_order_relaxed) > 0.5f ? modEnvs[1].getNextSample() : 0.0f),
            (pModOn[2]->load(std::memory_order_relaxed) > 0.5f ? modEnvs[2].getNextSample() : 0.0f),
            (pLfoOn[0]->load(std::memory_order_relaxed) > 0.5f ? lfos[0].getNextSample(currentBpm) : 0.0f),
            (pLfoOn[1]->load(std::memory_order_relaxed) > 0.5f ? lfos[1].getNextSample(currentBpm) : 0.0f),
            (pLfoOn[2]->load(std::memory_order_relaxed) > 0.5f ? lfos[2].getNextSample(currentBpm) : 0.0f)
        };

        float destMods[14] = { 0 };

        // ★ 変更: 8つの動的スロットをループ処理してターゲットに加算
        for (int slot = 0; slot < 8; ++slot) {
            int srcIdx = (int)pMatrixSrc[slot]->load(std::memory_order_relaxed);
            int destIdx = (int)pMatrixDest[slot]->load(std::memory_order_relaxed);
            float amt = pMatrixAmt[slot]->load(std::memory_order_relaxed);

            if (srcIdx > 0 && srcIdx < 7 && destIdx > 0 && destIdx < 14) {
                destMods[destIdx] += sources[srcIdx] * amt;
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
        oscillator.setWavetablePitchOffset(smoothedWtPitch.getNextValue());
        oscillator.setPitchDecay(smoothedPDecayAmt.getNextValue(), smoothedPDecayTime.getNextValue());
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
        float fValA = filterEnvA.getNextSample();
        float fValB = filterEnvB.getNextSample();

        if (ampEnv.popJustReset()) {
            oscillator.resetPhase();
        }

        tempEnvBuffer.setSample(0, i, aVal);
        tempEnvBuffer.setSample(1, i, fValA);
        tempEnvBuffer.setSample(2, i, fValB);
        tempEnvBuffer.setSample(3, i, destMods[9]);  // FltA Cutoff
        tempEnvBuffer.setSample(4, i, destMods[10]); // FltA Reso
        tempEnvBuffer.setSample(5, i, destMods[11]); // FltB Cutoff
        tempEnvBuffer.setSample(6, i, destMods[12]); // FltB Reso
        tempEnvBuffer.setSample(7, i, destMods[13]); // Gain

        float oL = 0.0f, oR = 0.0f, subL = 0.0f, subR = 0.0f;
        oscillator.getSampleStereo(oL, oR, subL, subR);
        wtL[i] = oL; wtR[i] = oR;
        tempSubBuffer.setSample(0, i, subL); tempSubBuffer.setSample(1, i, subR);
    }

    // --- STEP 3: Spectral Morphing ---
    spectralMorph.process(tempWavetableBuffer, currentModeA, stft_aA, stft_sA, currentModeB, stft_aB, stft_sB, currentModeC, stft_aC, stft_sC);

    // --- STEP 4: Distortion & Shaper ---
    for (int i = 0; i < numSamples; ++i) {
        float cd = smoothedDrive.getNextValue(), csa = smoothedShpAmt.getNextValue(), csr = smoothedShpRate.getNextValue(), csb = smoothedShpBit.getNextValue();
        float sL = wtL[i]; float sR = wtR[i];
        shaper.processStereo(sL, sR, cd, csa, csr, csb, sL, sR);
        wtL[i] = sL; wtR[i] = sR;
    }

    // --- STEP 5: Dual Filter Engine & Amp Envelope ---
    for (int i = 0; i < numSamples; ++i) {
        float ccA = smoothedFltACutoff.getNextValue(), crA = smoothedFltAReso.getNextValue();
        float ccB = smoothedFltBCutoff.getNextValue(), crB = smoothedFltBReso.getNextValue();

        float aVal = tempEnvBuffer.getSample(0, i);
        float fValA = tempEnvBuffer.getSample(1, i);
        float fValB = tempEnvBuffer.getSample(2, i);
        float cutModA = tempEnvBuffer.getSample(3, i);
        float resModA = tempEnvBuffer.getSample(4, i);
        float cutModB = tempEnvBuffer.getSample(5, i);
        float resModB = tempEnvBuffer.getSample(6, i);

        float sL = wtL[i] + tempSubBuffer.getSample(0, i);
        float sR = wtR[i] + tempSubBuffer.getSample(1, i);

        float mcA = ccA * std::exp2(cutModA * 8.0f); mcA += (fValA * pFltAEnvAmt->load() * 10000.0f);
        float mcB = ccB * std::exp2(cutModB * 8.0f); mcB += (fValB * pFltBEnvAmt->load() * 10000.0f);

        dualFilter.setFilterA((int)pFltAType->load(), mcA, crA + resModA);
        dualFilter.setFilterB((int)pFltBType->load(), mcB, crB + resModB);
        dualFilter.setRouting((int)pFltRouting->load(), smoothedFltMix.getNextValue());

        dualFilter.processStereo(sL, sR);

        left[i] = sL * aVal; right[i] = sR * aVal;
    }

    // --- STEP 6, 7, 8: ColorIR, Arp, Dynamics ---
    if (pColorOn->load() > 0.5f) {
        colorEngine.setParameters(pColorPreHp->load(), pColorPostHp->load(), pColorMix->load(), pColorIrVol->load());
        colorEngine.processIR(buffer);

        colorEngine.setArpParameters((int)pArpWave->load(), (int)pArpMode->load(), pArpSpeed->load(), (int)pArpPitch->load(), pArpLevel->load());
        std::vector<int> currentNotes;
        { std::lock_guard<std::mutex> lock(midiNotesMutex); currentNotes = activeMidiNotes; }
        colorEngine.processArp(buffer, currentNotes, tempEnvBuffer.getReadPointer(0));

        colorEngine.setOttParameters(pOttDepth->load(), pOttTime->load(), pOttUp->load(), pOttDown->load(), pOttGain->load());
        colorEngine.setSootheParameters(pOttDepth->load(), pOttTime->load(), pSootheSelectivity->load(), pSootheSharpness->load(), pSootheFocus->load());
        colorEngine.processDynamics(buffer);
    }

    // --- STEP 9: Master Gain & Scope ---
    for (int i = 0; i < numSamples; ++i) {
        float cg = smoothedGain.getNextValue();
        float gainMod = tempEnvBuffer.getSample(7, i);
        float finalGain = juce::jlimit(0.0f, 1.0f, cg + gainMod);

        left[i] *= finalGain;
        right[i] *= finalGain;

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