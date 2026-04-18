#pragma once
#include <JuceHeader.h>
#include <cmath>
#include <algorithm>

/**
 * ADAA (Antiderivative Anti-Aliasing) 搭載 Distortion & Shaper
 * 積分と微分を用いることで、オーバーサンプリングなしにエイリアシングを抑制します。
 */
class SineShaper
{
public:
    SineShaper() { reset(); }

    void prepare(double sampleRate)
    {
        currentSampleRate = std::max(1.0, sampleRate);
        reset();
    }

    void reset()
    {
        lastInDriveL = lastInDriveR = 0.0f;
        lastInShaperL = lastInShaperR = 0.0f;
        downsampleCounter = 0.0f;
        heldSampleL = heldSampleR = 0.0f;
    }

    /**
     * ステレオ処理: Drive(ADAA tanh) -> Shaper(ADAA sin) -> Bitcrush/Downsample
     */
    inline void processStereo(float inL, float inR,
        float driveAmt, float shpAmt,
        float rate, float bits,
        float& outL, float& outR)
    {
        // 1. Distortion Stage (Drive) using ADAA tanh
        float dL = applyDriveADAA(inL, driveAmt, lastInDriveL);
        float dR = applyDriveADAA(inR, driveAmt, lastInDriveR);

        // 2. Shaper Stage (Wavefolding) using ADAA sin
        float sL = applyShaperADAA(dL, shpAmt, lastInShaperL);
        float sR = applyShaperADAA(dR, shpAmt, lastInShaperR);

        // 3. Bitcrush & Downsample (Sample Rate Independent)
        float effectiveRate = rate * (float)(currentSampleRate / 44100.0);

        if (effectiveRate > 1.0f || bits < 24.0f)
        {
            downsampleCounter += 1.0f;
            if (downsampleCounter >= effectiveRate)
            {
                downsampleCounter -= effectiveRate;
                heldSampleL = sL;
                heldSampleR = sR;
            }

            float steps = std::exp2(bits);
            // NaNガードを追加し、床関数計算時のクラッシュを防止
            float safeL = std::isfinite(heldSampleL) ? heldSampleL : 0.0f;
            float safeR = std::isfinite(heldSampleR) ? heldSampleR : 0.0f;

            outL = std::floor(safeL * steps) / steps;
            outR = std::floor(safeR * steps) / steps;
        }
        else
        {
            outL = std::isfinite(sL) ? sL : 0.0f;
            outR = std::isfinite(sR) ? sR : 0.0f;
        }
    }

private:
    double currentSampleRate = 44100.0;
    float lastInDriveL = 0.0f, lastInDriveR = 0.0f;
    float lastInShaperL = 0.0f, lastInShaperR = 0.0f;
    float downsampleCounter = 0.0f, heldSampleL = 0.0f, heldSampleR = 0.0f;

    // --- ADAA Helper for tanh (Distortion) ---
    inline float applyDriveADAA(float x, float drive, float& lastX)
    {
        float curX = x * drive;
        float diff = curX - lastX;
        float result;

        // ゼロ除算の回避ガード (閾値を1.0e-5fに調整して精度を安定化)
        if (std::abs(diff) < 1.0e-5f) {
            result = std::tanh((curX + lastX) * 0.5f);
        }
        else {
            result = (antiderivTanh(curX) - antiderivTanh(lastX)) / diff;
        }

        lastX = curX;
        return result;
    }

    // tanh(x) の積分関数: ln(cosh(x))
    // 数値的安定性のために大きな |x| では |x| - ln(2) を使用
    inline float antiderivTanh(float x) const
    {
        float ax = std::abs(x);
        // floatの限界値付近でのオーバーフローを避けるため20.0fで分岐
        if (ax > 20.0f) return ax - 0.69314718f; // ln(2)

        // cosh(x)が大きな値になってもlogで正しく処理できるよう、
        // 念のため std::clamp や std::isfinite は使わず、数学的性質を維持
        return std::log(std::cosh(x));
    }

    // --- ADAA Helper for sin (Wavefolding) ---
    inline float applyShaperADAA(float x, float amount, float& lastX)
    {
        if (amount < 0.01f) {
            lastX = x;
            return x;
        }

        // 元の波形とサイン波形のブレンド
        // f(x) = (1-w)*x + w * sin(x * pi)
        float w = amount;
        float freq = 3.14159265f;

        float curX = x;
        float diff = curX - lastX;
        float result;

        if (std::abs(diff) < 1.0e-5f) {
            float mid = (curX + lastX) * 0.5f;
            result = (1.0f - w) * mid + w * std::sin(mid * freq);
        }
        else {
            // 積分値の計算 (ラムダ式を用いて元の構造を維持)
            auto F = [w, freq](float v) {
                // 数値的に巨大な値にならないよう、項を整理
                return (0.5f * (1.0f - w) * v * v) - (w / freq) * std::cos(v * freq);
                };
            result = (F(curX) - F(lastX)) / diff;
        }

        lastX = curX;
        return result;
    }
};