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

    pLimitOn = apvts.getRawParameterValue("limit_on");
    pLimitCeil = apvts.getRawParameterValue("limit_ceil");

    for (int i = 0; i < 3; ++i) {
        juce::String ms = "mod" + juce::String(i + 1) + "_";
        pModOn[i] = apvts.getRawParameterValue(ms + "on");
        pModAtk[i] = apvts.getRawParameterValue(ms + "atk"); pModDec[i] = apvts.getRawParameterValue(ms + "dec");
        pModSus[i] = apvts.getRawParameterValue(ms + "sus"); pModRel[i] = apvts.getRawParameterValue(ms + "rel");
        pModAmt[i] = apvts.getRawParameterValue(ms + "amt");
        pModBipolar[i] = apvts.getRawParameterValue(ms + "bipolar");

        juce::String ls = "lfo" + juce::String(i + 1) + "_";
        pLfoOn[i] = apvts.getRawParameterValue(ls + "on");
        pLfoWave[i] = apvts.getRawParameterValue(ls + "wave"); pLfoSync[i] = apvts.getRawParameterValue(ls + "sync");
        pLfoRate[i] = apvts.getRawParameterValue(ls + "rate"); pLfoBeat[i] = apvts.getRawParameterValue(ls + "beat");
        pLfoAmt[i] = apvts.getRawParameterValue(ls + "amt"); pLfoTrig[i] = apvts.getRawParameterValue(ls + "trig");
        pLfoUnipolar[i] = apvts.getRawParameterValue(ls + "unipolar");
    }

    for (int i = 0; i < 2; ++i) {
        juce::String msegId = "mseg" + juce::String(i + 1) + "_";
        pMsegOn[i] = apvts.getRawParameterValue(msegId + "on");
        pMsegSync[i] = apvts.getRawParameterValue(msegId + "sync");
        pMsegRate[i] = apvts.getRawParameterValue(msegId + "rate");
        pMsegBeat[i] = apvts.getRawParameterValue(msegId + "beat");
        pMsegAmt[i] = apvts.getRawParameterValue(msegId + "amt");
        pMsegTrig[i] = apvts.getRawParameterValue(msegId + "trig");
        pMsegUnipolar[i] = apvts.getRawParameterValue(msegId + "unipolar");
    }

    pMaxVoices = apvts.getRawParameterValue("max_voices");

    for (int i = 0; i < 10; ++i) {
        pMatrixSrc[i] = apvts.getRawParameterValue("matrix_src_" + juce::String(i));
        pMatrixDest[i] = apvts.getRawParameterValue("matrix_dest_" + juce::String(i));
        pMatrixAmt[i] = apvts.getRawParameterValue("matrix_amt_" + juce::String(i));
    }

    VoiceParams vp;
    vp.pOscOn = pOscOn; vp.pWave = pWave; vp.pPos = pPos; vp.pOscLevel = pOscLevel; vp.pOscPitch = pOscPitch;
    vp.pPDecayAmt = pPDecayAmt; vp.pPDecayTime = pPDecayTime; vp.pFm = pFm; vp.pFmWave = pFmWave;
    vp.pMorphAMode = pMorphAMode; vp.pMorphAAmt = pMorphAAmt; vp.pMorphAShift = pMorphAShift;
    vp.pMorphBMode = pMorphBMode; vp.pMorphBAmt = pMorphBAmt; vp.pMorphBShift = pMorphBShift;
    vp.pMorphCMode = pMorphCMode; vp.pMorphCAmt = pMorphCAmt; vp.pMorphCShift = pMorphCShift;
    vp.pUni = pUni; vp.pDetune = pDetune; vp.pWidth = pWidth; vp.pDrift = pDrift;
    vp.pSubOn = pSubOn; vp.pSubWave = pSubWave; vp.pSubVol = pSubVol; vp.pSubPitch = pSubPitch;
    vp.pFltAType = pFltAType; vp.pFltACutoff = pFltACutoff; vp.pFltAReso = pFltAReso;
    vp.pFltBType = pFltBType; vp.pFltBCutoff = pFltBCutoff; vp.pFltBReso = pFltBReso;
    vp.pFltRouting = pFltRouting; vp.pFltMix = pFltMix; vp.pFltAEnvAmt = pFltAEnvAmt; vp.pFltBEnvAmt = pFltBEnvAmt;
    vp.pAAtk = pAAtk; vp.pADec = pADec; vp.pASus = pASus; vp.pARel = pARel;
    vp.pFAtkA = pFAtkA; vp.pFDecA = pFDecA; vp.pFSusA = pFSusA; vp.pFRelA = pFRelA;
    vp.pFAtkB = pFAtkB; vp.pFDecB = pFDecB; vp.pFSusB = pFSusB; vp.pFRelB = pFRelB;
    vp.pDrive = pDrive; vp.pShpAmt = pShpAmt; vp.pShpRate = pShpRate; vp.pShpBit = pShpBit;
    vp.pColorMix = pColorMix; vp.pGlide = pGlide;
    
    for (int i = 0; i < 3; ++i) {
        vp.pModOn[i] = pModOn[i]; vp.pModAtk[i] = pModAtk[i]; vp.pModDec[i] = pModDec[i];
        vp.pModSus[i] = pModSus[i]; vp.pModRel[i] = pModRel[i]; vp.pModAmt[i] = pModAmt[i];
        vp.pModBipolar[i] = pModBipolar[i];
        vp.pLfoOn[i] = pLfoOn[i]; vp.pLfoWave[i] = pLfoWave[i]; vp.pLfoSync[i] = pLfoSync[i];
        vp.pLfoRate[i] = pLfoRate[i]; vp.pLfoBeat[i] = pLfoBeat[i]; vp.pLfoAmt[i] = pLfoAmt[i];
        vp.pLfoTrig[i] = pLfoTrig[i]; vp.pLfoUnipolar[i] = pLfoUnipolar[i];
    }
    for (int i = 0; i < 2; ++i) {
        vp.pMsegOn[i] = pMsegOn[i]; vp.pMsegSync[i] = pMsegSync[i]; vp.pMsegRate[i] = pMsegRate[i];
        vp.pMsegBeat[i] = pMsegBeat[i]; vp.pMsegAmt[i] = pMsegAmt[i]; vp.pMsegTrig[i] = pMsegTrig[i];
        vp.pMsegUnipolar[i] = pMsegUnipolar[i];
    }
    for (int i = 0; i < 10; ++i) {
        vp.pMatrixSrc[i] = pMatrixSrc[i]; vp.pMatrixDest[i] = pMatrixDest[i]; vp.pMatrixAmt[i] = pMatrixAmt[i];
    }

    voiceManager.init(vp);
    voiceManager.updateMsegStates(msegStates[0], msegStates[1]);

    // グローバル設定からのフォルダ復元
    juce::PropertiesFile::Options options;
    options.applicationName = "BassSynth";
    options.filenameSuffix = ".settings";
    options.osxLibrarySubFolder = "Application Support";
    options.folderName = "LiquidDreamAudio";
 
    juce::PropertiesFile propertiesFile(options);
    juce::String foldersStr = propertiesFile.getValue("UserFolders", "");
    if (foldersStr.isNotEmpty()) {
        userWavetableFolders.clear();
        userWavetableFolders.addTokens(foldersStr, "|", "");
    }
}

LiquidDreamAudioProcessor::~LiquidDreamAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout LiquidDreamAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterBool>("osc_on", "Osc On", true));
    params.push_back(std::make_unique<juce::AudioParameterInt>("osc_wave", "Waveform", -1, 9, 0));
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
    params.push_back(std::make_unique<juce::AudioParameterBool>("limit_on", "Limiter On", true));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("limit_ceil", "Ceiling", -24.0f, 0.0f, 0.0f));

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
    params.push_back(std::make_unique<juce::AudioParameterInt>("color_type", "IR Type", 0, 4, 0));
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
        params.push_back(std::make_unique<juce::AudioParameterBool>(pfx + "bipolar", nm + "Bipolar", false));
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
        params.push_back(std::make_unique<juce::AudioParameterBool>(pfx + "unipolar", nm + "Unipolar", false));
    }

    for (int i = 1; i <= 2; ++i) {
        juce::String pfx = "mseg" + juce::String(i) + "_";
        juce::String nm = "MSEG" + juce::String(i) + " ";
        params.push_back(std::make_unique<juce::AudioParameterBool>(pfx + "on", nm + "On", true));
        params.push_back(std::make_unique<juce::AudioParameterBool>(pfx + "sync", nm + "Sync", false));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(pfx + "rate", nm + "Rate", lfoHzRange, 1.0f));
        params.push_back(std::make_unique<juce::AudioParameterInt>(pfx + "beat", nm + "Beat", 0, 8, 2));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(pfx + "amt", nm + "Amt", 0.0f, 1.0f, 1.0f));
        params.push_back(std::make_unique<juce::AudioParameterInt>(pfx + "trig", nm + "Trig", 0, 2, 0));
        params.push_back(std::make_unique<juce::AudioParameterBool>(pfx + "unipolar", nm + "Unipolar", false));
    }

    for (int i = 0; i < 10; ++i) {
        juce::String sIdx = juce::String(i);
        params.push_back(std::make_unique<juce::AudioParameterInt>("matrix_src_" + sIdx, "Src", 0, 8, 0));
        params.push_back(std::make_unique<juce::AudioParameterInt>("matrix_dest_" + sIdx, "Dest", 0, 22, 0));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("matrix_amt_" + sIdx, "Amt", -1.0f, 1.0f, 0.0f));
    }

    params.push_back(std::make_unique<juce::AudioParameterInt>("max_voices", "Max Voices", 1, 24, 12));

    return { params.begin(), params.end() };
}

void LiquidDreamAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    if (xml != nullptr) {
        xml->setAttribute("CustomWavePath", currentCustomWavetablePath);
        xml->setAttribute("ColorChord", serializeChord(colorEngine.getLearnedNotes()));
        xml->setAttribute("mseg0_data", serializeMsegState(msegStates[0]));
        xml->setAttribute("mseg1_data", serializeMsegState(msegStates[1]));

        // ★ 変更：プリセットの「名前」を保存する
        xml->setAttribute("SelectedPresetName", lastSelectedPresetName);

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

        juce::String customPath = xmlState->getStringAttribute("CustomWavePath", "");
        if (customPath.isNotEmpty() && juce::File(customPath).existsAsFile()) {
            loadCustomWavetable(juce::File(customPath));
        }

        juce::String ms0 = xmlState->getStringAttribute("mseg0_data", "");
        if (ms0.isNotEmpty()) {
            msegStates[0] = deserializeMsegState(ms0);
        }
        juce::String ms1 = xmlState->getStringAttribute("mseg1_data", "");
        if (ms1.isNotEmpty()) {
            msegStates[1] = deserializeMsegState(ms1);
        }
        voiceManager.updateMsegStates(msegStates[0], msegStates[1]);

        juce::String chordStr = xmlState->getStringAttribute("ColorChord", "");
        if (chordStr.isNotEmpty()) {
            colorEngine.setLearnedNotes(deserializeChord(chordStr));
            colorEngine.setLearnState(ColorIREngine::LearnState::Active);

            float atk = pColorAtk->load(std::memory_order_relaxed);
            float dec = pColorDec->load(std::memory_order_relaxed);
            int type = (int)pColorType->load(std::memory_order_relaxed);

            colorEngine.finishLearningAndGenerate(atk, dec, type);
        }
        else {
            colorEngine.setLearnedNotes({});
            colorEngine.setLearnState(ColorIREngine::LearnState::Idle);
        }

        // ★ 変更：プリセットの「名前」を復元する
        lastSelectedPresetName = xmlState->getStringAttribute("SelectedPresetName", "Init");

        presetLoadedFlag.store(true);
    }
}

void LiquidDreamAudioProcessor::loadCustomWavetable(const juce::File& file) {
    currentCustomWavetablePath = file.getFullPathName();
    customWavetableLoaded.store(true);
    voiceManager.loadCustomWavetable(file);
    forceScopeUpdate.store(true);
}

void LiquidDreamAudioProcessor::loadFactoryWavetable(int index) {
    currentCustomWavetablePath = "";
    customWavetableLoaded.store(false);
    if (index >= 0 && index < 10) voiceManager.loadFactoryWavetable(index);
    else voiceManager.loadFactoryWavetable(0);
    forceScopeUpdate.store(true);
}

void LiquidDreamAudioProcessor::setUserFolders(const juce::StringArray& folders) {
    userWavetableFolders = folders;

    juce::PropertiesFile::Options options;
    options.applicationName = "BassSynth";
    options.filenameSuffix = ".settings";
    options.osxLibrarySubFolder = "Application Support";
    options.folderName = "LiquidDreamAudio";

    juce::PropertiesFile propertiesFile(options);
    propertiesFile.setValue("UserFolders", folders.joinIntoString("|"));
    propertiesFile.saveIfNeeded();
}

void LiquidDreamAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    voiceManager.prepare(sampleRate, samplesPerBlock);
    displaySpectralMorph.prepare(sampleRate, samplesPerBlock);
    colorEngine.prepare(sampleRate, samplesPerBlock);
    masterLimiter.prepare(sampleRate);

    double st = 0.02;
    smoothedGain.reset(sampleRate, st);
    smoothedSubPitch.reset(sampleRate, st);
    smoothedWidth.reset(sampleRate, st);

    lastOscFreq = -1.0f;
    lastModeA = -1; lastModeB = -1; lastModeC = -1;
    lastWaveIdxProcessor = -2;
}

void LiquidDreamAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals; buffer.clear(); if (buffer.getNumChannels() < 2) return;

    double currentBpm = 120.0;
    if (auto* playHead = getPlayHead()) {
        if (auto posInfo = playHead->getPosition()) {
            if (posInfo->getBpm().hasValue()) currentBpm = *posInfo->getBpm();
        }
    }



    smoothedGain.setTargetValue(pGain->load(std::memory_order_relaxed));

    // 最大発音数とMSEG状態の更新
    voiceManager.setMaxVoices((int)pMaxVoices->load(std::memory_order_relaxed));
    voiceManager.updateMsegStates(msegStates[0], msegStates[1]);

    int waveIdx = (int)pWave->load(std::memory_order_relaxed);
    if (waveIdx != lastWaveIdxProcessor) {
        if (lastWaveIdxProcessor == -2 && customWavetableLoaded.load()) {
            lastWaveIdxProcessor = waveIdx;
        }
        else {
            lastWaveIdxProcessor = waveIdx;
            customWavetableLoaded.store(false);
            currentCustomWavetablePath = "";
            if (waveIdx >= 0 && waveIdx < 10) voiceManager.loadFactoryWavetable(waveIdx);
            else if (waveIdx == -1) voiceManager.loadFactoryWavetable(0);
        }
    }

    for (const auto metadata : midiMessages) {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn()) {
            if (colorEngine.getLearnState() == ColorIREngine::LearnState::Learning) colorEngine.addNote(msg.getNoteNumber());
            {
                std::lock_guard<std::mutex> lock(midiNotesMutex);
                if (std::find(activeMidiNotes.begin(), activeMidiNotes.end(), msg.getNoteNumber()) == activeMidiNotes.end()) {
                    activeMidiNotes.push_back(msg.getNoteNumber());
                }
            }
            voiceManager.noteOn(msg.getNoteNumber(), msg.getVelocity() / 127.0f);
        }
        else if (msg.isNoteOff()) {
            {
                std::lock_guard<std::mutex> lock(midiNotesMutex);
                activeMidiNotes.erase(std::remove(activeMidiNotes.begin(), activeMidiNotes.end(), msg.getNoteNumber()), activeMidiNotes.end());
            }
            voiceManager.noteOff(msg.getNoteNumber());
        }
    }

    // 全ボイスのレンダリングと加算ミックス
    voiceManager.renderNextBlock(buffer, currentBpm);

    int numSamples = buffer.getNumSamples();
    auto* left = buffer.getWritePointer(0); auto* right = buffer.getWritePointer(1);

    // グローバルエフェクト
    if (pColorOn->load() > 0.5f) {
        colorEngine.setParameters(pColorPreHp->load(), pColorPostHp->load(), pColorMix->load(), pColorIrVol->load());
        colorEngine.processIR(buffer);

        colorEngine.setArpParameters((int)pArpWave->load(), (int)pArpMode->load(), pArpSpeed->load(), (int)pArpPitch->load(), pArpLevel->load());
        std::vector<int> currentNotes;
        { std::lock_guard<std::mutex> lock(midiNotesMutex); currentNotes = activeMidiNotes; }
        
        std::vector<float> dummyEnv(numSamples, 1.0f);
        colorEngine.processArp(buffer, currentNotes, dummyEnv.data());

        colorEngine.setOttParameters(pOttDepth->load(), pOttTime->load(), pOttUp->load(), pOttDown->load(), pOttGain->load());
        colorEngine.setSootheParameters(pOttDepth->load(), pOttTime->load(), pSootheSelectivity->load(), pSootheSharpness->load(), pSootheFocus->load());
        colorEngine.processDynamics(buffer);
    }

    for (int i = 0; i < numSamples; ++i) {
        float cg = smoothedGain.getNextValue();
        left[i] *= cg; right[i] *= cg;
    }

    if (pLimitOn->load() > 0.5f) {
        masterLimiter.setCeiling(pLimitCeil->load());
        masterLimiter.process(buffer);
    }

    for (int i = 0; i < numSamples; ++i) {
        tempScopeBuffer[(size_t)scopeWriteIndex] = (left[i] + right[i]) * 0.5f;
        scopeWriteIndex++;
        if (scopeWriteIndex >= 512) { outputScopeData = tempScopeBuffer; scopeWriteIndex = 0; }
    }
}

juce::AudioProcessorEditor* LiquidDreamAudioProcessor::createEditor() { return new LiquidDreamAudioProcessorEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new LiquidDreamAudioProcessor(); }