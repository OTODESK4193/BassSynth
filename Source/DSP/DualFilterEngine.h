// ==============================================================================
// Source/DSP/DualFilterEngine.h
// ==============================================================================
#pragma once
#include <JuceHeader.h>

class DualFilterEngine
{
public:
    enum FilterType { LPF, HPF, BPF, Notch, Comb };
    enum Routing { Serial, Parallel };

    DualFilterEngine() {
        filterA.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        filterB.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    }

    void prepare(double sr, int samplesPerBlock) {
        sampleRate = sr;
        juce::dsp::ProcessSpec spec{ sr, (juce::uint32)samplesPerBlock, 1 };

        filterA.prepare(spec);
        filterB.prepare(spec);

        // Combフィルター用のディレイライン (最大50ms = 最低20Hzまで対応)
        combDelayA.prepare(spec); combDelayA.setMaximumDelayInSamples((int)(sr * 0.05));
        combDelayB.prepare(spec); combDelayB.setMaximumDelayInSamples((int)(sr * 0.05));
    }

    void setFilterA(int type, float cutoff, float res) {
        typeA = static_cast<FilterType>(type);
        freqA = juce::jlimit(20.0f, 20000.0f, cutoff);
        resA = juce::jlimit(0.0f, 0.99f, res); // Combのフィードバックにも使用
        updateSVF(filterA, typeA, freqA, resA);
    }

    void setFilterB(int type, float cutoff, float res) {
        typeB = static_cast<FilterType>(type);
        freqB = juce::jlimit(20.0f, 20000.0f, cutoff);
        resB = juce::jlimit(0.0f, 0.99f, res);
        updateSVF(filterB, typeB, freqB, resB);
    }

    void setRouting(int routingMode, float mixVal) {
        currentRouting = static_cast<Routing>(routingMode);
        mix = juce::jlimit(0.0f, 1.0f, mixVal);
    }

    float processSample(float input) {
        float outA = processSingle(input, typeA, filterA, combDelayA, freqA, resA);

        if (currentRouting == Serial) {
            // 直列: Aの出力をBに入力 (MixはAとBのバランスではなく、Bの適用量として機能)
            float outB = processSingle(outA, typeB, filterB, combDelayB, freqB, resB);
            return outA * (1.0f - mix) + outB * mix;
        }
        else {
            // 並列: 入力を分配し、AとBをMixでブレンド
            float outB = processSingle(input, typeB, filterB, combDelayB, freqB, resB);
            return outA * (1.0f - mix) + outB * mix;
        }
    }

private:
    double sampleRate = 44100.0;
    juce::dsp::StateVariableTPTFilter<float> filterA, filterB;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> combDelayA, combDelayB;

    FilterType typeA = LPF, typeB = LPF;
    Routing currentRouting = Serial;
    float freqA = 20000.0f, resA = 0.0f, freqB = 20000.0f, resB = 0.0f, mix = 0.5f;

    void updateSVF(juce::dsp::StateVariableTPTFilter<float>& f, FilterType t, float freq, float r) {
        if (t == LPF) f.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        else if (t == HPF) f.setType(juce::dsp::StateVariableTPTFilterType::highpass);
        else if (t == BPF) f.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
        else if (t == Notch) f.setType(juce::dsp::StateVariableTPTFilterType::bandstop);

        f.setCutoffFrequency(freq);
        f.setResonance(juce::jmap(r, 0.0f, 1.0f, 0.707f, 10.0f)); // Q値のマッピング
    }

    float processSingle(float in, FilterType t, juce::dsp::StateVariableTPTFilter<float>& svf,
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>& comb,
        float freq, float res) {
        if (t == Comb) {
            // Combフィルター計算 (Delay時間 = 1.0 / 周波数)
            float delaySamples = (float)sampleRate / freq;
            comb.setDelay(delaySamples);
            float delayed = comb.popSample(0);

            // フィードバックとソフトクリップ (発散防止)
            float out = in + delayed * (res * 0.95f);
            out = std::tanh(out);
            comb.pushSample(0, out);
            return out;
        }
        else {
            return svf.processSample(0, in);
        }
    }
};