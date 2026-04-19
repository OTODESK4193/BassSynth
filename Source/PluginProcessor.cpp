// ==============================================================================
// Source/PluginProcessor.cpp
// ==============================================================================
#include "PluginProcessor.h"
#include "PluginEditor.h"

LiquidDreamAudioProcessor::LiquidDreamAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    apvts(*this, nullptr, "PARAMS", createParameterLayout())
{
    outputScopeData.fill(0.0f);

    pOscOn = apvts.getRawParameterValue("osc_on");
    pWave = apvts.getRawParameterValue("osc_wave");
    pPos = apvts.getRawParameterValue("osc_pos");
    pOscLevel = apvts.getRawParameterValue("osc_level");
    pOscPitch = apvts.getRawParameterValue("osc_pitch");
    pPDecayAmt = apvts.getRawParameterValue("osc_pdecay_amt");
    pPDecayTime = apvts.getRawParameterValue("osc_pdecay_time");
    pFm = apvts.getRawParameterValue("osc_fm");
    pFmWave = apvts.getRawParameterValue("osc_fm_wave");

    pMorphAMode = apvts.getRawParameterValue("osc_morph_a_mode");
    pMorphAAmt = apvts.getRawParameterValue("osc_morph_a_amt");
    pMorphAShift = apvts.getRawParameterValue("osc_morph_a_shift");

    pMorphBMode = apvts.getRawParameterValue("osc_morph_b_mode");
    pMorphBAmt = apvts.getRawParameterValue("osc_morph_b_amt");
    pMorphBShift = apvts.getRawParameterValue("osc_morph_b_shift");

    pMorphCMode = apvts.getRawParameterValue("osc_morph_c_mode");
    pMorphCAmt = apvts.getRawParameterValue("osc_morph_c_amt");
    pMorphCShift = apvts.getRawParameterValue("osc_morph_c_shift");

    pUni = apvts.getRawParameterValue("osc_uni");
    pDetune = apvts.getRawParameterValue("osc_detune");
    pWidth = apvts.getRawParameterValue("osc_width");
    pDrift = apvts.getRawParameterValue("osc_drift");

    pSubOn = apvts.getRawParameterValue("sub_on");
    pSubWave = apvts.getRawParameterValue("sub_wave");
    pSubVol = apvts.getRawParameterValue("sub_vol");
    pSubPitch = apvts.getRawParameterValue("sub_pitch");

    pCutoff = apvts.getRawParameterValue("flt_cutoff");
    pReso = apvts.getRawParameterValue("flt_res");
    pFltEnvAmt = apvts.getRawParameterValue("flt_env_amt");
    pDrive = apvts.getRawParameterValue("dist_drive");
    pShpAmt = apvts.getRawParameterValue("shp_amt");
    pShpRate = apvts.getRawParameterValue("shp_rate");
    pShpBit = apvts.getRawParameterValue("shp_bit");
    pGain = apvts.getRawParameterValue("m_gain");
    pGlide = apvts.getRawParameterValue("m_glide");

    pAAtk = apvts.getRawParameterValue("a_atk"); pADec = apvts.getRawParameterValue("a_dec"); pASus = apvts.getRawParameterValue("a_sus"); pARel = apvts.getRawParameterValue("a_rel");
    pFAtk = apvts.getRawParameterValue("f_atk"); pFDec = apvts.getRawParameterValue("f_dec"); pFSus = apvts.getRawParameterValue("f_sus"); pFRel = apvts.getRawParameterValue("f_rel");
}

LiquidDreamAudioProcessor::~LiquidDreamAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout LiquidDreamAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterBool>("osc_on", "Osc On", true));
    params.push_back(std::make_unique<juce::AudioParameterInt>("osc_wave", "Waveform", 0, EmbeddedWavetables::numTables - 1, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc_level", "WT Level", 0.0f, 1.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc_pos", "Position", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc_pitch", "WT Pitch", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc_pdecay_amt", "P.Decay", -24.0f, 24.0f, 0.0f));
    auto timeRange = juce::NormalisableRange<float>(1.0f, 2000.0f, 1.0f, 0.3f);
    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc_pdecay_time", "P.Time", timeRange, 100.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc_fm", "FM Amt", 0.0f, 3.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterInt>("osc_fm_wave", "FM Wave", 0, 3, 0));

    // 3 Stage Morph System
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
    params.push_back(std::make_unique<juce::AudioParameterFloat>("flt_res", "Reso", 0.0f, 0.95f, 0.1f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("flt_env_amt", "Env Amt", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("m_gain", "Gain", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("m_glide", "Glide", 0.0f, 1.0f, 0.1f));
    params.push_back(std::make_unique<juce::AudioParameterInt>("m_pb", "PB Range", 0, 24, 12));

    auto attRange = juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f);
    params.push_back(std::make_unique<juce::AudioParameterFloat>("a_atk", "Amp Atk", attRange, 0.01f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("a_dec", "Amp Dec", attRange, 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("a_sus", "Amp Sus", 0.0f, 1.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("a_rel", "Amp Rel", attRange, 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("f_atk", "Mod Atk", attRange, 0.01f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("f_dec", "Mod Dec", attRange, 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("f_sus", "Mod Sus", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("f_rel", "Mod Rel", attRange, 0.3f));

    return { params.begin(), params.end() };
}

void LiquidDreamAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    oscillator.prepare(sampleRate);
    spectralMorph.prepare(sampleRate, samplesPerBlock);
    filter.prepare(sampleRate);
    shaper.prepare(sampleRate);
    voiceManager.setSampleRate(sampleRate); ampEnv.setSampleRate(sampleRate); filterEnv.setSampleRate(sampleRate);

    double st = 0.02; // デフォルトのスムージングタイム (20ms)
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
    smoothedWtPitch.setCurrentAndTargetValue(pOscPitch->load(std::memory_order_relaxed));
    smoothedPDecayAmt.setCurrentAndTargetValue(pPDecayAmt->load(std::memory_order_relaxed));
    smoothedPDecayTime.setCurrentAndTargetValue(pPDecayTime->load(std::memory_order_relaxed));
    smoothedCutoff.setCurrentAndTargetValue(pCutoff->load(std::memory_order_relaxed));
    smoothedReso.setCurrentAndTargetValue(pReso->load(std::memory_order_relaxed));
    smoothedFltEnvAmt.setCurrentAndTargetValue(pFltEnvAmt->load(std::memory_order_relaxed));
    smoothedDrive.setCurrentAndTargetValue(pDrive->load(std::memory_order_relaxed));
    smoothedShpAmt.setCurrentAndTargetValue(pShpAmt->load(std::memory_order_relaxed));
    smoothedShpRate.setCurrentAndTargetValue(pShpRate->load(std::memory_order_relaxed));
    smoothedShpBit.setCurrentAndTargetValue(pShpBit->load(std::memory_order_relaxed));
    smoothedGain.setCurrentAndTargetValue(pGain->load(std::memory_order_relaxed));
    smoothedWtPos.setCurrentAndTargetValue(pPos->load(std::memory_order_relaxed));
    smoothedFm.setCurrentAndTargetValue(pFm->load(std::memory_order_relaxed));
    smoothedDrift.setCurrentAndTargetValue(pDrift->load(std::memory_order_relaxed));
    smoothedSubVol.setCurrentAndTargetValue(pSubVol->load(std::memory_order_relaxed));
    smoothedSubPitch.setCurrentAndTargetValue(pSubPitch->load(std::memory_order_relaxed));
    smoothedWidth.setCurrentAndTargetValue(pWidth->load(std::memory_order_relaxed));

    smoothedMorphAAmt.setCurrentAndTargetValue(pMorphAAmt->load(std::memory_order_relaxed));
    smoothedMorphAShift.setCurrentAndTargetValue(pMorphAShift->load(std::memory_order_relaxed));
    smoothedMorphBAmt.setCurrentAndTargetValue(pMorphBAmt->load(std::memory_order_relaxed));
    smoothedMorphBShift.setCurrentAndTargetValue(pMorphBShift->load(std::memory_order_relaxed));
    smoothedMorphCAmt.setCurrentAndTargetValue(pMorphCAmt->load(std::memory_order_relaxed));
    smoothedMorphCShift.setCurrentAndTargetValue(pMorphCShift->load(std::memory_order_relaxed));

    lastOscFreq = -1.0f;
    lastModeA = -1;
    lastModeB = -1;
    lastModeC = -1;
}

void LiquidDreamAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals; buffer.clear(); if (buffer.getNumChannels() < 2) return;

    // --- インテリジェント・スムージング (フレーム連動防護) ---
    auto updateSmoothTime = [this](std::atomic<float>* pMode, int& lastMode, juce::SmoothedValue<float>& sAmt, juce::SmoothedValue<float>& sShift) {
        int currentMode = (int)pMode->load(std::memory_order_relaxed);
        if (currentMode != lastMode) {
            // Spectral Mode (8以上) ならば、STFTホップレートより長い 25ms に強制延長して破綻を防ぐ
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
    // ----------------------------------------------------

    smoothedWtLevel.setTargetValue(pOscLevel->load(std::memory_order_relaxed));
    smoothedWtPitch.setTargetValue(pOscPitch->load(std::memory_order_relaxed));
    smoothedPDecayAmt.setTargetValue(pPDecayAmt->load(std::memory_order_relaxed));
    smoothedPDecayTime.setTargetValue(pPDecayTime->load(std::memory_order_relaxed));
    smoothedCutoff.setTargetValue(pCutoff->load(std::memory_order_relaxed)); smoothedReso.setTargetValue(pReso->load(std::memory_order_relaxed));
    smoothedFltEnvAmt.setTargetValue(pFltEnvAmt->load(std::memory_order_relaxed)); smoothedDrive.setTargetValue(pDrive->load(std::memory_order_relaxed));
    smoothedShpAmt.setTargetValue(pShpAmt->load(std::memory_order_relaxed)); smoothedShpRate.setTargetValue(pShpRate->load(std::memory_order_relaxed));
    smoothedShpBit.setTargetValue(pShpBit->load(std::memory_order_relaxed)); smoothedGain.setTargetValue(pGain->load(std::memory_order_relaxed));
    smoothedWtPos.setTargetValue(pPos->load(std::memory_order_relaxed)); smoothedFm.setTargetValue(pFm->load(std::memory_order_relaxed));
    smoothedDrift.setTargetValue(pDrift->load(std::memory_order_relaxed)); smoothedSubVol.setTargetValue(pSubVol->load(std::memory_order_relaxed));
    smoothedSubPitch.setTargetValue(pSubPitch->load(std::memory_order_relaxed)); smoothedWidth.setTargetValue(pWidth->load(std::memory_order_relaxed));

    smoothedMorphAAmt.setTargetValue(pMorphAAmt->load(std::memory_order_relaxed));
    smoothedMorphAShift.setTargetValue(pMorphAShift->load(std::memory_order_relaxed));
    smoothedMorphBAmt.setTargetValue(pMorphBAmt->load(std::memory_order_relaxed));
    smoothedMorphBShift.setTargetValue(pMorphBShift->load(std::memory_order_relaxed));
    smoothedMorphCAmt.setTargetValue(pMorphCAmt->load(std::memory_order_relaxed));
    smoothedMorphCShift.setTargetValue(pMorphCShift->load(std::memory_order_relaxed));

    int waveIdx = (int)pWave->load(std::memory_order_relaxed); static int lastWaveIdx = -1;
    if (waveIdx != lastWaveIdx && waveIdx >= 0 && waveIdx < EmbeddedWavetables::numTables) {
        oscillator.loadWavetableFile(EmbeddedWavetables::allNames[waveIdx]); lastWaveIdx = waveIdx;
    }

    for (const auto metadata : midiMessages) {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn()) {
            if (voiceManager.noteOn(msg.getNoteNumber(), msg.getVelocity())) {
                oscillator.resetPhase(); ampEnv.noteOn(); filterEnv.noteOn();
            }
        }
        else if (msg.isNoteOff()) {
            if (voiceManager.noteOff(msg.getNoteNumber())) {
                ampEnv.noteOff(); filterEnv.noteOff();
            }
        }
    }

    oscillator.setOscOn(pOscOn->load(std::memory_order_relaxed) > 0.5f);
    oscillator.setSubOn(pSubOn->load(std::memory_order_relaxed) > 0.5f);
    oscillator.setSubWaveform((int)pSubWave->load(std::memory_order_relaxed));
    oscillator.setUnisonCount((int)pUni->load(std::memory_order_relaxed));
    oscillator.setUnisonDetune(pDetune->load(std::memory_order_relaxed));
    oscillator.setStereoWidth(smoothedWidth.getNextValue());

    voiceManager.setGlideTime(pGlide->load(std::memory_order_relaxed));
    ampEnv.setParameters(pAAtk->load(std::memory_order_relaxed), pADec->load(std::memory_order_relaxed), pASus->load(std::memory_order_relaxed), pARel->load(std::memory_order_relaxed));
    filterEnv.setParameters(pFAtk->load(std::memory_order_relaxed), pFDec->load(std::memory_order_relaxed), pFSus->load(std::memory_order_relaxed), pFRel->load(std::memory_order_relaxed));

    oscillator.setFMWaveform((int)pFmWave->load(std::memory_order_relaxed));

    auto* left = buffer.getWritePointer(0); auto* right = buffer.getWritePointer(1);

    // 1. オシレーターの生成 (時間領域Morph適用済み)
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        oscillator.setWavetablePosition(smoothedWtPos.getNextValue());
        oscillator.setFMAmount(smoothedFm.getNextValue());
        oscillator.setDriftAmount(smoothedDrift.getNextValue());

        oscillator.setMorphA(currentModeA, smoothedMorphAAmt.getNextValue(), smoothedMorphAShift.getNextValue());
        oscillator.setMorphB(currentModeB, smoothedMorphBAmt.getNextValue(), smoothedMorphBShift.getNextValue());
        oscillator.setMorphC(currentModeC, smoothedMorphCAmt.getNextValue(), smoothedMorphCShift.getNextValue());

        oscillator.setWavetableLevel(smoothedWtLevel.getNextValue());
        oscillator.setWavetablePitchOffset(smoothedWtPitch.getNextValue());
        oscillator.setPitchDecay(smoothedPDecayAmt.getNextValue(), smoothedPDecayTime.getNextValue());

        oscillator.setSubVolume(smoothedSubVol.getNextValue());
        oscillator.setSubPitchOffset(smoothedSubPitch.getNextValue());

        float cf = voiceManager.getCurrentFrequency();
        if (cf < 1.0f) cf = 1.0f;
        if (std::abs(cf - lastOscFreq) > 0.01f) { oscillator.setFrequency(cf); lastOscFreq = cf; }

        float oL = 0.0f, oR = 0.0f; oscillator.getSampleStereo(oL, oR);
        left[i] = oL; right[i] = oR;
    }

    // 2. スペクトルモーフィングの適用 (STFTパイプライン)
    // 【重要】生のアトミック値ではなく、スムージングされた現在の安全な値を渡す
    float aA = smoothedMorphAAmt.getCurrentValue();
    float sA = smoothedMorphAShift.getCurrentValue();
    float aB = smoothedMorphBAmt.getCurrentValue();
    float sB = smoothedMorphBShift.getCurrentValue();
    float aC = smoothedMorphCAmt.getCurrentValue();
    float sC = smoothedMorphCShift.getCurrentValue();

    spectralMorph.process(buffer, currentModeA, aA, sA, currentModeB, aB, sB, currentModeC, aC, sC);

    // 3. 後段DSP (Filter, Shaper, AmpEnv)
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        float cc = smoothedCutoff.getNextValue(), cr = smoothedReso.getNextValue(), ce = smoothedFltEnvAmt.getNextValue();
        float cd = smoothedDrive.getNextValue(), csa = smoothedShpAmt.getNextValue(), csr = smoothedShpRate.getNextValue(), csb = smoothedShpBit.getNextValue(), cg = smoothedGain.getNextValue();
        float aVal = ampEnv.getNextSample(), fVal = filterEnv.getNextSample();

        float sL = left[i], sR = right[i], mc = cc + (fVal * ce * 10000.0f);

        filter.setParameters(juce::jlimit(20.0f, 20000.0f, mc), cr);
        sL = filter.processSample(sL); sR = filter.processSample(sR);

        shaper.processStereo(sL, sR, cd, csa, csr, csb, sL, sR);

        float fg = cg * aVal;
        left[i] = sL * fg; right[i] = sR * fg;

        outputScopeData[scopeWriteIndex] = (left[i] + right[i]) * 0.5f;
        scopeWriteIndex = (scopeWriteIndex + 1) % 512;
    }
}

juce::AudioProcessorEditor* LiquidDreamAudioProcessor::createEditor() { return new LiquidDreamAudioProcessorEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new LiquidDreamAudioProcessor(); }