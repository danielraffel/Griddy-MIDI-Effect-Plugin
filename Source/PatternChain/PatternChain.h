#pragma once

#include <JuceHeader.h>
#include <vector>
#include <functional>

#ifdef ENABLE_PATTERN_CHAIN

/**
 * PatternChain - Sequential pattern automation system
 * 
 * Allows chaining multiple pattern configurations in sequence.
 * Each step can have different X/Y positions, densities, and modulation settings.
 * Transitions between patterns can be smooth or instant.
 */
class PatternChain
{
public:
    // Transition types between patterns
    enum TransitionType
    {
        INSTANT,        // Jump immediately to new values
        SMOOTH_MORPH,   // Interpolate over one bar
        CROSSFADE       // Fade out old, fade in new
    };
    
    // A single step in the pattern chain
    struct Step
    {
        // Pattern position
        float x = 0.5f;
        float y = 0.5f;
        
        // Modulation
        float chaos = 0.0f;
        float swing = 0.0f;
        
        // Densities
        float bdDensity = 0.5f;
        float sdDensity = 0.5f;
        float hhDensity = 0.5f;
        
        // Velocities (if enabled)
        float bdVelocity = 0.8f;
        float sdVelocity = 0.8f;
        float hhVelocity = 0.8f;
        
        // Duration
        int bars = 4;           // How many bars to play this pattern
        
        // Metadata
        juce::String name = "Pattern";
        juce::Colour colour = juce::Colours::grey;
        
        // Transition settings
        TransitionType transitionType = SMOOTH_MORPH;
        float transitionTime = 1.0f; // In bars
        
        Step() = default;
        
        Step(float xPos, float yPos, int numBars, const juce::String& stepName = "Pattern")
            : x(xPos), y(yPos), bars(numBars), name(stepName) {}
    };
    
    PatternChain() = default;
    
    // Enable/disable the chain
    void setEnabled(bool enabled)
    {
        enabled_ = enabled;
        if (enabled && !chain_.empty())
        {
            currentIndex_ = 0;
            barsRemaining_ = chain_[0].bars;
            barProgress_ = 0.0f;
            isTransitioning_ = false;
        }
    }
    
    bool isEnabled() const { return enabled_; }
    
    // Chain management
    void addStep(const Step& step)
    {
        chain_.push_back(step);
    }
    
    void insertStep(int index, const Step& step)
    {
        if (index >= 0 && index <= static_cast<int>(chain_.size()))
        {
            chain_.insert(chain_.begin() + index, step);
        }
    }
    
    void removeStep(int index)
    {
        if (index >= 0 && index < static_cast<int>(chain_.size()))
        {
            chain_.erase(chain_.begin() + index);
            
            // Adjust current index if needed
            if (currentIndex_ >= static_cast<int>(chain_.size()))
            {
                currentIndex_ = chain_.empty() ? 0 : 0;
                if (!chain_.empty())
                {
                    barsRemaining_ = chain_[currentIndex_].bars;
                }
            }
        }
    }
    
    void clearChain()
    {
        chain_.clear();
        currentIndex_ = 0;
        barsRemaining_ = 0;
        barProgress_ = 0.0f;
        isTransitioning_ = false;
    }
    
    // Get chain info
    int getNumSteps() const { return static_cast<int>(chain_.size()); }
    
    const Step* getStep(int index) const
    {
        if (index >= 0 && index < static_cast<int>(chain_.size()))
            return &chain_[index];
        return nullptr;
    }
    
    Step* getStep(int index)
    {
        if (index >= 0 && index < static_cast<int>(chain_.size()))
            return &chain_[index];
        return nullptr;
    }
    
    int getCurrentIndex() const { return currentIndex_; }
    
    const Step* getCurrentStep() const
    {
        return getStep(currentIndex_);
    }
    
    // Progress tracking
    float getBarProgress() const { return barProgress_; }
    int getBarsRemaining() const { return barsRemaining_; }
    
    // Advance the chain (called once per bar)
    void tickBar()
    {
        if (!enabled_ || chain_.empty()) return;
        
        barProgress_ = 0.0f;
        
        if (--barsRemaining_ <= 0)
        {
            // Move to next step
            int previousIndex = currentIndex_;
            currentIndex_ = (currentIndex_ + 1) % static_cast<int>(chain_.size());
            barsRemaining_ = chain_[currentIndex_].bars;
            
            // Start transition if needed
            if (chain_[currentIndex_].transitionType != INSTANT)
            {
                isTransitioning_ = true;
                transitionProgress_ = 0.0f;
                transitionStartIndex_ = previousIndex;
                transitionEndIndex_ = currentIndex_;
            }
            
            // Notify callback
            if (onStepChange_)
            {
                onStepChange_(chain_[currentIndex_], currentIndex_);
            }
        }
    }
    
    // Update progress within a bar (for smooth transitions)
    void updateBarProgress(float progress)
    {
        barProgress_ = juce::jlimit(0.0f, 1.0f, progress);
        
        if (isTransitioning_)
        {
            const auto& currentStep = chain_[transitionEndIndex_];
            transitionProgress_ += progress / currentStep.transitionTime;
            
            if (transitionProgress_ >= 1.0f)
            {
                transitionProgress_ = 1.0f;
                isTransitioning_ = false;
            }
        }
    }
    
    // Get interpolated values during transitions
    float getInterpolatedValue(float startValue, float endValue) const
    {
        if (!isTransitioning_) return endValue;
        
        const auto& step = chain_[transitionEndIndex_];
        
        switch (step.transitionType)
        {
            case INSTANT:
                return endValue;
                
            case SMOOTH_MORPH:
                // Smooth interpolation
                return startValue + (endValue - startValue) * smoothstep(transitionProgress_);
                
            case CROSSFADE:
                // Could implement volume-based crossfade here
                return startValue + (endValue - startValue) * transitionProgress_;
                
            default:
                return endValue;
        }
    }
    
    // Get current pattern values (with transition interpolation)
    Step getInterpolatedStep() const
    {
        if (!isTransitioning_ || chain_.empty())
        {
            return chain_.empty() ? Step() : chain_[currentIndex_];
        }
        
        const auto& startStep = chain_[transitionStartIndex_];
        const auto& endStep = chain_[transitionEndIndex_];
        
        Step result;
        result.x = getInterpolatedValue(startStep.x, endStep.x);
        result.y = getInterpolatedValue(startStep.y, endStep.y);
        result.chaos = getInterpolatedValue(startStep.chaos, endStep.chaos);
        result.swing = getInterpolatedValue(startStep.swing, endStep.swing);
        result.bdDensity = getInterpolatedValue(startStep.bdDensity, endStep.bdDensity);
        result.sdDensity = getInterpolatedValue(startStep.sdDensity, endStep.sdDensity);
        result.hhDensity = getInterpolatedValue(startStep.hhDensity, endStep.hhDensity);
        result.bdVelocity = getInterpolatedValue(startStep.bdVelocity, endStep.bdVelocity);
        result.sdVelocity = getInterpolatedValue(startStep.sdVelocity, endStep.sdVelocity);
        result.hhVelocity = getInterpolatedValue(startStep.hhVelocity, endStep.hhVelocity);
        result.bars = endStep.bars;
        result.name = endStep.name;
        result.colour = endStep.colour;
        
        return result;
    }
    
    // Callbacks
    std::function<void(const Step&, int index)> onStepChange_;
    
    // Save/restore state
    void saveToValueTree(juce::ValueTree& tree) const
    {
        tree.setProperty("enabled", enabled_, nullptr);
        tree.setProperty("currentIndex", currentIndex_, nullptr);
        tree.setProperty("barsRemaining", barsRemaining_, nullptr);
        
        auto stepsTree = tree.getOrCreateChildWithName("Steps", nullptr);
        stepsTree.removeAllChildren(nullptr);
        
        for (size_t i = 0; i < chain_.size(); ++i)
        {
            auto stepTree = stepsTree.createChild("Step");
            const auto& step = chain_[i];
            
            stepTree.setProperty("x", step.x, nullptr);
            stepTree.setProperty("y", step.y, nullptr);
            stepTree.setProperty("chaos", step.chaos, nullptr);
            stepTree.setProperty("swing", step.swing, nullptr);
            stepTree.setProperty("bdDensity", step.bdDensity, nullptr);
            stepTree.setProperty("sdDensity", step.sdDensity, nullptr);
            stepTree.setProperty("hhDensity", step.hhDensity, nullptr);
            stepTree.setProperty("bdVelocity", step.bdVelocity, nullptr);
            stepTree.setProperty("sdVelocity", step.sdVelocity, nullptr);
            stepTree.setProperty("hhVelocity", step.hhVelocity, nullptr);
            stepTree.setProperty("bars", step.bars, nullptr);
            stepTree.setProperty("name", step.name, nullptr);
            stepTree.setProperty("colour", step.colour.toString(), nullptr);
            stepTree.setProperty("transitionType", static_cast<int>(step.transitionType), nullptr);
            stepTree.setProperty("transitionTime", step.transitionTime, nullptr);
        }
    }
    
    void loadFromValueTree(const juce::ValueTree& tree)
    {
        enabled_ = tree.getProperty("enabled", false);
        currentIndex_ = tree.getProperty("currentIndex", 0);
        barsRemaining_ = tree.getProperty("barsRemaining", 4);
        
        chain_.clear();
        
        auto stepsTree = tree.getChildWithName("Steps");
        if (stepsTree.isValid())
        {
            for (int i = 0; i < stepsTree.getNumChildren(); ++i)
            {
                auto stepTree = stepsTree.getChild(i);
                Step step;
                
                step.x = stepTree.getProperty("x", 0.5f);
                step.y = stepTree.getProperty("y", 0.5f);
                step.chaos = stepTree.getProperty("chaos", 0.0f);
                step.swing = stepTree.getProperty("swing", 0.0f);
                step.bdDensity = stepTree.getProperty("bdDensity", 0.5f);
                step.sdDensity = stepTree.getProperty("sdDensity", 0.5f);
                step.hhDensity = stepTree.getProperty("hhDensity", 0.5f);
                step.bdVelocity = stepTree.getProperty("bdVelocity", 0.8f);
                step.sdVelocity = stepTree.getProperty("sdVelocity", 0.8f);
                step.hhVelocity = stepTree.getProperty("hhVelocity", 0.8f);
                step.bars = stepTree.getProperty("bars", 4);
                step.name = stepTree.getProperty("name", "Pattern").toString();
                
                juce::String colourStr = stepTree.getProperty("colour", "ff808080").toString();
                step.colour = juce::Colour::fromString(colourStr);
                
                step.transitionType = static_cast<TransitionType>(
                    static_cast<int>(stepTree.getProperty("transitionType", 1)));
                step.transitionTime = stepTree.getProperty("transitionTime", 1.0f);
                
                chain_.push_back(step);
            }
        }
    }
    
private:
    // Smooth interpolation function
    static float smoothstep(float t)
    {
        t = juce::jlimit(0.0f, 1.0f, t);
        return t * t * (3.0f - 2.0f * t);
    }
    
    std::vector<Step> chain_;
    bool enabled_ = false;
    
    // Playback state
    int currentIndex_ = 0;
    int barsRemaining_ = 0;
    float barProgress_ = 0.0f;
    
    // Transition state
    bool isTransitioning_ = false;
    float transitionProgress_ = 0.0f;
    int transitionStartIndex_ = 0;
    int transitionEndIndex_ = 0;
};

#endif // ENABLE_PATTERN_CHAIN