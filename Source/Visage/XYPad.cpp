#include "XYPad.h"

XYPad::XYPad()
{
    setRepaintsOnMouseActivity(true);
}

void XYPad::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background gradient
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff1a1a1a), bounds.getCentreX(), bounds.getY(),
        juce::Colour(0xff0a0a0a), bounds.getCentreX(), bounds.getBottom(),
        false));
    g.fillRoundedRectangle(bounds, 8.0f);
    
    // Border
    g.setColour(juce::Colour(0xff303030));
    g.drawRoundedRectangle(bounds.reduced(1.0f), 8.0f, 2.0f);
    
    // Grid lines
    g.setColour(juce::Colour(0x20ffffff));
    const int gridLines = 5;
    for (int i = 1; i < gridLines; ++i)
    {
        float x = bounds.getX() + (bounds.getWidth() * i / gridLines);
        g.drawLine(x, bounds.getY() + 10, x, bounds.getBottom() - 10, 0.5f);
        
        float y = bounds.getY() + (bounds.getHeight() * i / gridLines);
        g.drawLine(bounds.getX() + 10, y, bounds.getRight() - 10, y, 0.5f);
    }
    
    // Crosshair at current position
    g.setColour(juce::Colour(0x40ffffff));
    g.drawLine(thumbPosition.x, bounds.getY() + 10, 
               thumbPosition.x, bounds.getBottom() - 10, 1.0f);
    g.drawLine(bounds.getX() + 10, thumbPosition.y, 
               bounds.getRight() - 10, thumbPosition.y, 1.0f);
    
    // Thumb shadow
    g.setColour(juce::Colour(0x80000000));
    g.fillEllipse(thumbPosition.x - 10, thumbPosition.y - 10 + 2, 20, 20);
    
    // Thumb gradient
    auto thumbBounds = juce::Rectangle<float>(thumbPosition.x - 10, thumbPosition.y - 10, 20, 20);
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xffffaa00), thumbPosition.x, thumbPosition.y - 10,
        juce::Colour(0xffff6600), thumbPosition.x, thumbPosition.y + 10,
        false));
    g.fillEllipse(thumbBounds);
    
    // Thumb highlight
    g.setColour(juce::Colour(0x60ffffff));
    g.drawEllipse(thumbBounds.reduced(1.0f), 1.5f);
    
    // Inner glow
    g.setColour(juce::Colour(0x40ffaa00));
    g.fillEllipse(thumbBounds.reduced(5.0f));
    
    // Axis labels
    g.setColour(juce::Colour(0x80ffffff));
    g.setFont(10.0f);
    g.drawText("X", bounds.getRight() - 20, bounds.getCentreY() - 10, 20, 20, 
               juce::Justification::centred);
    g.drawText("Y", bounds.getCentreX() - 10, bounds.getY(), 20, 20, 
               juce::Justification::centred);
}

void XYPad::mouseDown(const juce::MouseEvent& event)
{
    setValuesFromMousePosition(event);
}

void XYPad::mouseDrag(const juce::MouseEvent& event)
{
    setValuesFromMousePosition(event);
}

void XYPad::resized()
{
    updateThumbPosition();
}

void XYPad::setValues(float newX, float newY)
{
    xValue = juce::jlimit(0.0f, 1.0f, newX);
    yValue = juce::jlimit(0.0f, 1.0f, newY);
    updateThumbPosition();
    repaint();
}

void XYPad::addListener(Listener* listener)
{
    listeners.add(listener);
}

void XYPad::removeListener(Listener* listener)
{
    listeners.remove(listener);
}

void XYPad::updateThumbPosition()
{
    auto bounds = getLocalBounds().toFloat().reduced(15.0f);
    thumbPosition.x = bounds.getX() + xValue * bounds.getWidth();
    thumbPosition.y = bounds.getY() + (1.0f - yValue) * bounds.getHeight();
}

void XYPad::setValuesFromMousePosition(const juce::MouseEvent& event)
{
    auto bounds = getLocalBounds().toFloat().reduced(15.0f);
    
    float newX = (event.position.x - bounds.getX()) / bounds.getWidth();
    float newY = 1.0f - (event.position.y - bounds.getY()) / bounds.getHeight();
    
    newX = juce::jlimit(0.0f, 1.0f, newX);
    newY = juce::jlimit(0.0f, 1.0f, newY);
    
    if (newX != xValue || newY != yValue)
    {
        xValue = newX;
        yValue = newY;
        updateThumbPosition();
        repaint();
        
        listeners.call(&Listener::xyPadValueChanged, this, xValue, yValue);
        
        if (onValueChange)
            onValueChange(xValue, yValue);
    }
}