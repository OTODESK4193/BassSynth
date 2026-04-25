// ==============================================================================
// Source/Logic/Mseg.h
// ==============================================================================
#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <cmath>
#include <algorithm>

// ★ 最大32ノード（固定長）でリアルタイム安全性を確保
constexpr int MAX_MSEG_POINTS = 32;

struct MsegPoint {
    float x{ 0.0f };     // 位相 (0.0 ~ 1.0)
    float y{ 0.0f };     // 値 (-1.0 ~ 1.0)
    float curve{ 0.0f }; // カーブのテンション (-1.0 ~ 1.0)
};

struct MsegState {
    std::array<MsegPoint, MAX_MSEG_POINTS> points;
    int numPoints{ 0 };

    // デフォルト状態（ノコギリ波のような形）で初期化
    MsegState() {
        points[0] = { 0.0f, 1.0f, 0.0f };
        points[1] = { 1.0f, -1.0f, 0.0f };
        numPoints = 2;
    }
};

class Mseg
{
public:
    Mseg() {
        activeState = MsegState();
        pendingState = MsegState();
    }

    void setSampleRate(double sr) {
        sampleRate = std::max(1.0, sr);
    }

    // パラメータの設定 (Lfo.h と完全互換の設計)
    void setParameters(bool sync, float rate, int beat, float amt, int tMode) {
        isSync = sync;
        rateHz = rate;
        beatIdx = beat;
        depth = std::clamp(amt, 0.0f, 1.0f);
        trigMode = std::clamp(tMode, 0, 2);
    }

    // ★ GUIスレッドから呼ばれる安全なデータ更新（ダブルバッファリング）
    void pushNewState(const MsegState& newState) {
        pendingState = newState;
        hasNewState.store(true, std::memory_order_release);
    }

    void noteOn(bool isLegato = false) {
        if (isLegato) return;
        if (trigMode == 0) return; // Free モード時は位相リセットを無視

        phase = 0.0f;
        oneShotDone = false;
    }

    inline float getNextSample(double bpm) {
        // ★ オーディオスレッド側：新しい波形データが届いていたらコピーしてフラグを下ろす（ロックフリー）
        if (hasNewState.exchange(false, std::memory_order_acquire)) {
            activeState = pendingState;
        }

        if (trigMode == 2 && oneShotDone) {
            return evaluateMseg(1.0f) * depth; // One-Shot 完了時は終端の値を維持
        }

        float freq = rateHz;

        if (isSync && bpm > 0.0) {
            float beatsPerCycle = 1.0f;
            switch (beatIdx) {
            case 0: beatsPerCycle = 4.0f; break;
            case 1: beatsPerCycle = 2.0f; break;
            case 2: beatsPerCycle = 1.0f; break;
            case 3: beatsPerCycle = 0.5f; break;
            case 4: beatsPerCycle = 0.25f; break;
            case 5: beatsPerCycle = 0.125f; break;
            case 6: beatsPerCycle = 2.0f / 3.0f; break;
            case 7: beatsPerCycle = 1.0f / 3.0f; break;
            case 8: beatsPerCycle = 0.5f / 3.0f; break;
            }
            freq = static_cast<float>(bpm / 60.0) / beatsPerCycle;
        }

        float inc = freq / static_cast<float>(sampleRate);
        phase += inc;

        if (phase >= 1.0f) {
            if (trigMode == 2) {
                phase = 1.0f;
                oneShotDone = true;
            }
            else {
                phase -= std::floor(phase);
            }
        }

        return evaluateMseg(phase) * depth;
    }

private:
    double sampleRate = 44100.0;
    float phase = 0.0f;

    bool isSync = false;
    float rateHz = 1.0f;
    int beatIdx = 2;
    float depth = 1.0f;
    int trigMode = 0;     // 0:Free, 1:Retrig, 2:One-Shot
    bool oneShotDone = false;

    MsegState activeState;
    MsegState pendingState;
    std::atomic<bool> hasNewState{ false };

    // 位相(0.0~1.0)に対応するY値を計算
    inline float evaluateMseg(float p) const {
        if (activeState.numPoints == 0) return 0.0f;
        if (activeState.numPoints == 1) return activeState.points[0].y;

        // 範囲外の保護
        if (p <= activeState.points[0].x) return activeState.points[0].y;
        if (p >= activeState.points[activeState.numPoints - 1].x) return activeState.points[activeState.numPoints - 1].y;

        // 現在の位相が含まれるセグメント（ノード間）を探す
        for (int i = 0; i < activeState.numPoints - 1; ++i) {
            const auto& p0 = activeState.points[i];
            const auto& p1 = activeState.points[i + 1];

            if (p >= p0.x && p <= p1.x) {
                float rangeX = p1.x - p0.x;
                if (rangeX <= 0.0f) return p1.y;

                // セグメント内の正規化された進行度 (0.0 ~ 1.0)
                float t = (p - p0.x) / rangeX;

                // ★ カーブ（曲率）の適用
                if (p0.curve > 0.001f) {
                    // Exponential: 曲率がプラスの場合、急激に立ち上がる/落ちる
                    float expFactor = 1.0f + p0.curve * 5.0f;
                    t = std::pow(t, expFactor);
                }
                else if (p0.curve < -0.001f) {
                    // Logarithmic: 曲率がマイナスの場合、ゆっくり立ち上がる/落ちる
                    float logFactor = 1.0f / (1.0f - p0.curve * 5.0f);
                    t = std::pow(t, logFactor);
                }

                // Y値の線形補間（カーブ適用後の t を使用）
                return p0.y + (p1.y - p0.y) * t;
            }
        }
        return 0.0f;
    }
};