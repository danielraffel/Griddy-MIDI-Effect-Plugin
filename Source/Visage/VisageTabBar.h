#pragma once

#include <JuceHeader.h>
#include <functional>

/**
 * VisageTabBar - Custom tab bar component for settings panels
 * 
 * Features:
 * - Multiple tabs with smooth transitions
 * - Orange accent color for active tab (0xffff8833)
 * - Dark theme matching Visage aesthetic
 * - Keyboard navigation support
 * - Customizable tab content via callback
 */
class VisageTabBar : public juce::Component,
                     public juce::KeyListener
{
public:
    //==============================================================================
    struct Tab
    {
        juce::String name;
        juce::String id;
        
        Tab() = default;
        Tab(const juce::String& tabName, const juce::String& tabId)
            : name(tabName), id(tabId) {}
    };
    
    //==============================================================================
    VisageTabBar();
    ~VisageTabBar() override = default;
    
    //==============================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    
    //==============================================================================
    // KeyListener overrides
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;
    
    //==============================================================================
    // Tab management
    void addTab(const juce::String& name, const juce::String& id);
    void removeTab(const juce::String& id);
    void clearTabs();
    
    //==============================================================================
    // Selection
    void setActiveTab(const juce::String& id);
    void setActiveTab(int index);
    juce::String getActiveTabId() const;
    int getActiveTabIndex() const;
    
    //==============================================================================
    // Callbacks
    std::function<void(const juce::String&, int)> onTabChanged;
    
    //==============================================================================
    // Styling
    void setTabHeight(int height);
    int getTabHeight() const { return tabHeight; }
    
private:
    //==============================================================================
    // Tab data
    juce::Array<Tab> tabs;
    int activeTabIndex = 0;
    int hoveredTabIndex = -1;
    
    //==============================================================================
    // Layout and styling
    int tabHeight = 40;
    int tabSpacing = 2;
    int tabPadding = 16;
    
    //==============================================================================
    // Colors (matching Visage theme)
    const juce::Colour activeTabColor = juce::Colour(0xffff8833);     // Orange accent
    const juce::Colour inactiveTabColor = juce::Colour(0xff2a2a2a);   // Dark gray
    const juce::Colour hoveredTabColor = juce::Colour(0xff3a3a3a);    // Slightly lighter gray
    const juce::Colour textColor = juce::Colour(0xffcccccc);          // Light gray text
    const juce::Colour activeTextColor = juce::Colour(0xff000000);    // Black text on orange
    const juce::Colour borderColor = juce::Colour(0xff1a1a1a);        // Very dark border
    
    //==============================================================================
    // Internal methods
    int getTabIndexAtPosition(int x, int y) const;
    juce::Rectangle<int> getTabBounds(int tabIndex) const;
    void selectTab(int index);
    
    //==============================================================================
    // Animation support (for future smooth transitions)
    float animationProgress = 1.0f;
    juce::ComponentAnimator animator;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VisageTabBar)
};