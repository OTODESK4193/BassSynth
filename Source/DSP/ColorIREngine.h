// ==============================================================================
// Source/DSP/ColorIREngine.h
// ==============================================================================
#pragma once
#include <JuceHeader.h>
#include <vector>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <cmath>

// ==============================================================================
// True OTT Module (3-Band Upward/Downward Compressor with Phase Compensation)
// ==============================================================================
class TrueOTT
{
public:
    TrueOTT()
    {
        // Linkwitz-Riley 4次フィルターの初期化
        lp1.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
        hp1.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
        lp2.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
        hp2.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
        ap2.setType(juce::dsp::LinkwitzRileyFilterType::allpass); // Low帯域の位相補償用
    }

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        lp1.prepare(spec); hp1.prepare(spec);
        lp2.prepare(spec); hp2.prepare(spec);
        ap2.prepare(spec);

        // クロスオーバー周波数の設定 (Low-Mid: 250Hz, Mid-High: 2500Hz)
        lp1.setCutoffFrequency(250.0f); hp1.setCutoffFrequency(250.0f);
        lp2.setCutoffFrequency(2500.0f); hp2.setCutoffFrequency(2500.0f);
        ap2.setCutoffFrequency(2500.0f);

        envLowL = 1.0f; envLowR = 1.0f;
        envMidL = 1.0f; envMidR = 1.0f;
        envHighL = 1.0f; envHighR = 1.0f;
    }

    // マクロパラメーターの更新
    void setParameters(float depthPct, float timePct, float upPct, float downPct, float outGainDb)
    {
        depth = juce::jlimit(0.0f, 1.0f, depthPct);
        upwardAmount = juce::jlimit(0.0f, 1.0f, upPct);
        downwardAmount = juce::jlimit(0.0f, 1.0f, downPct);
        outGainLinear = juce::Decibels::decibelsToGain(outGainDb);

        // Timeパラメーターによるアタック・リリースのスケーリング (0.0=Fast/Glitchy, 1.0=Slow/Smear)
        attackMs = juce::jmap(timePct, 0.5f, 50.0f);
        releaseMs = juce::jmap(timePct, 10.0f, 500.0f);
    }

    void process(juce::AudioBuffer<float>& buffer)
    {
        if (depth <= 0.001f && outGainLinear == 1.0f) return;

        const int numSamples = buffer.getNumSamples();
        auto* outL = buffer.getWritePointer(0);
        auto* outR = buffer.getWritePointer(1);

        for (int i = 0; i < numSamples; ++i) {
            float inL = outL[i];
            float inR = outR[i];

            // 1. 帯域分割 (3-Band Crossover)
            float lowL = lp1.processSample(0, inL);
            float lowR = lp1.processSample(1, inR);
            float midHighL = hp1.processSample(0, inL);
            float midHighR = hp1.processSample(1, inR);

            float midL = lp2.processSample(0, midHighL);
            float midR = lp2.processSample(1, midHighR);
            float highL = hp2.processSample(0, midHighL);
            float highR = hp2.processSample(1, midHighR);

            // Low帯域の完全位相補償 (Perfect Reconstruction)
            lowL = ap2.processSample(0, lowL);
            lowR = ap2.processSample(1, lowR);

            // 2. 対数領域コンプレッション (Upward / Downward)
            lowL *= calculateGain(lowL, envLowL, 0);
            lowR *= calculateGain(lowR, envLowR, 0);

            midL *= calculateGain(midL, envMidL, 1);
            midR *= calculateGain(midR, envMidR, 1);

            highL *= calculateGain(highL, envHighL, 2);
            highR *= calculateGain(highR, envHighR, 2);

            // 3. 再合成とパラレル・ミックス
            float processedL = (lowL + midL + highL) * outGainLinear;
            float processedR = (lowR + midR + highR) * outGainLinear;

            outL[i] = inL * (1.0f - depth) + processedL * depth;
            outR[i] = inR * (1.0f - depth) + processedR * depth;
        }
    }

private:
    juce::dsp::LinkwitzRileyFilter<float> lp1, hp1, lp2, hp2, ap2;
    double sampleRate = 44100.0;

    float depth = 1.0f, upwardAmount = 0.5f, downwardAmount = 0.5f, outGainLinear = 1.0f;
    float attackMs = 10.0f, releaseMs = 100.0f;

    float envLowL, envLowR, envMidL, envMidR, envHighL, envHighR;

    float calculateGain(float inputAbs, float& envState, int bandIndex)
    {
        float inputDb = juce::Decibels::gainToDecibels(std::abs(inputAbs) + 1e-6f);
        float targetDb = inputDb;

        // 帯域ごとのスレッショルド (MidはWiggle Roomを広くとる)
        float threshDown = (bandIndex == 1) ? -10.0f : -20.0f;
        float threshUp = (bandIndex == 1) ? -40.0f : -35.0f;
        float activeRange = -80.0f; // -80dB以下はノイズフロアとして無視

        if (inputDb > threshDown) {
            // Downward Compression
            float ratio = 1.0f + 4.0f * downwardAmount;
            targetDb = threshDown + (inputDb - threshDown) / ratio;
        }
        else if (inputDb < threshUp && inputDb > activeRange) {
            // Upward Compression
            float ratio = 1.0f - 0.7f * upwardAmount;
            targetDb = threshUp - (threshUp - inputDb) * ratio;
        }

        float targetGain = juce::Decibels::decibelsToGain(targetDb - inputDb);

        // Envelope Follower
        float alphaAtk = std::exp(-1.0f / (sampleRate * attackMs * 0.001f));
        float alphaRel = std::exp(-1.0f / (sampleRate * releaseMs * 0.001f));

        // ゲインが下がる時が「アタック（潰す）」、上がる時が「リリース（戻る）」
        float alpha = (targetGain < envState) ? alphaAtk : alphaRel;
        envState = alpha * envState + (1.0f - alpha) * targetGain;

        return envState;
    }
};

// ==============================================================================
// ColorIREngine Main Class
// ==============================================================================
class ColorIREngine
{
public:
    enum class LearnState { Idle, Learning, Active };

    ColorIREngine()
    {
        convEngineA.loadImpulseResponse(juce::AudioBuffer<float>(2, 256), 44100.0, juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::no, juce::dsp::Convolution::Normalise::no);
        convEngineB.loadImpulseResponse(juce::AudioBuffer<float>(2, 256), 44100.0, juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::no, juce::dsp::Convolution::Normalise::no);

        preFilterL.setType(juce::dsp::StateVariableTPTFilterType::highpass); preFilterR.setType(juce::dsp::StateVariableTPTFilterType::highpass);
        postFilterL.setType(juce::dsp::StateVariableTPTFilterType::highpass); postFilterR.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    }

    ~ColorIREngine() {
        irGenJobId++;
        threadPool.removeAllJobs(true, 1000);
    }

    void prepare(double sampleRate, int samplesPerBlock)
    {
        currentSampleRate = std::max(1.0, sampleRate);
        juce::dsp::ProcessSpec spec{ currentSampleRate, (juce::uint32)samplesPerBlock, 2 };
        convEngineA.prepare(spec); convEngineB.prepare(spec);
        preFilterL.prepare(spec); preFilterR.prepare(spec);
        postFilterL.prepare(spec); postFilterR.prepare(spec);

        trueOtt.prepare(spec); // True OTT 準備

        wetBufferA.setSize(2, samplesPerBlock); wetBufferB.setSize(2, samplesPerBlock);
    }

    LearnState getLearnState() const { return state.load(); }
    void setLearnState(LearnState newState) {
        if (newState == LearnState::Learning) { std::lock_guard<std::mutex> lock(chordMutex); learnedNotes.clear(); }
        state.store(newState);
    }

    void addNote(int note) {
        std::lock_guard<std::mutex> lock(chordMutex);
        if (learnedNotes.size() < 7 && std::find(learnedNotes.begin(), learnedNotes.end(), note) == learnedNotes.end()) {
            learnedNotes.push_back(note);
        }
    }

    juce::String getLearnedChordNames() const {
        std::lock_guard<std::mutex> lock(chordMutex);
        if (learnedNotes.empty()) return "NO CHORD";
        juce::StringArray names;
        for (int note : learnedNotes) names.add(juce::MidiMessage::getMidiNoteName(note, true, true, 4));
        return names.joinIntoString(", ");
    }

    void finishLearningAndGenerate(float attackMs, float decayMs, int irType)
    {
        std::vector<int> currentNotes;
        {
            std::lock_guard<std::mutex> lock(chordMutex);
            if (learnedNotes.empty()) { state.store(LearnState::Idle); return; }
            currentNotes = learnedNotes;
        }

        std::sort(currentNotes.begin(), currentNotes.end());

        const int myJobId = ++irGenJobId;
        const double sr = currentSampleRate;

        threadPool.addJob([this, currentNotes, attackMs, decayMs, irType, sr, myJobId]() {
            if (myJobId != irGenJobId.load()) return;

            float actualDecayMs = std::max(100.0f, decayMs);
            int numSamples = (int)(sr * (actualDecayMs + attackMs) / 1000.0);
            if (numSamples < 2048) numSamples = 2048;

            juce::AudioBuffer<float> tempIR(2, numSamples); tempIR.clear();

            std::vector<float> phases(currentNotes.size(), 0.0f);
            std::vector<float> incs(currentNotes.size(), 0.0f);
            std::vector<int> delayLengths(currentNotes.size(), 0);
            auto& random = juce::Random::getSystemRandom();

            for (size_t n = 0; n < currentNotes.size(); ++n) {
                float freq = (float)juce::MidiMessage::getMidiNoteInHertz(currentNotes[n]);
                incs[n] = freq / (float)sr; phases[n] = 0.0f; delayLengths[n] = (int)(sr / freq);
            }

            float atkSamples = std::max(1.0f, (attackMs / 1000.0f) * (float)sr);
            float decSamples = (actualDecayMs / 1000.0f) * (float)sr;
            float noiseSamples = sr * 0.02f;

            auto* outL = tempIR.getWritePointer(0); auto* outR = tempIR.getWritePointer(1);
            float norm = 1.0f / (float)currentNotes.size();

            for (int i = 0; i < numSamples; ++i) {
                if (myJobId != irGenJobId.load()) return;
                float sample = 0.0f;

                if (irType == 3) {
                    for (size_t n = 0; n < currentNotes.size(); ++n) {
                        float noise = (i < 100) ? (random.nextFloat() * 2.0f - 1.0f) : 0.0f;
                        sample += noise + ((i >= delayLengths[n]) ? outL[i - delayLengths[n]] : 0.0f) * 0.995f;
                    }
                    sample *= (norm * 0.15f);
                }
                else {
                    for (size_t n = 0; n < currentNotes.size(); ++n) {
                        float phase = phases[n]; float val = 0.0f;
                        if (irType == 0) val = (2.0f * phase - 1.0f);
                        else if (irType == 1) val = (phase < 0.5f) ? 1.0f : -1.0f;
                        else if (irType == 2) {
                            float pPi = phase * juce::MathConstants<float>::twoPi;
                            val = std::sin(pPi) + 0.6f * std::sin(pPi * 2.76f) + 0.4f * std::sin(pPi * 5.4f) + 0.2f * std::sin(pPi * 8.9f);
                            val *= 0.6f;
                        }
                        sample += val;
                        phases[n] += incs[n]; if (phases[n] >= 1.0f) phases[n] -= 1.0f;
                    }
                    sample *= norm;
                    if (i < noiseSamples) {
                        float noiseEnv = 1.0f - ((float)i / noiseSamples);
                        sample += (random.nextFloat() * 2.0f - 1.0f) * noiseEnv * 0.3f;
                    }
                }

                float drive = (irType == 3) ? 1.0f : 4.0f;
                sample = std::tanh(sample * drive);

                float env = (i < atkSamples) ? ((float)i / atkSamples) : std::pow(std::max(0.0f, 1.0f - ((float)(i - atkSamples) / decSamples)), 4.0f);
                float safeGain = 0.025f;
                outL[i] = sample * env * safeGain; outR[i] = sample * env * safeGain;
            }

            if (activeEngineIsA.load()) convEngineB.loadImpulseResponse(std::move(tempIR), sr, juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::no, juce::dsp::Convolution::Normalise::no);
            else convEngineA.loadImpulseResponse(std::move(tempIR), sr, juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::no, juce::dsp::Convolution::Normalise::no);

            triggerCrossfade();
            state.store(LearnState::Active);
            });
    }

    void setOttParameters(float depth, float time, float up, float down, float gainDb) {
        trueOtt.setParameters(depth, time, up, down, gainDb);
    }

    void setParameters(float preCutoffHz, float postCutoffHz, float mixVal)
    {
        preFilterL.setCutoffFrequency(std::clamp(preCutoffHz, 20.0f, 2000.0f)); preFilterR.setCutoffFrequency(std::clamp(preCutoffHz, 20.0f, 2000.0f));
        postFilterL.setCutoffFrequency(std::clamp(postCutoffHz, 20.0f, 2000.0f)); postFilterR.setCutoffFrequency(std::clamp(postCutoffHz, 20.0f, 2000.0f));
        mix = std::clamp(mixVal, 0.0f, 1.0f);
    }

    void process(juce::AudioBuffer<float>& buffer)
    {
        if (mix <= 0.001f || state.load() != LearnState::Active) return;

        const int numSamples = buffer.getNumSamples();
        if (wetBufferA.getNumSamples() < numSamples) {
            wetBufferA.setSize(2, numSamples, false, false, true);
            wetBufferB.setSize(2, numSamples, false, false, true);
        }

        for (int ch = 0; ch < 2; ++ch) wetBufferA.copyFrom(ch, 0, buffer, ch, 0, numSamples);

        auto* wAL = wetBufferA.getWritePointer(0); auto* wAR = wetBufferA.getWritePointer(1);
        for (int i = 0; i < numSamples; ++i) {
            wAL[i] = preFilterL.processSample(0, wAL[i]); wAR[i] = preFilterR.processSample(1, wAR[i]);
        }

        for (int ch = 0; ch < 2; ++ch) wetBufferB.copyFrom(ch, 0, wetBufferA, ch, 0, numSamples);

        juce::dsp::AudioBlock<float> blockA(wetBufferA); juce::dsp::ProcessContextReplacing<float> contextA(blockA);
        convEngineA.process(contextA);
        juce::dsp::AudioBlock<float> blockB(wetBufferB); juce::dsp::ProcessContextReplacing<float> contextB(blockB);
        convEngineB.process(contextB);

        auto* inL = buffer.getReadPointer(0); auto* inR = buffer.getReadPointer(1);
        auto* outL = buffer.getWritePointer(0); auto* outR = buffer.getWritePointer(1);
        auto* wBL = wetBufferB.getReadPointer(0); auto* wBR = wetBufferB.getReadPointer(1);

        float fadeInc = 1.0f / (float)(currentSampleRate * 0.05);

        for (int i = 0; i < numSamples; ++i) {
            if (fadeVolA < fadeTargetA) fadeVolA = std::min(fadeVolA + fadeInc, fadeTargetA);
            else if (fadeVolA > fadeTargetA) fadeVolA = std::max(fadeVolA - fadeInc, fadeTargetA);
            if (fadeVolB < fadeTargetB) fadeVolB = std::min(fadeVolB + fadeInc, fadeTargetB);
            else if (fadeVolB > fadeTargetB) fadeVolB = std::max(fadeVolB - fadeInc, fadeTargetB);

            float currentWetL = std::tanh(wAL[i] * fadeVolA + wBL[i] * fadeVolB);
            float currentWetR = std::tanh(wAR[i] * fadeVolA + wBR[i] * fadeVolB);

            currentWetL = postFilterL.processSample(0, currentWetL);
            currentWetR = postFilterR.processSample(1, currentWetR);

            outL[i] = inL[i] * (1.0f - mix) + currentWetL * mix;
            outR[i] = inR[i] * (1.0f - mix) + currentWetR * mix;
        }

        // True OTT 処理 (コンボリューション直後の信号全体に適用)
        trueOtt.process(buffer);
    }

private:
    double currentSampleRate = 44100.0;
    std::atomic<LearnState> state{ LearnState::Idle };
    std::vector<int> learnedNotes;
    mutable std::mutex chordMutex;

    juce::dsp::Convolution convEngineA, convEngineB;
    juce::ThreadPool threadPool{ 1 };
    std::atomic<int> irGenJobId{ 0 };

    juce::AudioBuffer<float> wetBufferA, wetBufferB;
    juce::dsp::StateVariableTPTFilter<float> preFilterL, preFilterR, postFilterL, postFilterR;

    TrueOTT trueOtt; // ★ 新規組み込み

    std::atomic<bool> activeEngineIsA{ true };
    float fadeVolA = 1.0f, fadeVolB = 0.0f, fadeTargetA = 1.0f, fadeTargetB = 0.0f, mix = 0.0f;

    void triggerCrossfade() {
        if (activeEngineIsA.load()) { activeEngineIsA.store(false); fadeTargetA = 0.0f; fadeTargetB = 1.0f; }
        else { activeEngineIsA.store(true); fadeTargetA = 1.0f; fadeTargetB = 0.0f; }
    }
};