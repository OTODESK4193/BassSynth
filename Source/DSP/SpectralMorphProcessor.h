// ==============================================================================
// Source/DSP/SpectralMorphProcessor.h
// ==============================================================================
#pragma once
#include <JuceHeader.h>
#include <array>
#include <complex>
#include <algorithm>

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

    void process(juce::AudioBuffer<float>& buffer, int mA, float aA, float sA, int mB, float aB, float sB, int mC, float aC, float sC)
    {
        bool isSpA = (mA >= 8 && mA <= 13) && (std::abs(aA) > 0.001f);
        bool isSpB = (mB >= 8 && mB <= 13) && (std::abs(aB) > 0.001f);
        bool isSpC = (mC >= 8 && mC <= 13) && (std::abs(aC) > 0.001f);
        if (!isSpA && !isSpB && !isSpC) return;

        const int numSamples = buffer.getNumSamples();
        auto* chL = buffer.getWritePointer(0);
        auto* chR = buffer.getWritePointer(1);

        for (int i = 0; i < numSamples; ++i)
        {
            inputFifoL[(size_t)fifoWritePos] = chL[i];
            inputFifoR[(size_t)fifoWritePos] = chR[i];
            chL[i] = outputFifoL[(size_t)fifoWritePos];
            chR[i] = outputFifoR[(size_t)fifoWritePos];
            outputFifoL[(size_t)fifoWritePos] = 0.0f;
            outputFifoR[(size_t)fifoWritePos] = 0.0f;

            fifoWritePos = (fifoWritePos + 1) % fifoSize;
            if (++hopCounter >= hopSize)
            {
                hopCounter = 0;
                processSTFTFrame(mA, aA, sA, mB, aB, sB, mC, aC, sC);
            }
        }
    }

    void processSingleCycleForDisplay(std::array<float, 512>& buffer, int mA, float aA, float sA, int mB, float aB, float sB, int mC, float aC, float sC)
    {
        bool isSpA = (mA >= 8 && mA <= 13) && (std::abs(aA) > 0.001f);
        bool isSpB = (mB >= 8 && mB <= 13) && (std::abs(aB) > 0.001f);
        bool isSpC = (mC >= 8 && mC <= 13) && (std::abs(aC) > 0.001f);

        if (isSpA || isSpB || isSpC) {
            for (size_t i = 0; i < 512; ++i) dWS[i] = buffer[i];
            for (size_t i = 512; i < 1024; ++i) dWS[i] = 0.0f;
            dFFT.performRealOnlyForwardTransform(dWS.data());
            for (size_t k = 1; k < 256; ++k) {
                std::complex<float> c(dWS[k * 2], dWS[k * 2 + 1]);
                dMag[k] = std::abs(c); dPhase[k] = std::arg(c);
            }

            if (isSpA) applyMorphToMagnitude(dMag.data(), dTMag.data(), mA, aA, sA, 256);
            if (isSpB) applyMorphToMagnitude(dMag.data(), dTMag.data(), mB, aB, sB, 256);
            if (isSpC) applyMorphToMagnitude(dMag.data(), dTMag.data(), mC, aC, sC, 256);

            for (size_t k = 1; k < 256; ++k) {
                std::complex<float> c = std::polar(dMag[k], dPhase[k]);
                dWS[k * 2] = c.real(); dWS[k * 2 + 1] = c.imag();
            }
            dFFT.performRealOnlyInverseTransform(dWS.data());
            for (size_t i = 0; i < 512; ++i) buffer[i] = dWS[i];
        }
        float peak = 1e-9f;
        for (size_t i = 0; i < 512; ++i) peak = std::max(peak, std::abs(buffer[i]));
        float scale = 1.0f / peak;
        for (size_t i = 0; i < 512; ++i) buffer[i] *= scale;
    }

private:
    static constexpr int fftOrder = 11;
    static constexpr size_t fftSize = 2048;
    static constexpr int hopSize = 512; // 4倍オーバーラップ（超高音質）
    static constexpr int fifoSize = 4096;

    juce::dsp::FFT forwardFFT, inverseFFT;
    juce::dsp::WindowingFunction<float> window;
    std::array<float, fifoSize> inputFifoL, inputFifoR, outputFifoL, outputFifoR;
    std::array<float, fftSize * 2> fftWorkspaceL, fftWorkspaceR;
    std::array<float, fftSize / 2> magL, magR, phaseL, phaseR, tempMag;
    int fifoWritePos = 0, hopCounter = 0;
    juce::dsp::FFT dFFT{ 9 };
    std::array<float, 1024> dWS{};
    std::array<float, 256> dMag{}, dPhase{}, dTMag{};

    void processSTFTFrame(int mA, float aA, float sA, int mB, float aB, float sB, int mC, float aC, float sC)
    {
        int readPos = (fifoWritePos - (int)fftSize + fifoSize) % fifoSize;
        for (size_t i = 0; i < fftSize; ++i) {
            size_t idx = (size_t)((readPos + i) % fifoSize);
            fftWorkspaceL[i] = inputFifoL[idx]; fftWorkspaceR[i] = inputFifoR[idx];
        }
        window.multiplyWithWindowingTable(fftWorkspaceL.data(), fftSize);
        window.multiplyWithWindowingTable(fftWorkspaceR.data(), fftSize);
        for (size_t i = fftSize; i < fftSize * 2; ++i) { fftWorkspaceL[i] = 0; fftWorkspaceR[i] = 0; }

        forwardFFT.performRealOnlyForwardTransform(fftWorkspaceL.data());
        forwardFFT.performRealOnlyForwardTransform(fftWorkspaceR.data());

        size_t nB = fftSize / 2;
        for (size_t k = 1; k < nB; ++k) {
            std::complex<float> cL(fftWorkspaceL[k * 2], fftWorkspaceL[k * 2 + 1]);
            magL[k] = std::abs(cL); phaseL[k] = std::arg(cL);
            std::complex<float> cR(fftWorkspaceR[k * 2], fftWorkspaceR[k * 2 + 1]);
            magR[k] = std::abs(cR); phaseR[k] = std::arg(cR);
        }

        if (mA >= 8 && mA <= 13) { applyMorphToMagnitude(magL.data(), tempMag.data(), mA, aA, sA, nB); applyMorphToMagnitude(magR.data(), tempMag.data(), mA, aA, sA, nB); }
        if (mB >= 8 && mB <= 13) { applyMorphToMagnitude(magL.data(), tempMag.data(), mB, aB, sB, nB); applyMorphToMagnitude(magR.data(), tempMag.data(), mB, aB, sB, nB); }
        if (mC >= 8 && mC <= 13) { applyMorphToMagnitude(magL.data(), tempMag.data(), mC, aC, sC, nB); applyMorphToMagnitude(magR.data(), tempMag.data(), mC, aC, sC, nB); }

        for (size_t k = 1; k < nB; ++k) {
            std::complex<float> cL = std::polar(magL[k], phaseL[k]);
            fftWorkspaceL[k * 2] = cL.real(); fftWorkspaceL[k * 2 + 1] = cL.imag();
            std::complex<float> cR = std::polar(magR[k], phaseR[k]);
            fftWorkspaceR[k * 2] = cR.real(); fftWorkspaceR[k * 2 + 1] = cR.imag();
        }

        inverseFFT.performRealOnlyInverseTransform(fftWorkspaceL.data());
        inverseFFT.performRealOnlyInverseTransform(fftWorkspaceR.data());
        window.multiplyWithWindowingTable(fftWorkspaceL.data(), fftSize);
        window.multiplyWithWindowingTable(fftWorkspaceR.data(), fftSize);

        float gainCorrection = 1.0f / 2.0f; // 4x overlap correction
        for (size_t i = 0; i < fftSize; ++i) {
            size_t wIdx = (size_t)((readPos + i) % fifoSize);
            outputFifoL[wIdx] += fftWorkspaceL[i] * gainCorrection;
            outputFifoR[wIdx] += fftWorkspaceR[i] * gainCorrection;
        }
    }

    void applyMorphToMagnitude(float* mag, float* tMag, int mode, float amt, float shift, size_t nB)
    {
        if (mode == 8) { // Smear
            float fR = std::abs(amt) * 15.0f + 0.001f; int r0 = (int)fR; float rF = fR - r0;
            float cK = (shift + 1.0f) * 0.5f * (float)nB;
            for (size_t k = 1; k < nB; ++k) {
                float dist = std::abs((float)k - cK) / (float)nB;
                float lRF = std::max(0.0f, (1.0f - dist) * fR); int lr0 = (int)lRF; float lrF = lRF - lr0;
                if (lRF < 0.5f) { tMag[k] = mag[k]; continue; }
                float s0 = 0, s1 = 0; int c0 = 0, c1 = 0;
                for (int r = -(lr0 + 1); r <= (lr0 + 1); ++r) {
                    size_t idx = (size_t)std::clamp((int)k + r, 1, (int)nB - 1);
                    s1 += mag[idx]; c1++;
                    if (std::abs(r) <= lr0) { s0 += mag[idx]; c0++; }
                }
                tMag[k] = (s0 / (float)std::max(1, c0)) * (1.0f - lrF) + (s1 / (float)std::max(1, c1)) * lrF;
            }
            for (size_t k = 1; k < nB; ++k) mag[k] = tMag[k];
        }
        else if (mode == 9) { // Vocode (A-I-U-E-O Formant Filter)
            float s = shift;
            int seq[5];
            if (s >= 0.0f) { seq[0] = 0; seq[1] = 1; seq[2] = 2; seq[3] = 3; seq[4] = 4; } // A, I, U, E, O
            else { seq[0] = 0; seq[1] = 3; seq[2] = 1; seq[3] = 4; seq[4] = 2; s = -s; } // A, E, I, O, U

            float pos = s * 4.0f;
            int i0 = std::clamp((int)pos, 0, 3);
            int i1 = i0 + 1;
            float frac = pos - i0;

            const float fmts[5][3] = {
                { 32.0f, 55.0f, 120.0f }, // A
                { 14.0f, 102.0f, 139.0f }, // I
                { 14.0f, 41.0f, 116.0f }, // U
                { 18.0f, 74.0f, 111.0f }, // E
                { 18.0f, 37.0f, 120.0f }  // O
            };

            float curF[3];
            for (int j = 0; j < 3; ++j) curF[j] = fmts[seq[i0]][j] * (1.0f - frac) + fmts[seq[i1]][j] * frac;

            float scale = (float)nB / 1024.0f;

            for (size_t k = 1; k < nB; ++k) {
                float env = 0.001f;
                for (int j = 0; j < 3; ++j) {
                    float target = curF[j] * scale;
                    float width = target * 0.15f + 4.0f * scale;
                    float dist = std::abs((float)k - target);
                    env += std::exp(-(dist * dist) / (width * width));
                }
                mag[k] = mag[k] * (1.0f - std::abs(amt)) + (mag[k] * env * 4.0f) * std::abs(amt);
            }
        }
        else if (mode == 10) { // Stretch (Sweet Spot Mapping)
            float mappedAmt = amt * 0.10f;
            float str = 1.0f + mappedAmt;
            if (str < 0.1f) str = 0.1f;

            float cnt = (shift + 1.0f) * 0.5f * (float)nB;
            for (size_t k = 1; k < nB; ++k) {
                float src = cnt + ((float)k - cnt) / str;
                int k0 = std::clamp((int)src, 1, (int)nB - 1);
                float f = src - (int)src;
                int k1 = std::min(k0 + 1, (int)nB - 1);
                tMag[k] = mag[k0] * (1.0f - f) + mag[k1] * f;
            }
            for (size_t k = 1; k < nB; ++k) mag[k] = tMag[k];
        }
        else if (mode == 11) { // SpecCut (Perfect Low/High Pass logic)
            // UI Amt (-1.0 to 1.0) を (-0.08 to 1.0) にスケーリング
            float mappedAmt = amt;
            if (mappedAmt < 0.0f) mappedAmt *= 0.08f;

            float absAmt = std::abs(mappedAmt);
            if (absAmt < 0.001f) return;

            bool isHighCut = (mappedAmt > 0.0f);
            float cutBin = isHighCut ? (float)nB * (1.0f - absAmt) : (float)nB * absAmt;

            float reso = std::abs(shift) * 4.0f;
            float rW = 10.0f * ((float)nB / 1024.0f);
            if (rW < 2.0f) rW = 2.0f;

            for (size_t k = 1; k < nB; ++k) {
                float dist = isHighCut ? ((float)k - cutBin) : (cutBin - (float)k);
                float g = 1.0f;
                if (dist > 0.0f) {
                    g = (dist > rW) ? 0.0f : 0.5f * (1.0f + std::cos(juce::MathConstants<float>::pi * (dist / rW)));
                }
                mag[k] *= g;

                if (reso > 0.0f && std::abs(dist) < rW * 2.0f && dist <= 0.0f) {
                    mag[k] *= (1.0f + (1.0f - std::abs(dist) / (rW * 2.0f)) * reso);
                }
            }
        }
        else if (mode == 12) { // Shepard
            float wC = (shift + 1.0f) * 0.5f;
            for (size_t k = 1; k < nB; ++k) {
                float lP = std::log2f((float)k) / std::log2f((float)(nB - 1));
                float sh = std::fmod(lP + (amt * 5.0f), 1.0f); if (sh < 0) sh += 1.0f;
                mag[k] *= std::sin(sh * 3.14159f) * std::exp(-std::pow((lP - wC) * 4.0f, 2.0f)) * 1.5f;
            }
        }
        else if (mode == 13) { // Comb
            float d = 2.0f + std::abs(amt) * 30.0f, pO = shift * 6.283185f;
            for (size_t k = 1; k < nB; ++k) mag[k] *= (0.5f + 0.5f * std::cos((float)k * d * 3.14159f / (float)nB + pO));
        }
    }
};