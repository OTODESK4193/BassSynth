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
// True OTT Module
// ==============================================================================
class TrueOTT
{
public:
    TrueOTT()
    {
        lp1.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
        hp1.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
        lp2.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
        hp2.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
        ap2.setType(juce::dsp::LinkwitzRileyFilterType::allpass);
    }

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        lp1.prepare(spec); hp1.prepare(spec);
        lp2.prepare(spec); hp2.prepare(spec);
        ap2.prepare(spec);

        lp1.setCutoffFrequency(150.0f); hp1.setCutoffFrequency(150.0f);
        lp2.setCutoffFrequency(2500.0f); hp2.setCutoffFrequency(2500.0f);
        ap2.setCutoffFrequency(2500.0f);

        envLowL = 1.0f; envLowR = 1.0f; envMidL = 1.0f; envMidR = 1.0f; envHighL = 1.0f; envHighR = 1.0f;
    }

    void setParameters(float depthPct, float timePct, float upPct, float downPct, float outGainDb)
    {
        depth = juce::jlimit(0.0f, 1.0f, depthPct);
        upwardAmount = juce::jlimit(0.0f, 1.0f, upPct);
        downwardAmount = juce::jlimit(0.0f, 1.0f, downPct);
        outGainLinear = juce::Decibels::decibelsToGain(outGainDb);
        attackMs = juce::jmap(timePct, 0.5f, 50.0f);
        releaseMs = juce::jmap(timePct, 10.0f, 500.0f);
    }

    void process(juce::AudioBuffer<float>& buffer)
    {
        if (depth <= 0.001f && outGainLinear == 1.0f) return;

        const int numSamples = buffer.getNumSamples();
        auto* outL = buffer.getWritePointer(0); auto* outR = buffer.getWritePointer(1);

        for (int i = 0; i < numSamples; ++i) {
            float inL = outL[i], inR = outR[i];
            float lowL = lp1.processSample(0, inL), lowR = lp1.processSample(1, inR);
            float midHighL = hp1.processSample(0, inL), midHighR = hp1.processSample(1, inR);
            float midL = lp2.processSample(0, midHighL), midR = lp2.processSample(1, midHighR);
            float highL = hp2.processSample(0, midHighL), highR = hp2.processSample(1, midHighR);

            lowL = ap2.processSample(0, lowL); lowR = ap2.processSample(1, lowR);

            lowL *= calculateGain(lowL, envLowL, 0); lowR *= calculateGain(lowR, envLowR, 0);
            midL *= calculateGain(midL, envMidL, 1); midR *= calculateGain(midR, envMidR, 1);
            highL *= calculateGain(highL, envHighL, 2); highR *= calculateGain(highR, envHighR, 2);

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

        float threshDown = (bandIndex == 1) ? -5.0f : -20.0f;
        float threshUp = (bandIndex == 1) ? -40.0f : -35.0f;
        float activeRange = -80.0f;

        if (inputDb > threshDown) {
            targetDb = threshDown + (inputDb - threshDown) / (1.0f + 4.0f * downwardAmount);
        }
        else if (inputDb < threshUp && inputDb > activeRange) {
            float upRatio = (bandIndex == 2) ? 0.9f : 0.7f;
            targetDb = threshUp - (threshUp - inputDb) * (1.0f - upRatio * upwardAmount);
        }

        float targetGain = juce::Decibels::decibelsToGain(targetDb - inputDb);
        float alpha = (targetGain < envState) ? std::exp(-1.0f / (sampleRate * attackMs * 0.001f)) : std::exp(-1.0f / (sampleRate * releaseMs * 0.001f));
        envState = alpha * envState + (1.0f - alpha) * targetGain;

        return envState;
    }
};

// ==============================================================================
// Sparkle Arp Module
// ==============================================================================
class SparkleArp
{
public:
    void prepare(double sr) { sampleRate = sr; }

    void setParameters(int wave, int mode, float speedHz, int octIdx, float level) {
        currentWave = wave; currentMode = mode;
        currentOctave = (octIdx + 2) * 12;
        targetLevel = level;
        samplesPerStep = (int)(sampleRate / std::max(1.0f, speedHz));
    }

    void updateChord(const std::vector<int>& learnedNotes) {
        std::lock_guard<std::mutex> lock(arpMutex);
        if (learnedNotes.empty()) { intervals.clear(); return; }
        intervals.clear();
        int root = learnedNotes[0];
        for (int note : learnedNotes) intervals.push_back(note - root);
        std::sort(intervals.begin(), intervals.end());
        intervals.erase(std::unique(intervals.begin(), intervals.end()), intervals.end());
    }

    void process(juce::AudioBuffer<float>& buffer, const std::vector<int>& activeNotes, const float* mainAmpEnvBuffer) {
        if (targetLevel <= 0.001f || activeNotes.empty() || samplesPerStep <= 0) {
            currentLevel = 0.0f; return;
        }

        std::lock_guard<std::mutex> lock(arpMutex);
        if (intervals.empty()) return;

        int baseNote = activeNotes.back();
        const int numSamples = buffer.getNumSamples();
        auto* outL = buffer.getWritePointer(0); auto* outR = buffer.getWritePointer(1);

        for (int i = 0; i < numSamples; ++i) {
            if (sampleCounter >= samplesPerStep) {
                sampleCounter = 0; advanceStep(); envPhase = 0.0f;
            }

            int note = baseNote + currentOctave + intervals[currentStepIndex];
            float freq = (float)juce::MidiMessage::getMidiNoteInHertz(note);
            float inc = freq / (float)sampleRate;

            float sample = generateOscillator(oscPhase, inc, currentWave);
            oscPhase += inc; if (oscPhase >= 1.0f) oscPhase -= 1.0f;

            float env = std::max(0.0f, 1.0f - (envPhase / (float)(samplesPerStep)));
            env = std::pow(env, 4.0f);
            envPhase += 1.0f;

            currentLevel = currentLevel * 0.999f + targetLevel * 0.001f;
            float val = sample * env * currentLevel * 0.24f * mainAmpEnvBuffer[i];

            outL[i] += val; outR[i] += val;
            sampleCounter++;
        }
    }

private:
    double sampleRate = 44100.0;
    int currentWave = 0, currentMode = 0, currentOctave = 12, samplesPerStep = 10000, sampleCounter = 0;
    float targetLevel = 0.0f, currentLevel = 0.0f, oscPhase = 0.0f, envPhase = 0.0f;

    std::vector<int> intervals;
    int currentStepIndex = 0;
    bool movingUp = true;
    std::mutex arpMutex;

    void advanceStep() {
        if (intervals.empty()) return;
        if (currentMode == 0) { // Up
            currentStepIndex = (currentStepIndex + 1) % intervals.size();
        }
        else if (currentMode == 1) { // Down
            currentStepIndex = (currentStepIndex - 1 + intervals.size()) % intervals.size();
        }
        else if (currentMode == 2) { // Up/Down
            if (movingUp) {
                currentStepIndex++;
                if (currentStepIndex >= intervals.size()) { currentStepIndex = std::max(0, (int)intervals.size() - 2); movingUp = false; }
            }
            else {
                currentStepIndex--;
                if (currentStepIndex < 0) { currentStepIndex = std::min(1, (int)intervals.size() - 1); movingUp = true; }
            }
        }
        else if (currentMode == 3) { // Random
            currentStepIndex = juce::Random::getSystemRandom().nextInt(intervals.size());
        }
    }

    float generateOscillator(float phase, float inc, int wave) {
        float out = 0.0f;
        float pPi = phase * juce::MathConstants<float>::twoPi;
        int maxHarmonic = (int)(0.5f / inc);
        if (maxHarmonic > 40) maxHarmonic = 40;

        if (wave == 0) { out = std::sin(pPi); }
        else if (wave == 1) {
            for (int h = 1; h <= maxHarmonic; ++h) out += std::sin(pPi * h) / (float)h;
            out *= 0.6366f;
        }
        else if (wave == 2) {
            for (int h = 1; h <= maxHarmonic; h += 2) out += std::sin(pPi * h) / (float)h;
            out *= 1.273f;
        }
        else if (wave == 3 || wave == 4) {
            float duty = (wave == 3) ? 0.25f : 0.125f;
            for (int h = 1; h <= maxHarmonic; ++h) out += std::sin(h * duty * juce::MathConstants<float>::pi) * std::cos(pPi * h) / (float)h;
            out *= 1.273f;
        }
        return out;
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
        // ★ 修正: ポインタアクセスへ変更
        threadPool->removeAllJobs(true, 1000);
    }

    void prepare(double sampleRate, int samplesPerBlock)
    {
        currentSampleRate = std::max(1.0, sampleRate);
        juce::dsp::ProcessSpec spec{ currentSampleRate, (juce::uint32)samplesPerBlock, 2 };
        convEngineA.prepare(spec); convEngineB.prepare(spec);
        preFilterL.prepare(spec); preFilterR.prepare(spec);
        postFilterL.prepare(spec); postFilterR.prepare(spec);
        trueOtt.prepare(spec);
        sparkleArp.prepare(sampleRate);
        wetBufferA.setSize(2, samplesPerBlock); wetBufferB.setSize(2, samplesPerBlock);
    }

    LearnState getLearnState() const { return state.load(); }
    void setLearnState(LearnState newState) {
        if (newState == LearnState::Learning) { std::lock_guard<std::mutex> lock(chordMutex); learnedNotes.clear(); }
        state.store(newState);
    }

    void addNote(int note) {
        std::lock_guard<std::mutex> lock(chordMutex);
        if (learnedNotes.size() < 7 && std::find(learnedNotes.begin(), learnedNotes.end(), note) == learnedNotes.end()) learnedNotes.push_back(note);
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
        sparkleArp.updateChord(currentNotes);

        const int myJobId = ++irGenJobId;
        const double sr = currentSampleRate;

        // ★ 修正: ポインタアクセスへ変更
        threadPool->addJob([this, currentNotes, attackMs, decayMs, irType, sr, myJobId]() {
            if (myJobId != irGenJobId.load()) return;

            float actualDecayMs = std::max(100.0f, decayMs);
            int numSamples = (int)(sr * (actualDecayMs + attackMs) / 1000.0);
            if (numSamples < 2048) numSamples = 2048;

            juce::AudioBuffer<float> tempIR(2, numSamples); tempIR.clear();
            std::vector<float> phases(currentNotes.size(), 0.0f), incs(currentNotes.size(), 0.0f);
            auto& random = juce::Random::getSystemRandom();
            float baseFreq = (float)juce::MidiMessage::getMidiNoteInHertz(currentNotes[0]);

            // ★ 修正: すべての初期位相を0.0fで同期させ、強烈なコード感（Zero-Phase）を確保
            for (size_t n = 0; n < currentNotes.size(); ++n) {
                float freq = (float)juce::MidiMessage::getMidiNoteInHertz(currentNotes[n]);
                incs[n] = freq / (float)sr;
                phases[n] = 0.0f;
            }

            float atkSamples = std::max(1.0f, (attackMs / 1000.0f) * (float)sr);
            float decSamples = (actualDecayMs / 1000.0f) * (float)sr;
            auto* outL = tempIR.getWritePointer(0); auto* outR = tempIR.getWritePointer(1);
            float norm = 1.0f / (float)currentNotes.size();

            for (int i = 0; i < numSamples; ++i) {
                if (myJobId != irGenJobId.load()) return;
                float sample = 0.0f;

                if (irType == 0) {
                    for (size_t n = 0; n < currentNotes.size(); ++n) {
                        float saw = 2.0f * phases[n] - 1.0f;
                        sample += saw;
                        phases[n] += incs[n]; if (phases[n] >= 1.0f) phases[n] -= 1.0f;
                    }
                    sample *= norm;
                }
                else if (irType == 1) {
                    for (size_t n = 0; n < currentNotes.size(); ++n) {
                        float sq = (phases[n] < 0.5f) ? 1.0f : -1.0f;
                        sample += sq;
                        phases[n] += incs[n]; if (phases[n] >= 1.0f) phases[n] -= 1.0f;
                    }
                    sample *= norm;
                }
                else if (irType == 2) {
                    for (size_t n = 0; n < currentNotes.size(); ++n) {
                        float freq = (float)juce::MidiMessage::getMidiNoteInHertz(currentNotes[n]);
                        float ratio = freq / baseFreq;
                        float pPi = phases[n] * juce::MathConstants<float>::twoPi;

                        float overtoneMix = std::min(1.0f, (ratio - 1.0f) * 0.4f);
                        float decayMult = std::exp(-(float)i / (sr * (actualDecayMs / 1000.0f) * (1.0f / std::sqrt(ratio))));

                        float val = std::sin(pPi);
                        if (overtoneMix > 0.01f) {
                            val += overtoneMix * (0.6f * std::sin(pPi * 2.76f) + 0.4f * std::sin(pPi * 5.4f) + 0.2f * std::sin(pPi * 8.9f));
                        }
                        sample += val * decayMult * 0.8f;
                        phases[n] += incs[n]; if (phases[n] >= 1.0f) phases[n] -= 1.0f;
                    }
                    sample *= norm;
                }
                else if (irType == 3) {
                    // ★ 修正: 発散しないピュアなAdditive Resonator（倍音加算）へ進化
                    for (size_t n = 0; n < currentNotes.size(); ++n) {
                        float val = 0.0f;
                        // 1倍音（基音）から5倍音までのサイン波を合成
                        for (int h = 1; h <= 5; ++h) {
                            float hPhase = std::fmod(phases[n] * h, 1.0f);
                            // 高次倍音ほど早く減衰させる
                            float hDecay = std::exp(-(float)i / (sr * (actualDecayMs / 1000.0f) / (float)h));
                            val += std::sin(hPhase * juce::MathConstants<float>::twoPi) * hDecay * (1.0f / (float)h);
                        }
                        sample += val;
                        phases[n] += incs[n]; if (phases[n] >= 1.0f) phases[n] -= 1.0f;
                    }
                    sample *= (norm * 0.8f);
                    // アタックの瞬間にのみ、微量の煌めきノイズを付与
                    sample += (random.nextFloat() * 2.0f - 1.0f) * 0.1f * std::exp(-(float)i / (sr * 0.02f));
                }

                sample = std::tanh(sample * ((irType == 3) ? 1.5f : 4.0f));

                float env = (i < atkSamples) ? ((float)i / atkSamples) : std::pow(std::max(0.0f, 1.0f - ((float)(i - atkSamples) / decSamples)), 4.0f);
                outL[i] = sample * env * 0.025f; outR[i] = sample * env * 0.025f;
            }

            if (activeEngineIsA.load()) convEngineB.loadImpulseResponse(std::move(tempIR), sr, juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::no, juce::dsp::Convolution::Normalise::no);
            else convEngineA.loadImpulseResponse(std::move(tempIR), sr, juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::no, juce::dsp::Convolution::Normalise::no);

            triggerCrossfade();
            state.store(LearnState::Active);
            });
    }

    void setOttParameters(float depth, float time, float up, float down, float gainDb) { trueOtt.setParameters(depth, time, up, down, gainDb); }

    void setSootheParameters(float depth, float time, float selectivity, float sharpness, float focus) {
        sootheDepth = depth; sootheTime = time;
        sootheSelectivity = selectivity; sootheSharpness = sharpness; sootheFocus = focus;
    }

    void setArpParameters(int wave, int mode, float speedHz, int pitch, float level) { sparkleArp.setParameters(wave, mode, speedHz, pitch, level); }

    void setParameters(float preCutoffHz, float postCutoffHz, float mixVal, float irVolDb)
    {
        preFilterL.setCutoffFrequency(std::clamp(preCutoffHz, 20.0f, 2000.0f)); preFilterR.setCutoffFrequency(std::clamp(preCutoffHz, 20.0f, 2000.0f));
        postFilterL.setCutoffFrequency(std::clamp(postCutoffHz, 20.0f, 2000.0f)); postFilterR.setCutoffFrequency(std::clamp(postCutoffHz, 20.0f, 2000.0f));
        mix = std::clamp(mixVal, 0.0f, 1.0f);
        irVolumeLinear = juce::Decibels::decibelsToGain(irVolDb);
    }

    void process(juce::AudioBuffer<float>& buffer)
    {
        if (mix > 0.001f && state.load() == LearnState::Active) {
            const int numSamples = buffer.getNumSamples();
            if (wetBufferA.getNumSamples() < numSamples) {
                wetBufferA.setSize(2, numSamples, false, false, true); wetBufferB.setSize(2, numSamples, false, false, true);
            }

            for (int ch = 0; ch < 2; ++ch) wetBufferA.copyFrom(ch, 0, buffer, ch, 0, numSamples);

            auto* wAL = wetBufferA.getWritePointer(0); auto* wAR = wetBufferA.getWritePointer(1);
            for (int i = 0; i < numSamples; ++i) {
                wAL[i] = preFilterL.processSample(0, wAL[i]); wAR[i] = preFilterR.processSample(1, wAR[i]);
            }

            for (int ch = 0; ch < 2; ++ch) wetBufferB.copyFrom(ch, 0, wetBufferA, ch, 0, numSamples);

            juce::dsp::AudioBlock<float> blockA(wetBufferA); juce::dsp::ProcessContextReplacing<float> contextA(blockA); convEngineA.process(contextA);
            juce::dsp::AudioBlock<float> blockB(wetBufferB); juce::dsp::ProcessContextReplacing<float> contextB(blockB); convEngineB.process(contextB);

            auto* inL = buffer.getReadPointer(0); auto* inR = buffer.getReadPointer(1);
            auto* outL = buffer.getWritePointer(0); auto* outR = buffer.getWritePointer(1);
            auto* wBL = wetBufferB.getReadPointer(0); auto* wBR = wetBufferB.getReadPointer(1);

            float fadeInc = 1.0f / (float)(currentSampleRate * 0.05);
            for (int i = 0; i < numSamples; ++i) {
                if (fadeVolA < fadeTargetA) fadeVolA = std::min(fadeVolA + fadeInc, fadeTargetA); else if (fadeVolA > fadeTargetA) fadeVolA = std::max(fadeVolA - fadeInc, fadeTargetA);
                if (fadeVolB < fadeTargetB) fadeVolB = std::min(fadeVolB + fadeInc, fadeTargetB); else if (fadeVolB > fadeTargetB) fadeVolB = std::max(fadeVolB - fadeInc, fadeTargetB);

                float currentWetL = std::tanh(wAL[i] * fadeVolA + wBL[i] * fadeVolB);
                float currentWetR = std::tanh(wAR[i] * fadeVolA + wBR[i] * fadeVolB);

                currentWetL = postFilterL.processSample(0, currentWetL); currentWetR = postFilterR.processSample(1, currentWetR);

                currentWetL *= irVolumeLinear; currentWetR *= irVolumeLinear;

                outL[i] = inL[i] * (1.0f - mix) + currentWetL * mix; outR[i] = inR[i] * (1.0f - mix) + currentWetR * mix;
            }
        }

        trueOtt.process(buffer);
    }

    void processArp(juce::AudioBuffer<float>& masterBuffer, const std::vector<int>& activeNotes, const float* mainAmpEnvBuffer) {
        if (state.load() == LearnState::Active) sparkleArp.process(masterBuffer, activeNotes, mainAmpEnvBuffer);
    }

private:
    double currentSampleRate = 44100.0;
    std::atomic<LearnState> state{ LearnState::Idle };
    std::vector<int> learnedNotes;
    mutable std::mutex chordMutex;

    juce::dsp::Convolution convEngineA, convEngineB;

    // ★ 修正: マルチインスタンス時のCPUスパイクを防ぐためのシングルトン共有スレッド
    juce::SharedResourcePointer<juce::ThreadPool> threadPool;

    std::atomic<int> irGenJobId{ 0 };

    juce::AudioBuffer<float> wetBufferA, wetBufferB;
    juce::dsp::StateVariableTPTFilter<float> preFilterL, preFilterR, postFilterL, postFilterR;
    TrueOTT trueOtt;
    SparkleArp sparkleArp;

    std::atomic<bool> activeEngineIsA{ true };
    float fadeVolA = 1.0f, fadeVolB = 0.0f, fadeTargetA = 1.0f, fadeTargetB = 0.0f, mix = 0.0f;
    float irVolumeLinear = 1.0f;

    float sootheDepth = 0.0f, sootheTime = 0.0f;
    float sootheSelectivity = 0.5f, sootheSharpness = 0.5f, sootheFocus = 0.0f;

    void triggerCrossfade() {
        if (activeEngineIsA.load()) { activeEngineIsA.store(false); fadeTargetA = 0.0f; fadeTargetB = 1.0f; }
        else { activeEngineIsA.store(true); fadeTargetA = 1.0f; fadeTargetB = 0.0f; }
    }
};