#pragma once

#include <JuceHeader.h>
#include "VisageTabBar.h"

// Forward declaration
class GridsAudioProcessor;

/**
 * VisageSettingsPanel - Tabbed settings panel for Griddy plugin
 * 
 * Features:
 * - Simple juce::Component (like PlunderTube) - no complex overlay
 * - Tab bar at the top (General, Advanced, Modulation)
 * - Content area that switches based on selected tab
 * - Close button at bottom right
 * - License button at bottom left
 * - 600x500 pixel size with proper spacing and padding
 * - Matches Visage dark theme aesthetic
 */
class VisageSettingsPanel : public juce::Component
{
public:
    //==============================================================================
    VisageSettingsPanel(GridsAudioProcessor& processor);
    ~VisageSettingsPanel() override = default;
    
    //==============================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;
    void mouseDown(const juce::MouseEvent& event) override;
    
    //==============================================================================
    // Public methods  
    void showPanel();
    void hidePanel();
    
    //==============================================================================
    // Settings management
    void showTab(const juce::String& tabId);
    juce::String getCurrentTab() const;
    
    //==============================================================================
    // Callbacks for external interaction
    std::function<void()> onCloseClicked;
    std::function<void()> onLicenseClicked;
    
private:
    //==============================================================================
    // UI Components
    // std::unique_ptr<VisageTabBar> tabBar; // Using simple buttons instead
    std::unique_ptr<juce::TextButton> generalTabButton;
    std::unique_ptr<juce::TextButton> advancedTabButton;
    std::unique_ptr<juce::TextButton> modulationTabButton;
    std::unique_ptr<juce::TextButton> closeButton;
    
    // Content components for each tab
    std::unique_ptr<juce::Component> generalContent;
    std::unique_ptr<juce::Component> advancedContent;
    std::unique_ptr<juce::Component> modulationContent;
    
    // Viewports for scrolling
    std::unique_ptr<juce::Viewport> generalViewport;
    std::unique_ptr<juce::Viewport> advancedViewport;
    std::unique_ptr<juce::Viewport> modulationViewport;
    
    juce::Component* currentContentComponent = nullptr;
    
    // Reference to processor
    GridsAudioProcessor& audioProcessor;
    
    //==============================================================================
    // Layout constants
    static constexpr int kPanelWidth = 560;
    static constexpr int kPanelHeight = 380;
    static constexpr int kTabBarHeight = 40;
    static constexpr int kButtonHeight = 24;
    static constexpr int kButtonWidth = 100;
    static constexpr int kBottomPadding = 8;
    static constexpr int kSidePadding = 20;
    static constexpr int kTabContentPadding = 16;
    
    //==============================================================================
    // Tab content creation
    void createTabContent();
    std::unique_ptr<juce::Component> createGeneralTabContent();
    std::unique_ptr<juce::Component> createAdvancedTabContent();
    std::unique_ptr<juce::Component> createModulationTabContent();
    
    //==============================================================================
    // Tab switching
    void onTabChanged(const juce::String& tabId, int tabIndex);
    void showTabContent(juce::Component* contentToShow);
    
    //==============================================================================
    // Button handlers
    void handleCloseButtonClicked();
    void handleLicenseButtonClicked();
    
    //==============================================================================
    // Styling
    void setupLookAndFeel();
    void styleButton(juce::TextButton& button, bool isPrimary = false);
    void updateTabButtonStates(int activeIndex);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VisageSettingsPanel)
};