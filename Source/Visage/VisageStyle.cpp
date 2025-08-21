#include "VisageStyle.h"

VisageLookAndFeel::VisageLookAndFeel()
{
    // Set default colors
    setColour(juce::Slider::textBoxTextColourId, textColour);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentWhite);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentWhite);
    setColour(juce::Label::textColourId, textColour);
}

void VisageLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                        juce::Slider& slider)
{
    auto radius = (float)juce::jmin(width / 2, height / 2) - 6.0f;
    auto centreX = (float)x + (float)width * 0.5f;
    auto centreY = (float)y + (float)height * 0.5f;
    auto rx = centreX - radius;
    auto ry = centreY - radius;
    auto rw = radius * 2.0f;
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    
    // Check if this is the reset button (by checking the component name)
    bool isResetButton = (slider.getName() == "ResetButton");
    bool isPressed = isResetButton && sliderPos > 0.5f;
    
    // Background circle
    g.setColour(isPressed ? trackColour.brighter(0.3f) : trackColour);
    g.fillEllipse(rx, ry, rw, rw);
    
    // Outer ring
    g.setColour(isPressed ? juce::Colour(0xff606060) : juce::Colour(0xff404040));
    g.drawEllipse(rx, ry, rw, rw, 2.0f);
    
    if (!isResetButton) {
        // Value arc for normal knobs
        juce::Path arcPath;
        arcPath.addCentredArc(centreX, centreY, radius - 4.0f, radius - 4.0f,
                              0.0f, rotaryStartAngle, angle, true);
        
        g.setColour(thumbColour.withAlpha(0.8f));
        g.strokePath(arcPath, juce::PathStrokeType(3.0f));
    }
    
    // Center knob
    auto knobRadius = radius * 0.6f;
    
    if (isResetButton) {
        // Reset button - simple filled circle that glows when pressed
        if (isPressed) {
            // Outer glow effect
            g.setColour(highlightColour.withAlpha(0.5f));
            g.fillEllipse(centreX - knobRadius * 1.5f, centreY - knobRadius * 1.5f, 
                         knobRadius * 3.0f, knobRadius * 3.0f);
            
            // Inner glow
            g.setColour(highlightColour.withAlpha(0.8f));
            g.fillEllipse(centreX - knobRadius * 1.1f, centreY - knobRadius * 1.1f, 
                         knobRadius * 2.2f, knobRadius * 2.2f);
        }
        
        // Main button - bright orange when pressed, dark gray when not
        g.setColour(isPressed ? highlightColour : juce::Colour(0xff1a1a1a));
        g.fillEllipse(centreX - knobRadius, centreY - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);
        
        // Add subtle inner highlight when pressed
        if (isPressed) {
            g.setColour(highlightColour.brighter(0.3f));
            g.fillEllipse(centreX - knobRadius * 0.7f, centreY - knobRadius * 0.7f, 
                         knobRadius * 1.4f, knobRadius * 1.4f);
        }
    } else {
        // Normal knob gradient
        g.setGradientFill(juce::ColourGradient(
            juce::Colour(0xff3a3a3a), centreX, centreY - knobRadius,
            juce::Colour(0xff1a1a1a), centreX, centreY + knobRadius,
            false));
        g.fillEllipse(centreX - knobRadius, centreY - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);
        
        // Pointer for normal knobs
        juce::Path pointer;
        auto pointerLength = knobRadius * 0.8f;
        auto pointerThickness = 3.0f;
        pointer.addRectangle(-pointerThickness * 0.5f, -knobRadius, pointerThickness, pointerLength);
        pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
        
        g.setColour(highlightColour);
        g.fillPath(pointer);
    }
    
    // Center dot
    g.setColour(isResetButton && isPressed ? juce::Colour(0xff202020) : juce::Colour(0xff606060));
    g.fillEllipse(centreX - 2.0f, centreY - 2.0f, 4.0f, 4.0f);
}

void VisageLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float minSliderPos, float maxSliderPos,
                                        const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style == juce::Slider::LinearVertical)
    {
        drawLinearSliderBackground(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        drawLinearSliderThumb(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
    }
    else if (style == juce::Slider::LinearHorizontal)
    {
        // Draw horizontal slider with constrained thumb
        const float thumbRadius = 8.0f;
        const float trackHeight = 4.0f;
        const float trackY = y + height * 0.5f - trackHeight * 0.5f;
        
        // Constrain thumb position to track bounds
        float constrainedPos = juce::jlimit(x + thumbRadius, x + width - thumbRadius, sliderPos);
        
        // Draw track
        g.setColour(juce::Colour(0xff333333));
        g.fillRoundedRectangle(x + thumbRadius, trackY, width - thumbRadius * 2, trackHeight, 2.0f);
        
        // Draw filled portion
        g.setColour(juce::Colour(0xffff8833));
        g.fillRoundedRectangle(x + thumbRadius, trackY, constrainedPos - x - thumbRadius, trackHeight, 2.0f);
        
        // Draw thumb
        g.setColour(juce::Colour(0xffff8833));
        g.fillEllipse(constrainedPos - thumbRadius, y + height * 0.5f - thumbRadius, 
                      thumbRadius * 2, thumbRadius * 2);
    }
    else
    {
        LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
    }
}

void VisageLookAndFeel::drawLinearSliderBackground(juce::Graphics& g, int x, int y, int width, int height,
                                                  float sliderPos, float minSliderPos, float maxSliderPos,
                                                  const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style == juce::Slider::LinearVertical)
    {
        auto trackWidth = juce::jmin(6.0f, width * 0.25f);
        auto trackX = x + width * 0.5f - trackWidth * 0.5f;
        
        // Background track groove (the empty/inactive part)
        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRoundedRectangle(trackX, y + 4.0f, trackWidth, height - 8.0f, trackWidth * 0.5f);
        
        // Draw subtle border around track
        g.setColour(juce::Colour(0xff303030));
        g.drawRoundedRectangle(trackX, y + 4.0f, trackWidth, height - 8.0f, trackWidth * 0.5f, 1.0f);
        
        // Calculate fill based on slider value (inverted for vertical sliders)
        // sliderPos goes from bottom (max) to top (min) for vertical sliders
        float sliderValue = 1.0f - ((sliderPos - y) / (float)height);
        float fillHeight = sliderValue * (height - 8.0f);
        
        if (fillHeight > 2.0f)  // Only draw if there's something to draw
        {
            // Fill from bottom up to the current value
            float fillY = y + height - 4.0f - fillHeight;
            
            // Gradient fill that's brighter at the top (current position)
            g.setGradientFill(juce::ColourGradient(
                thumbColour, trackX + trackWidth * 0.5f, fillY,
                thumbColour.withAlpha(0.6f), trackX + trackWidth * 0.5f, y + height - 4.0f,
                false));
            g.fillRoundedRectangle(trackX, fillY, trackWidth, fillHeight, trackWidth * 0.5f);
        }
        
        // Draw subtle tick marks at 0%, 50%, and 100%
        g.setColour(juce::Colour(0x30ffffff));
        // Bottom (0%)
        g.drawLine(trackX - 3.0f, y + height - 4.0f, trackX + trackWidth + 3.0f, y + height - 4.0f, 0.5f);
        // Middle (50%)
        float centerY = y + height * 0.5f;
        g.drawLine(trackX - 2.0f, centerY, trackX + trackWidth + 2.0f, centerY, 0.5f);
        // Top (100%)
        g.drawLine(trackX - 3.0f, y + 4.0f, trackX + trackWidth + 3.0f, y + 4.0f, 0.5f);
    }
}

void VisageLookAndFeel::drawLinearSliderThumb(juce::Graphics& g, int x, int y, int width, int height,
                                             float sliderPos, float minSliderPos, float maxSliderPos,
                                             const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style == juce::Slider::LinearVertical)
    {
        auto thumbWidth = juce::jmin(20.0f, width - 2.0f);
        auto thumbX = x + width * 0.5f - thumbWidth * 0.5f;
        
        // Thumb shadow
        g.setColour(juce::Colour(0x80000000));
        g.fillEllipse(thumbX, sliderPos - thumbWidth * 0.5f + 2.0f, thumbWidth, thumbWidth);
        
        // Thumb gradient
        g.setGradientFill(juce::ColourGradient(
            highlightColour, thumbX + thumbWidth * 0.5f, sliderPos - thumbWidth * 0.5f,
            thumbColour, thumbX + thumbWidth * 0.5f, sliderPos + thumbWidth * 0.5f,
            false));
        g.fillEllipse(thumbX, sliderPos - thumbWidth * 0.5f, thumbWidth, thumbWidth);
        
        // Thumb highlight
        g.setColour(juce::Colour(0x40ffffff));
        g.drawEllipse(thumbX + 1.0f, sliderPos - thumbWidth * 0.5f + 1.0f, 
                     thumbWidth - 2.0f, thumbWidth - 2.0f, 1.0f);
        
        // Center dot
        g.setColour(juce::Colour(0xff202020));
        g.fillEllipse(thumbX + thumbWidth * 0.5f - 2.0f, sliderPos - 2.0f, 4.0f, 4.0f);
    }
}