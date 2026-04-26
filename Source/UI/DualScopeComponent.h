#pragma once
#include <JuceHeader.h>
#include <array>

class DualScopeComponent : public juce::Component, public juce::Timer
{
public:
    DualScopeComponent(float* outDataPtr) : outputData(outDataPtr) {
        sourceWave.fill(0.0f);
        startTimerHz(60);
    }

    void updateStaticWave(const float* data, int size) {
        if (size <= 0) return;
        for (int i = 0; i < 512; ++i) {
            sourceWave[i] = data[juce::jlimit(0, size - 1, (i * size) / 512)];
        }
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour::fromString("FF121212"));
        auto area = getLocalBounds();

        auto top = area.removeFromTop(area.getHeight() / 2).reduced(5);
        drawWave(g, top, sourceWave.data(), 512, "SOURCE (Wavetable Shape)", juce::Colour::fromString("FF00FFCC"));

        auto bottom = area.reduced(5);
        drawWave(g, bottom, outputData, 512, "OUTPUT (Dynamic Stream)", juce::Colour::fromString("FFFF764D"));
    }

    void timerCallback() override { repaint(); }

private:
    void drawWave(juce::Graphics& g, juce::Rectangle<int> r, const float* data, int size, const char* label, juce::Colour col) {
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.fillRoundedRectangle(r.toFloat(), 4.0f);
        g.setColour(col);

        juce::Path p;
        float xStep = (float)r.getWidth() / size;
        for (int i = 0; i < size; ++i) {
            float x = r.getX() + i * xStep;
            float y = r.getCentreY() - (data[i] * r.getHeight() * 0.45f);
            if (i == 0) p.startNewSubPath(x, y);
            else p.lineTo(x, y);
        }
        g.strokePath(p, juce::PathStrokeType(1.5f));

        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.setFont(10.0f);
        g.drawText(label, r.reduced(5), juce::Justification::topRight);
    }

    std::array<float, 512> sourceWave;
    float* outputData;
};