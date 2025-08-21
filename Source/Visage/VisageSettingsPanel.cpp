#include "VisageSettingsPanel.h"
#include "../PluginProcessor.h"
#include "../Settings/SettingsManager.h"
#include "BinaryData.h"

#ifdef ENABLE_MODULATION_MATRIX
#include "../Modulation/ModulationMatrix.h"
#include "../Modulation/LFO.h"
#endif

//==============================================================================
VisageSettingsPanel::VisageSettingsPanel(GridsAudioProcessor& processor)
    : audioProcessor(processor)
{
    setSize(kPanelWidth, kPanelHeight);
    
    // Create simple tab buttons (simplified approach)
    generalTabButton = std::make_unique<juce::TextButton>("General");
    generalTabButton->onClick = [this] { 
        onTabChanged("general", 0);
        updateTabButtonStates(0);
    };
    generalTabButton->setClickingTogglesState(true);
    generalTabButton->setRadioGroupId(1001);
    generalTabButton->setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(generalTabButton.get());
    
    advancedTabButton = std::make_unique<juce::TextButton>("Advanced");
    advancedTabButton->onClick = [this] { 
        onTabChanged("advanced", 1);
        updateTabButtonStates(1);
    };
    advancedTabButton->setClickingTogglesState(true);
    advancedTabButton->setRadioGroupId(1001);
    addAndMakeVisible(advancedTabButton.get());
    
    modulationTabButton = std::make_unique<juce::TextButton>("Modulation");
    modulationTabButton->onClick = [this] { 
        onTabChanged("modulation", 2);
        updateTabButtonStates(2);
    };
    modulationTabButton->setClickingTogglesState(true);
    modulationTabButton->setRadioGroupId(1001);
    addAndMakeVisible(modulationTabButton.get());
    
    // Set initial tab button colors
    updateTabButtonStates(0);
    
    // Create close button in top-right corner
    closeButton = std::make_unique<juce::TextButton>("Close");
    closeButton->onClick = [this] { handleCloseButtonClicked(); };
    closeButton->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a3a3a));
    closeButton->setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff4a4a4a));
    closeButton->setColour(juce::TextButton::textColourOffId, juce::Colour(0xffeeeeee));
    closeButton->setColour(juce::TextButton::textColourOnId, juce::Colour(0xffff8833));
    closeButton->setAlwaysOnTop(true);
    addAndMakeVisible(closeButton.get());
    
    // License button moved to General tab content
    
    // Create tab content
    createTabContent();
    
    // Show the first tab by default
    onTabChanged("general", 0);
    
    // Set up look and feel
    setupLookAndFeel();
    
    // Make sure we can receive keyboard focus
    setWantsKeyboardFocus(true);
    
    // Start with focus so ESC key works immediately
    grabKeyboardFocus();
}

//==============================================================================
void VisageSettingsPanel::paint(juce::Graphics& g)
{
    // Draw background overlay
    g.fillAll(juce::Colour(0x99000000)); // Semi-transparent black
    
    // Get the panel bounds (centered in parent)
    auto bounds = getLocalBounds().withSizeKeepingCentre(kPanelWidth, kPanelHeight);
    auto panelBounds = bounds; // Keep a copy for the close button
    
    // Draw main panel background
    g.setColour(juce::Colour(0xff1e1e1e)); // Dark background
    g.fillRoundedRectangle(bounds.toFloat(), 8.0f);
    
    // Draw subtle border
    g.setColour(juce::Colour(0xff404040)); // Border color
    g.drawRoundedRectangle(bounds.toFloat(), 8.0f, 1.0f);
    
    // Draw title in the top area (reduced to 45px for compact layout)
    auto titleBounds = bounds.removeFromTop(45);
    auto titleTextBounds = titleBounds; // Use full width for centering
    g.setColour(juce::Colour(0xffffffff));
    g.setFont(juce::Font(20.0f, juce::Font::bold));
    g.drawText("Settings", titleTextBounds, juce::Justification::centred);
    
    // No X button - using Close button instead
    
    // Additional spacing after title (reduced for compact layout)
    bounds.removeFromTop(10);
    
    // Tab bar area
    auto tabBarBounds = bounds.removeFromTop(kTabBarHeight);
    
    // Draw separator line 17px below tab bar to align with scrollbar edge
    g.setColour(juce::Colour(0xff404040)); // Match border color
    g.drawHorizontalLine(tabBarBounds.getBottom() + 17, 
                        panelBounds.getX(), 
                        panelBounds.getRight());
    
    // No footer area needed - content goes to bottom
}

//==============================================================================
void VisageSettingsPanel::resized()
{
    // Get the panel bounds (centered in parent)
    auto bounds = getLocalBounds().withSizeKeepingCentre(kPanelWidth, kPanelHeight);
    
    // Leave space at the top for the window title bar and title text (reduced for compact)
    bounds.removeFromTop(45);
    
    // Small additional spacing to ensure tabs are fully visible (reduced for compact)
    bounds.removeFromTop(10);
    
    // Position tab buttons below the title area with proper spacing
    auto tabBarBounds = bounds.removeFromTop(kTabBarHeight);
    tabBarBounds = tabBarBounds.reduced(kSidePadding, 0);
    
    // Divide width equally among tab buttons
    int tabWidth = tabBarBounds.getWidth() / 3;
    if (generalTabButton)
        generalTabButton->setBounds(tabBarBounds.removeFromLeft(tabWidth));
    if (advancedTabButton)
        advancedTabButton->setBounds(tabBarBounds.removeFromLeft(tabWidth));
    if (modulationTabButton)
        modulationTabButton->setBounds(tabBarBounds);
    
    // Add spacing after tab buttons
    bounds.removeFromTop(14); // Space for separator line and a bit more
    
    // Position Close button in top-right corner
    if (closeButton)
    {
        auto panelBounds = getLocalBounds().withSizeKeepingCentre(kPanelWidth, kPanelHeight);
        auto closeButtonBounds = panelBounds.removeFromTop(45).removeFromRight(80);
        closeButtonBounds = closeButtonBounds.reduced(10, 10);
        closeButton->setBounds(closeButtonBounds);
        closeButton->toFront(true);
        closeButton->setAlwaysOnTop(true);
    }
    
    // Content area takes all remaining space (no footer)
    auto contentBounds = bounds.reduced(kSidePadding, 0);
    contentBounds.removeFromTop(5); // Small padding from tabs
    contentBounds.removeFromBottom(10); // Small bottom padding
    
    // Position current content component
    if (currentContentComponent)
    {
        currentContentComponent->setBounds(contentBounds);
    }
    
    // Set bounds for all viewport components (even if not visible)
    if (generalViewport) generalViewport->setBounds(contentBounds);
    if (advancedViewport) advancedViewport->setBounds(contentBounds);
    if (modulationViewport) modulationViewport->setBounds(contentBounds);
}

//==============================================================================
void VisageSettingsPanel::showPanel()
{
    setVisible(true);
    // Grab keyboard focus for ESC key handling
    grabKeyboardFocus();
}

//==============================================================================
void VisageSettingsPanel::hidePanel()
{
    setVisible(false);
}

//==============================================================================
bool VisageSettingsPanel::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        handleCloseButtonClicked();
        return true;
    }
    return false;
}

//==============================================================================
void VisageSettingsPanel::mouseDown(const juce::MouseEvent& event)
{
    // Mouse down handled by Close button directly
}

//==============================================================================
void VisageSettingsPanel::showTab(const juce::String& tabId)
{
    if (tabId == "general")
    {
        onTabChanged("general", 0);
        updateTabButtonStates(0);
    }
    else if (tabId == "advanced")
    {
        onTabChanged("advanced", 1);
        updateTabButtonStates(1);
    }
    else if (tabId == "modulation")
    {
        onTabChanged("modulation", 2);
        updateTabButtonStates(2);
    }
}

//==============================================================================
juce::String VisageSettingsPanel::getCurrentTab() const
{
    if (generalTabButton && generalTabButton->getToggleState())
        return "general";
    if (advancedTabButton && advancedTabButton->getToggleState())
        return "advanced";
    if (modulationTabButton && modulationTabButton->getToggleState())
        return "modulation";
    
    return "general";
}

//==============================================================================
void VisageSettingsPanel::createTabContent()
{
    // Create content for each tab
    DBG("Creating general tab content...");
    generalContent = createGeneralTabContent();
    generalViewport = std::make_unique<juce::Viewport>();
    generalViewport->setViewedComponent(generalContent.get(), false);
    generalViewport->setScrollBarsShown(true, false);
    generalViewport->getViewedComponent()->setSize(560, 500); // Set content size to include Acknowledgements
    addChildComponent(generalViewport.get());
    DBG("General tab created");
    
    DBG("Creating advanced tab content...");
    advancedContent = createAdvancedTabContent();
    advancedViewport = std::make_unique<juce::Viewport>();
    advancedViewport->setViewedComponent(advancedContent.get(), false);
    advancedViewport->setScrollBarsShown(true, false);
    advancedViewport->getViewedComponent()->setSize(560, 800); // Set content size for scrolling
    addChildComponent(advancedViewport.get());
    DBG("Advanced tab created");
    
    DBG("Creating modulation tab content...");
    modulationContent = createModulationTabContent();
    modulationViewport = std::make_unique<juce::Viewport>();
    modulationViewport->setViewedComponent(modulationContent.get(), false);
    modulationViewport->setScrollBarsShown(true, false);
    modulationViewport->getViewedComponent()->setSize(560, 1100); // Further increased to show all destination rows including velocity and MIDI notes
    addChildComponent(modulationViewport.get());
    DBG("Modulation tab created");
    
    DBG("All tab content created and added");
}

//==============================================================================
std::unique_ptr<juce::Component> VisageSettingsPanel::createGeneralTabContent()
{
    // Create a custom component that handles its own layout
    class GeneralTabContent : public juce::Component
    {
    public:
        GeneralTabContent(GridsAudioProcessor& processor) : audioProcessor(processor)
        {
            // Initialize settings manager
            auto& settings = SettingsManager::getInstance();
            settings.initialise();
            
            // Header
            headerLabel.setText("Default Settings for New Sessions", juce::dontSendNotification);
            headerLabel.setFont(juce::Font(18.0f, juce::Font::bold));
            headerLabel.setColour(juce::Label::textColourId, juce::Colour(0xffeeeeee));
            addAndMakeVisible(headerLabel);
            
            subHeaderLabel.setText("These preferences apply to new projects. Existing projects keep their saved settings.", 
                                 juce::dontSendNotification);
            subHeaderLabel.setFont(juce::Font(12.0f));
            subHeaderLabel.setColour(juce::Label::textColourId, juce::Colour(0xff999999));
            addAndMakeVisible(subHeaderLabel);
            
            // Pattern Reset Section
            resetSectionLabel.setText("Pattern Reset", juce::dontSendNotification);
            resetSectionLabel.setFont(juce::Font(14.0f, juce::Font::bold));
            resetSectionLabel.setColour(juce::Label::textColourId, juce::Colour(0xffdddddd));
            addAndMakeVisible(resetSectionLabel);
            
            // Reset Mode Radio Buttons
            resetModeLabel.setText("Default Reset Mode:", juce::dontSendNotification);
            resetModeLabel.setFont(juce::Font(12.0f));
            resetModeLabel.setColour(juce::Label::textColourId, juce::Colour(0xffcccccc));
            addAndMakeVisible(resetModeLabel);
            
            transparentButton.setButtonText("Transparent (Silent position reset)");
            transparentButton.setRadioGroupId(1001);
            transparentButton.setColour(juce::ToggleButton::textColourId, juce::Colour(0xffcccccc));
            transparentButton.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xffff8833));
            addAndMakeVisible(transparentButton);
            
            retriggerButton.setButtonText("Retrigger (Drill'n'Bass instant fire)");
            retriggerButton.setRadioGroupId(1001);
            retriggerButton.setColour(juce::ToggleButton::textColourId, juce::Colour(0xffcccccc));
            retriggerButton.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xffff8833));
            addAndMakeVisible(retriggerButton);
            
            // Load current reset mode setting
            auto resetMode = settings.getString(SettingsManager::Keys::defaultResetMode, "transparent");
            transparentButton.setToggleState(resetMode == "transparent", juce::dontSendNotification);
            retriggerButton.setToggleState(resetMode == "retrigger", juce::dontSendNotification);
            
            // Save when changed
            transparentButton.onClick = [this] {
                SettingsManager::getInstance().setString(SettingsManager::Keys::defaultResetMode, "transparent");
            };
            retriggerButton.onClick = [this] {
                SettingsManager::getInstance().setString(SettingsManager::Keys::defaultResetMode, "retrigger");
            };
            
            // MIDI Section
            midiSectionLabel.setText("MIDI Defaults", juce::dontSendNotification);
            midiSectionLabel.setFont(juce::Font(14.0f, juce::Font::bold));
            midiSectionLabel.setColour(juce::Label::textColourId, juce::Colour(0xffdddddd));
            addAndMakeVisible(midiSectionLabel);
            
            // MIDI Thru
            midiThruBox.setButtonText("MIDI Thru enabled by default");
            midiThruBox.setToggleState(settings.getBool(SettingsManager::Keys::midiThruDefault, false),
                                      juce::dontSendNotification);
            midiThruBox.setColour(juce::ToggleButton::textColourId, juce::Colour(0xffcccccc));
            midiThruBox.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xffff8833));
            midiThruBox.onClick = [this] {
                SettingsManager::getInstance().setBool(SettingsManager::Keys::midiThruDefault,
                                                      midiThruBox.getToggleState());
            };
            addAndMakeVisible(midiThruBox);
            
            // MIDI Note Numbers
            noteInfoLabel.setText("Default MIDI note mappings:", juce::dontSendNotification);
            noteInfoLabel.setFont(juce::Font(12.0f));
            noteInfoLabel.setColour(juce::Label::textColourId, juce::Colour(0xffcccccc));
            addAndMakeVisible(noteInfoLabel);
            
            // BD Note selector
            bdNoteLabel.setText("BD:", juce::dontSendNotification);
            bdNoteLabel.setFont(juce::Font(12.0f));
            bdNoteLabel.setColour(juce::Label::textColourId, juce::Colour(0xffcccccc));
            addAndMakeVisible(bdNoteLabel);
            
            setupNoteDropdown(bdNoteBox, 36); // Default C1
            bdNoteBox.onChange = [this] {
                int noteNum = bdNoteBox.getSelectedId() - 1;
                SettingsManager::getInstance().setInt("defaultBDNote", noteNum);
            };
            addAndMakeVisible(bdNoteBox);
            
            // SD Note selector
            sdNoteLabel.setText("SD:", juce::dontSendNotification);
            sdNoteLabel.setFont(juce::Font(12.0f));
            sdNoteLabel.setColour(juce::Label::textColourId, juce::Colour(0xffcccccc));
            addAndMakeVisible(sdNoteLabel);
            
            setupNoteDropdown(sdNoteBox, 38); // Default D1
            sdNoteBox.onChange = [this] {
                int noteNum = sdNoteBox.getSelectedId() - 1;
                SettingsManager::getInstance().setInt("defaultSDNote", noteNum);
            };
            addAndMakeVisible(sdNoteBox);
            
            // HH Note selector
            hhNoteLabel.setText("HH:", juce::dontSendNotification);
            hhNoteLabel.setFont(juce::Font(12.0f));
            hhNoteLabel.setColour(juce::Label::textColourId, juce::Colour(0xffcccccc));
            addAndMakeVisible(hhNoteLabel);
            
            setupNoteDropdown(hhNoteBox, 42); // Default F#1
            hhNoteBox.onChange = [this] {
                int noteNum = hhNoteBox.getSelectedId() - 1;
                SettingsManager::getInstance().setInt("defaultHHNote", noteNum);
            };
            addAndMakeVisible(hhNoteBox);
            
            // Acknowledgements section
            acknowledgementsSectionLabel.setText("About", juce::dontSendNotification);
            acknowledgementsSectionLabel.setFont(juce::Font(14.0f, juce::Font::bold));
            acknowledgementsSectionLabel.setColour(juce::Label::textColourId, juce::Colour(0xffdddddd));
            addAndMakeVisible(acknowledgementsSectionLabel);
            
            // Acknowledgements button
            acknowledgementsButton.setButtonText("Acknowledgements");
            acknowledgementsButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a2a));
            acknowledgementsButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffcccccc));
            acknowledgementsButton.onClick = [] {
                // Get the griddy-licenses.html content from JUCE binary data
                // The binary data name is created from the filename with hyphens and dots replaced
                int dataSize = 0;
                const char* htmlData = BinaryData::griddylicenses_html;
                dataSize = BinaryData::griddylicenses_htmlSize;
                
                if (htmlData != nullptr && dataSize > 0)
                {
                    juce::String htmlContent = juce::String::fromUTF8(htmlData, dataSize);
                    
                    // Create a temporary HTML file
                    auto tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
                    auto htmlFile = tempDir.getChildFile("Griddy_Licenses.html");
                    
                    // Write the HTML content to the file
                    if (htmlFile.replaceWithText(htmlContent))
                    {
                        // Use URL launching (safer for plugins)
                        juce::URL fileURL(htmlFile);
                        fileURL.launchInDefaultBrowser();
                    }
                    else
                    {
                        // Fallback: show an alert with basic license info
                        juce::AlertWindow::showMessageBoxAsync(
                            juce::AlertWindow::InfoIcon,
                            "Griddy Licenses",
                            "Could not create temporary HTML file. Please check the project documentation for license information."
                        );
                    }
                }
                else
                {
                    DBG("Could not find griddy-licenses.html in binary data");
                }
            };
            addAndMakeVisible(acknowledgementsButton);
        }
        
        void resized() override
        {
            auto bounds = getLocalBounds().reduced(10);
            
            // Header
            headerLabel.setBounds(bounds.removeFromTop(25));
            bounds.removeFromTop(5);
            subHeaderLabel.setBounds(bounds.removeFromTop(40));
            bounds.removeFromTop(20);
            
            // Pattern Reset Section
            resetSectionLabel.setBounds(bounds.removeFromTop(20));
            bounds.removeFromTop(10);
            resetModeLabel.setBounds(bounds.removeFromTop(20));
            bounds.removeFromTop(5);
            transparentButton.setBounds(bounds.removeFromTop(24));
            retriggerButton.setBounds(bounds.removeFromTop(24));
            bounds.removeFromTop(20);
            
            // MIDI Section
            midiSectionLabel.setBounds(bounds.removeFromTop(20));
            bounds.removeFromTop(10);
            midiThruBox.setBounds(bounds.removeFromTop(24));
            bounds.removeFromTop(10);
            noteInfoLabel.setBounds(bounds.removeFromTop(20));
            bounds.removeFromTop(5);
            
            // MIDI note dropdowns in a row
            auto noteRow = bounds.removeFromTop(30);
            bdNoteLabel.setBounds(noteRow.removeFromLeft(30));
            bdNoteBox.setBounds(noteRow.removeFromLeft(120));
            noteRow.removeFromLeft(20);
            sdNoteLabel.setBounds(noteRow.removeFromLeft(30));
            sdNoteBox.setBounds(noteRow.removeFromLeft(120));
            noteRow.removeFromLeft(20);
            hhNoteLabel.setBounds(noteRow.removeFromLeft(30));
            hhNoteBox.setBounds(noteRow.removeFromLeft(120));
            bounds.removeFromTop(20);
            
            // Acknowledgements Section
            acknowledgementsSectionLabel.setBounds(bounds.removeFromTop(20));
            bounds.removeFromTop(10);
            acknowledgementsButton.setBounds(bounds.removeFromTop(30).removeFromLeft(150));
        }
        
    private:
        GridsAudioProcessor& audioProcessor;
        
        // Helper to populate note dropdowns
        void setupNoteDropdown(juce::ComboBox& box, int defaultNote)
        {
            const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
            
            for (int i = 0; i < 128; i++)
            {
                int octave = (i / 12) - 2; // MIDI octave starts at -2
                int noteIndex = i % 12;
                juce::String noteName = juce::String(noteNames[noteIndex]) + juce::String(octave) + " (" + juce::String(i) + ")";
                box.addItem(noteName, i + 1); // ComboBox IDs start at 1
            }
            
            box.setSelectedId(defaultNote + 1);
            box.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2a2a2a));
            box.setColour(juce::ComboBox::textColourId, juce::Colour(0xffcccccc));
            box.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff404040));
            box.setColour(juce::ComboBox::arrowColourId, juce::Colour(0xff888888));
            box.setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff2a2a2a));
            box.setColour(juce::PopupMenu::textColourId, juce::Colour(0xffcccccc));
            box.setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff404040));
        }
        
        // Labels
        juce::Label headerLabel;
        juce::Label subHeaderLabel;
        juce::Label resetSectionLabel;
        juce::Label resetModeLabel;
        juce::Label midiSectionLabel;
        juce::Label noteInfoLabel;
        juce::Label bdNoteLabel, sdNoteLabel, hhNoteLabel;
        juce::Label acknowledgementsSectionLabel;
        
        // Controls
        juce::ToggleButton transparentButton;
        juce::ToggleButton retriggerButton;
        juce::ToggleButton midiThruBox;
        juce::ComboBox bdNoteBox, sdNoteBox, hhNoteBox;
        juce::TextButton acknowledgementsButton;
    };
    
    return std::make_unique<GeneralTabContent>(audioProcessor);
}

//==============================================================================
std::unique_ptr<juce::Component> VisageSettingsPanel::createAdvancedTabContent()
{
    // Create a custom component that handles its own layout
    class AdvancedTabContent : public juce::Component, private juce::Timer
    {
    public:
        AdvancedTabContent(GridsAudioProcessor& processor) : audioProcessor(processor)
        {
            startTimerHz(4); // Update 4 times per second
            // Initialize settings manager
            auto& settings = SettingsManager::getInstance();
            settings.initialise();
            
            // Header
            headerLabel.setText("Advanced Pattern Settings", juce::dontSendNotification);
            headerLabel.setFont(juce::Font(18.0f, juce::Font::bold));
            headerLabel.setColour(juce::Label::textColourId, juce::Colour(0xffeeeeee));
            addAndMakeVisible(headerLabel);
            
            // Pattern Reset Section
            resetSectionLabel.setText("Pattern Reset", juce::dontSendNotification);
            resetSectionLabel.setFont(juce::Font(14.0f, juce::Font::bold));
            resetSectionLabel.setColour(juce::Label::textColourId, juce::Colour(0xffdddddd));
            addAndMakeVisible(resetSectionLabel);
            
            // Reset Quantization
            resetQuantizeLabel.setText("Default Reset Quantization:", juce::dontSendNotification);
            resetQuantizeLabel.setFont(juce::Font(12.0f));
            resetQuantizeLabel.setColour(juce::Label::textColourId, juce::Colour(0xffcccccc));
            addAndMakeVisible(resetQuantizeLabel);
            
            resetQuantizeBox.addItem("Off (Immediate)", 1);
            resetQuantizeBox.addItem("2 Bars", 2);
            resetQuantizeBox.addItem("1 Bar", 3);
            resetQuantizeBox.addItem("1/2 Note", 4);
            resetQuantizeBox.addItem("1/4 Note (Beat)", 5);
            resetQuantizeBox.addItem("1/8 Note", 6);
            resetQuantizeBox.addItem("1/16 Note", 7);
            resetQuantizeBox.addItem("1/32 Note", 8);
            resetQuantizeBox.addItem("1/4 Triplet", 9);
            resetQuantizeBox.addItem("1/8 Triplet", 10);
            resetQuantizeBox.addItem("1/16 Triplet", 11);
            resetQuantizeBox.setSelectedId(settings.getInt(SettingsManager::Keys::defaultResetQuantize, 1));
            resetQuantizeBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2a2a2a));
            resetQuantizeBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xffcccccc));
            resetQuantizeBox.onChange = [this] {
                SettingsManager::getInstance().setInt(SettingsManager::Keys::defaultResetQuantize,
                                                     resetQuantizeBox.getSelectedId());
                // Apply to current session
                if (auto* processor = dynamic_cast<GridsAudioProcessor*>(&audioProcessor)) {
                    processor->setResetQuantize(static_cast<QuantizeValue>(resetQuantizeBox.getSelectedId() - 1));
                }
            };
            addAndMakeVisible(resetQuantizeBox);
            
            // MIDI Learn section
            midiLearnLabel.setText("MIDI Learn", juce::dontSendNotification);
            midiLearnLabel.setFont(juce::Font(14.0f, juce::Font::bold));
            midiLearnLabel.setColour(juce::Label::textColourId, juce::Colour(0xffdddddd));
            addAndMakeVisible(midiLearnLabel);
            
            // MIDI learn button for reset
            resetMidiLearnButton.setButtonText("Learn Reset CC");
            resetMidiLearnButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a2a));
            resetMidiLearnButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffcccccc));
            resetMidiLearnButton.onClick = [this] {
                if (auto* processor = dynamic_cast<GridsAudioProcessor*>(&audioProcessor)) {
                    if (processor->isMidiLearning()) {
                        processor->stopMidiLearn();
                        resetMidiLearnButton.setButtonText("Learn Reset CC");
                    } else {
                        processor->startMidiLearnForReset();
                        resetMidiLearnButton.setButtonText("Learning... (move a CC)");
                    }
                }
            };
            addAndMakeVisible(resetMidiLearnButton);
            
            // Display current CC assignment
            resetCCLabel.setText("Reset CC: None", juce::dontSendNotification);
            resetCCLabel.setFont(juce::Font(12.0f));
            resetCCLabel.setColour(juce::Label::textColourId, juce::Colour(0xffcccccc));
            addAndMakeVisible(resetCCLabel);
            
            // Pattern Generation Section
            patternSectionLabel.setText("Pattern Generation", juce::dontSendNotification);
            patternSectionLabel.setFont(juce::Font(14.0f, juce::Font::bold));
            patternSectionLabel.setColour(juce::Label::textColourId, juce::Colour(0xffdddddd));
            addAndMakeVisible(patternSectionLabel);
            
#ifdef ENABLE_EUCLIDEAN_MODE
            // Euclidean mode preference
            euclideanBox.setButtonText("Prefer Euclidean mode for new sessions");
            euclideanBox.setToggleState(settings.getBool(SettingsManager::Keys::preferEuclideanMode, false),
                                       juce::dontSendNotification);
            euclideanBox.setColour(juce::ToggleButton::textColourId, juce::Colour(0xffcccccc));
            euclideanBox.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xffff8833));
            euclideanBox.onClick = [this] {
                SettingsManager::getInstance().setBool(SettingsManager::Keys::preferEuclideanMode,
                                                      euclideanBox.getToggleState());
            };
            addAndMakeVisible(euclideanBox);
            
            // Euclidean Length Section
            euclideanLengthLabel.setText("Default Euclidean Lengths", juce::dontSendNotification);
            euclideanLengthLabel.setFont(juce::Font(12.0f, juce::Font::bold));
            euclideanLengthLabel.setColour(juce::Label::textColourId, juce::Colour(0xffcccccc));
            addAndMakeVisible(euclideanLengthLabel);
            
            // BD Length
            bdLengthLabel.setText("BD:", juce::dontSendNotification);
            bdLengthLabel.setFont(juce::Font(12.0f));
            bdLengthLabel.setColour(juce::Label::textColourId, juce::Colour(0xffcccccc));
            addAndMakeVisible(bdLengthLabel);
            
            for (int i = 1; i <= 32; ++i)
                bdLengthBox.addItem(juce::String(i) + " steps", i);
            bdLengthBox.setSelectedId(settings.getInt(SettingsManager::Keys::euclideanBDLength, 16));
            bdLengthBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2a2a2a));
            bdLengthBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xffcccccc));
            bdLengthBox.onChange = [this] {
                SettingsManager::getInstance().setInt(SettingsManager::Keys::euclideanBDLength,
                                                     bdLengthBox.getSelectedId());
            };
            addAndMakeVisible(bdLengthBox);
            
            // SD Length
            sdLengthLabel.setText("SD:", juce::dontSendNotification);
            sdLengthLabel.setFont(juce::Font(12.0f));
            sdLengthLabel.setColour(juce::Label::textColourId, juce::Colour(0xffcccccc));
            addAndMakeVisible(sdLengthLabel);
            
            for (int i = 1; i <= 32; ++i)
                sdLengthBox.addItem(juce::String(i) + " steps", i);
            sdLengthBox.setSelectedId(settings.getInt(SettingsManager::Keys::euclideanSDLength, 12));
            sdLengthBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2a2a2a));
            sdLengthBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xffcccccc));
            sdLengthBox.onChange = [this] {
                SettingsManager::getInstance().setInt(SettingsManager::Keys::euclideanSDLength,
                                                     sdLengthBox.getSelectedId());
            };
            addAndMakeVisible(sdLengthBox);
            
            // HH Length
            hhLengthLabel.setText("HH:", juce::dontSendNotification);
            hhLengthLabel.setFont(juce::Font(12.0f));
            hhLengthLabel.setColour(juce::Label::textColourId, juce::Colour(0xffcccccc));
            addAndMakeVisible(hhLengthLabel);
            
            for (int i = 1; i <= 32; ++i)
                hhLengthBox.addItem(juce::String(i) + " steps", i);
            hhLengthBox.setSelectedId(settings.getInt(SettingsManager::Keys::euclideanHHLength, 8));
            hhLengthBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2a2a2a));
            hhLengthBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xffcccccc));
            hhLengthBox.onChange = [this] {
                SettingsManager::getInstance().setInt(SettingsManager::Keys::euclideanHHLength,
                                                     hhLengthBox.getSelectedId());
            };
            addAndMakeVisible(hhLengthBox);
#endif // ENABLE_EUCLIDEAN_MODE
            
            // Pattern Output Section
            outputSectionLabel.setText("Pattern Output", juce::dontSendNotification);
            outputSectionLabel.setFont(juce::Font(14.0f, juce::Font::bold));
            outputSectionLabel.setColour(juce::Label::textColourId, juce::Colour(0xffdddddd));
            addAndMakeVisible(outputSectionLabel);
            
            gateModeBox.setButtonText("Gate Mode by default (sustained notes)");
            gateModeBox.setToggleState(settings.getBool(SettingsManager::Keys::defaultGateMode, false),
                                      juce::dontSendNotification);
            gateModeBox.setColour(juce::ToggleButton::textColourId, juce::Colour(0xffcccccc));
            gateModeBox.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xffff8833));
            gateModeBox.onClick = [this] {
                SettingsManager::getInstance().setBool(SettingsManager::Keys::defaultGateMode,
                                                      gateModeBox.getToggleState());
            };
            addAndMakeVisible(gateModeBox);
            
#ifdef ENABLE_PATTERN_CHAIN
            // Pattern Chaining Section
            chainSectionLabel.setText("Pattern Chaining", juce::dontSendNotification);
            chainSectionLabel.setFont(juce::Font(14.0f, juce::Font::bold));
            chainSectionLabel.setColour(juce::Label::textColourId, juce::Colour(0xffdddddd));
            addAndMakeVisible(chainSectionLabel);
            
            // Transition mode
            transitionLabel.setText("Default transition:", juce::dontSendNotification);
            transitionLabel.setFont(juce::Font(12.0f));
            transitionLabel.setColour(juce::Label::textColourId, juce::Colour(0xffcccccc));
            addAndMakeVisible(transitionLabel);
            
            transitionBox.addItem("Smooth Morph", 1);
            transitionBox.addItem("Instant Switch", 2);
            transitionBox.addItem("Crossfade", 3);
            auto transition = settings.getString(SettingsManager::Keys::defaultTransitionMode, "smooth");
            if (transition == "smooth") transitionBox.setSelectedId(1);
            else if (transition == "instant") transitionBox.setSelectedId(2);
            else if (transition == "crossfade") transitionBox.setSelectedId(3);
            transitionBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2a2a2a));
            transitionBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xffcccccc));
            transitionBox.onChange = [this] {
                const char* modes[] = {"smooth", "instant", "crossfade"};
                int id = transitionBox.getSelectedId();
                if (id > 0 && id <= 3)
                    SettingsManager::getInstance().setString(SettingsManager::Keys::defaultTransitionMode,
                                                            modes[id - 1]);
            };
            addAndMakeVisible(transitionBox);
            
            // Bars per pattern
            barsLabel.setText("Default bars per pattern:", juce::dontSendNotification);
            barsLabel.setFont(juce::Font(12.0f));
            barsLabel.setColour(juce::Label::textColourId, juce::Colour(0xffcccccc));
            addAndMakeVisible(barsLabel);
            
            for (int i = 1; i <= 16; ++i)
                barsBox.addItem(juce::String(i), i);
            barsBox.setSelectedId(settings.getInt(SettingsManager::Keys::defaultBarsPerPattern, 4));
            barsBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2a2a2a));
            barsBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xffcccccc));
            barsBox.onChange = [this] {
                SettingsManager::getInstance().setInt(SettingsManager::Keys::defaultBarsPerPattern,
                                                     barsBox.getSelectedId());
            };
            addAndMakeVisible(barsBox);
#endif // ENABLE_PATTERN_CHAIN
            
            // Performance Section
            perfSectionLabel.setText("Performance", juce::dontSendNotification);
            perfSectionLabel.setFont(juce::Font(14.0f, juce::Font::bold));
            perfSectionLabel.setColour(juce::Label::textColourId, juce::Colour(0xffdddddd));
            addAndMakeVisible(perfSectionLabel);
            
            highResBox.setButtonText("High resolution LED matrix");
            highResBox.setToggleState(false, juce::dontSendNotification);
            highResBox.setColour(juce::ToggleButton::textColourId, juce::Colour(0xffcccccc));
            highResBox.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xffff8833));
            addAndMakeVisible(highResBox);
            
            openGLBox.setButtonText("Enable OpenGL rendering");
            openGLBox.setToggleState(false, juce::dontSendNotification);
            openGLBox.setColour(juce::ToggleButton::textColourId, juce::Colour(0xffcccccc));
            openGLBox.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xffff8833));
            addAndMakeVisible(openGLBox);
        }
        
        void resized() override
        {
            auto bounds = getLocalBounds().reduced(10);
            
            // Header
            headerLabel.setBounds(bounds.removeFromTop(25));
            bounds.removeFromTop(15);
            
            // Pattern Reset Section
            resetSectionLabel.setBounds(bounds.removeFromTop(20));
            bounds.removeFromTop(10);
            resetQuantizeLabel.setBounds(bounds.removeFromTop(20));
            resetQuantizeBox.setBounds(bounds.removeFromTop(30).removeFromLeft(200));
            bounds.removeFromTop(20);
            
            // MIDI Learn Section
            midiLearnLabel.setBounds(bounds.removeFromTop(20));
            bounds.removeFromTop(10);
            auto midiLearnRow = bounds.removeFromTop(30);
            resetMidiLearnButton.setBounds(midiLearnRow.removeFromLeft(150));
            resetCCLabel.setBounds(midiLearnRow.removeFromLeft(150).translated(10, 0));
            bounds.removeFromTop(20);
            
            // Pattern Generation Section
            patternSectionLabel.setBounds(bounds.removeFromTop(20));
            bounds.removeFromTop(10);
            
#ifdef ENABLE_EUCLIDEAN_MODE
            euclideanBox.setBounds(bounds.removeFromTop(24));
            bounds.removeFromTop(15);
            
            euclideanLengthLabel.setBounds(bounds.removeFromTop(20));
            bounds.removeFromTop(5);
            
            // Length rows
            auto bdRow = bounds.removeFromTop(30);
            bdLengthLabel.setBounds(bdRow.removeFromLeft(40));
            bdLengthBox.setBounds(bdRow.removeFromLeft(120));
            
            auto sdRow = bounds.removeFromTop(30);
            sdLengthLabel.setBounds(sdRow.removeFromLeft(40));
            sdLengthBox.setBounds(sdRow.removeFromLeft(120));
            
            auto hhRow = bounds.removeFromTop(30);
            hhLengthLabel.setBounds(hhRow.removeFromLeft(40));
            hhLengthBox.setBounds(hhRow.removeFromLeft(120));
            
            bounds.removeFromTop(20);
#endif
            
            // Pattern Output Section
            outputSectionLabel.setBounds(bounds.removeFromTop(20));
            bounds.removeFromTop(10);
            gateModeBox.setBounds(bounds.removeFromTop(24));
            
#ifdef ENABLE_PATTERN_CHAIN
            bounds.removeFromTop(20);
            
            // Pattern Chaining Section
            chainSectionLabel.setBounds(bounds.removeFromTop(20));
            bounds.removeFromTop(10);
            
            auto transRow = bounds.removeFromTop(30);
            transitionLabel.setBounds(transRow.removeFromLeft(120));
            transitionBox.setBounds(transRow.removeFromLeft(150));
            
            auto barsRow = bounds.removeFromTop(30);
            barsLabel.setBounds(barsRow.removeFromLeft(150));
            barsBox.setBounds(barsRow.removeFromLeft(80));
#endif
            
            bounds.removeFromTop(20);
            
            // Performance Section
            perfSectionLabel.setBounds(bounds.removeFromTop(20));
            bounds.removeFromTop(10);
            highResBox.setBounds(bounds.removeFromTop(24));
            openGLBox.setBounds(bounds.removeFromTop(24));
        }
        
        void timerCallback() override
        {
            // Update MIDI learn status
            if (auto* processor = dynamic_cast<GridsAudioProcessor*>(&audioProcessor)) 
            {
                bool isLearning = processor->isMidiLearning();
                int currentCC = processor->getResetMidiCC();
                
                // Update button text
                if (isLearning) {
                    resetMidiLearnButton.setButtonText("Learning... (move a CC)");
                } else {
                    resetMidiLearnButton.setButtonText("Learn Reset CC");
                }
                
                // Update CC label
                if (currentCC >= 0) {
                    resetCCLabel.setText("Reset CC: " + juce::String(currentCC), juce::dontSendNotification);
                } else {
                    resetCCLabel.setText("Reset CC: None", juce::dontSendNotification);
                }
            }
        }
        
    private:
        GridsAudioProcessor& audioProcessor;
        
        // Labels
        juce::Label headerLabel;
        juce::Label resetSectionLabel;
        juce::Label resetQuantizeLabel;
        juce::ComboBox resetQuantizeBox;
        
        juce::Label midiLearnLabel;
        juce::TextButton resetMidiLearnButton;
        juce::Label resetCCLabel;
        juce::Label patternSectionLabel;
        juce::Label outputSectionLabel;
        juce::Label perfSectionLabel;
        
#ifdef ENABLE_EUCLIDEAN_MODE
        juce::Label euclideanLengthLabel;
        juce::Label bdLengthLabel;
        juce::Label sdLengthLabel;
        juce::Label hhLengthLabel;
        juce::ToggleButton euclideanBox;
        juce::ComboBox bdLengthBox;
        juce::ComboBox sdLengthBox;
        juce::ComboBox hhLengthBox;
#endif
        
        juce::ToggleButton gateModeBox;
        
#ifdef ENABLE_PATTERN_CHAIN
        juce::Label chainSectionLabel;
        juce::Label transitionLabel;
        juce::Label barsLabel;
        juce::ComboBox transitionBox;
        juce::ComboBox barsBox;
#endif
        
        juce::ToggleButton highResBox;
        juce::ToggleButton openGLBox;
    };
    
    return std::make_unique<AdvancedTabContent>(audioProcessor);
}

//==============================================================================
std::unique_ptr<juce::Component> VisageSettingsPanel::createModulationTabContent()
{
    // Create a custom component that handles its own layout
    class ModulationTabContent : public juce::Component
    {
    public:
#ifdef ENABLE_MODULATION_MATRIX
        struct LFOComponents
        {
            juce::Label label;
            juce::ToggleButton enableBox;
            juce::Label shapeLabel;
            juce::ComboBox shapeBox;
            juce::Label rateLabel;
            juce::Slider rateSlider;
            juce::Label rateDescriptionLabel;  // New label for rate description
            juce::Label depthLabel;
            juce::Slider depthSlider;
            juce::Label depthDescriptionLabel;  // New label for depth description
            juce::Label bipolarLabel;
            juce::ToggleButton bipolarBox;
            juce::Label destLabel;
            // Destination checkboxes
            juce::ToggleButton destPatternX;
            juce::ToggleButton destPatternY;
            juce::ToggleButton destChaos;
            juce::ToggleButton destSwing;
            juce::ToggleButton destReset;
            juce::ToggleButton destBDDensity;
            juce::ToggleButton destSDDensity;
            juce::ToggleButton destHHDensity;
            juce::ToggleButton destBDVelocity;
            juce::ToggleButton destSDVelocity;
            juce::ToggleButton destHHVelocity;
            juce::ToggleButton destBDNote;
            juce::ToggleButton destSDNote;
            juce::ToggleButton destHHNote;
        };
#endif
        
        // Helper function to generate rate description text
        static juce::String getRateDescription(float rate)
        {
            if (rate < 1.0f) {
                // Less than 1: multiple cycles per beat
                float cyclesPerBeat = 1.0f / rate;
                if (cyclesPerBeat == 2.0f)
                    return "2 cycles per beat";
                else if (cyclesPerBeat == 4.0f)
                    return "4 cycles per beat";
                else
                    return juce::String(cyclesPerBeat, 2) + " cycles per beat";
            }
            else if (rate == 1.0f) {
                return "One cycle per beat";
            }
            else {
                // Greater than 1: one cycle every X beats
                if (rate == 2.0f)
                    return "One cycle every 2 beats";
                else if (rate == 4.0f)
                    return "One cycle every 4 beats (1 bar)";
                else if (rate == 8.0f)
                    return "One cycle every 8 beats (2 bars)";
                else if (rate == 16.0f)
                    return "One cycle every 16 beats (4 bars)";
                else
                    return "One cycle every " + juce::String(rate, 2) + " beats";
            }
        }
        
        ModulationTabContent(GridsAudioProcessor& processor) : audioProcessor(processor)
        {
            // Header
            headerLabel.setText("Modulation Matrix", juce::dontSendNotification);
            headerLabel.setFont(juce::Font(18.0f, juce::Font::bold));
            headerLabel.setColour(juce::Label::textColourId, juce::Colour(0xffeeeeee));
            addAndMakeVisible(headerLabel);
            
            subHeaderLabel.setText("LFO modulation routing for pattern parameters", juce::dontSendNotification);
            subHeaderLabel.setFont(juce::Font(12.0f));
            subHeaderLabel.setColour(juce::Label::textColourId, juce::Colour(0xff999999));
            addAndMakeVisible(subHeaderLabel);
            
#ifdef ENABLE_MODULATION_MATRIX
            setupLFOSection(1);
            setupLFOSection(2);
#else
            // Feature disabled message
            disabledLabel.setText("Modulation Matrix\n\nThis feature is currently disabled.\nEnable ENABLE_MODULATION_MATRIX in CMakeLists.txt to activate.", 
                                juce::dontSendNotification);
            disabledLabel.setFont(juce::Font(14.0f));
            disabledLabel.setColour(juce::Label::textColourId, juce::Colour(0xff666666));
            disabledLabel.setJustificationType(juce::Justification::centred);
            addAndMakeVisible(disabledLabel);
#endif
        }
        
#ifdef ENABLE_MODULATION_MATRIX
        void setupDestCheckbox(juce::ToggleButton& checkbox, const juce::String& text, bool defaultState)
        {
            checkbox.setButtonText(text);
            checkbox.setToggleState(defaultState, juce::dontSendNotification);
            checkbox.setColour(juce::ToggleButton::textColourId, juce::Colour(0xffcccccc));
            checkbox.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xffff8833));
        }
        
        void setupLFOSection(int lfoIndex)
        {
            auto& components = lfoIndex == 1 ? lfo1Components : lfo2Components;
            
            // LFO label
            components.label.setText("LFO " + juce::String(lfoIndex), juce::dontSendNotification);
            components.label.setFont(juce::Font(14.0f, juce::Font::bold));
            components.label.setColour(juce::Label::textColourId, juce::Colour(0xffdddddd));
            addAndMakeVisible(components.label);
            
            // Enable checkbox
            components.enableBox.setButtonText("Enable");
            components.enableBox.setColour(juce::ToggleButton::textColourId, juce::Colour(0xffcccccc));
            components.enableBox.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xffff8833));
            addAndMakeVisible(components.enableBox);
            
            // Shape selector
            components.shapeLabel.setText("Shape", juce::dontSendNotification);
            components.shapeLabel.setFont(juce::Font(12.0f));
            components.shapeLabel.setColour(juce::Label::textColourId, juce::Colour(0xffcccccc));
            addAndMakeVisible(components.shapeLabel);
            
            components.shapeBox.addItem("Sine", 1);
            components.shapeBox.addItem("Triangle", 2);
            components.shapeBox.addItem("Square", 3);
            components.shapeBox.addItem("Saw", 4);
            components.shapeBox.addItem("Random", 5);
            components.shapeBox.setSelectedId(1); // Default to Sine
            components.shapeBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2a2a2a));
            components.shapeBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xffcccccc));
            components.shapeBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff404040));
            components.shapeBox.setColour(juce::ComboBox::arrowColourId, juce::Colour(0xff888888));
            components.shapeBox.setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff2a2a2a));
            components.shapeBox.setColour(juce::PopupMenu::textColourId, juce::Colour(0xffcccccc));
            components.shapeBox.setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff404040));
            addAndMakeVisible(components.shapeBox);
            
            // Rate slider
            components.rateLabel.setText("Rate (beats)", juce::dontSendNotification);
            components.rateLabel.setFont(juce::Font(12.0f));
            components.rateLabel.setColour(juce::Label::textColourId, juce::Colour(0xffcccccc));
            addAndMakeVisible(components.rateLabel);
            
            // Rate description label (shows what the rate means)
            components.rateDescriptionLabel.setText(getRateDescription(4.0f), juce::dontSendNotification);
            components.rateDescriptionLabel.setFont(juce::Font(11.0f));
            components.rateDescriptionLabel.setColour(juce::Label::textColourId, juce::Colour(0xff888888));
            addAndMakeVisible(components.rateDescriptionLabel);
            
            components.rateSlider.setSliderStyle(juce::Slider::LinearHorizontal);
            components.rateSlider.setRange(0.25, 16.0, 0.25);
            components.rateSlider.setValue(4.0); // Default 4 beats
            components.rateSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 20);
            components.rateSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xff333333));
            components.rateSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xffff8833));
            components.rateSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffcccccc));
            components.rateSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentWhite);
            components.rateSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentWhite);
            components.rateSlider.setScrollWheelEnabled(false); // Prevent accidental changes while scrolling
            addAndMakeVisible(components.rateSlider);
            
            // Depth slider
            components.depthLabel.setText("Depth (%)", juce::dontSendNotification);
            components.depthLabel.setFont(juce::Font(12.0f));
            components.depthLabel.setColour(juce::Label::textColourId, juce::Colour(0xffcccccc));
            addAndMakeVisible(components.depthLabel);
            
            // Depth description label
            components.depthDescriptionLabel.setText("Modulation amount (Reset >60%)", juce::dontSendNotification);
            components.depthDescriptionLabel.setFont(juce::Font(11.0f));
            components.depthDescriptionLabel.setColour(juce::Label::textColourId, juce::Colour(0xff888888));
            addAndMakeVisible(components.depthDescriptionLabel);
            
            components.depthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
            components.depthSlider.setRange(0.0, 100.0, 1.0);
            components.depthSlider.setValue(70.0); // Default 70%
            components.depthSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 20);
            components.depthSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xff333333));
            components.depthSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xffff8833));
            components.depthSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffcccccc));
            components.depthSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentWhite);
            components.depthSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentWhite);
            components.depthSlider.setScrollWheelEnabled(false); // Prevent accidental changes while scrolling
            addAndMakeVisible(components.depthSlider);
            
            // Bipolar mode
            components.bipolarLabel.setText("Modulation Mode:", juce::dontSendNotification);
            components.bipolarLabel.setFont(juce::Font(12.0f));
            components.bipolarLabel.setColour(juce::Label::textColourId, juce::Colour(0xffcccccc));
            addAndMakeVisible(components.bipolarLabel);
            
            components.bipolarBox.setButtonText("Bipolar (modulates +/- range from center)");
            components.bipolarBox.setToggleState(true, juce::dontSendNotification); // Default to bipolar
            components.bipolarBox.setColour(juce::ToggleButton::textColourId, juce::Colour(0xffcccccc));
            components.bipolarBox.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xffff8833));
            addAndMakeVisible(components.bipolarBox);
            
            // Destination checkboxes
            components.destLabel.setText("Destinations (select one or more):", juce::dontSendNotification);
            components.destLabel.setFont(juce::Font(12.0f));
            components.destLabel.setColour(juce::Label::textColourId, juce::Colour(0xffcccccc));
            addAndMakeVisible(components.destLabel);
            
            // Setup destination checkboxes
            setupDestCheckbox(components.destPatternX, "Pattern X", true); // Default selected
            setupDestCheckbox(components.destPatternY, "Pattern Y", false);
            setupDestCheckbox(components.destChaos, "Chaos", false);
            setupDestCheckbox(components.destSwing, "Swing", false);
            setupDestCheckbox(components.destReset, "Reset", false);
            setupDestCheckbox(components.destBDDensity, "BD Density", false);
            setupDestCheckbox(components.destSDDensity, "SD Density", false);
            setupDestCheckbox(components.destHHDensity, "HH Density", false);
            setupDestCheckbox(components.destBDVelocity, "BD Velocity", false);
            setupDestCheckbox(components.destSDVelocity, "SD Velocity", false);
            setupDestCheckbox(components.destHHVelocity, "HH Velocity", false);
            setupDestCheckbox(components.destBDNote, "BD MIDI Note", false);
            setupDestCheckbox(components.destSDNote, "SD MIDI Note", false);
            setupDestCheckbox(components.destHHNote, "HH MIDI Note", false);
            
            addAndMakeVisible(components.destPatternX);
            addAndMakeVisible(components.destPatternY);
            addAndMakeVisible(components.destChaos);
            addAndMakeVisible(components.destSwing);
            addAndMakeVisible(components.destReset);
            addAndMakeVisible(components.destBDDensity);
            addAndMakeVisible(components.destSDDensity);
            addAndMakeVisible(components.destHHDensity);
            addAndMakeVisible(components.destBDVelocity);
            addAndMakeVisible(components.destSDVelocity);
            addAndMakeVisible(components.destHHVelocity);
            addAndMakeVisible(components.destBDNote);
            addAndMakeVisible(components.destSDNote);
            addAndMakeVisible(components.destHHNote);
            
            
            // Connect callbacks
            setupLFOCallbacks(lfoIndex, components);
        }
        
        void setupLFOCallbacks(int lfoIndex, LFOComponents& components)
        {
            auto& modMatrix = audioProcessor.getModulationMatrix();
            auto& lfo = modMatrix.getLFO(lfoIndex - 1); // Convert to 0-based index
            
            // Enable checkbox callback
            components.enableBox.onStateChange = [&lfo, &components]()
            {
                lfo.setEnabled(components.enableBox.getToggleState());
            };
            
            // Shape selector callback
            components.shapeBox.onChange = [&lfo, &components]()
            {
                int selectedId = components.shapeBox.getSelectedId();
                if (selectedId >= 1 && selectedId <= 5)
                {
                    lfo.setShape(static_cast<LFO::Shape>(selectedId - 1));
                }
            };
            
            // Destination checkbox callbacks
            auto updateDestinations = [&modMatrix, lfoIndex, &components]() {
                float amount = static_cast<float>(components.depthSlider.getValue() / 100.0);
                bool bipolar = components.bipolarBox.getToggleState();
                
                // Clear all routings first, then set only the selected ones
                // Pattern controls
                modMatrix.setRouting(lfoIndex - 1, ModulationMatrix::PATTERN_X, 
                    components.destPatternX.getToggleState() ? amount : 0.0f, bipolar);
                modMatrix.setRouting(lfoIndex - 1, ModulationMatrix::PATTERN_Y, 
                    components.destPatternY.getToggleState() ? amount : 0.0f, bipolar);
                modMatrix.setRouting(lfoIndex - 1, ModulationMatrix::CHAOS, 
                    components.destChaos.getToggleState() ? amount : 0.0f, bipolar);
                modMatrix.setRouting(lfoIndex - 1, ModulationMatrix::SWING, 
                    components.destSwing.getToggleState() ? amount : 0.0f, bipolar);
                modMatrix.setRouting(lfoIndex - 1, ModulationMatrix::PATTERN_RESET, 
                    components.destReset.getToggleState() ? amount : 0.0f, bipolar);
                
                // Density controls
                modMatrix.setRouting(lfoIndex - 1, ModulationMatrix::BD_DENSITY, 
                    components.destBDDensity.getToggleState() ? amount : 0.0f, bipolar);
                modMatrix.setRouting(lfoIndex - 1, ModulationMatrix::SD_DENSITY, 
                    components.destSDDensity.getToggleState() ? amount : 0.0f, bipolar);
                modMatrix.setRouting(lfoIndex - 1, ModulationMatrix::HH_DENSITY, 
                    components.destHHDensity.getToggleState() ? amount : 0.0f, bipolar);
                
                // Velocity controls
                modMatrix.setRouting(lfoIndex - 1, ModulationMatrix::BD_VELOCITY, 
                    components.destBDVelocity.getToggleState() ? amount : 0.0f, bipolar);
                modMatrix.setRouting(lfoIndex - 1, ModulationMatrix::SD_VELOCITY, 
                    components.destSDVelocity.getToggleState() ? amount : 0.0f, bipolar);
                modMatrix.setRouting(lfoIndex - 1, ModulationMatrix::HH_VELOCITY, 
                    components.destHHVelocity.getToggleState() ? amount : 0.0f, bipolar);
                
                // MIDI Note controls
                modMatrix.setRouting(lfoIndex - 1, ModulationMatrix::BD_MIDI_NOTE, 
                    components.destBDNote.getToggleState() ? amount : 0.0f, bipolar);
                modMatrix.setRouting(lfoIndex - 1, ModulationMatrix::SD_MIDI_NOTE, 
                    components.destSDNote.getToggleState() ? amount : 0.0f, bipolar);
                modMatrix.setRouting(lfoIndex - 1, ModulationMatrix::HH_MIDI_NOTE, 
                    components.destHHNote.getToggleState() ? amount : 0.0f, bipolar);
            };
            
            components.destPatternX.onClick = updateDestinations;
            components.destPatternY.onClick = updateDestinations;
            components.destChaos.onClick = updateDestinations;
            components.destSwing.onClick = updateDestinations;
            components.destReset.onClick = updateDestinations;
            components.destBDDensity.onClick = updateDestinations;
            components.destSDDensity.onClick = updateDestinations;
            components.destHHDensity.onClick = updateDestinations;
            components.destBDVelocity.onClick = updateDestinations;
            components.destSDVelocity.onClick = updateDestinations;
            components.destHHVelocity.onClick = updateDestinations;
            components.destBDNote.onClick = updateDestinations;
            components.destSDNote.onClick = updateDestinations;
            components.destHHNote.onClick = updateDestinations;
            
            // Rate slider callback
            components.rateSlider.onValueChange = [&lfo, &components]()
            {
                float rate = static_cast<float>(components.rateSlider.getValue());
                lfo.setRate(rate);
                // Update the description text
                components.rateDescriptionLabel.setText(getRateDescription(rate), juce::dontSendNotification);
            };
            
            // Depth slider callback
            components.depthSlider.onValueChange = [&lfo, &components, updateDestinations]()
            {
                float depthPercent = static_cast<float>(components.depthSlider.getValue());
                lfo.setDepth(depthPercent / 100.0f);
                
                // Update depth description
                juce::String desc = "Modulation amount";
                if (depthPercent > 60.0f && components.destReset.getToggleState())
                {
                    desc += " (Reset active)";
                }
                components.depthDescriptionLabel.setText(desc, juce::dontSendNotification);
                
                updateDestinations(); // Update all routing amounts when depth changes
            };
            
            // Bipolar callback - reuse the updateDestinations logic
            components.bipolarBox.onStateChange = updateDestinations;
            
            // Initialize UI with current values
            components.enableBox.setToggleState(lfo.isEnabled(), juce::dontSendNotification);
            components.shapeBox.setSelectedId(static_cast<int>(lfo.getShape()) + 1, juce::dontSendNotification);
            components.rateSlider.setValue(lfo.getRate(), juce::dontSendNotification);
            components.rateDescriptionLabel.setText(getRateDescription(lfo.getRate()), juce::dontSendNotification);
            components.depthSlider.setValue(lfo.getDepth() * 100.0, juce::dontSendNotification);
        }
#endif
        
        void resized() override
        {
            auto bounds = getLocalBounds().reduced(10);
            
            // Header
            headerLabel.setBounds(bounds.removeFromTop(25));
            bounds.removeFromTop(5);
            subHeaderLabel.setBounds(bounds.removeFromTop(20));
            bounds.removeFromTop(20);
            
#ifdef ENABLE_MODULATION_MATRIX
            layoutLFOSection(bounds, lfo1Components);
            bounds.removeFromTop(15); // Spacing between LFO sections
            layoutLFOSection(bounds, lfo2Components);
#else
            disabledLabel.setBounds(bounds.removeFromTop(150));
#endif
        }
        
#ifdef ENABLE_MODULATION_MATRIX
        void layoutLFOSection(juce::Rectangle<int>& bounds, LFOComponents& components)
        {
            // LFO label
            components.label.setBounds(bounds.removeFromTop(25));
            bounds.removeFromTop(10);
            
            // Enable checkbox and Shape selector (separate rows like Advanced tab)
            components.enableBox.setBounds(bounds.removeFromTop(24));
            bounds.removeFromTop(10);
            
            // Shape label and dropdown (proper height like Advanced tab)
            components.shapeLabel.setBounds(bounds.removeFromTop(20));
            components.shapeBox.setBounds(bounds.removeFromTop(30).removeFromLeft(200));
            bounds.removeFromTop(10);
            
            // Rate slider
            components.rateLabel.setBounds(bounds.removeFromTop(20));
            // Limit width to prevent overlap with scrollbar - leave 30px margin for scrollbar
            auto descBounds = bounds.removeFromTop(18);
            descBounds.setWidth(juce::jmin(descBounds.getWidth() - 30, 400));
            components.rateDescriptionLabel.setBounds(descBounds);
            // Reduce slider width to prevent going under scrollbar
            auto rateSliderBounds = bounds.removeFromTop(30);
            rateSliderBounds.setWidth(rateSliderBounds.getWidth() - 25); // Leave space for scrollbar
            components.rateSlider.setBounds(rateSliderBounds);
            bounds.removeFromTop(10);
            
            // Depth slider
            components.depthLabel.setBounds(bounds.removeFromTop(20));
            // Limit width to prevent overlap with scrollbar
            auto depthDescBounds = bounds.removeFromTop(18);
            depthDescBounds.setWidth(juce::jmin(depthDescBounds.getWidth() - 30, 400));
            components.depthDescriptionLabel.setBounds(depthDescBounds);
            // Reduce slider width to prevent going under scrollbar
            auto depthSliderBounds = bounds.removeFromTop(30);
            depthSliderBounds.setWidth(depthSliderBounds.getWidth() - 25); // Leave space for scrollbar
            components.depthSlider.setBounds(depthSliderBounds);
            bounds.removeFromTop(10);
            
            // Bipolar mode section
            components.bipolarLabel.setBounds(bounds.removeFromTop(20));
            components.bipolarBox.setBounds(bounds.removeFromTop(24));
            bounds.removeFromTop(15);
            
            // Destination label and checkboxes
            components.destLabel.setBounds(bounds.removeFromTop(20));
            bounds.removeFromTop(5);
            
            // First row: Pattern controls (5 items)
            auto destRow1 = bounds.removeFromTop(24);
            components.destPatternX.setBounds(destRow1.removeFromLeft(95));
            components.destPatternY.setBounds(destRow1.removeFromLeft(95));
            components.destChaos.setBounds(destRow1.removeFromLeft(85));
            components.destSwing.setBounds(destRow1.removeFromLeft(85));
            components.destReset.setBounds(destRow1.removeFromLeft(85));
            
            // Second row: Density controls (3 items)
            auto destRow2 = bounds.removeFromTop(24);
            components.destBDDensity.setBounds(destRow2.removeFromLeft(140));
            components.destSDDensity.setBounds(destRow2.removeFromLeft(140));
            components.destHHDensity.setBounds(destRow2.removeFromLeft(140));
            
            // Third row: Velocity controls (3 items)
            auto destRow3 = bounds.removeFromTop(24);
            components.destBDVelocity.setBounds(destRow3.removeFromLeft(140));
            components.destSDVelocity.setBounds(destRow3.removeFromLeft(140));
            components.destHHVelocity.setBounds(destRow3.removeFromLeft(140));
            
            // Fourth row: MIDI Note controls (3 items)
            auto destRow4 = bounds.removeFromTop(24);
            components.destBDNote.setBounds(destRow4.removeFromLeft(140));
            components.destSDNote.setBounds(destRow4.removeFromLeft(140));
            components.destHHNote.setBounds(destRow4.removeFromLeft(140));
            
            bounds.removeFromTop(20);
        }
#endif
        
    private:
        GridsAudioProcessor& audioProcessor;
        
        juce::Label headerLabel;
        juce::Label subHeaderLabel;
        
#ifdef ENABLE_MODULATION_MATRIX
        LFOComponents lfo1Components;
        LFOComponents lfo2Components;
#else
        juce::Label disabledLabel;
#endif
    };
    
    return std::make_unique<ModulationTabContent>(audioProcessor);
}

//==============================================================================
void VisageSettingsPanel::onTabChanged(const juce::String& tabId, int tabIndex)
{
    juce::Component* newContent = nullptr;
    
    if (tabId == "general")
        newContent = generalViewport.get();
    else if (tabId == "advanced")
        newContent = advancedViewport.get();
    else if (tabId == "modulation")
        newContent = modulationViewport.get();
    
    showTabContent(newContent);
}

//==============================================================================
void VisageSettingsPanel::showTabContent(juce::Component* contentToShow)
{
    // Hide current content
    if (currentContentComponent)
        currentContentComponent->setVisible(false);
    
    // Show new content
    currentContentComponent = contentToShow;
    if (currentContentComponent)
    {
        currentContentComponent->setVisible(true);
        currentContentComponent->toFront(false);
        
        // Update bounds if we're already sized - match the logic in resized()
        if (getWidth() > 0 && getHeight() > 0)
        {
            auto bounds = getLocalBounds().withSizeKeepingCentre(kPanelWidth, kPanelHeight);
            
            // Match the exact same calculation as resized()
            bounds.removeFromTop(45); // Space for title bar and title text
            bounds.removeFromTop(10); // Extra spacing to push tabs down
            bounds.removeFromTop(kTabBarHeight); // Tab bar height
            bounds.removeFromTop(14); // Spacing after tabs and separator
            
            // No footer - content goes to bottom
            auto contentBounds = bounds.reduced(kSidePadding, 0);
            contentBounds.removeFromTop(5); // Small padding from tabs
            contentBounds.removeFromBottom(10); // Small bottom padding
            
            currentContentComponent->setBounds(contentBounds);
        }
    }
}

//==============================================================================
void VisageSettingsPanel::handleCloseButtonClicked()
{
    if (onCloseClicked)
        onCloseClicked();
    else
        hidePanel(); // Default behavior - just hide with setVisible(false)
}

//==============================================================================
void VisageSettingsPanel::handleLicenseButtonClicked()
{
    if (onLicenseClicked)
        onLicenseClicked();
}

//==============================================================================
void VisageSettingsPanel::setupLookAndFeel()
{
    // Set up the look and feel to match Visage theme
    // This could be expanded to use a custom LookAndFeel if needed
}

//==============================================================================
void VisageSettingsPanel::styleButton(juce::TextButton& button, bool isPrimary)
{
    if (isPrimary)
    {
        // Primary button (Close) - orange background
        button.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff8833));
        button.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff000000));
        button.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffff6600));
    }
    else
    {
        // Secondary button (License) - dark background
        button.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a2a));
        button.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffcccccc));
        button.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff3a3a3a));
    }
    
    // Common button styling
    button.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff1a1a1a));
    // Note: TextButton font is handled by the LookAndFeel, not directly on the button
}

//==============================================================================
void VisageSettingsPanel::updateTabButtonStates(int activeIndex)
{
    // Style tab buttons based on active state
    if (generalTabButton)
    {
        bool isActive = (activeIndex == 0);
        // For toggle buttons: buttonColourId is off state, buttonOnColourId is on (selected) state
        generalTabButton->setColour(juce::TextButton::buttonColourId, 
            juce::Colour(0xff2a2a2a));  // Dark grey for unselected
        generalTabButton->setColour(juce::TextButton::buttonOnColourId,
            juce::Colour(0xff3a3a3a));  // Lighter grey for selected
        generalTabButton->setColour(juce::TextButton::textColourOffId,
            juce::Colour(0xff999999));  // Grey text when unselected
        generalTabButton->setColour(juce::TextButton::textColourOnId,
            juce::Colour(0xffff8833));  // Orange text when selected
    }
    
    if (advancedTabButton)
    {
        bool isActive = (activeIndex == 1);
        // For toggle buttons: buttonColourId is off state, buttonOnColourId is on (selected) state
        advancedTabButton->setColour(juce::TextButton::buttonColourId,
            juce::Colour(0xff2a2a2a));  // Dark grey for unselected
        advancedTabButton->setColour(juce::TextButton::buttonOnColourId,
            juce::Colour(0xff3a3a3a));  // Lighter grey for selected
        advancedTabButton->setColour(juce::TextButton::textColourOffId,
            juce::Colour(0xff999999));  // Grey text when unselected
        advancedTabButton->setColour(juce::TextButton::textColourOnId,
            juce::Colour(0xffff8833));  // Orange text when selected
    }
    
    if (modulationTabButton)
    {
        bool isActive = (activeIndex == 2);
        // For toggle buttons: buttonColourId is off state, buttonOnColourId is on (selected) state
        modulationTabButton->setColour(juce::TextButton::buttonColourId,
            juce::Colour(0xff2a2a2a));  // Dark grey for unselected
        modulationTabButton->setColour(juce::TextButton::buttonOnColourId,
            juce::Colour(0xff3a3a3a));  // Lighter grey for selected
        modulationTabButton->setColour(juce::TextButton::textColourOffId,
            juce::Colour(0xff999999));  // Grey text when unselected
        modulationTabButton->setColour(juce::TextButton::textColourOnId,
            juce::Colour(0xffff8833));  // Orange text when selected
    }
}