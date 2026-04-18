#pragma once
#include <JuceHeader.h>

class AbletonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AbletonLookAndFeel()
    {
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour::fromString("FF1E1E1E"));
        setColour(juce::Slider::thumbColourId, juce::Colour::fromString("FFFF764D")); // Ableton Orange
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromString("FFFF764D"));
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour::fromString("FF454545"));
        setColour(juce::Label::textColourId, juce::Colour::fromString("FFDDDDDD"));
        setColour(juce::GroupComponent::outlineColourId, juce::Colour::fromString("FF454545"));
        setColour(juce::GroupComponent::textColourId, juce::Colour::fromString("FFAAAAAA"));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
        const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto center = bounds.getCentre();
        auto trackWidth = 4.0f;

        // Background Track
        juce::Path backgroundArc;
        backgroundArc.addCentredArc(center.x, center.y, radius - trackWidth, radius - trackWidth,
            0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(findColour(juce::Slider::rotarySliderOutlineColourId));
        g.strokePath(backgroundArc, juce::PathStrokeType(trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        bool isModulated = slider.getProperties().getWithDefault("mod_active", false);

        if (isModulated)
        {
            float modMin = juce::jlimit(0.0f, 1.0f, (float)slider.getProperties().getWithDefault("mod_min", 0.0f));
            float modMax = juce::jlimit(0.0f, 1.0f, (float)slider.getProperties().getWithDefault("mod_max", 1.0f));
            float startVal = std::min(modMin, modMax);
            float endVal = std::max(modMin, modMax);

            auto angleMin = rotaryStartAngle + startVal * (rotaryEndAngle - rotaryStartAngle);
            auto angleMax = rotaryStartAngle + endVal * (rotaryEndAngle - rotaryStartAngle);

            if (std::abs(angleMax - angleMin) > 0.01f)
            {
                juce::Path modArc;
                modArc.addCentredArc(center.x, center.y, radius - trackWidth, radius - trackWidth,
                    0.0f, angleMin, angleMax, true);
                g.setColour(juce::Colour::fromString("FFFF8CA1")); // Soft Pink
                g.strokePath(modArc, juce::PathStrokeType(trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
        }
        else
        {
            if (slider.isEnabled())
            {
                auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
                juce::Path valueArc;
                valueArc.addCentredArc(center.x, center.y, radius - trackWidth, radius - trackWidth,
                    0.0f, rotaryStartAngle, toAngle, true);
                g.setColour(findColour(juce::Slider::rotarySliderFillColourId));
                g.strokePath(valueArc, juce::PathStrokeType(trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
        }
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
        bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        juce::ignoreUnused(shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
        auto bounds = button.getLocalBounds().toFloat().reduced(2.0f);
        bool isOn = button.getToggleState();

        g.setColour(isOn ? juce::Colour::fromString("FFFF764D") : juce::Colours::darkgrey);
        g.fillRoundedRectangle(bounds, 4.0f);

        g.setColour(isOn ? juce::Colours::black : juce::Colours::lightgrey);
        g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        g.drawText(button.getButtonText(), bounds, juce::Justification::centred, true);

        g.setColour(juce::Colours::black.withAlpha(0.3f));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    }
}; 


