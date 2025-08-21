#pragma once

#include <JuceHeader.h>

/**
 * VisageOverlayBase - Modal overlay base class for settings panels and dialogs
 * 
 * Features:
 * - Modal background dimming (60% black)
 * - ESC key handling to close
 * - Click outside to close
 * - Smooth fade in/out animations
 * - Proper z-ordering (always on top)
 * - Visage aesthetic with dark theme and rounded corners
 */
class VisageOverlayBase : public juce::Component,
                         public juce::ComponentListener
{
public:
    //==============================================================================
    VisageOverlayBase();
    ~VisageOverlayBase() override;
    
    //==============================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void parentHierarchyChanged() override;
    void visibilityChanged() override;
    
    //==============================================================================
    // ComponentListener overrides
    void componentBeingDeleted(juce::Component& component) override;
    
    //==============================================================================
    // Modal overlay functionality
    void showOverlay(juce::Component* parent);
    void hideOverlay();
    bool isOverlayVisible() const;
    
    //==============================================================================
    // Animation settings
    void setAnimationDuration(int durationMs);
    void setBackgroundOpacity(float opacity);
    
    //==============================================================================
    // Override points for derived classes
    virtual void overlayShown() {}
    virtual void overlayHidden() {}
    virtual bool shouldCloseOnEscape() { return true; }
    virtual bool shouldCloseOnBackgroundClick() { return true; }
    
protected:
    //==============================================================================
    // Content area for derived classes
    juce::Rectangle<int> getContentBounds() const;
    void setContentComponent(std::unique_ptr<juce::Component> content);
    juce::Component* getContentComponent() const;
    
    //==============================================================================
    // Visage styling constants
    static constexpr float kCornerRadius = 10.0f;
    static constexpr int kDefaultWidth = 400;
    static constexpr int kDefaultHeight = 300;
    static constexpr int kPadding = 20;
    
    // Visage color scheme (matching VisageStyle.h)
    static const juce::Colour kOverlayBackground;
    static const juce::Colour kContentBackground;
    static const juce::Colour kBorderColor;
    static const juce::Colour kModalDimColor;

private:
    //==============================================================================
    // Animation and state
    class OverlayAnimator;
    std::unique_ptr<OverlayAnimator> animator;
    
    juce::Component* parentComponent = nullptr;
    std::unique_ptr<juce::Component> contentComponent;
    
    bool isVisible = false;
    int animationDurationMs = 200;
    float backgroundOpacity = 0.6f;
    float currentAlpha = 0.0f;
    
    //==============================================================================
    // Internal methods
    void updateBounds();
    void startFadeInAnimation();
    void startFadeOutAnimation();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VisageOverlayBase)
};

/**
 * Animator class for smooth fade in/out transitions
 */
class VisageOverlayBase::OverlayAnimator : public juce::Timer
{
public:
    OverlayAnimator(VisageOverlayBase& owner);
    
    void timerCallback() override;
    void startFadeAnimation(float targetAlpha, int durationMs);
    void stopAnimation();
    
private:
    VisageOverlayBase& overlayOwner;
    float startAlpha = 0.0f;
    float targetAlpha = 0.0f;
    int totalSteps = 0;
    int currentStep = 0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OverlayAnimator)
};