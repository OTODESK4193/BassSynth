#pragma once
#include <JuceHeader.h>
#include <cmath>
#include <algorithm>

/**
 * ADAA バイパス・テスト用 Distortion & Shaper
 * ※エイリアシング抑制（積分・微分）を無効化し、純粋な波形整形のみを行います。
 * これで金切り音が消えれば、原因はADAAの数値演算（桁落ち・発散）に確定します。
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

    inline void processStereo(float inL, float inR,
        float driveAmt, float shpAmt,
        float rate, float bits,
        float& outL, float& outR)
    {
        // 1. Distortion Stage (Drive) - ADAAバイパス
        float dL = applyDriveBypass(inL, driveAmt, lastInDriveL);
        float dR = applyDriveBypass(inR, driveAmt, lastInDriveR);

        // 2. Shaper Stage (Wavefolding) - ADAAバイパス
        float sL = applyShaperBypass(dL, shpAmt, lastInShaperL);
        float sR = applyShaperBypass(dR, shpAmt, lastInShaperR);

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

    // --- ADAAをバイパスした純粋な tanh (Distortion) ---
    inline float applyDriveBypass(float x, float drive, float& lastX)
    {
        float curX = x * drive;
        lastX = curX; // 状態は一応保存（今回は使わないが構造維持のため）
        return std::tanh(curX); // 直接tanhを計算
    }

    // --- ADAAをバイパスした純粋な sin (Wavefolding) ---
    inline float applyShaperBypass(float x, float amount, float& lastX)
    {
        lastX = x;
        if (amount < 0.01f) {
            return x;
        }

        // 積分・微分を行わず、直接波形をブレンドして出力
        float w = amount;
        float freq = 3.14159265f;
        return (1.0f - w) * x + w * std::sin(x * freq);
    }
};