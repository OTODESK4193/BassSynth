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

        ottCompressor.setAttack(1.0f);
        ottCompressor.setRelease(150.0f);
        ottCompressor.setThreshold(-30.0f);
        ottCompressor.setRatio(10.0f);
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
        ottCompressor.prepare(spec);

        wetBufferA.setSize(2, samplesPerBlock); wetBufferB.setSize(2, samplesPerBlock);
    }

    LearnState getLearnState() const { return state.load(); }
    void setLearnState(LearnState newState) {
        if (newState == LearnState::Learning) {
            std::lock_guard<std::mutex> lock(chordMutex);
            learnedNotes.clear();
        }
        // UI側で点滅を止めさせるため状態は即座に反映
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

        // ★修正1: ここで Active にせず、一時的に Learning 状態のまま裏スレッドを走らせる
        // （ロード完了前に無音バッファが再生されるのを防ぐため）

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
                incs[n] = freq / (float)sr;
                phases[n] = 0.0f;
                delayLengths[n] = (int)(sr / freq);
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
                        int dl = delayLengths[n];
                        float fb = (i >= dl) ? outL[i - dl] : 0.0f;
                        sample += noise + fb * 0.995f;
                    }
                    sample *= (norm * 0.15f);
                }
                else {
                    for (size_t n = 0; n < currentNotes.size(); ++n) {
                        float phase = phases[n];
                        float val = 0.0f;

                        if (irType == 0) {
                            val = (2.0f * phase - 1.0f);
                        }
                        else if (irType == 1) {
                            val = (phase < 0.5f) ? 1.0f : -1.0f;
                        }
                        else if (irType == 2) {
                            float pPi = phase * juce::MathConstants<float>::twoPi;
                            val = std::sin(pPi) + 0.6f * std::sin(pPi * 2.76f) + 0.4f * std::sin(pPi * 5.4f) + 0.2f * std::sin(pPi * 8.9f);
                            val *= 0.6f;
                        }

                        sample += val;
                        phases[n] += incs[n];
                        if (phases[n] >= 1.0f) phases[n] -= 1.0f;
                    }
                    sample *= norm;

                    if (i < noiseSamples) {
                        float noiseEnv = 1.0f - ((float)i / noiseSamples);
                        float noise = (random.nextFloat() * 2.0f - 1.0f) * noiseEnv * 0.3f;
                        sample += noise;
                    }
                }

                float drive = (irType == 3) ? 1.0f : 4.0f;
                sample = std::tanh(sample * drive);

                float env = 0.0f;
                if (i < atkSamples) {
                    env = (float)i / atkSamples;
                }
                else {
                    float decPos = (float)(i - atkSamples) / decSamples;
                    env = std::pow(std::max(0.0f, 1.0f - decPos), 4.0f);
                }

                // ★修正2: コンボリューション出力が爆音(NaN)にならないよう、IRのゲインを強力に抑える
                float safeGain = 0.025f;
                outL[i] = sample * env * safeGain;
                outR[i] = sample * env * safeGain;
            }

            if (activeEngineIsA.load()) {
                convEngineB.loadImpulseResponse(std::move(tempIR), sr, juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::no, juce::dsp::Convolution::Normalise::no);
            }
            else {
                convEngineA.loadImpulseResponse(std::move(tempIR), sr, juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::no, juce::dsp::Convolution::Normalise::no);
            }

            triggerCrossfade();

            // ★ロード完了後に初めて状態をActiveにし、安全に音を通す
            state.store(LearnState::Active);
            });
    }

    void setParameters(float preCutoffHz, float postCutoffHz, float ottAmt, float mixVal)
    {
        preFilterL.setCutoffFrequency(std::clamp(preCutoffHz, 20.0f, 2000.0f)); preFilterR.setCutoffFrequency(std::clamp(preCutoffHz, 20.0f, 2000.0f));
        postFilterL.setCutoffFrequency(std::clamp(postCutoffHz, 20.0f, 2000.0f)); postFilterR.setCutoffFrequency(std::clamp(postCutoffHz, 20.0f, 2000.0f));
        ottAmount = std::clamp(ottAmt, 0.0f, 1.0f);
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
            // ★修正3: サンプル単位での滑らかなクロスフェード適用
            if (fadeVolA < fadeTargetA) fadeVolA = std::min(fadeVolA + fadeInc, fadeTargetA);
            else if (fadeVolA > fadeTargetA) fadeVolA = std::max(fadeVolA - fadeInc, fadeTargetA);
            if (fadeVolB < fadeTargetB) fadeVolB = std::min(fadeVolB + fadeInc, fadeTargetB);
            else if (fadeVolB > fadeTargetB) fadeVolB = std::max(fadeVolB - fadeInc, fadeTargetB);

            float currentWetL = wAL[i] * fadeVolA + wBL[i] * fadeVolB;
            float currentWetR = wAR[i] * fadeVolA + wBR[i] * fadeVolB;

            // ★修正4: コンボリューション後の異常共鳴からシステムを保護するセーフティ・クリッパー
            currentWetL = std::tanh(currentWetL);
            currentWetR = std::tanh(currentWetR);

            currentWetL = postFilterL.processSample(0, currentWetL);
            currentWetR = postFilterR.processSample(1, currentWetR);

            outL[i] = inL[i] * (1.0f - mix) + currentWetL * mix;
            outR[i] = inR[i] * (1.0f - mix) + currentWetR * mix;
        }

        if (ottAmount > 0.001f) {
            juce::AudioBuffer<float> ottBuf;
            ottBuf.makeCopyOf(buffer);
            juce::dsp::AudioBlock<float> outBlock(ottBuf);
            juce::dsp::ProcessContextReplacing<float> outCtx(outBlock);
            ottCompressor.process(outCtx);

            float mkup = 5.0f;
            for (int i = 0; i < numSamples; ++i) {
                float oL = outL[i] * (1.0f - ottAmount) + (ottBuf.getSample(0, i) * mkup * ottAmount);
                float oR = outR[i] * (1.0f - ottAmount) + (ottBuf.getSample(1, i) * mkup * ottAmount);
                // 最終出力も確実に保護
                outL[i] = std::tanh(oL);
                outR[i] = std::tanh(oR);
            }
        }
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
    juce::dsp::Compressor<float> ottCompressor;
    float ottAmount = 0.0f;

    std::atomic<bool> activeEngineIsA{ true };
    float fadeVolA = 1.0f, fadeVolB = 0.0f, fadeTargetA = 1.0f, fadeTargetB = 0.0f, mix = 0.0f;

    void triggerCrossfade() {
        if (activeEngineIsA.load()) { activeEngineIsA.store(false); fadeTargetA = 0.0f; fadeTargetB = 1.0f; }
        else { activeEngineIsA.store(true); fadeTargetA = 1.0f; fadeTargetB = 0.0f; }
    }
};