// ==============================================================================
// Source/DSP/DualFilterEngine.h
// ==============================================================================
#pragma once
#include <JuceHeader.h>
#include "LadderFilter.h"

class DualFilterEngine
{
public:
    enum FilterType { LPF = 0, HPF, BPF, Notch, Comb, LadderLPF };
    enum Routing { Serial = 0, Parallel };

    DualFilterEngine() {
        filterA.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        filterB.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    }

    void prepare(double sr, int samplesPerBlock) {
        sampleRate = sr;
        juce::dsp::ProcessSpec spec{ sr, static_cast<juce::uint32>(samplesPerBlock), 2 };

        filterA.prepare(spec);
        filterB.prepare(spec);

        ladderA_L.prepare(sr); ladderA_R.prepare(sr);
        ladderB_L.prepare(sr); ladderB_R.prepare(sr);

        combDelayA.prepare(spec); combDelayA.setMaximumDelayInSamples(static_cast<int>(sr * 0.05));
        combDelayB.prepare(spec); combDelayB.setMaximumDelayInSamples(static_cast<int>(sr * 0.05));
    }

    void setFilterA(int type, float cutoff, float res) {
        typeA = static_cast<FilterType>(type);
        freqA = juce::jlimit(20.0f, 20000.0f, cutoff);
        resA = juce::jlimit(0.0f, 1.0f, res);

        if (typeA != LadderLPF) {
            updateSVF(filterA, typeA, freqA, resA);
        }
        else {
            ladderA_L.setParameters(freqA, resA);
            ladderA_R.setParameters(freqA, resA);
        }
    }

    void setFilterB(int type, float cutoff, float res) {
        typeB = static_cast<FilterType>(type);
        freqB = juce::jlimit(20.0f, 20000.0f, cutoff);
        resB = juce::jlimit(0.0f, 1.0f, res);

        if (typeB != LadderLPF) {
            updateSVF(filterB, typeB, freqB, resB);
        }
        else {
            ladderB_L.setParameters(freqB, resB);
            ladderB_R.setParameters(freqB, resB);
        }
    }

    void setRouting(int routingMode, float mixVal) {
        currentRouting = static_cast<Routing>(routingMode);
        mix = juce::jlimit(0.0f, 1.0f, mixVal);
    }

    void processStereo(float& inL, float& inR) {
        // Filter A
        float aL = processSingle(0, inL, typeA, filterA, ladderA_L, combDelayA, freqA, resA);
        float aR = processSingle(1, inR, typeA, filterA, ladderA_R, combDelayA, freqA, resA);

        if (currentRouting == Serial) {
            // 直列: Aの出力をBへ (MixはBのWet量)
            float bL = processSingle(0, aL, typeB, filterB, ladderB_L, combDelayB, freqB, resB);
            float bR = processSingle(1, aR, typeB, filterB, ladderB_R, combDelayB, freqB, resB);
            inL = aL * (1.0f - mix) + bL * mix;
            inR = aR * (1.0f - mix) + bR * mix;
        }
        else {
            // 並列: 原音をBにも入れ、AとBの結果をブレンド
            float bL = processSingle(0, inL, typeB, filterB, ladderB_L, combDelayB, freqB, resB);
            float bR = processSingle(1, inR, typeB, filterB, ladderB_R, combDelayB, freqB, resB);

            inL = aL * (1.0f - mix) + bL * mix;
            inR = aR * (1.0f - mix) + bR * mix;
        }
    }

private:
    double sampleRate{ 44100.0 };
    juce::dsp::StateVariableTPTFilter<float> filterA, filterB;
    LadderFilter ladderA_L, ladderA_R, ladderB_L, ladderB_R;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> combDelayA, combDelayB;

    FilterType typeA{ LPF }, typeB{ LPF };
    Routing currentRouting{ Serial };
    float freqA{ 20000.0f }, resA{ 0.0f }, freqB{ 20000.0f }, resB{ 0.0f }, mix{ 0.5f };

    // const 化および Notch の BPF 割り当てロジック
    void updateSVF(juce::dsp::StateVariableTPTFilter<float>& f, FilterType t, float freq, float r) const {
        if (t == LPF) f.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        else if (t == HPF) f.setType(juce::dsp::StateVariableTPTFilterType::highpass);
        else if (t == BPF || t == Notch) f.setType(juce::dsp::StateVariableTPTFilterType::bandpass); // Notch時は内部をBPFに設定

        f.setCutoffFrequency(freq);
        f.setResonance(juce::jmap(r, 0.0f, 1.0f, 0.707f, 10.0f));
    }

    float processSingle(int ch, float in, FilterType t,
        juce::dsp::StateVariableTPTFilter<float>& svf,
        LadderFilter& ladder,
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>& comb,
        float freq, float res)
    {
        if (t == LadderLPF) {
            return ladder.processSample(in);
        }
        else if (t == Comb) {
            float delaySamples = static_cast<float>(sampleRate) / freq;
            comb.setDelay(delaySamples);
            float delayed = comb.popSample(ch);
            float out = std::tanh(in + delayed * (res * 0.98f));
            comb.pushSample(ch, out);
            return out;
        }
        else if (t == Notch) {
            // TPTフィルターの特性を利用: Notch = Input - Bandpass
            return in - svf.processSample(ch, in);
        }
        else {
            return svf.processSample(ch, in);
        }
    }
};