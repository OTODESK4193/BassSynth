#include "PluginProcessor.h"
#include "PluginEditor.h"

LiquidDreamAudioProcessor::LiquidDreamAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    apvts(*this, nullptr, "PARAMS", createParameterLayout())
{
    outputScopeData.fill(0.0f);
}

LiquidDreamAudioProcessor::~LiquidDreamAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout LiquidDreamAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterInt>("osc_wave", "Waveform", 0, 1000, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc_pos", "Position", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc_fm", "FM Amt", 0.0f, 3.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc_sync", "Sync", 1.0f, 4.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc_morph", "Warp", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterInt>("osc_uni", "Unison", 1, 12, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc_detune", "Detune", 0.0f, 1.0f, 0.2f));

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
    filter.prepare(sampleRate);
    shaper.prepare(sampleRate);
    voiceManager.setSampleRate(sampleRate);
    ampEnv.setSampleRate(sampleRate);
    filterEnv.setSampleRate(sampleRate);
}

void LiquidDreamAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    if (buffer.getNumChannels() < 2) return;

    int waveIdx = (int)*apvts.getRawParameterValue("osc_wave");
    static int lastWaveIdx = -1;
    if (waveIdx != lastWaveIdx && waveIdx >= 0 && waveIdx < EmbeddedWavetables::numTables) {
        oscillator.loadWavetableFile(EmbeddedWavetables::allNames[waveIdx]);
        lastWaveIdx = waveIdx;
    }

    for (const auto metadata : midiMessages) {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn()) {
            voiceManager.noteOn(msg.getNoteNumber(), msg.getVelocity());
            oscillator.resetPhase();
            ampEnv.noteOn();
            filterEnv.noteOn();
        }
        else if (msg.isNoteOff()) {
            voiceManager.noteOff(msg.getNoteNumber());
            ampEnv.noteOff();
            filterEnv.noteOff();
        }
    }

    oscillator.setWavetablePosition(*apvts.getRawParameterValue("osc_pos"));
    oscillator.setFMAmount(*apvts.getRawParameterValue("osc_fm"));
    oscillator.setSyncAmount(*apvts.getRawParameterValue("osc_sync"));
    oscillator.setMorph(*apvts.getRawParameterValue("osc_morph"));
    oscillator.setUnisonCount((int)*apvts.getRawParameterValue("osc_uni"));
    oscillator.setUnisonDetune(*apvts.getRawParameterValue("osc_detune"));

    voiceManager.setGlideTime(*apvts.getRawParameterValue("m_glide"));
    ampEnv.setParameters(*apvts.getRawParameterValue("a_atk"), *apvts.getRawParameterValue("a_dec"), *apvts.getRawParameterValue("a_sus"), *apvts.getRawParameterValue("a_rel"));
    filterEnv.setParameters(*apvts.getRawParameterValue("f_atk"), *apvts.getRawParameterValue("f_dec"), *apvts.getRawParameterValue("f_sus"), *apvts.getRawParameterValue("f_rel"));

    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);

    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        float aVal = ampEnv.getNextSample();
        float fVal = filterEnv.getNextSample();

        float currentFreq = voiceManager.getCurrentFrequency();
        if (currentFreq < 1.0f) currentFreq = 1.0f; // Guard against 0Hz
        oscillator.setFrequency(currentFreq);

        float oL = 0.0f, oR = 0.0f;
        oscillator.getSampleStereo(oL, oR);

        float sL = oL, sR = oR;

        // --- FILTER SAFEGUARD ---
        float cutoff = *apvts.getRawParameterValue("flt_cutoff") + (fVal * (*apvts.getRawParameterValue("flt_env_amt")) * 10000.0f);
        filter.setParameters(juce::jlimit(20.0f, 20000.0f, cutoff), *apvts.getRawParameterValue("flt_res"));
        sL = filter.processSample(sL);
        sR = filter.processSample(sR);

        // --- SHAPER SAFEGUARD ---
        float drive = *apvts.getRawParameterValue("dist_drive");
        sL = std::tanh(sL * drive);
        sR = std::tanh(sR * drive);

        float shpAmt = *apvts.getRawParameterValue("shp_amt");
        if (shpAmt > 0.01f) {
            shaper.processStereo(sL, sR, shpAmt, *apvts.getRawParameterValue("shp_rate"), *apvts.getRawParameterValue("shp_bit"), sL, sR);
        }

        // --- AMP SAFEGUARD ---
        float gain = *apvts.getRawParameterValue("m_gain") * aVal;
        left[i] = sL * gain;
        right[i] = sR * gain;

        outputScopeData[scopeWriteIndex] = (left[i] + right[i]) * 0.5f;
        scopeWriteIndex = (scopeWriteIndex + 1) % 512;
    }
}

juce::AudioProcessorEditor* LiquidDreamAudioProcessor::createEditor() { return new LiquidDreamAudioProcessorEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new LiquidDreamAudioProcessor(); }