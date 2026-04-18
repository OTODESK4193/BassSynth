#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
LiquidDreamAudioProcessor::LiquidDreamAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    apvts(*this, nullptr, "PARAMS", createParameterLayout())
{
    outputScopeData.fill(0.0f);

    pWave = apvts.getRawParameterValue("osc_wave");
    pPos = apvts.getRawParameterValue("osc_pos");
    pOscPitch = apvts.getRawParameterValue("osc_pitch");
    pFm = apvts.getRawParameterValue("osc_fm");
    pSync = apvts.getRawParameterValue("osc_sync");
    pMorph = apvts.getRawParameterValue("osc_morph");
    pUni = apvts.getRawParameterValue("osc_uni");
    pDetune = apvts.getRawParameterValue("osc_detune");
    pWidth = apvts.getRawParameterValue("osc_width"); // ’Ç‰Á
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
    params.push_back(std::make_unique<juce::AudioParameterInt>("osc_wave", "Waveform", 0, EmbeddedWavetables::numTables - 1, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc_pos", "Position", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc_pitch", "Pitch", -12.0f, 12.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc_fm", "FM Amt", 0.0f, 3.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc_sync", "Sync", 1.0f, 4.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc_morph", "Warp", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterInt>("osc_uni", "Unison", 1, 12, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc_detune", "Detune", 0.0f, 1.0f, 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc_width", "Width", 0.0f, 1.0f, 1.0f)); // ’Ç‰Á
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
    oscillator.prepare(sampleRate); filter.prepare(sampleRate); shaper.prepare(sampleRate);
    voiceManager.setSampleRate(sampleRate); ampEnv.setSampleRate(sampleRate); filterEnv.setSampleRate(sampleRate);

    double st = 0.02;
    smoothedPitchMult.reset(sampleRate, st); smoothedCutoff.reset(sampleRate, st); smoothedReso.reset(sampleRate, st);
    smoothedFltEnvAmt.reset(sampleRate, st); smoothedDrive.reset(sampleRate, st); smoothedShpAmt.reset(sampleRate, st);
    smoothedShpRate.reset(sampleRate, st); smoothedShpBit.reset(sampleRate, st); smoothedGain.reset(sampleRate, st);
    smoothedWtPos.reset(sampleRate, st); smoothedFm.reset(sampleRate, st); smoothedSync.reset(sampleRate, st);
    smoothedMorph.reset(sampleRate, st); smoothedDrift.reset(sampleRate, st); smoothedSubVol.reset(sampleRate, st);
    smoothedSubPitch.reset(sampleRate, st);
    smoothedWidth.reset(sampleRate, st); // ’Ç‰Á

    smoothedPitchMult.setCurrentAndTargetValue(std::pow(2.0f, pOscPitch->load(std::memory_order_relaxed) / 12.0f));
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
    smoothedSync.setCurrentAndTargetValue(pSync->load(std::memory_order_relaxed));
    smoothedMorph.setCurrentAndTargetValue(pMorph->load(std::memory_order_relaxed));
    smoothedDrift.setCurrentAndTargetValue(pDrift->load(std::memory_order_relaxed));
    smoothedSubVol.setCurrentAndTargetValue(pSubVol->load(std::memory_order_relaxed));
    smoothedSubPitch.setCurrentAndTargetValue(pSubPitch->load(std::memory_order_relaxed));
    smoothedWidth.setCurrentAndTargetValue(pWidth->load(std::memory_order_relaxed)); // ’Ç‰Á
    lastOscFreq = -1.0f;
}

void LiquidDreamAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals; buffer.clear(); if (buffer.getNumChannels() < 2) return;

    smoothedPitchMult.setTargetValue(std::pow(2.0f, pOscPitch->load(std::memory_order_relaxed) / 12.0f));
    smoothedCutoff.setTargetValue(pCutoff->load(std::memory_order_relaxed)); smoothedReso.setTargetValue(pReso->load(std::memory_order_relaxed));
    smoothedFltEnvAmt.setTargetValue(pFltEnvAmt->load(std::memory_order_relaxed)); smoothedDrive.setTargetValue(pDrive->load(std::memory_order_relaxed));
    smoothedShpAmt.setTargetValue(pShpAmt->load(std::memory_order_relaxed)); smoothedShpRate.setTargetValue(pShpRate->load(std::memory_order_relaxed));
    smoothedShpBit.setTargetValue(pShpBit->load(std::memory_order_relaxed)); smoothedGain.setTargetValue(pGain->load(std::memory_order_relaxed));
    smoothedWtPos.setTargetValue(pPos->load(std::memory_order_relaxed)); smoothedFm.setTargetValue(pFm->load(std::memory_order_relaxed));
    smoothedSync.setTargetValue(pSync->load(std::memory_order_relaxed)); smoothedMorph.setTargetValue(pMorph->load(std::memory_order_relaxed));
    smoothedDrift.setTargetValue(pDrift->load(std::memory_order_relaxed)); smoothedSubVol.setTargetValue(pSubVol->load(std::memory_order_relaxed));
    smoothedSubPitch.setTargetValue(pSubPitch->load(std::memory_order_relaxed));
    smoothedWidth.setTargetValue(pWidth->load(std::memory_order_relaxed)); // ’Ç‰Á

    int waveIdx = (int)pWave->load(std::memory_order_relaxed); static int lastWaveIdx = -1;
    if (waveIdx != lastWaveIdx && waveIdx >= 0 && waveIdx < EmbeddedWavetables::numTables) {
        oscillator.loadWavetableFile(EmbeddedWavetables::allNames[waveIdx]); lastWaveIdx = waveIdx;
    }
    for (const auto metadata : midiMessages) {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn()) { if (voiceManager.noteOn(msg.getNoteNumber(), msg.getVelocity())) { oscillator.resetPhase(); ampEnv.noteOn(); filterEnv.noteOn(); } }
        else if (msg.isNoteOff()) { if (voiceManager.noteOff(msg.getNoteNumber())) { ampEnv.noteOff(); filterEnv.noteOff(); } }
    }

    oscillator.setSubOn(pSubOn->load(std::memory_order_relaxed) > 0.5f);
    oscillator.setSubWaveform((int)pSubWave->load(std::memory_order_relaxed));
    oscillator.setUnisonCount((int)pUni->load(std::memory_order_relaxed));
    oscillator.setUnisonDetune(pDetune->load(std::memory_order_relaxed));
    oscillator.setStereoWidth(pWidth->load(std::memory_order_relaxed)); // ’Ç‰Á

    voiceManager.setGlideTime(pGlide->load(std::memory_order_relaxed));
    ampEnv.setParameters(pAAtk->load(std::memory_order_relaxed), pADec->load(std::memory_order_relaxed), pASus->load(std::memory_order_relaxed), pARel->load(std::memory_order_relaxed));
    filterEnv.setParameters(pFAtk->load(std::memory_order_relaxed), pFDec->load(std::memory_order_relaxed), pFSus->load(std::memory_order_relaxed), pFRel->load(std::memory_order_relaxed));

    auto* left = buffer.getWritePointer(0); auto* right = buffer.getWritePointer(1);
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        oscillator.setWavetablePosition(smoothedWtPos.getNextValue()); oscillator.setFMAmount(smoothedFm.getNextValue());
        oscillator.setSyncAmount(smoothedSync.getNextValue()); oscillator.setMorph(smoothedMorph.getNextValue());
        oscillator.setDriftAmount(smoothedDrift.getNextValue()); oscillator.setSubVolume(smoothedSubVol.getNextValue());
        oscillator.setSubPitchOffset(smoothedSubPitch.getNextValue());

        float cp = smoothedPitchMult.getNextValue(), cc = smoothedCutoff.getNextValue(), cr = smoothedReso.getNextValue(), ce = smoothedFltEnvAmt.getNextValue();
        float cd = smoothedDrive.getNextValue(), csa = smoothedShpAmt.getNextValue(), csr = smoothedShpRate.getNextValue(), csb = smoothedShpBit.getNextValue(), cg = smoothedGain.getNextValue();
        float aVal = ampEnv.getNextSample(), fVal = filterEnv.getNextSample();

        float cf = voiceManager.getCurrentFrequency() * cp; if (cf < 1.0f) cf = 1.0f;
        if (std::abs(cf - lastOscFreq) > 0.01f) { oscillator.setFrequency(cf); lastOscFreq = cf; }

        float oL = 0.0f, oR = 0.0f; oscillator.getSampleStereo(oL, oR);
        float sL = oL, sR = oR, mc = cc + (fVal * ce * 10000.0f);
        filter.setParameters(juce::jlimit(20.0f, 20000.0f, mc), cr); sL = filter.processSample(sL); sR = filter.processSample(sR);
        shaper.processStereo(sL, sR, cd, csa, csr, csb, sL, sR);
        float fg = cg * aVal; left[i] = sL * fg; right[i] = sR * fg;
        outputScopeData[scopeWriteIndex] = (left[i] + right[i]) * 0.5f; scopeWriteIndex = (scopeWriteIndex + 1) % 512;
    }
}


juce::AudioProcessorEditor* LiquidDreamAudioProcessor::createEditor() { return new LiquidDreamAudioProcessorEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new LiquidDreamAudioProcessor(); }