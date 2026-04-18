#pragma once
#include <JuceHeader.h>
#include <cmath>
#include <algorithm>
#include "../Generated/WavetableData_Generated.h"

class WavetableData
{
public:
    WavetableData() = default;

    static WavetableData& getInstance() {
        static WavetableData instance;
        return instance;
    }

    // 1次元配列からフレームと位相を計算して読み出す
    inline float readSampleFromTable(const float* tableData, int numFrames, float phase, float position) const
    {
        float posScaled = position * (float)(numFrames - 1);
        int frame0 = (int)posScaled;
        int frame1 = std::min(frame0 + 1, numFrames - 1);
        float frameFrac = posScaled - (float)frame0;

        phase -= std::floor(phase);
        float phaseScaled = phase * (float)EmbeddedWavetables::frameSize;
        int idx0 = (int)phaseScaled;
        float phaseFrac = phaseScaled - (float)idx0;

        int idxM1 = (idx0 - 1 + EmbeddedWavetables::frameSize) % EmbeddedWavetables::frameSize;
        int idx1 = (idx0 + 1) % EmbeddedWavetables::frameSize;
        int idx2 = (idx0 + 2) % EmbeddedWavetables::frameSize;

        // フレーム0の補間
        int f0_off = frame0 * EmbeddedWavetables::frameSize;
        float s0 = hermite(tableData[f0_off + idxM1], tableData[f0_off + idx0], tableData[f0_off + idx1], tableData[f0_off + idx2], phaseFrac);

        // フレーム1の補間
        int f1_off = frame1 * EmbeddedWavetables::frameSize;
        float s1 = hermite(tableData[f1_off + idxM1], tableData[f1_off + idx0], tableData[f1_off + idx1], tableData[f1_off + idx2], phaseFrac);

        return s0 + frameFrac * (s1 - s0);
    }

private:
    inline float hermite(float y0, float y1, float y2, float y3, float frac) const {
        float c0 = y1;
        float c1 = 0.5f * (y2 - y0);
        float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }
};

