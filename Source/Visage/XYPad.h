#pragma once

#include <JuceHeader.h>

class XYPad : public juce::Component
{
public:
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void xyPadValueChanged(XYPad* pad, float x, float y) = 0;
    };
    
    XYPad();
    ~XYPad() override = default;
    
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void resized() override;
    
    void setValues(float newX, float newY);
    float getXValue() const { return xValue; }
    float getYValue() const { return yValue; }
    
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
    std::function<void(float, float)> onValueChange;
    
private:
    float xValue = 0.5f;
    float yValue = 0.5f;
    
    juce::Point<float> thumbPosition;
    juce::ListenerList<Listener> listeners;
    
    void updateThumbPosition();
    void setValuesFromMousePosition(const juce::MouseEvent& event);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XYPad)
};