#include "VisageOverlayBase.h"

//==============================================================================
// Visage color constants (matching the aesthetic from VisageStyle.h)
const juce::Colour VisageOverlayBase::kOverlayBackground = juce::Colour(0x99000000); // 60% black
const juce::Colour VisageOverlayBase::kContentBackground = juce::Colour(0xff1e1e1e); // Dark background
const juce::Colour VisageOverlayBase::kBorderColor = juce::Colour(0xff404040);       // Subtle border
const juce::Colour VisageOverlayBase::kModalDimColor = juce::Colour(0x99000000);     // Modal dim

//==============================================================================
VisageOverlayBase::VisageOverlayBase()
    : animator(std::make_unique<OverlayAnimator>(*this))
{
    setWantsKeyboardFocus(true);
    setAlwaysOnTop(true);
    setVisible(false);
}

VisageOverlayBase::~VisageOverlayBase()
{
    if (parentComponent != nullptr)
        parentComponent->removeComponentListener(this);
}

//==============================================================================
void VisageOverlayBase::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Draw modal background with current alpha
    auto dimColor = kModalDimColor.withAlpha(currentAlpha * backgroundOpacity);
    g.setColour(dimColor);
    g.fillAll();
    
    // Calculate content area
    auto contentBounds = getContentBounds().toFloat();
    
    // Draw shadow effect
    if (currentAlpha > 0.0f)
    {
        auto shadowBounds = contentBounds.expanded(8.0f);
        auto shadowColor = juce::Colours::black.withAlpha(currentAlpha * 0.3f);
        
        // Multiple shadows for softer effect
        for (int i = 0; i < 4; ++i)
        {
            g.setColour(shadowColor.withAlpha(shadowColor.getFloatAlpha() * (0.8f - i * 0.15f)));
            auto shadowRect = contentBounds.expanded(2.0f + i * 2.0f);
            g.fillRoundedRectangle(shadowRect, kCornerRadius + i);
        }
    }
    
    // Draw content background
    auto contentColor = kContentBackground.withAlpha(currentAlpha);
    g.setColour(contentColor);
    g.fillRoundedRectangle(contentBounds, kCornerRadius);
    
    // Draw border
    auto borderColor = kBorderColor.withAlpha(currentAlpha);
    g.setColour(borderColor);
    g.drawRoundedRectangle(contentBounds, kCornerRadius, 1.0f);
}

void VisageOverlayBase::resized()
{
    updateBounds();
    
    if (contentComponent != nullptr)
    {
        contentComponent->setBounds(getContentBounds());
    }
}

bool VisageOverlayBase::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey && shouldCloseOnEscape())
    {
        hideOverlay();
        return true;
    }
    
    return Component::keyPressed(key);
}

void VisageOverlayBase::mouseDown(const juce::MouseEvent& e)
{
    if (shouldCloseOnBackgroundClick())
    {
        // Check if click is outside content area
        if (!getContentBounds().contains(e.getPosition()))
        {
            hideOverlay();
        }
    }
}

void VisageOverlayBase::parentHierarchyChanged()
{
    updateBounds();
}

void VisageOverlayBase::visibilityChanged()
{
    if (Component::isVisible())
    {
        grabKeyboardFocus();
        toFront(true);
    }
}

//==============================================================================
void VisageOverlayBase::componentBeingDeleted(juce::Component& component)
{
    if (&component == parentComponent)
    {
        parentComponent = nullptr;
        hideOverlay();
    }
}

//==============================================================================
void VisageOverlayBase::showOverlay(juce::Component* parent)
{
    if (parent == nullptr)
        return;
        
    // Clean up previous parent
    if (parentComponent != nullptr)
        parentComponent->removeComponentListener(this);
    
    parentComponent = parent;
    parentComponent->addComponentListener(this);
    
    // Add to parent and setup
    parent->addAndMakeVisible(this);
    setBounds(parent->getLocalBounds());
    
    isVisible = true;
    startFadeInAnimation();
    
    overlayShown();
}

void VisageOverlayBase::hideOverlay()
{
    if (!isVisible)
        return;
        
    isVisible = false;
    startFadeOutAnimation();
}

bool VisageOverlayBase::isOverlayVisible() const
{
    return isVisible;
}

//==============================================================================
void VisageOverlayBase::setAnimationDuration(int durationMs)
{
    animationDurationMs = juce::jmax(50, durationMs);
}

void VisageOverlayBase::setBackgroundOpacity(float opacity)
{
    backgroundOpacity = juce::jlimit(0.0f, 1.0f, opacity);
    repaint();
}

//==============================================================================
juce::Rectangle<int> VisageOverlayBase::getContentBounds() const
{
    auto bounds = getLocalBounds();
    auto contentWidth = juce::jmin(bounds.getWidth() - kPadding * 2, kDefaultWidth);
    auto contentHeight = juce::jmin(bounds.getHeight() - kPadding * 2, kDefaultHeight);
    
    return juce::Rectangle<int>(contentWidth, contentHeight)
        .withCentre(bounds.getCentre());
}

void VisageOverlayBase::setContentComponent(std::unique_ptr<juce::Component> content)
{
    if (contentComponent != nullptr)
        removeChildComponent(contentComponent.get());
    
    contentComponent = std::move(content);
    
    if (contentComponent != nullptr)
    {
        addAndMakeVisible(*contentComponent);
        contentComponent->setBounds(getContentBounds());
    }
}

juce::Component* VisageOverlayBase::getContentComponent() const
{
    return contentComponent.get();
}

//==============================================================================
void VisageOverlayBase::updateBounds()
{
    if (parentComponent != nullptr)
    {
        setBounds(parentComponent->getLocalBounds());
    }
}

void VisageOverlayBase::startFadeInAnimation()
{
    setVisible(true);
    toFront(true);
    grabKeyboardFocus();
    
    animator->startFadeAnimation(1.0f, animationDurationMs);
}

void VisageOverlayBase::startFadeOutAnimation()
{
    animator->startFadeAnimation(0.0f, animationDurationMs);
}

//==============================================================================
// OverlayAnimator Implementation
//==============================================================================

VisageOverlayBase::OverlayAnimator::OverlayAnimator(VisageOverlayBase& owner)
    : overlayOwner(owner)
{
}

void VisageOverlayBase::OverlayAnimator::timerCallback()
{
    currentStep++;
    
    if (currentStep >= totalSteps)
    {
        // Animation complete
        overlayOwner.currentAlpha = targetAlpha;
        stopTimer();
        
        // Handle fade out completion
        if (targetAlpha <= 0.0f && !overlayOwner.isVisible)
        {
            overlayOwner.setVisible(false);
            overlayOwner.overlayHidden();
            
            // Remove from parent
            if (overlayOwner.getParentComponent() != nullptr)
                overlayOwner.getParentComponent()->removeChildComponent(&overlayOwner);
        }
    }
    else
    {
        // Interpolate alpha using smooth easing
        float progress = static_cast<float>(currentStep) / static_cast<float>(totalSteps);
        
        // Smooth easing function (ease-in-out)
        progress = progress * progress * (3.0f - 2.0f * progress);
        
        overlayOwner.currentAlpha = startAlpha + (targetAlpha - startAlpha) * progress;
    }
    
    overlayOwner.repaint();
}

void VisageOverlayBase::OverlayAnimator::startFadeAnimation(float newTargetAlpha, int durationMs)
{
    startAlpha = overlayOwner.currentAlpha;
    targetAlpha = newTargetAlpha;
    
    // Calculate animation steps (60 FPS)
    totalSteps = juce::jmax(1, durationMs * 60 / 1000);
    currentStep = 0;
    
    // Start timer at 60 FPS
    startTimer(1000 / 60);
}

void VisageOverlayBase::OverlayAnimator::stopAnimation()
{
    stopTimer();
}