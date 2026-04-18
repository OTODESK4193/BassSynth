// ==============================================================================
// Source/DSP/SpectralWavetable.h
// ==============================================================================
#pragma once
#include <JuceHeader.h>
#include <complex>
#include <vector>
#include <memory>

enum class MorphMode {
    None = 0, Bend, PWM, Sync, Mirror, Flip, Quantize, Remap,
    FM, AM, RM, Stretch, Smear, SpecCut, Vocode, Shepard
};

static constexpr int kTableSize = 2048;
static constexpr int kNumMipmaps = 11; // 1(Nyquist) to 1024 harmonics

struct SpectralFrame
{
    std::vector<std::complex<float>> bins;
    std::vector<std::vector<float>> mipmaps; // [mipmap_level][sample]

    SpectralFrame() {
        bins.resize(kTableSize / 2 + 1, { 0.0f, 0.0f });
        mipmaps.resize(kNumMipmaps, std::vector<float>(kTableSize + 1, 0.0f)); // +1 for interpolation wrap
    }
};

struct SpectralWavetable
{
    std::vector<SpectralFrame> frames;
    int numFrames = 1;
};

class SpectralWavetableBuilder
{
public:
    SpectralWavetableBuilder() : fft(11) {}

    std::unique_ptr<SpectralWavetable> build(
        const std::vector<std::vector<float>>& rawFrames,
        int modeA, float amtA, int modeB, float amtB, int modWave)
    {
        auto wt = std::make_unique<SpectralWavetable>();
        wt->numFrames = (int)rawFrames.size();
        wt->frames.resize(wt->numFrames);

        for (int f = 0; f < wt->numFrames; ++f)
        {
            std::vector<float> timeBuf = rawFrames[f];

            // 1. 時間領域Morphの適用 (直列)
            applyTimeDomainMorph(timeBuf, static_cast<MorphMode>(modeA), amtA, modWave);
            applyTimeDomainMorph(timeBuf, static_cast<MorphMode>(modeB), amtB, modWave);

            // 2. 時間領域 -> 周波数領域 (FFT)
            timeToBins(timeBuf, wt->frames[f].bins);

            // 3. 周波数領域Morphの適用 (直列)
            applySpectralMorph(wt->frames[f].bins, static_cast<MorphMode>(modeA), amtA);
            applySpectralMorph(wt->frames[f].bins, static_cast<MorphMode>(modeB), amtB);

            // 4. Mipmap の生成
            buildMipmaps(wt->frames[f]);
        }
        return wt;
    }

private:
    juce::dsp::FFT fft;
    std::vector<float> fftBuf;

    bool isTimeDomain(MorphMode m) {
        return m >= MorphMode::Bend && m <= MorphMode::RM;
    }
    bool isSpectral(MorphMode m) {
        return m >= MorphMode::Stretch && m <= MorphMode::Shepard;
    }

    void applyTimeDomainMorph(std::vector<float>& buf, MorphMode mode, float amount, int modWave)
    {
        if (!isTimeDomain(mode) || std::abs(amount) < 0.001f) return;

        std::vector<float> temp(kTableSize);
        for (int i = 0; i < kTableSize; ++i) {
            float p = (float)i / kTableSize;
            float warpedPhase = p;

            if (mode == MorphMode::Bend) {
                warpedPhase = std::pow(p, std::exp(-amount * 2.0f));
            }
            else if (mode == MorphMode::PWM) {
                float pw = 0.5f + (amount * 0.49f);
                warpedPhase = (p < pw) ? (p / pw * 0.5f) : (0.5f + (p - pw) / (1.0f - pw) * 0.5f);
            }
            else if (mode == MorphMode::Sync) {
                float ratio = 1.0f + std::abs(amount) * 7.0f;
                warpedPhase = std::fmod(p * ratio, 1.0f);
                if (amount < 0.0f) warpedPhase = 1.0f - warpedPhase;
            }
            else if (mode == MorphMode::Mirror) {
                if (amount > 0.0f) {
                    float mirrored = (p < 0.5f) ? p * 2.0f : (1.0f - p) * 2.0f;
                    warpedPhase = p * (1.0f - amount) + mirrored * amount;
                }
                else {
                    float mirrored = (p > 0.5f) ? (p - 0.5f) * 2.0f : (0.5f - p) * 2.0f;
                    warpedPhase = p * (1.0f + amount) + mirrored * -amount;
                }
            }
            else if (mode == MorphMode::FM) {
                float modSig = getModulator(p, modWave);
                warpedPhase = std::fmod(p + modSig * amount, 1.0f);
                if (warpedPhase < 0.0f) warpedPhase += 1.0f;
            }

            // Phase Warp適用後のサンプリング (線形補間)
            float idx = warpedPhase * (kTableSize - 1);
            int i0 = (int)idx;
            float frac = idx - i0;
            int i1 = std::min(i0 + 1, kTableSize - 1);
            float val = buf[i0] * (1.0f - frac) + buf[i1] * frac;

            // Amplitude Shapingの適用
            if (mode == MorphMode::Flip) {
                float flipped = amount > 0.0f ? std::abs(val) : -std::abs(val);
                val = val * (1.0f - std::abs(amount)) + flipped * std::abs(amount);
            }
            else if (mode == MorphMode::Quantize) {
                float steps = std::pow(2.0f, 2.0f + (1.0f - std::abs(amount)) * 14.0f);
                val = std::round(val * steps) / steps;
            }
            else if (mode == MorphMode::Remap) {
                val = amount >= 0.0f ? std::tanh(val * (1.0f + amount * 5.0f)) : std::sin(val * (1.0f + std::abs(amount) * 3.0f) * juce::MathConstants<float>::halfPi);
            }
            else if (mode == MorphMode::AM) {
                float unipolarMod = (getModulator(p, modWave) + 1.0f) * 0.5f;
                val = val * (1.0f - std::abs(amount) + std::abs(amount) * unipolarMod);
            }
            else if (mode == MorphMode::RM) {
                val = val * (1.0f - std::abs(amount)) + val * getModulator(p, modWave) * std::abs(amount);
            }

            temp[i] = val;
        }
        buf = temp;
    }

    float getModulator(float p, int wave) {
        if (wave == 0) return std::sin(p * juce::MathConstants<float>::twoPi);
        if (wave == 1) return p * 2.0f - 1.0f;
        if (wave == 2) return p < 0.5f ? 1.0f : -1.0f;
        return 4.0f * std::abs(p - 0.5f) - 1.0f; // Triangle
    }

    void timeToBins(const std::vector<float>& timeDomain, std::vector<std::complex<float>>& bins)
    {
        fftBuf.assign(kTableSize * 2, 0.0f);
        for (int i = 0; i < kTableSize; ++i) {
            fftBuf[i] = timeDomain[i];
        }

        // JUCE 実数FFTの実行
        fft.performRealOnlyForwardTransform(fftBuf.data());

        // 正しいインターリーブ配列からの抽出: [Re0, Im0, Re1, Im1, ... ReN/2, ImN/2]
        // 注意: Im0 と ImN/2 は常に 0 です。
        for (int k = 0; k <= kTableSize / 2; ++k) {
            bins[k] = { fftBuf[k * 2], fftBuf[k * 2 + 1] };
        }
    }

    void applySpectralMorph(std::vector<std::complex<float>>& bins, MorphMode mode, float amount)
    {
        if (!isSpectral(mode) || std::abs(amount) < 0.001f) return;
        int numBins = (int)bins.size();
        std::vector<std::complex<float>> newBins = bins;

        if (mode == MorphMode::Stretch) {
            float stretch = (amount >= 0.0f) ? (1.0f + amount * 2.0f) : (1.0f / (1.0f + std::abs(amount) * 2.0f));
            for (int k = 1; k < numBins; ++k) {
                float srcK = (float)k / stretch;
                int k0 = (int)srcK;
                float frac = srcK - k0;
                int k1 = std::min(k0 + 1, numBins - 1);
                k0 = std::min(k0, numBins - 1);
                newBins[k] = bins[k0] * (1.0f - frac) + bins[k1] * frac;
            }
        }
        else if (mode == MorphMode::Smear) {
            int radius = (int)(std::abs(amount) * 15.0f) + 1;
            for (int k = 1; k < numBins; ++k) {
                std::complex<float> sum = { 0.0f, 0.0f };
                int count = 0;
                for (int r = -radius; r <= radius; ++r) {
                    int idx = juce::jlimit(0, numBins - 1, k + r);
                    sum += bins[idx]; ++count;
                }
                newBins[k] = sum / (float)count;
            }
        }
        else if (mode == MorphMode::SpecCut) {
            int cutBin = (int)(std::abs(amount) * numBins);
            bool isHighCut = (amount > 0.0f);
            for (int k = 1; k < numBins; ++k) {
                if (isHighCut ? (k > numBins - cutBin) : (k < cutBin)) newBins[k] = { 0.0f, 0.0f };
            }
        }
        else if (mode == MorphMode::Shepard) {
            for (int k = 1; k < numBins; ++k) {
                float logPos = std::log2f((float)k) / std::log2f((float)(numBins - 1));
                float shifted = std::fmod(logPos + (amount * 5.0f), 1.0f);
                if (shifted < 0.0f) shifted += 1.0f;
                newBins[k] = bins[k] * std::sin(shifted * juce::MathConstants<float>::pi);
            }
        }
        else if (mode == MorphMode::Vocode) {
            float formants[4] = { 0.1f, 0.3f, 0.6f, 0.8f };
            for (int k = 1; k < numBins; ++k) {
                float mag = std::abs(bins[k]);
                if (mag < 1e-6f) continue;
                float env = 0.0f, p = (float)k / (numBins - 1);
                for (int f = 0; f < 4; ++f) env += std::exp(-std::abs(p - formants[f]) * 20.0f);
                float targetMag = mag * env * 2.0f;
                float newMag = mag * (1.0f - std::abs(amount)) + targetMag * std::abs(amount);
                newBins[k] = std::polar(newMag, std::arg(bins[k]));
            }
        }
        bins = newBins;
    }

    void buildMipmaps(SpectralFrame& frame)
    {
        int maxHarmonic = kTableSize / 2;
        while (maxHarmonic > 1 && std::abs(frame.bins[maxHarmonic]) < 1e-5f) --maxHarmonic;

        int currentMax = 1;
        for (int m = 0; m < kNumMipmaps; ++m) {
            int harmonicLimit = std::min(currentMax, maxHarmonic);
            fftBuf.assign(kTableSize * 2, 0.0f);

            // JUCE IFFT用のバッファ作成: 前半のペアのみを詰める
            for (int k = 0; k <= harmonicLimit; ++k) {
                fftBuf[k * 2] = frame.bins[k].real();
                fftBuf[k * 2 + 1] = frame.bins[k].imag();
            }

            // JUCE 実数逆FFTの実行
            fft.performRealOnlyInverseTransform(fftBuf.data());

            // 時間領域データの取得と正規化
            float peak = 0.0f;
            for (int i = 0; i < kTableSize; ++i) {
                peak = std::max(peak, std::abs(fftBuf[i]));
            }
            float scale = (peak > 1e-8f) ? (1.0f / peak) : 1.0f;

            for (int i = 0; i < kTableSize; ++i) {
                frame.mipmaps[m][i] = fftBuf[i] * scale;
            }
            frame.mipmaps[m][kTableSize] = frame.mipmaps[m][0]; // ラップアラウンド

            currentMax *= 2;
        }
    }
};