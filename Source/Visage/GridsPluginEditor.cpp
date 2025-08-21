#include "GridsPluginEditor.h"
#include "../MaterialIcons.h"

GridsPluginEditor::GridsPluginEditor(GridsAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), gridsEngine(p.getGridsEngine()),
      ledMatrix(gridsEngine)
{
    DBG("GridsPluginEditor constructor started");
    
    // Set initial size - more compact with less bottom padding
    setSize(580, 400);
    setResizable(false, false);
    
    DBG("Editor size set");
    
    // Apply custom look and feel
    setLookAndFeel(&visageLookAndFeel);
    
    // XY Pad for pattern navigation
    addAndMakeVisible(xyPad);
    
    // LED Matrix display
    addAndMakeVisible(ledMatrix);
    
    // Set up XY pad callback to update parameters and engine
    xyPad.onValueChange = [this](float x, float y) {
        if (auto* xParam = audioProcessor.parameters.getParameter("x"))
            xParam->setValueNotifyingHost(xParam->convertTo0to1(x));
        if (auto* yParam = audioProcessor.parameters.getParameter("y"))
            yParam->setValueNotifyingHost(yParam->convertTo0to1(y));
        
        // Update the engine immediately so LED matrix updates even when not playing
        audioProcessor.getGridsEngine().setX(x);
        audioProcessor.getGridsEngine().setY(y);
    };
    
    // Initialize XY pad with current parameter values
    auto xValue = audioProcessor.parameters.getRawParameterValue("x")->load();
    auto yValue = audioProcessor.parameters.getRawParameterValue("y")->load();
    xyPad.setValues(xValue, yValue);
    
    // Density Controls - without value text boxes
    bdDensitySlider.setSliderStyle(juce::Slider::LinearVertical);
    bdDensitySlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    bdDensitySlider.addListener(this);  // Add listener to update engine
    addAndMakeVisible(bdDensitySlider);
    bdDensityLabel.setText("BD", juce::dontSendNotification);
    bdDensityLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(bdDensityLabel);
    
    sdDensitySlider.setSliderStyle(juce::Slider::LinearVertical);
    sdDensitySlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    sdDensitySlider.addListener(this);  // Add listener to update engine
    addAndMakeVisible(sdDensitySlider);
    sdDensityLabel.setText("SD", juce::dontSendNotification);
    sdDensityLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(sdDensityLabel);
    
    hhDensitySlider.setSliderStyle(juce::Slider::LinearVertical);
    hhDensitySlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    hhDensitySlider.addListener(this);  // Add listener to update engine
    addAndMakeVisible(hhDensitySlider);
    hhDensityLabel.setText("HH", juce::dontSendNotification);
    hhDensityLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(hhDensityLabel);
    
    // Modulation - without value text boxes
    chaosSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    chaosSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    chaosSlider.addListener(this);  // Add listener to update engine
    addAndMakeVisible(chaosSlider);
    chaosLabel.setText("Chaos", juce::dontSendNotification);
    chaosLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(chaosLabel);
    
    swingSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    swingSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    swingSlider.addListener(this);  // Add listener to update engine
    addAndMakeVisible(swingSlider);
    swingLabel.setText("Swing", juce::dontSendNotification);
    swingLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(swingLabel);
    
#ifdef ENABLE_VELOCITY_SYSTEM
    // Velocity controls - small rotary knobs below density sliders
    bdVelocitySlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    bdVelocitySlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(bdVelocitySlider);
    
    sdVelocitySlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    sdVelocitySlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(sdVelocitySlider);
    
    hhVelocitySlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    hhVelocitySlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(hhVelocitySlider);
    
    velocityLabel.setText("Velocity", juce::dontSendNotification);
    velocityLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(velocityLabel);
#endif
    
    // Reset button - custom component with mouse handling
    struct ResetButtonListener : public juce::MouseListener
    {
        GridsPluginEditor* editor;
        ResetButtonListener(GridsPluginEditor* e) : editor(e) {}
        
        void mouseDown(const juce::MouseEvent&) override
        {
            DBG("Reset button pressed");
            editor->resetPressed = true;
            editor->resetButton.repaint();
            
            // Trigger reset
            if (auto* resetParam = editor->audioProcessor.parameters.getParameter("reset")) {
                DBG("Setting reset parameter to 1.0");
                resetParam->setValueNotifyingHost(1.0f);
            } else {
                DBG("ERROR: Could not find reset parameter!");
            }
        }
        
        void mouseUp(const juce::MouseEvent&) override
        {
            editor->resetPressed = false;
            editor->resetButton.repaint();
        }
    };
    
    resetButton.addMouseListener(new ResetButtonListener(this), true);
    addAndMakeVisible(resetButton);
    
    // Reset label
    resetLabel.setText("Reset", juce::dontSendNotification);
    resetLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(resetLabel);
    
    // Settings button is now drawn directly in paint()
    
    // Reset mode selector (hidden by default, could go in settings)
    resetModeBox.addItem("Transparent", 1);
    resetModeBox.addItem("Retrigger", 2);
    resetModeBox.setSelectedId(1);
    resetModeBox.setLookAndFeel(&visageLookAndFeel);
    // Don't make visible for now - could be in settings panel
    // addAndMakeVisible(resetModeBox);
    
    // Create parameter attachments
    bdDensityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "density_1_bd", bdDensitySlider);
    sdDensityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "density_2_sd", sdDensitySlider);
    hhDensityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "density_3_hh", hhDensitySlider);
    chaosAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "chaos", chaosSlider);
    swingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "swing", swingSlider);
    
#ifdef ENABLE_VELOCITY_SYSTEM
    // Create velocity parameter attachments
    bdVelocityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "velocity_1_bd", bdVelocitySlider);
    sdVelocityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "velocity_2_sd", sdVelocitySlider);
    hhVelocityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "velocity_3_hh", hhVelocitySlider);
#endif
    
    // Reset mode attachment (even though combo box is hidden)
    resetModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        (audioProcessor.parameters, "reset_mode", resetModeBox);
    
    // Create settings panel (initially hidden) - like PlunderTube
    DBG("Creating settings panel...");
    settingsPanel = std::make_unique<VisageSettingsPanel>(audioProcessor);
    DBG("Settings panel created");
    addChildComponent(settingsPanel.get()); // Use addChildComponent for initially hidden components
    DBG("Settings panel added as child component");
    
    // Set up close callback to properly hide the panel
    settingsPanel->onCloseClicked = [this] {
        if (settingsPanel)
            settingsPanel->setVisible(false);
    };
    
    // Make sure the editor is visible and can receive keyboard focus
    setVisible(true);
    setWantsKeyboardFocus(true);
    
    // Start timer to update XY pad from parameter changes
    startTimerHz(30);
}

GridsPluginEditor::~GridsPluginEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void GridsPluginEditor::paint(juce::Graphics& g)
{
    // Dark background
    g.fillAll(juce::Colour(0xff1e1e1e));
    
    // Draw background panels
    g.setColour(juce::Colour(0xff2a2a2a));
    
    // XY Pad area
    g.fillRoundedRectangle(15, 30, 230, 230, 10);
    
    // Controls area (Chaos, Swing, Density) - next to XY Pad
    g.fillRoundedRectangle(255, 30, 310, 230, 10);
    
    // LED Matrix area - bottom section  
    g.fillRoundedRectangle(15, 275, 550, 100, 10);
    
    // Draw reset button
    auto resetBounds = resetButton.getBounds();
    auto centreX = resetBounds.getCentreX();
    auto centreY = resetBounds.getCentreY();
    auto radius = juce::jmin(resetBounds.getWidth(), resetBounds.getHeight()) / 2.0f - 4.0f;
    
    // Button background
    g.setColour(juce::Colour(0xff0a0a0a));
    g.fillEllipse(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);
    
    // Button border
    g.setColour(juce::Colour(0xff404040));
    g.drawEllipse(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f, 2.0f);
    
    // Inner button
    auto innerRadius = radius * 0.7f;
    
    if (resetPressed || resetModulatedActive) {
        // Glow effect when pressed or modulated
        g.setColour(juce::Colour(0xffff8833).withAlpha(0.3f));
        g.fillEllipse(centreX - radius * 1.2f, centreY - radius * 1.2f, radius * 2.4f, radius * 2.4f);
        
        g.setColour(juce::Colour(0xffff8833));
        g.fillEllipse(centreX - innerRadius, centreY - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f);
    } else {
        // Dimmed when not pressed
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillEllipse(centreX - innerRadius, centreY - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f);
    }
    
    // Center dot
    g.setColour((resetPressed || resetModulatedActive) ? juce::Colour(0xff202020) : juce::Colour(0xff606060));
    g.fillEllipse(centreX - 2.0f, centreY - 2.0f, 4.0f, 4.0f);
    
    // Draw settings button manually in top-right corner with better alignment
    float settingsX = getWidth() - 35;
    float settingsY = 5;  // Moved down 2px from 3 to 5 for better alignment
    float settingsSize = 20;
    
    // Store bounds for click detection
    const_cast<GridsPluginEditor*>(this)->settingsButtonBounds = 
        juce::Rectangle<int>(settingsX, settingsY, settingsSize, settingsSize);
    
    // Background - highlight on hover
    g.setColour(settingsHovered ? juce::Colour(0xff4a4a4a) : juce::Colour(0xff3a3a3a));
    g.fillRoundedRectangle(settingsX, settingsY, settingsSize, settingsSize, 4.0f);
    
    // Three dots
    float dotSize = 2.0f;
    float spacing = 3.5f;
    float centerX = settingsX + settingsSize * 0.5f;
    float centerY = settingsY + settingsSize * 0.5f;
    
    // Dots turn orange on hover
    g.setColour(settingsHovered ? juce::Colour(0xffff8833) : juce::Colour(0xffcccccc));
    g.fillEllipse(centerX - dotSize/2, centerY - spacing - dotSize/2, dotSize, dotSize);
    g.fillEllipse(centerX - dotSize/2, centerY - dotSize/2, dotSize, dotSize);
    g.fillEllipse(centerX - dotSize/2, centerY + spacing - dotSize/2, dotSize, dotSize);
}

void GridsPluginEditor::resized()
{
    // XY Pad
    xyPad.setBounds(20, 40, 220, 210);
    
    // Labels all on same row
    chaosLabel.setBounds(270, 45, 70, 20);
    swingLabel.setBounds(350, 45, 70, 20);
    bdDensityLabel.setBounds(445, 45, 45, 20);
    sdDensityLabel.setBounds(485, 45, 45, 20);
    hhDensityLabel.setBounds(525, 45, 45, 20);
    
    // Chaos and Swing knobs - positioned below labels
    chaosSlider.setBounds(270, 70, 70, 70);
    swingSlider.setBounds(350, 70, 70, 70);
    
    // Reset button - positioned below Chaos/Swing, styled as small knob
    resetLabel.setBounds(310, 155, 70, 15);
    resetButton.setBounds(320, 170, 50, 50);
    
    // Density sliders - positioned below labels, moved up
    bdDensitySlider.setBounds(450, 70, 35, 120);
    sdDensitySlider.setBounds(490, 70, 35, 120);
    hhDensitySlider.setBounds(530, 70, 35, 120);
    
#ifdef ENABLE_VELOCITY_SYSTEM
    // Velocity label - positioned below density sliders
    velocityLabel.setBounds(445, 200, 120, 15);
    
    // Velocity knobs - small knobs below density sliders
    bdVelocitySlider.setBounds(445, 215, 45, 45);
    sdVelocitySlider.setBounds(485, 215, 45, 45);
    hhVelocitySlider.setBounds(525, 215, 45, 45);
#endif
    
    // LED Matrix - positioned in bottom section to fit properly in container
    ledMatrix.setBounds(20, 285, 540, 80);
    
    // Settings panel - full overlay when visible
    if (settingsPanel && settingsPanel->Component::isVisible())
    {
        settingsPanel->setBounds(0, 0, getWidth(), getHeight());
    }
    
    // Settings button bounds are set in paint() method
}

void GridsPluginEditor::timerCallback()
{
#ifdef ENABLE_MODULATION_MATRIX
    // Update XY pad with modulated values
    auto xValue = audioProcessor.getModulatedX();
    auto yValue = audioProcessor.getModulatedY();
    xyPad.setValues(xValue, yValue);
    
    // Check if reset is being modulated
    bool isResetMod = audioProcessor.isResetModulated();
    if (isResetMod && !resetModulatedActive) {
        // Just triggered - start the visible timer
        resetModulatedActive = true;
        resetModulatedTimer = 8; // Keep visible for 8 frames (~266ms at 30Hz)
        resetButton.repaint();
    } else if (resetModulatedActive && resetModulatedTimer > 0) {
        resetModulatedTimer--;
        if (resetModulatedTimer == 0) {
            resetModulatedActive = false;
            resetButton.repaint();
        }
    }
    
    // Update sliders to show modulated values
    // We need to temporarily bypass the attachments to avoid fighting with them
    
    // Update density sliders with modulated values
    bdDensitySlider.setValue(audioProcessor.getModulatedBDDensity(), juce::dontSendNotification);
    sdDensitySlider.setValue(audioProcessor.getModulatedSDDensity(), juce::dontSendNotification);
    hhDensitySlider.setValue(audioProcessor.getModulatedHHDensity(), juce::dontSendNotification);
    
    // Update modulation sliders with modulated values
    chaosSlider.setValue(audioProcessor.getModulatedChaos(), juce::dontSendNotification);
    swingSlider.setValue(audioProcessor.getModulatedSwing(), juce::dontSendNotification);
    
#ifdef ENABLE_VELOCITY_SYSTEM
    // Update velocity sliders with modulated values
    bdVelocitySlider.setValue(audioProcessor.getModulatedBDVelocity(), juce::dontSendNotification);
    sdVelocitySlider.setValue(audioProcessor.getModulatedSDVelocity(), juce::dontSendNotification);
    hhVelocitySlider.setValue(audioProcessor.getModulatedHHVelocity(), juce::dontSendNotification);
#endif
#else
    // Update XY pad position from parameters
    auto xValue = audioProcessor.parameters.getRawParameterValue("x")->load();
    auto yValue = audioProcessor.parameters.getRawParameterValue("y")->load();
    xyPad.setValues(xValue, yValue);
#endif
    
    // Check if reset occurred and trigger LED matrix animation
    if (audioProcessor.hasResetOccurred())
    {
        ledMatrix.triggerReset(audioProcessor.wasResetRetrigger());
    }
}

void GridsPluginEditor::mouseMove(const juce::MouseEvent& event) {
    bool wasHovered = settingsHovered;
    settingsHovered = settingsButtonBounds.contains(event.getPosition());
    
    if (settingsHovered != wasHovered) {
        repaint();
    }
}

void GridsPluginEditor::mouseExit(const juce::MouseEvent& event) {
    if (settingsHovered) {
        settingsHovered = false;
        repaint();
    }
}

void GridsPluginEditor::sliderValueChanged(juce::Slider* slider) {
    // Update the engine immediately when sliders change
    // This ensures the LED matrix updates even when DAW is not playing
    if (slider == &bdDensitySlider) {
        gridsEngine.setBDDensity(slider->getValue());
    }
    else if (slider == &sdDensitySlider) {
        gridsEngine.setSDDensity(slider->getValue());
    }
    else if (slider == &hhDensitySlider) {
        gridsEngine.setHHDensity(slider->getValue());
    }
    else if (slider == &chaosSlider) {
        gridsEngine.setChaos(slider->getValue());
    }
    else if (slider == &swingSlider) {
        gridsEngine.setSwing(slider->getValue());
    }
}

void GridsPluginEditor::mouseDown(const juce::MouseEvent& event) {
    if (settingsButtonBounds.contains(event.getPosition())) {
        openSettings();
    }
}

bool GridsPluginEditor::keyPressed(const juce::KeyPress& key) {
    // ';' key opens settings
    if (key.getTextCharacter() == ';') {
        openSettings();
        return true;
    }
    
    return false;
}

void GridsPluginEditor::openSettings() {
    if (!settingsPanel)
    {
        // Settings panel should already be created in constructor
        DBG("ERROR: Settings panel not found!");
        return;
    }
    
    // Simple visibility toggle - like PlunderTube
    bool shouldShow = !settingsPanel->isVisible();
    DBG("Settings panel should show: " + juce::String(shouldShow ? "true" : "false"));
    
    settingsPanel->setVisible(shouldShow);
    
    // If showing, bring to front and ensure proper layout
    if (shouldShow)
    {
        settingsPanel->toFront(true);
        settingsPanel->setBounds(0, 0, getWidth(), getHeight());
    }
}