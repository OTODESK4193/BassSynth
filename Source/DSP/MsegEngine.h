// ==============================================================================
// Source/DSP/MsegEngine.h
// ==============================================================================
#pragma once
#include <JuceHeader.h>
#include <vector>
#include <algorithm>

struct MsegPoint {
    float x; // 0.0 to 1.0 (Time / Phase)
    float y; // -1.0 to 1.0 (Value)
    float curve; // -1.0 (Log) to 0.0 (Linear) to 1.0 (Exp) - 今後の拡張用
};

class MsegEngine
{
public:
    MsegEngine() {
        // デフォルトはシンプルなサイン波風のトライアングル
        points = { {0.0f, 0.0f, 0.0f}, {0.25f, 1.0f, 0.0f}, {0.75f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f} };
    }

    void prepare(double sr) { sampleRate = sr; }

    // GUIから点を更新するためのメソッド
    void setPoints(const std::vector<MsegPoint>& newPoints) {
        points = newPoints;
        // 常にX軸（時間）順にソートしておく
        std::sort(points.begin(), points.end(), [](const MsegPoint& a, const MsegPoint& b) { return a.x < b.x; });
    }

    // Trigger (Note On) 時に呼び出す
    void trigger() {
        phase = 0.0f;
        active = true;
    }

    // LFOとしての処理
    float getNextSample(float rateHz, bool isSync, double bpm, int beatDiv, bool loop = true) {
        if (!active && !loop) return points.back().y;

        float inc = 0.0f;
        if (isSync) {
            // Beat同期計算 (例: 1/4音符, 1/8音符など)
            float beatsPerSecond = (float)bpm / 60.0f;
            float durationSeconds = 1.0f; // beatDivに基づく秒数計算(実装に応じて調整)
            inc = 1.0f / (durationSeconds * (float)sampleRate);
        }
        else {
            // Hzベース計算
            inc = rateHz / (float)sampleRate;
        }

        float out = evaluateAt(phase);

        phase += inc;
        if (phase >= 1.0f) {
            if (loop) phase -= 1.0f;
            else { phase = 1.0f; active = false; } // Envelopeモード（ワンショット）
        }

        return out;
    }

private:
    double sampleRate = 44100.0;
    std::vector<MsegPoint> points;
    float phase = 0.0f;
    bool active = false;

    // 現在のフェーズ（位相）からY値を補間計算するコアロジック
    float evaluateAt(float p) const {
        if (points.empty()) return 0.0f;
        if (points.size() == 1) return points[0].y;
        if (p <= points.front().x) return points.front().y;
        if (p >= points.back().x) return points.back().y;

        // p が含まれるセグメント（点Aと点Bの間）を探す
        for (size_t i = 0; i < points.size() - 1; ++i) {
            if (p >= points[i].x && p <= points[i + 1].x) {
                const auto& p1 = points[i];
                const auto& p2 = points[i + 1];

                float rangeX = p2.x - p1.x;
                if (rangeX <= 0.0001f) return p2.y;

                float t = (p - p1.x) / rangeX; // 0.0 to 1.0

                // ※ここに curve パラメータを用いたテンション（カーブ）計算を追加可能
                // 現在はLinear補間
                return p1.y + t * (p2.y - p1.y);
            }
        }
        return 0.0f;
    }
};