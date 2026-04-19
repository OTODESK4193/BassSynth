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
 * @brief リアルタイムSTFT（短時間フーリエ変換）を用いた周波数領域モーフィングエンジン
 * * オーディオスレッド内で完結するよう、動的メモリ確保（new/malloc）を一切排除し、
 * 固定長配列（std::array）によるリングバッファとオーバーラップアド（OLA）を用いて実装されています。
 * 位相キャンセレーションを防ぐため、極座標系（振幅・位相）に変換して処理を行います。
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
    }

    // Morph Index: 9=Smear, 10=Vocode, 11=Stretch, 12=SpecCut, 13=Shepard
    void process(juce::AudioBuffer<float>& buffer, int modeA, float amtA, int modeB, float amtB)
    {
        // どちらのモードもSpectral(9〜13)でなければ何もしない（バイパス）
        bool isSpectralA = (modeA >= 9 && modeA <= 13) && (std::abs(amtA) > 0.001f);
        bool isSpectralB = (modeB >= 9 && modeB <= 13) && (std::abs(amtB) > 0.001f);

        if (!isSpectralA && !isSpectralB) return;

        const int numSamples = buffer.getNumSamples();
        auto* channelDataL = buffer.getWritePointer(0);
        auto* channelDataR = buffer.getWritePointer(1);

        for (int i = 0; i < numSamples; ++i)
        {
            // リングバッファに入力サンプルを書き込む
            inputFifoL[(size_t)fifoWritePos] = channelDataL[i];
            inputFifoR[(size_t)fifoWritePos] = channelDataR[i];

            // 出力バッファからサンプルを読み出し、元のバッファを上書き（レイテンシ分遅れます）
            channelDataL[i] = outputFifoL[(size_t)fifoWritePos];
            channelDataR[i] = outputFifoR[(size_t)fifoWritePos];

            // 出力したらその位置をクリア（オーバーラップ加算のため）
            outputFifoL[(size_t)fifoWritePos] = 0.0f;
            outputFifoR[(size_t)fifoWritePos] = 0.0f;

            fifoWritePos++;
            if (fifoWritePos >= fifoSize) fifoWritePos = 0;

            // ホップサイズに達したらFFT処理を実行
            hopCounter++;
            if (hopCounter >= hopSize)
            {
                hopCounter = 0;
                processSTFTFrame(modeA, amtA, modeB, amtB);
            }
        }
    }

    // --- GUI描画用の軽量1周期FFT関数 ---
    void processSingleCycleForDisplay(std::array<float, 512>& buffer, int modeA, float amtA, int modeB, float amtB)
    {
        bool isSpectralA = (modeA >= 9 && modeA <= 13) && (std::abs(amtA) > 0.001f);
        bool isSpectralB = (modeB >= 9 && modeB <= 13) && (std::abs(amtB) > 0.001f);

        if (!isSpectralA && !isSpectralB) return;

        // 1. 直近のデータをワークスペースにコピー (窓関数なし)
        for (size_t i = 0; i < 512; ++i) {
            displayWorkspace[i] = buffer[i];
        }
        for (size_t i = 512; i < 1024; ++i) {
            displayWorkspace[i] = 0.0f;
        }

        // 2. FFT
        displayFFT.performRealOnlyForwardTransform(displayWorkspace.data());

        // 3. 極座標変換
        size_t numBins = 256;
        for (size_t k = 1; k < numBins; ++k) {
            std::complex<float> c(displayWorkspace[k * 2], displayWorkspace[k * 2 + 1]);
            displayMag[k] = std::abs(c);
            displayPhase[k] = std::arg(c);
        }

        // 4. Morph適用 (汎用化した関数を使用)
        if (isSpectralA) applyMorphToMagnitude(displayMag.data(), displayTempMag.data(), modeA, amtA, numBins);
        if (isSpectralB) applyMorphToMagnitude(displayMag.data(), displayTempMag.data(), modeB, amtB, numBins);

        // 5. 直交座標再構築
        for (size_t k = 1; k < numBins; ++k) {
            std::complex<float> c = std::polar(displayMag[k], displayPhase[k]);
            displayWorkspace[k * 2] = c.real();
            displayWorkspace[k * 2 + 1] = c.imag();
        }

        // 6. 逆FFT
        displayFFT.performRealOnlyInverseTransform(displayWorkspace.data());

        // 7. ピークノーマライズ（はみ出し防止）して元のバッファに戻す
        float peak = 1e-9f;
        // まず最大振幅を探す
        for (size_t i = 0; i < 512; ++i) {
            peak = std::max(peak, std::abs(displayWorkspace[i]));
        }

        // 常に-1.0〜1.0の枠内に美しく収まるようにスケーリング係数を計算
        float scale = 1.0f / peak;

        for (size_t i = 0; i < 512; ++i) {
            buffer[i] = displayWorkspace[i] * scale;
        }
    }

private:
    // --- Audio Thread STFT Variables ---
    static constexpr int fftOrder = 11;
    static constexpr size_t fftSize = 1 << fftOrder; // 2048
    static constexpr int hopSize = 512;              // 4x Overlap
    static constexpr int fifoSize = 4096;            // リングバッファ長

    juce::dsp::FFT forwardFFT;
    juce::dsp::FFT inverseFFT;
    juce::dsp::WindowingFunction<float> window;

    std::array<float, fifoSize> inputFifoL, inputFifoR;
    std::array<float, fifoSize> outputFifoL, outputFifoR;
    std::array<float, fftSize * 2> fftWorkspaceL, fftWorkspaceR;
    std::array<float, fftSize / 2> magL, magR, phaseL, phaseR, tempMag;

    int fifoWritePos = 0;
    int hopCounter = 0;

    // --- GUI Display FFT Variables ---
    juce::dsp::FFT displayFFT{ 9 }; // 512 = 2^9
    std::array<float, 1024> displayWorkspace{};
    std::array<float, 256> displayMag{}, displayPhase{}, displayTempMag{};

    void processSTFTFrame(int modeA, float amtA, int modeB, float amtB)
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

        if (modeA >= 9 && modeA <= 13) {
            applyMorphToMagnitude(magL.data(), tempMag.data(), modeA, amtA, numBins);
            applyMorphToMagnitude(magR.data(), tempMag.data(), modeA, amtA, numBins);
        }
        if (modeB >= 9 && modeB <= 13) {
            applyMorphToMagnitude(magL.data(), tempMag.data(), modeB, amtB, numBins);
            applyMorphToMagnitude(magR.data(), tempMag.data(), modeB, amtB, numBins);
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

    void applyMorphToMagnitude(float* mag, float* tMag, int mode, float amount, size_t numBins)
    {
        if (mode == 9) // Smear
        {
            int radius = (int)(std::abs(amount) * 15.0f) + 1;
            for (size_t k = 1; k < numBins; ++k) {
                float sum = 0.0f;
                int count = 0;
                for (int r = -radius; r <= radius; ++r) {
                    size_t idx = (size_t)std::clamp((int)k + r, 1, (int)numBins - 1);
                    sum += mag[idx];
                    count++;
                }
                tMag[k] = sum / (float)count;
            }
            for (size_t k = 1; k < numBins; ++k) mag[k] = tMag[k];
        }
        else if (mode == 10) // Vocode
        {
            float formants[4] = { 0.1f, 0.3f, 0.6f, 0.8f };
            for (size_t k = 1; k < numBins; ++k) {
                float env = 0.0f;
                float p = (float)k / (float)(numBins - 1);
                for (int f = 0; f < 4; ++f) {
                    env += std::exp(-std::abs(p - formants[f]) * 20.0f);
                }
                float targetMag = mag[k] * env * 2.0f;
                mag[k] = mag[k] * (1.0f - std::abs(amount)) + targetMag * std::abs(amount);
            }
        }
        else if (mode == 11) // Stretch
        {
            float stretch = (amount >= 0.0f) ? (1.0f + amount * 2.0f) : (1.0f / (1.0f + std::abs(amount) * 2.0f));
            for (size_t k = 1; k < numBins; ++k) {
                float srcK = (float)k / stretch;
                int k0 = (int)srcK;
                float frac = srcK - k0;
                int k1 = std::min(k0 + 1, (int)numBins - 1);
                k0 = std::clamp(k0, 1, (int)numBins - 1);
                k1 = std::clamp(k1, 1, (int)numBins - 1);
                tMag[k] = mag[(size_t)k0] * (1.0f - frac) + mag[(size_t)k1] * frac;
            }
            for (size_t k = 1; k < numBins; ++k) mag[k] = tMag[k];
        }
        else if (mode == 12) // SpecCut
        {
            int cutBin = (int)(std::abs(amount) * numBins);
            bool isHighCut = (amount > 0.0f);
            for (size_t k = 1; k < numBins; ++k) {
                if (isHighCut ? ((int)k > (int)numBins - cutBin) : ((int)k < cutBin)) mag[k] = 0.0f;
            }
        }
        else if (mode == 13) // Shepard
        {
            for (size_t k = 1; k < numBins; ++k) {
                float logPos = std::log2f((float)k) / std::log2f((float)(numBins - 1));
                float shifted = std::fmod(logPos + (amount * 5.0f), 1.0f);
                if (shifted < 0.0f) shifted += 1.0f;
                mag[k] *= std::sin(shifted * juce::MathConstants<float>::pi);
            }
        }
    }
};