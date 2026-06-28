#pragma once
#include <JuceHeader.h>
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>

//==============================================================================
// SimpleReverb (Diffusion Cloud + Comb tail)  -- ported & extended for BassSynth
//   * 並列コムフィルタ(フィードバック)で長い残響(ロングリバーブ)を生成し、
//     その後に直列オールパスで拡散(Liquid感)を付与する。
//   * Decay で残響長(コムのフィードバック量)を、Size でオールパス拡散量を調整。
//==============================================================================
class SimpleReverb
{
public:
    static constexpr int NUM_COMB = 4;

    SimpleReverb() = default;

    void prepare(double sampleRate)
    {
        currentSampleRate = std::max(1.0, sampleRate);
        float scale = (float)currentSampleRate / 44100.0f;

        // --- 直列オールパス(拡散) ---
        const int sizesL[] = { 223, 337, 461, 593, 751, 919 };
        const int sizesR[] = { 227, 347, 463, 599, 757, 929 };
        numStages = 6;
        buffersL.resize(numStages); buffersR.resize(numStages);
        indicesL.assign(numStages, 0); indicesR.assign(numStages, 0);
        for (int i = 0; i < numStages; ++i) {
            buffersL[i].assign(sizesL[i], 0.0f);
            buffersR[i].assign(sizesR[i], 0.0f);
        }

        // --- 並列コムフィルタ(長い残響の生成) ---
        const int combBaseL[NUM_COMB] = { 1557, 1617, 1491, 1422 };
        const int combBaseR[NUM_COMB] = { 1617, 1557, 1422, 1491 }; // 左右で入れ替えて広がりを確保
        for (int c = 0; c < NUM_COMB; ++c) {
            combBufL[c].assign(std::max(1, (int)(combBaseL[c] * scale)), 0.0f);
            combBufR[c].assign(std::max(1, (int)(combBaseR[c] * scale)), 0.0f);
            combIdxL[c] = 0; combIdxR[c] = 0;
            combDampL[c] = 0.0f; combDampR[c] = 0.0f;
        }
    }

    // decay: 0..1 で残響長(短〜十数秒)を制御
    void setParameters(float mixV, float size, float widthV, float decay)
    {
        this->mix = juce::jlimit(0.0f, 1.0f, mixV);
        this->apFeedback = 0.5f + (juce::jlimit(0.0f, 1.0f, size) * 0.2f); // 0.5..0.7 拡散
        this->width = juce::jlimit(0.0f, 1.0f, widthV);

        float d = juce::jlimit(0.0f, 1.0f, decay);
        this->combFb = 0.70f + d * 0.285f; // 0.70..0.985 (上限0.99未満で安定)
        this->damp = 0.20f;                // 高域の自然な減衰(固定)
    }

    void processBlock(float* left, float* right, int numSamples)
    {
        if (mix <= 0.001f) return;

        // 残響長が変わっても体感音量がほぼ一定になるようにする正規化係数（必要なら微調整可）
        const float combInGain = 0.15f;
        const float wetMakeup = (1.0f - combFb) * 3.0f;

        for (int n = 0; n < numSamples; ++n)
        {
            float inL = left[n];
            float inR = (right) ? right[n] : inL;

            // ステレオ拡張(プリ)
            float mono = (inL + inR) * 0.5f;
            float side = (inL - inR) * 0.5f;
            float wL = mono + side * (1.0f + width * 0.5f);
            float wR = mono - side * (1.0f + width * 0.5f);

            // 並列コムフィルタ(長い残響)
            float cInL = wL * combInGain;
            float cInR = wR * combInGain;
            float sumL = 0.0f, sumR = 0.0f;
            for (int c = 0; c < NUM_COMB; ++c) {
                sumL += processComb(cInL, combBufL[c], combIdxL[c], combFb, combDampL[c], damp);
                sumR += processComb(cInR, combBufR[c], combIdxR[c], combFb, combDampR[c], damp);
            }
            float outL = sumL * wetMakeup;
            float outR = sumR * wetMakeup;

            // 直列オールパス(拡散)
            for (int i = 0; i < numStages; ++i) {
                outL = processAllpass(outL, buffersL[i], indicesL[i], apFeedback);
                outR = processAllpass(outR, buffersR[i], indicesR[i], apFeedback);
            }

            left[n] = inL * (1.0f - mix * 0.5f) + outL * mix;
            if (right)
                right[n] = inR * (1.0f - mix * 0.5f) + outR * mix;
        }
    }

private:
    double currentSampleRate = 44100.0;

    int numStages = 0;
    std::vector<std::vector<float>> buffersL, buffersR;
    std::vector<int> indicesL, indicesR;

    std::array<std::vector<float>, NUM_COMB> combBufL, combBufR;
    std::array<int, NUM_COMB> combIdxL{}, combIdxR{};
    std::array<float, NUM_COMB> combDampL{}, combDampR{};

    float mix = 0.0f;
    float apFeedback = 0.5f;
    float width = 0.5f;
    float combFb = 0.70f;
    float damp = 0.2f;

    inline float processComb(float input, std::vector<float>& buf, int& index, float fb, float& dampState, float dmp)
    {
        int size = (int)buf.size();
        float out = buf[index];
        dampState = out * (1.0f - dmp) + dampState * dmp; // フィードバック経路の1次LPF
        if (std::abs(dampState) < 1.0e-15f) dampState = 0.0f;
        float w = input + dampState * fb;
        if (std::abs(w) < 1.0e-15f) w = 0.0f;
        buf[index] = w;
        index++;
        if (index >= size) index = 0;
        return out;
    }

    inline float processAllpass(float input, std::vector<float>& buffer, int& index, float fb)
    {
        int size = (int)buffer.size();
        float bufOut = buffer[index];

        float inVal = input + (bufOut * -fb);
        float outVal = bufOut + (inVal * fb);

        if (std::abs(inVal) < 1.0e-15f) inVal = 0.0f;

        buffer[index] = inVal;

        index++;
        if (index >= size) index = 0;

        return outVal;
    }
};
