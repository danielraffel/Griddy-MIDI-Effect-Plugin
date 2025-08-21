#pragma once

#include <JuceHeader.h>
#include "GridsPatternData.h"
#include <random>

class GridsEngine {
public:
    GridsEngine();
    ~GridsEngine() = default;
    
    // Pattern position (0.0 to 1.0)
    void setX(float x) { x_ = juce::jlimit(0.0f, 1.0f, x); }
    void setY(float y) { y_ = juce::jlimit(0.0f, 1.0f, y); }
    float getX() const { return x_; }
    float getY() const { return y_; }
    
    // Density controls (0.0 to 1.0)
    void setBDDensity(float density) { bdDensity_ = juce::jlimit(0.0f, 1.0f, density); }
    void setSDDensity(float density) { sdDensity_ = juce::jlimit(0.0f, 1.0f, density); }
    void setHHDensity(float density) { hhDensity_ = juce::jlimit(0.0f, 1.0f, density); }
    float getBDDensity() const { return bdDensity_; }
    float getSDDensity() const { return sdDensity_; }
    float getHHDensity() const { return hhDensity_; }
    
    // Chaos/randomness (0.0 to 1.0)
    void setChaos(float chaos) { chaos_ = juce::jlimit(0.0f, 1.0f, chaos); }
    
    // Swing amount (0.0 to 1.0, where 0.5 is no swing)
    void setSwing(float swing) { swing_ = juce::jlimit(0.0f, 1.0f, swing); }
    
    // Pattern reset
    void reset();
    
    // Advance the pattern by one step
    void tick();
    
    // Set the current step directly (for PPQ sync)
    void setCurrentStep(int step) { currentStep_ = step % 32; }
    
    // Get current step triggers (after tick)
    bool getBDTrigger() const { return bdTrigger_; }
    bool getSDTrigger() const { return sdTrigger_; }
    bool getHHTrigger() const { return hhTrigger_; }
    
    // Get accent triggers
    bool getBDAccent() const { return bdAccent_; }
    bool getSDAccent() const { return sdAccent_; }
    bool getHHAccent() const { return hhAccent_; }
    
    // Get current pattern step (0-31)
    int getCurrentStep() const { return currentStep_; }
    
    // Get interpolated pattern values for visualization
    std::array<uint8_t, 32> getBDPattern() const;
    std::array<uint8_t, 32> getSDPattern() const;
    std::array<uint8_t, 32> getHHPattern() const;
    
    // Evaluate drums for current step (public for retrigger mode)
    void evaluateDrums();
    
private:
    // Bilinear interpolation of pattern nodes
    uint8_t readDrumMap(int instrument, int step);
    
    // Apply density threshold
    bool applyDensity(uint8_t value, float density);
    
    // Apply chaos/randomness
    bool applyChaos(bool trigger);
    
    // Pattern position (0.0 to 1.0)
    float x_ = 0.5f;
    float y_ = 0.5f;
    
    // Density controls
    float bdDensity_ = 1.0f;
    float sdDensity_ = 1.0f;
    float hhDensity_ = 1.0f;
    
    // Chaos/randomness
    float chaos_ = 0.0f;
    
    // Swing
    float swing_ = 0.5f;
    
    // Current step in pattern (0-31)
    int currentStep_ = 0;
    int swingCounter_ = 0;
    
    // Trigger outputs
    bool bdTrigger_ = false;
    bool sdTrigger_ = false;
    bool hhTrigger_ = false;
    
    // Accent outputs
    bool bdAccent_ = false;
    bool sdAccent_ = false;
    bool hhAccent_ = false;
    
    // Random number generator
    std::mt19937 rng_;
    std::uniform_real_distribution<float> randomDist_{0.0f, 1.0f};
};