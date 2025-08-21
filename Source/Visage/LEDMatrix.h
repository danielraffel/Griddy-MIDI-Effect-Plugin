#pragma once

#include <JuceHeader.h>
#include "../Grids/GridsEngine.h"

class LEDMatrix : public juce::Component, private juce::Timer
{
public:
    explicit LEDMatrix(GridsEngine& engine);
    ~LEDMatrix() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void setCurrentStep(int step);
    void updatePattern();
    void triggerReset(bool isRetrigger);
    
private:
    void timerCallback() override;
    
    GridsEngine& gridsEngine;
    
    int currentStep = 0;
    std::array<bool, 32> bdPattern;
    std::array<bool, 32> sdPattern;
    std::array<bool, 32> hhPattern;
    
    std::array<bool, 32> bdAccents;
    std::array<bool, 32> sdAccents;
    std::array<bool, 32> hhAccents;
    
    // Track last parameter values to detect changes
    float lastX = -1.0f;
    float lastY = -1.0f;
    float lastBDDensity = -1.0f;
    float lastSDDensity = -1.0f;
    float lastHHDensity = -1.0f;
    
    const float ledSize = 10.0f;
    const float ledSpacing = 14.0f;
    const float rowSpacing = 25.0f;
    
    // Reset animation
    bool isResetting = false;
    bool isRetriggerReset = false;
    float resetAnimationProgress = 0.0f;
    int lastResetStep = -1;
    
    juce::Colour getBDColour(int step) const;
    juce::Colour getSDColour(int step) const;
    juce::Colour getHHColour(int step) const;
    
    void drawLED(juce::Graphics& g, float x, float y, juce::Colour colour, bool isActive, bool isCurrent);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LEDMatrix)
};