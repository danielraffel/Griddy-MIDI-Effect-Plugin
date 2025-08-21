#pragma once

#include <JuceHeader.h>

class GridsXYPad : public juce::Component
{
public:
    GridsXYPad();
    ~GridsXYPad() override;
    
    void paint(juce::Graphics&) override;
    void resized() override;
    
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GridsXYPad)
};