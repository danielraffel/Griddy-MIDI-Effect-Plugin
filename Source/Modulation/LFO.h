#pragma once

#include <JuceHeader.h>
#include <cmath>

#ifdef ENABLE_MODULATION_MATRIX

/**
 * LFO - Low Frequency Oscillator for modulation
 * 
 * Generates periodic waveforms at musical rates for parameter modulation.
 * Supports multiple waveform shapes and syncs to host tempo.
 */
class LFO
{
public:
    enum Shape
    {
        SINE,
        TRIANGLE,
        SQUARE,
        SAW,
        RANDOM
    };
    
    LFO() = default;
    
    // Enable/disable the LFO
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }
    
    // Set rate in beats per cycle
    void setRate(float beatsPerCycle)
    {
        rate_ = juce::jlimit(0.25f, 16.0f, beatsPerCycle);
    }
    float getRate() const { return rate_; }
    
    // Set waveform shape
    void setShape(Shape shape) { shape_ = shape; }
    Shape getShape() const { return shape_; }
    
    // Set depth/amount (0-1)
    void setDepth(float depth)
    {
        depth_ = juce::jlimit(0.0f, 1.0f, depth);
    }
    float getDepth() const { return depth_; }
    
    // Advance the LFO by one audio block
    void advance(double samplesPerBeat, int numSamples)
    {
        if (!enabled_) return;
        
        double phaseIncrement = 1.0 / (samplesPerBeat * rate_);
        
        for (int i = 0; i < numSamples; ++i)
        {
            phase_ += phaseIncrement;
            
            // Wrap phase
            while (phase_ >= 1.0) 
            {
                phase_ -= 1.0;
                
                // Generate new random value at cycle boundary
                if (shape_ == RANDOM)
                {
                    lastRandom_ = (random_.nextFloat() * 2.0f) - 1.0f;
                }
            }
        }
    }
    
    // Get current LFO value (-1 to +1)
    float getValue() const
    {
        if (!enabled_) return 0.0f;
        
        float rawValue = 0.0f;
        
        switch (shape_)
        {
            case SINE:
                rawValue = std::sin(phase_ * 2.0 * M_PI);
                break;
                
            case TRIANGLE:
                if (phase_ < 0.5)
                    rawValue = 4.0f * phase_ - 1.0f;
                else
                    rawValue = 3.0f - 4.0f * phase_;
                break;
                
            case SQUARE:
                rawValue = phase_ < 0.5f ? 1.0f : -1.0f;
                break;
                
            case SAW:
                rawValue = 2.0f * phase_ - 1.0f;
                break;
                
            case RANDOM:
                rawValue = lastRandom_;
                break;
        }
        
        return rawValue * depth_;
    }
    
    // Get unipolar value (0 to 1) for modulating positive-only parameters
    float getUnipolarValue() const
    {
        return (getValue() + 1.0f) * 0.5f;
    }
    
    // Reset phase to beginning
    void reset()
    {
        phase_ = 0.0;
        lastRandom_ = 0.0f;
    }
    
    // Sync phase to a specific position (0-1)
    void syncPhase(double phase)
    {
        phase_ = juce::jlimit(0.0, 1.0, phase);
    }
    
    // Save/restore state
    void saveToValueTree(juce::ValueTree& tree) const
    {
        tree.setProperty("enabled", enabled_, nullptr);
        tree.setProperty("rate", rate_, nullptr);
        tree.setProperty("shape", static_cast<int>(shape_), nullptr);
        tree.setProperty("depth", depth_, nullptr);
        tree.setProperty("phase", phase_, nullptr);
    }
    
    void loadFromValueTree(const juce::ValueTree& tree)
    {
        enabled_ = tree.getProperty("enabled", false);
        rate_ = tree.getProperty("rate", 4.0f);
        shape_ = static_cast<Shape>(static_cast<int>(tree.getProperty("shape", 0)));
        depth_ = tree.getProperty("depth", 0.5f);
        phase_ = tree.getProperty("phase", 0.0);
    }
    
private:
    bool enabled_ = false;
    float rate_ = 4.0f;      // Beats per cycle
    Shape shape_ = SINE;
    float depth_ = 0.5f;      // Modulation amount (0-1)
    double phase_ = 0.0;      // Current phase (0-1)
    float lastRandom_ = 0.0f; // Last random value
    juce::Random random_;     // Random number generator
};

#endif // ENABLE_MODULATION_MATRIX