#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// 1. コンストラクタ (Constructor)
// ここでパラメータのポインタをキャッシュし、初期設定を行います。
//==============================================================================
LiquidDreamAudioProcessor::LiquidDreamAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    apvts(*this, nullptr, "PARAMS", createParameterLayout())
{
    outputScopeData.fill(0.0f);

    // APVTSからパラメータの生ポインタを取得してキャッシュ（オーディオスレッドでの文字列検索を回避）
    pWave = apvts.getRawParameterValue("osc_wave");
    pPos = apvts.getRawParameterValue("osc_pos");
    pOscPitch = apvts.getRawParameterValue("osc_pitch");
    pFm = apvts.getRawParameterValue("osc_fm");
    pSync = apvts.getRawParameterValue("osc_sync");
    pMorph = apvts.getRawParameterValue("osc_morph");
    pUni = apvts.getRawParameterValue("osc_uni");
    pDetune = apvts.getRawParameterValue("osc_detune");

    pCutoff = apvts.getRawParameterValue("flt_cutoff");
    pReso = apvts.getRawParameterValue("flt_res");
    pFltEnvAmt = apvts.getRawParameterValue("flt_env_amt");

    pDrive = apvts.getRawParameterValue("dist_drive");
    pShpAmt = apvts.getRawParameterValue("shp_amt");
    pShpRate = apvts.getRawParameterValue("shp_rate");
    pShpBit = apvts.getRawParameterValue("shp_bit");

    pGain = apvts.getRawParameterValue("m_gain");
    pGlide = apvts.getRawParameterValue("m_glide");

    pAAtk = apvts.getRawParameterValue("a_atk");
    pADec = apvts.getRawParameterValue("a_dec");
    pASus = apvts.getRawParameterValue("a_sus");
    pARel = apvts.getRawParameterValue("a_rel");

    pFAtk = apvts.getRawParameterValue("f_atk");
    pFDec = apvts.getRawParameterValue("f_dec");
    pFSus = apvts.getRawParameterValue("f_sus");
    pFRel = apvts.getRawParameterValue("f_rel");
}

//==============================================================================
// 2. デストラクタ (Destructor)
// 未解決の外部シンボルエラーを防ぐため、実体が必要です。
//==============================================================================
LiquidDreamAudioProcessor::~LiquidDreamAudioProcessor()
{
}

//==============================================================================
// 3. パラメータレイアウトの定義
//==============================================================================
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

//==============================================================================
// 4. 再生準備 (prepareToPlay)
//==============================================================================
void LiquidDreamAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    oscillator.prepare(sampleRate);
    filter.prepare(sampleRate);
    shaper.prepare(sampleRate);
    voiceManager.setSampleRate(sampleRate);
    ampEnv.setSampleRate(sampleRate);
    filterEnv.setSampleRate(sampleRate);

    double smoothingTime = 0.02; // 20msのスムージング
    smoothedPitchMult.reset(sampleRate, smoothingTime);
    smoothedCutoff.reset(sampleRate, smoothingTime);
    smoothedReso.reset(sampleRate, smoothingTime);
    smoothedFltEnvAmt.reset(sampleRate, smoothingTime);
    smoothedDrive.reset(sampleRate, smoothingTime);
    smoothedShpAmt.reset(sampleRate, smoothingTime);
    smoothedShpRate.reset(sampleRate, smoothingTime);
    smoothedShpBit.reset(sampleRate, smoothingTime);
    smoothedGain.reset(sampleRate, smoothingTime);

    // 初期状態のスナップ（音の立ち上がりでのジャンプ防止）
    smoothedPitchMult.setCurrentAndTargetValue(std::pow(2.0f, pOscPitch->load(std::memory_order_relaxed) / 12.0f));
    smoothedCutoff.setCurrentAndTargetValue(pCutoff->load(std::memory_order_relaxed));
    smoothedReso.setCurrentAndTargetValue(pReso->load(std::memory_order_relaxed));
    smoothedFltEnvAmt.setCurrentAndTargetValue(pFltEnvAmt->load(std::memory_order_relaxed));
    smoothedDrive.setCurrentAndTargetValue(pDrive->load(std::memory_order_relaxed));
    smoothedShpAmt.setCurrentAndTargetValue(pShpAmt->load(std::memory_order_relaxed));
    smoothedShpRate.setCurrentAndTargetValue(pShpRate->load(std::memory_order_relaxed));
    smoothedShpBit.setCurrentAndTargetValue(pShpBit->load(std::memory_order_relaxed));
    smoothedGain.setCurrentAndTargetValue(pGain->load(std::memory_order_relaxed));
}

//==============================================================================
// 5. オーディオ処理ループ (processBlock)
//==============================================================================
void LiquidDreamAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    if (buffer.getNumChannels() < 2) return;

    // --- ブロック単位のパラメータ更新 (CPU負荷削減) ---
    smoothedPitchMult.setTargetValue(std::pow(2.0f, pOscPitch->load(std::memory_order_relaxed) / 12.0f));
    smoothedCutoff.setTargetValue(pCutoff->load(std::memory_order_relaxed));
    smoothedReso.setTargetValue(pReso->load(std::memory_order_relaxed));
    smoothedFltEnvAmt.setTargetValue(pFltEnvAmt->load(std::memory_order_relaxed));
    smoothedDrive.setTargetValue(pDrive->load(std::memory_order_relaxed));
    smoothedShpAmt.setTargetValue(pShpAmt->load(std::memory_order_relaxed));
    smoothedShpRate.setTargetValue(pShpRate->load(std::memory_order_relaxed));
    smoothedShpBit.setTargetValue(pShpBit->load(std::memory_order_relaxed));
    smoothedGain.setTargetValue(pGain->load(std::memory_order_relaxed));

    int waveIdx = (int)pWave->load(std::memory_order_relaxed);
    static int lastWaveIdx = -1;
    if (waveIdx != lastWaveIdx && waveIdx >= 0 && waveIdx < EmbeddedWavetables::numTables) {
        oscillator.loadWavetableFile(EmbeddedWavetables::allNames[waveIdx]);
        lastWaveIdx = waveIdx;
    }

    // --- MIDI処理 (MonoVoiceManagerの戻り値に従う) ---
    for (const auto metadata : midiMessages) {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn()) {
            if (voiceManager.noteOn(msg.getNoteNumber(), msg.getVelocity())) {
                oscillator.resetPhase();
                ampEnv.noteOn();
                filterEnv.noteOn();
            }
        }
        else if (msg.isNoteOff()) {
            if (voiceManager.noteOff(msg.getNoteNumber())) {
                ampEnv.noteOff();
                filterEnv.noteOff();
            }
        }
    }

    // 非スムージング・パラメータの更新
    oscillator.setWavetablePosition(pPos->load(std::memory_order_relaxed));
    oscillator.setFMAmount(pFm->load(std::memory_order_relaxed));
    oscillator.setSyncAmount(pSync->load(std::memory_order_relaxed));
    oscillator.setMorph(pMorph->load(std::memory_order_relaxed));
    oscillator.setUnisonCount((int)pUni->load(std::memory_order_relaxed));
    oscillator.setUnisonDetune(pDetune->load(std::memory_order_relaxed));

    voiceManager.setGlideTime(pGlide->load(std::memory_order_relaxed));
    ampEnv.setParameters(pAAtk->load(std::memory_order_relaxed), pADec->load(std::memory_order_relaxed),
        pASus->load(std::memory_order_relaxed), pARel->load(std::memory_order_relaxed));
    filterEnv.setParameters(pFAtk->load(std::memory_order_relaxed), pFDec->load(std::memory_order_relaxed),
        pFSus->load(std::memory_order_relaxed), pFRel->load(std::memory_order_relaxed));

    // --- サンプル単位のDSPループ ---
    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);

    for (int i = 0; i < buffer.getNumSamples(); ++i) {

        // 補間されたパラメータ値の取得
        float curPitchMult = smoothedPitchMult.getNextValue();
        float curCutoff = smoothedCutoff.getNextValue();
        float curReso = smoothedReso.getNextValue();
        float curFltEnvAmt = smoothedFltEnvAmt.getNextValue();
        float curDrive = smoothedDrive.getNextValue();
        float curShpAmt = smoothedShpAmt.getNextValue();
        float curShpRate = smoothedShpRate.getNextValue();
        float curShpBit = smoothedShpBit.getNextValue();
        float curGain = smoothedGain.getNextValue();

        float aVal = ampEnv.getNextSample();
        float fVal = filterEnv.getNextSample();

        float currentFreq = voiceManager.getCurrentFrequency() * curPitchMult;
        if (currentFreq < 1.0f) currentFreq = 1.0f;
        oscillator.setFrequency(currentFreq);

        float oL = 0.0f, oR = 0.0f;
        oscillator.getSampleStereo(oL, oR);

        float sL = oL, sR = oR;

        // FILTER
        float modCutoff = curCutoff + (fVal * curFltEnvAmt * 10000.0f);
        filter.setParameters(juce::jlimit(20.0f, 20000.0f, modCutoff), curReso);
        sL = filter.processSample(sL);
        sR = filter.processSample(sR);

        // SHAPER (Saturation & Bitcrush)
        sL = std::tanh(sL * curDrive);
        sR = std::tanh(sR * curDrive);

        if (curShpAmt > 0.01f) {
            shaper.processStereo(sL, sR, curShpAmt, curShpRate, curShpBit, sL, sR);
        }

        // AMP & MASTER GAIN
        float finalGain = curGain * aVal;
        left[i] = sL * finalGain;
        right[i] = sR * finalGain;

        // スコープへの書き込み
        outputScopeData[scopeWriteIndex] = (left[i] + right[i]) * 0.5f;
        scopeWriteIndex = (scopeWriteIndex + 1) % 512;
    }
}

//==============================================================================
// 6. プラグイン・インスタンス生成のエントリポイント (必達)
//==============================================================================
juce::AudioProcessorEditor* LiquidDreamAudioProcessor::createEditor()
{
    return new LiquidDreamAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LiquidDreamAudioProcessor();
}