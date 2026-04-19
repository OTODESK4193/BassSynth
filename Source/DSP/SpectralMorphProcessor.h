// ==============================================================================
// Source/DSP/SpectralMorphProcessor.h
// ==============================================================================
#pragma once
#include <JuceHeader.h>
#include <array>
#include <complex>
#include <algorithm>

/**
 * @class SpectralMorphProcessor
 * @brief リアルタイムSTFTを用いた周波数領域モーフィングエンジン（高解像度・アンチエイリアス対応版）
 */
class SpectralMorphProcessor
{
public:
    SpectralMorphProcessor() : forwardFFT(fftOrder), inverseFFT(fftOrder), window(fftSize, juce::dsp::WindowingFunction<float>::hann)
    {
        reset();
    }

    void prepare(double sampleRate, int samplesPerBlock)
    {
        juce::ignoreUnused(sampleRate, samplesPerBlock);
        reset();
    }

    void reset()
    {
        inputFifoL.fill(0.0f); inputFifoR.fill(0.0f);
        outputFifoL.fill(0.0f); outputFifoR.fill(0.0f);
        fftWorkspaceL.fill(0.0f); fftWorkspaceR.fill(0.0f);
        fifoWritePos = 0;
        hopCounter = 0;
    }

    void process(juce::AudioBuffer<float>& buffer, int modeA, float amtA, float shiftA, int modeB, float amtB, float shiftB)
    {
        bool isSpectralA = (modeA >= 8 && modeA <= 13) && (std::abs(amtA) > 0.001f);
        bool isSpectralB = (modeB >= 8 && modeB <= 13) && (std::abs(amtB) > 0.001f);

        if (!isSpectralA && !isSpectralB) return;

        const int numSamples = buffer.getNumSamples();
        auto* channelDataL = buffer.getWritePointer(0);
        auto* channelDataR = buffer.getWritePointer(1);

        for (int i = 0; i < numSamples; ++i)
        {
            inputFifoL[(size_t)fifoWritePos] = channelDataL[i];
            inputFifoR[(size_t)fifoWritePos] = channelDataR[i];

            channelDataL[i] = outputFifoL[(size_t)fifoWritePos];
            channelDataR[i] = outputFifoR[(size_t)fifoWritePos];

            outputFifoL[(size_t)fifoWritePos] = 0.0f;
            outputFifoR[(size_t)fifoWritePos] = 0.0f;

            fifoWritePos++;
            if (fifoWritePos >= fifoSize) fifoWritePos = 0;

            hopCounter++;
            if (hopCounter >= hopSize)
            {
                hopCounter = 0;
                // STFTのフレーム境界でのみパラメータを取得（フレーム間ティアリングノイズの防止）
                processSTFTFrame(modeA, amtA, shiftA, modeB, amtB, shiftB);
            }
        }
    }

    void processSingleCycleForDisplay(std::array<float, 512>& buffer, int modeA, float amtA, float shiftA, int modeB, float amtB, float shiftB)
    {
        bool isSpectralA = (modeA >= 8 && modeA <= 13) && (std::abs(amtA) > 0.001f);
        bool isSpectralB = (modeB >= 8 && modeB <= 13) && (std::abs(amtB) > 0.001f);

        if (isSpectralA || isSpectralB) {
            for (size_t i = 0; i < 512; ++i) displayWorkspace[i] = buffer[i];
            for (size_t i = 512; i < 1024; ++i) displayWorkspace[i] = 0.0f;

            displayFFT.performRealOnlyForwardTransform(displayWorkspace.data());

            size_t numBins = 256;
            for (size_t k = 1; k < numBins; ++k) {
                std::complex<float> c(displayWorkspace[k * 2], displayWorkspace[k * 2 + 1]);
                displayMag[k] = std::abs(c);
                displayPhase[k] = std::arg(c);
            }

            if (isSpectralA) applyMorphToMagnitude(displayMag.data(), displayTempMag.data(), modeA, amtA, shiftA, numBins);
            if (isSpectralB) applyMorphToMagnitude(displayMag.data(), displayTempMag.data(), modeB, amtB, shiftB, numBins);

            for (size_t k = 1; k < numBins; ++k) {
                std::complex<float> c = std::polar(displayMag[k], displayPhase[k]);
                displayWorkspace[k * 2] = c.real();
                displayWorkspace[k * 2 + 1] = c.imag();
            }

            displayFFT.performRealOnlyInverseTransform(displayWorkspace.data());

            for (size_t i = 0; i < 512; ++i) buffer[i] = displayWorkspace[i];
        }

        // GUI描画時のみ：無条件のピークノーマライズ
        float peak = 1e-9f;
        for (size_t i = 0; i < 512; ++i) {
            peak = std::max(peak, std::abs(buffer[i]));
        }
        float scale = 1.0f / peak;
        for (size_t i = 0; i < 512; ++i) {
            buffer[i] *= scale;
        }
    }

private:
    static constexpr int fftOrder = 11;
    static constexpr size_t fftSize = 1 << fftOrder; // 2048
    static constexpr int hopSize = 512;              // 4x Overlap
    static constexpr int fifoSize = 4096;

    juce::dsp::FFT forwardFFT;
    juce::dsp::FFT inverseFFT;
    juce::dsp::WindowingFunction<float> window;

    std::array<float, fifoSize> inputFifoL, inputFifoR;
    std::array<float, fifoSize> outputFifoL, outputFifoR;
    std::array<float, fftSize * 2> fftWorkspaceL, fftWorkspaceR;
    std::array<float, fftSize / 2> magL, magR, phaseL, phaseR, tempMag;

    int fifoWritePos = 0;
    int hopCounter = 0;

    juce::dsp::FFT displayFFT{ 9 }; // 512 = 2^9
    std::array<float, 1024> displayWorkspace{};
    std::array<float, 256> displayMag{}, displayPhase{}, displayTempMag{};

    void processSTFTFrame(int modeA, float amtA, float shiftA, int modeB, float amtB, float shiftB)
    {
        int readPos = (fifoWritePos - (int)fftSize + fifoSize) % fifoSize;

        for (size_t i = 0; i < fftSize; ++i)
        {
            size_t idx = (size_t)((readPos + i) % fifoSize);
            fftWorkspaceL[i] = inputFifoL[idx];
            fftWorkspaceR[i] = inputFifoR[idx];
        }

        window.multiplyWithWindowingTable(fftWorkspaceL.data(), fftSize);
        window.multiplyWithWindowingTable(fftWorkspaceR.data(), fftSize);

        for (size_t i = fftSize; i < fftSize * 2; ++i)
        {
            fftWorkspaceL[i] = 0.0f;
            fftWorkspaceR[i] = 0.0f;
        }

        forwardFFT.performRealOnlyForwardTransform(fftWorkspaceL.data());
        forwardFFT.performRealOnlyForwardTransform(fftWorkspaceR.data());

        size_t numBins = fftSize / 2;
        for (size_t k = 1; k < numBins; ++k)
        {
            std::complex<float> cL(fftWorkspaceL[k * 2], fftWorkspaceL[k * 2 + 1]);
            magL[k] = std::abs(cL);
            phaseL[k] = std::arg(cL);

            std::complex<float> cR(fftWorkspaceR[k * 2], fftWorkspaceR[k * 2 + 1]);
            magR[k] = std::abs(cR);
            phaseR[k] = std::arg(cR);
        }

        if (modeA >= 8 && modeA <= 13) {
            applyMorphToMagnitude(magL.data(), tempMag.data(), modeA, amtA, shiftA, numBins);
            applyMorphToMagnitude(magR.data(), tempMag.data(), modeA, amtA, shiftA, numBins);
        }
        if (modeB >= 8 && modeB <= 13) {
            applyMorphToMagnitude(magL.data(), tempMag.data(), modeB, amtB, shiftB, numBins);
            applyMorphToMagnitude(magR.data(), tempMag.data(), modeB, amtB, shiftB, numBins);
        }

        for (size_t k = 1; k < numBins; ++k)
        {
            std::complex<float> cL = std::polar(magL[k], phaseL[k]);
            fftWorkspaceL[k * 2] = cL.real();
            fftWorkspaceL[k * 2 + 1] = cL.imag();

            std::complex<float> cR = std::polar(magR[k], phaseR[k]);
            fftWorkspaceR[k * 2] = cR.real();
            fftWorkspaceR[k * 2 + 1] = cR.imag();
        }

        inverseFFT.performRealOnlyInverseTransform(fftWorkspaceL.data());
        inverseFFT.performRealOnlyInverseTransform(fftWorkspaceR.data());

        window.multiplyWithWindowingTable(fftWorkspaceL.data(), fftSize);
        window.multiplyWithWindowingTable(fftWorkspaceR.data(), fftSize);

        float gainCorrection = 1.0f / 2.0f;

        for (size_t i = 0; i < fftSize; ++i)
        {
            size_t writeIdx = (size_t)((readPos + i) % fifoSize);
            outputFifoL[writeIdx] += fftWorkspaceL[i] * gainCorrection;
            outputFifoR[writeIdx] += fftWorkspaceR[i] * gainCorrection;
        }
    }

    void applyMorphToMagnitude(float* mag, float* tMag, int mode, float amount, float shift, size_t numBins)
    {
        if (mode == 8) // Smear -> Fractional Interpolation化
        {
            float floatRadius = std::abs(amount) * 15.0f + 0.001f;
            int r0 = (int)floatRadius;
            int r1 = r0 + 1;
            float rFrac = floatRadius - r0;
            float centerK = (shift + 1.0f) * 0.5f * (float)numBins;

            for (size_t k = 1; k < numBins; ++k) {
                float distance = std::abs((float)k - centerK) / (float)numBins;
                float localRadiusF = std::max(0.0f, (1.0f - distance) * floatRadius);
                int lr0 = (int)localRadiusF;
                int lr1 = lr0 + 1;
                float lrFrac = localRadiusF - lr0;

                if (localRadiusF < 0.5f) {
                    tMag[k] = mag[k];
                    continue;
                }

                float sum0 = 0.0f, sum1 = 0.0f;
                int count0 = 0, count1 = 0;

                for (int r = -lr1; r <= lr1; ++r) {
                    size_t idx = (size_t)std::clamp((int)k + r, 1, (int)numBins - 1);
                    float val = mag[idx];
                    sum1 += val; count1++;
                    if (std::abs(r) <= lr0) { sum0 += val; count0++; }
                }
                float val0 = sum0 / (float)std::max(1, count0);
                float val1 = sum1 / (float)std::max(1, count1);
                tMag[k] = val0 * (1.0f - lrFrac) + val1 * lrFrac; // 小数補間でジッパーノイズ消去
            }
            for (size_t k = 1; k < numBins; ++k) mag[k] = tMag[k];
        }
        else if (mode == 9) // Vocode 
        {
            float formants[4] = { 0.1f, 0.3f, 0.6f, 0.8f };
            float s = shift * 0.4f;

            for (size_t k = 1; k < numBins; ++k) {
                float env = 0.0f;
                float p = (float)k / (float)(numBins - 1);
                for (int f = 0; f < 4; ++f) {
                    float pos = std::clamp(formants[f] + s, 0.0f, 1.0f);
                    env += std::exp(-std::abs(p - pos) * 20.0f);
                }
                float targetMag = mag[k] * env * 2.0f;
                mag[k] = mag[k] * (1.0f - std::abs(amount)) + targetMag * std::abs(amount);
            }
        }
        else if (mode == 10) // Stretch
        {
            float stretch = (amount >= 0.0f) ? (1.0f + amount * 2.0f) : (1.0f / (1.0f + std::abs(amount) * 2.0f));
            float center = (shift + 1.0f) * 0.5f * (float)numBins;

            for (size_t k = 1; k < numBins; ++k) {
                float srcK = center + ((float)k - center) / stretch;
                int k0 = (int)srcK;
                float frac = srcK - k0;
                int k1 = std::min(k0 + 1, (int)numBins - 1);
                k0 = std::clamp(k0, 1, (int)numBins - 1);
                k1 = std::clamp(k1, 1, (int)numBins - 1);
                tMag[k] = mag[(size_t)k0] * (1.0f - frac) + mag[(size_t)k1] * frac;
            }
            for (size_t k = 1; k < numBins; ++k) mag[k] = tMag[k];
        }
        else if (mode == 11) // SpecCut -> Fractional Edge (Anti-Aliasing)
        {
            float cutFloat = std::abs(amount) * (float)numBins;
            bool isHighCut = (amount > 0.0f);
            float reso = std::max(0.0f, shift) * 4.0f;
            float rollOffWidth = 8.0f; // 滑らかにカットするビン幅

            for (size_t k = 1; k < numBins; ++k) {
                float dist = isHighCut ? ((float)k - ((float)numBins - cutFloat)) : (cutFloat - (float)k);

                // カットのエッジを滑らかにする (Anti-aliasing)
                float gain = 1.0f;
                if (dist > 0.0f) {
                    if (dist > rollOffWidth) gain = 0.0f;
                    else gain = 0.5f * (1.0f + std::cos(juce::MathConstants<float>::pi * (dist / rollOffWidth)));
                }

                mag[k] *= gain;

                // レゾナンス（境界付近の強調）
                if (reso > 0.0f && dist <= 0.0f && dist > -20.0f) {
                    float rEnv = 1.0f - (std::abs(dist) / 20.0f);
                    mag[k] *= (1.0f + rEnv * reso);
                }
            }
        }
        else if (mode == 12) // Shepard
        {
            float wCenter = (shift + 1.0f) * 0.5f;

            for (size_t k = 1; k < numBins; ++k) {
                float logPos = std::log2f((float)k) / std::log2f((float)(numBins - 1));
                float shifted = std::fmod(logPos + (amount * 5.0f), 1.0f);
                if (shifted < 0.0f) shifted += 1.0f;

                float windowEnv = std::exp(-std::pow((logPos - wCenter) * 4.0f, 2.0f));
                mag[k] *= std::sin(shifted * juce::MathConstants<float>::pi) * windowEnv * 1.5f;
            }
        }
        else if (mode == 13) // Comb
        {
            float density = 2.0f + std::abs(amount) * 30.0f;
            float pOffset = shift * juce::MathConstants<float>::twoPi;

            for (size_t k = 1; k < numBins; ++k) {
                float combMultiplier = 0.5f + 0.5f * std::cos((float)k * density * juce::MathConstants<float>::pi / (float)numBins + pOffset);
                mag[k] *= combMultiplier;
            }
        }
    }
};