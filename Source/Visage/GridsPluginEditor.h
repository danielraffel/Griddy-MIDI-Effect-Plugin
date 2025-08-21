#pragma once

#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "XYPad.h"
#include "LEDMatrix.h"
#include "VisageStyle.h"
#include "VisageSettingsPanel.h"

class GridsPluginEditor : public juce::AudioProcessorEditor,
                         private juce::Timer,
                         private juce::Slider::Listener
{
public:
    explicit GridsPluginEditor(GridsAudioProcessor&);
    ~GridsPluginEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    
    // Keyboard events
    bool keyPressed(const juce::KeyPress& key) override;
    
    // Mouse events
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    
    // Slider listener
    void sliderValueChanged(juce::Slider* slider) override;
    
    // Open settings panel
    void openSettings();

private:
    GridsAudioProcessor& audioProcessor;
    GridsEngine& gridsEngine;
    
    // Custom look and feel
    VisageLookAndFeel visageLookAndFeel;
    
    // XY Pad for pattern navigation
    XYPad xyPad;
    
    // LED Matrix display
    LEDMatrix ledMatrix;
    juce::Slider bdDensitySlider;
    juce::Slider sdDensitySlider;
    juce::Slider hhDensitySlider;
    juce::Slider chaosSlider;
    juce::Slider swingSlider;
    
    // Labels
    juce::Label bdDensityLabel;
    juce::Label sdDensityLabel;
    juce::Label hhDensityLabel;
    juce::Label chaosLabel;
    juce::Label swingLabel;
    
    // Reset controls
    juce::Component resetButton;  // Custom button component
    juce::Label resetLabel;
    juce::ComboBox resetModeBox;
    bool resetPressed = false;
    bool resetModulatedActive = false;  // Track if reset is triggered by LFO
    int resetModulatedTimer = 0;  // Timer to keep button visible
    
    // Settings button area (for click detection)
    juce::Rectangle<int> settingsButtonBounds;
    bool settingsHovered = false;
    
    // Settings panel
    std::unique_ptr<VisageSettingsPanel> settingsPanel;
    
#ifdef ENABLE_VELOCITY_SYSTEM
    // Velocity controls - mini knobs below density sliders
    juce::Slider bdVelocitySlider;
    juce::Slider sdVelocitySlider;
    juce::Slider hhVelocitySlider;
    juce::Label velocityLabel;  // Single label saying "Vel" above all three
#endif
    
    // Parameter attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bdDensityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sdDensityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> hhDensityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> chaosAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> swingAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> resetModeAttachment;
    
#ifdef ENABLE_VELOCITY_SYSTEM
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bdVelocityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sdVelocityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> hhVelocityAttachment;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GridsPluginEditor)
};