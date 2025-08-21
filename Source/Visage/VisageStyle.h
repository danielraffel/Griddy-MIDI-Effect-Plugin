#pragma once

#include <JuceHeader.h>

class VisageLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VisageLookAndFeel();
    ~VisageLookAndFeel() override = default;
    
    // Rotary Slider
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                         juce::Slider& slider) override;
    
    // Linear Slider
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPos, float minSliderPos, float maxSliderPos,
                         const juce::Slider::SliderStyle style, juce::Slider& slider) override;
    
    // Slider thumb
    void drawLinearSliderThumb(juce::Graphics& g, int x, int y, int width, int height,
                               float sliderPos, float minSliderPos, float maxSliderPos,
                               const juce::Slider::SliderStyle style, juce::Slider& slider) override;
    
    // Slider background
    void drawLinearSliderBackground(juce::Graphics& g, int x, int y, int width, int height,
                                   float sliderPos, float minSliderPos, float maxSliderPos,
                                   const juce::Slider::SliderStyle style, juce::Slider& slider) override;
    
private:
    // Color scheme
    juce::Colour backgroundColour = juce::Colour(0xff1a1a1a);
    juce::Colour trackColour = juce::Colour(0xff2a2a2a);
    juce::Colour thumbColour = juce::Colour(0xffffaa00);
    juce::Colour highlightColour = juce::Colour(0xffff6600);
    juce::Colour textColour = juce::Colour(0xffcccccc);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VisageLookAndFeel)
};