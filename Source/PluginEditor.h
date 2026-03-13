#pragma once

#include "PluginProcessor.h"
#include "Visage/JuceVisageBridge.h"
#include "UI/XYPadFrame.h"
#include "UI/LEDMatrixFrame.h"
#include "UI/DensitySliderFrame.h"
#include "UI/RotaryKnobFrame.h"
#include "UI/ResetButtonFrame.h"
#include "UI/SettingsButtonFrame.h"
#include "UI/SettingsPanelFrame.h"
#include "UI/EuclideanOverlayFrame.h"
#include "UI/MidiNoteDisplayFrame.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <visage_ui/frame.h>
#include <unordered_set>

class GriddyAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    public juce::Timer,
                                    public juce::KeyListener {
public:
    explicit GriddyAudioProcessorEditor(GriddyAudioProcessor&);
    ~GriddyAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;

    /** Toggle the settings panel open/closed (for Cmd+, menu shortcut). */
    void toggleSettings() {
        if (settingsPanel_)
            settingsPanel_->toggleVisible();
    }

private:
    void createVisageUI();
    void layoutChildren();
    void updateUIFromProcessor();
    void syncSettingsPanelFromProcessor();
    void beginParameterGesture(juce::RangedAudioParameter* param);
    void endParameterGesture(juce::RangedAudioParameter* param);
    void performDiscreteParameterChange(juce::RangedAudioParameter* param, float normalizedValue);
    void launchAcknowledgements() const;

    GriddyAudioProcessor& processorRef;
    std::unique_ptr<JuceVisageBridge> bridge_;
    std::unique_ptr<visage::Frame> rootFrame_;

    // UI components (owned by rootFrame_ via addChild, but we keep pointers)
    XYPadFrame* xyPad_ = nullptr;
    LEDMatrixFrame* ledMatrix_ = nullptr;
    DensitySliderFrame* bdDensity_ = nullptr;
    DensitySliderFrame* sdDensity_ = nullptr;
    DensitySliderFrame* hhDensity_ = nullptr;
    RotaryKnobFrame* chaosKnob_ = nullptr;
    RotaryKnobFrame* swingKnob_ = nullptr;
    ResetButtonFrame* resetButton_ = nullptr;
    SettingsButtonFrame* settingsButton_ = nullptr;
    SettingsPanelFrame* settingsPanel_ = nullptr;
    EuclideanOverlayFrame* euclideanOverlay_ = nullptr;
    MidiNoteDisplayFrame* midiNoteDisplay_ = nullptr;
    RotaryKnobFrame* bdVelKnob_ = nullptr;
    RotaryKnobFrame* sdVelKnob_ = nullptr;
    RotaryKnobFrame* hhVelKnob_ = nullptr;
    std::unordered_set<juce::RangedAudioParameter*> activeParameterGestures_;
    int resetGlowFramesRemaining_ = 0;
    uint64_t lastRealtimePreviewGeneration_ = 0;
    int staleRealtimePreviewFrames_ = 0;

    bool uiCreated_ = false;

#if JUCE_MAC
    // macOS Settings menu item (standalone only)
    class SettingsMenuBarModel : public juce::MenuBarModel {
    public:
        std::function<void()> onSettings;
        juce::StringArray getMenuBarNames() override { return {}; }
        juce::PopupMenu getMenuForIndex(int, const juce::String&) override { return {}; }
        void menuItemSelected(int id, int) override {
            if (id == 1 && onSettings) onSettings();
        }
    };
    std::unique_ptr<SettingsMenuBarModel> settingsMenuModel_;
    std::unique_ptr<juce::PopupMenu> settingsMenuPopup_;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GriddyAudioProcessorEditor)
};
