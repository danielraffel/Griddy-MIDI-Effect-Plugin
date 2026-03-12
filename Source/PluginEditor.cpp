#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <BinaryData.h>
#include <visage_graphics/font.h>
#include <optional>

namespace visage::fonts { extern ::visage::EmbeddedFile Lato_Regular_ttf; }

namespace {
#ifdef ENABLE_MODULATION_MATRIX
std::optional<ModulationMatrix::Destination> destinationForIndex(int destIndex) {
    if (destIndex < 0 || destIndex >= ModulationMatrix::NUM_DESTINATIONS)
        return std::nullopt;

    return static_cast<ModulationMatrix::Destination>(destIndex);
}
#endif
}

GriddyAudioProcessorEditor::GriddyAudioProcessorEditor(GriddyAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p) {
    setSize(580, 330);
    setResizable(false, false);
    startTimer(10);
}

void GriddyAudioProcessorEditor::beginParameterGesture(juce::RangedAudioParameter* param) {
    if (param && activeParameterGestures_.insert(param).second)
        param->beginChangeGesture();
}

void GriddyAudioProcessorEditor::endParameterGesture(juce::RangedAudioParameter* param) {
    if (param && activeParameterGestures_.erase(param) > 0)
        param->endChangeGesture();
}

void GriddyAudioProcessorEditor::performDiscreteParameterChange(juce::RangedAudioParameter* param,
                                                                float normalizedValue) {
    if (!param)
        return;

    param->beginChangeGesture();
    param->setValueNotifyingHost(normalizedValue);
    param->endChangeGesture();
}

void GriddyAudioProcessorEditor::launchAcknowledgements() const {
    // Use platform-appropriate temp/cache directory
    auto cacheDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                        .getChildFile("Griddy");
    auto licensesFile = cacheDir.getChildFile("Griddy_Licenses.html");

    if (!cacheDir.exists() && !cacheDir.createDirectory()) {
        juce::NativeMessageBox::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon,
            "Acknowledgements",
            "Could not create cache directory for the licenses file.");
        return;
    }

    if (!licensesFile.replaceWithData(BinaryData::griddylicenses_html,
                                      static_cast<size_t>(BinaryData::griddylicenses_htmlSize))) {
        juce::NativeMessageBox::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon,
            "Acknowledgements",
            "Could not write the licenses file to cache directory.");
        return;
    }

    // startAsProcess opens local files with the system default handler,
    // which works reliably on both macOS and Windows (unlike URL::launchInDefaultBrowser
    // which can fail for local file:// URLs on Windows).
    if (!licensesFile.startAsProcess()) {
        juce::NativeMessageBox::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon,
            "Acknowledgements",
            "Could not open the Griddy licenses file in the default browser.");
    }
}

GriddyAudioProcessorEditor::~GriddyAudioProcessorEditor() {
    if (auto* topLevel = getTopLevelComponent())
        topLevel->removeKeyListener(this);

#if JUCE_MAC
    juce::MenuBarModel::setMacMainMenu(nullptr);
    settingsMenuModel_.reset();
    settingsMenuPopup_.reset();
#endif
    stopTimer();
    for (auto* param : activeParameterGestures_) {
        if (param)
            param->endChangeGesture();
    }
    activeParameterGestures_.clear();
    if (bridge_)
        bridge_->shutdownRendering();
    if (rootFrame_)
        rootFrame_->removeAllChildren();
    xyPad_ = nullptr;
    ledMatrix_ = nullptr;
    bdDensity_ = nullptr;
    sdDensity_ = nullptr;
    hhDensity_ = nullptr;
    chaosKnob_ = nullptr;
    swingKnob_ = nullptr;
    resetButton_ = nullptr;
    settingsButton_ = nullptr;
    settingsPanel_ = nullptr;
    bdVelKnob_ = nullptr;
    sdVelKnob_ = nullptr;
    hhVelKnob_ = nullptr;
    if (bridge_)
        bridge_->setRootFrame(nullptr);
    rootFrame_.reset();
    bridge_.reset();
}

bool GriddyAudioProcessorEditor::keyPressed(const juce::KeyPress& key, juce::Component*) {
    // Cmd+, toggles settings (standalone)
    if (key == juce::KeyPress(',', juce::ModifierKeys::commandModifier, 0)) {
        toggleSettings();
        return true;
    }
    // ESC closes settings panel
    if (key == juce::KeyPress::escapeKey) {
        if (settingsPanel_ && settingsPanel_->isVisible()) {
            settingsPanel_->hide();
            return true;
        }
    }
    return false;
}

void GriddyAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff1e1e1e));
}

void GriddyAudioProcessorEditor::resized() {
    if (bridge_)
        bridge_->setBounds(getLocalBounds());
    if (rootFrame_) {
        rootFrame_->setBounds(0, 0, static_cast<float>(getWidth()),
                              static_cast<float>(getHeight()));
        layoutChildren();
    }
}

void GriddyAudioProcessorEditor::layoutChildren() {
    // Layout uses 8px margins on all sides and between rows/columns.
    // Total: 8 + 248 + 8 + 308 + 8 = 580 width
    //        8 + 232 + 8 + 74  + 8 = 330 height

    // XY pad in left panel
    if (xyPad_)
        xyPad_->setBounds(14, 14, 236, 222);

    // Chaos and Swing knobs (upper area of controls panel)
    if (chaosKnob_)
        chaosKnob_->setBounds(276, 14, 68, 68);
    if (swingKnob_)
        swingKnob_->setBounds(354, 14, 68, 68);

    // Reset button — centered below chaos/swing, 3/4 their size
    if (resetButton_)
        resetButton_->setBounds(332, 90, 34, 34);

    // Density sliders (shifted left for settings button breathing room)
    if (bdDensity_)
        bdDensity_->setBounds(438, 14, 30, 148);
    if (sdDensity_)
        sdDensity_->setBounds(472, 14, 30, 148);
    if (hhDensity_)
        hhDensity_->setBounds(506, 14, 30, 148);

    // Velocity knobs below density sliders
    if (bdVelKnob_)
        bdVelKnob_->setBounds(434, 178, 38, 50);
    if (sdVelKnob_)
        sdVelKnob_->setBounds(468, 178, 38, 50);
    if (hhVelKnob_)
        hhVelKnob_->setBounds(502, 178, 38, 50);

    // LED matrix at bottom (4px padding inside the 74px panel background)
    if (ledMatrix_)
        ledMatrix_->setBounds(14, 252, 552, 66);

    // Settings button — with clear gap from HH slider
    if (settingsButton_)
        settingsButton_->setBounds(550, 12, 16, 16);

    // Settings overlay covers everything
    if (settingsPanel_)
        settingsPanel_->setBounds(0, 0, static_cast<float>(getWidth()), static_cast<float>(getHeight()));
}

void GriddyAudioProcessorEditor::syncSettingsPanelFromProcessor() {
    if (!settingsPanel_)
        return;

    settingsPanel_->setMidiLearnActive(processorRef.isMidiLearning());
    settingsPanel_->setResetMidiCC(processorRef.getResetMidiCC());

#ifdef ENABLE_MODULATION_MATRIX
    auto& modulationMatrix = processorRef.getModulationMatrix();
    for (int lfoIdx = 0; lfoIdx < 2; ++lfoIdx) {
        const auto& lfo = modulationMatrix.getLFO(lfoIdx);
        settingsPanel_->setLFOEnabled(lfoIdx, lfo.isEnabled());
        settingsPanel_->setLFOShape(lfoIdx, static_cast<int>(lfo.getShape()));
        settingsPanel_->setLFORate(lfoIdx, lfo.getRate());
        settingsPanel_->setLFODepth(lfoIdx, lfo.getDepth());
    }

    for (int destIdx = 0; destIdx < ModulationMatrix::NUM_DESTINATIONS; ++destIdx) {
        auto dest = static_cast<ModulationMatrix::Destination>(destIdx);
        const auto& routing = modulationMatrix.getRouting(dest);
        for (int lfoIdx = 0; lfoIdx < 2; ++lfoIdx)
            settingsPanel_->setLFODest(lfoIdx, destIdx, routing.enabled && routing.sourceId == lfoIdx);
    }
#endif
}

void GriddyAudioProcessorEditor::timerCallback() {
    if (!uiCreated_ && getWidth() > 0 && isShowing()) {
        stopTimer();
        createVisageUI();
        startTimer(33); // 30fps update polling
        return;
    }

    if (uiCreated_)
        updateUIFromProcessor();
}

void GriddyAudioProcessorEditor::createVisageUI() {
    rootFrame_ = std::make_unique<visage::Frame>();

    // Draw dark background and panels
    rootFrame_->onDraw() += [](visage::Canvas& canvas) {
        float w = static_cast<float>(canvas.width());
        float h = static_cast<float>(canvas.height());
        canvas.setColor(0xff1e1e1e);
        canvas.fill(0, 0, w, h);

        visage::Font labelFont(10.0f, visage::fonts::Lato_Regular_ttf);

        // Background panel behind XY pad area
        canvas.setColor(0xff2a2a2a);
        canvas.roundedRectangle(8, 8, 248, 232, 8.0f);
        canvas.setColor(0xff333333);
        canvas.roundedRectangleBorder(8, 8, 248, 232, 8.0f, 1.0f);

        // Background panel behind controls area
        canvas.setColor(0xff2a2a2a);
        canvas.roundedRectangle(264, 8, 308, 232, 8.0f);
        canvas.setColor(0xff333333);
        canvas.roundedRectangleBorder(264, 8, 308, 232, 8.0f, 1.0f);

        // "Velocity" label above velocity knobs
        canvas.setColor(0xffaaaaaa);
        canvas.text("Velocity", labelFont, visage::Font::kCenter, 434, 164, 106, 14);

        // "Reset" label below reset button
        canvas.setColor(0xffaaaaaa);
        canvas.text("Reset", labelFont, visage::Font::kCenter, 316, 126, 66, 14);

        // Background panel behind LED matrix (y=248, h=74, ends at 322, 8px from bottom)
        canvas.setColor(0xff2a2a2a);
        canvas.roundedRectangle(8, 248, 564, 74, 8.0f);
        canvas.setColor(0xff333333);
        canvas.roundedRectangleBorder(8, 248, 564, 74, 8.0f, 1.0f);
    };

    auto bindContinuousParameter = [this](auto* control, juce::RangedAudioParameter* param) {
        control->onGestureStart = [this, param]() { beginParameterGesture(param); };
        control->onGestureEnd = [this, param]() { endParameterGesture(param); };
        control->onValueChange = [param](float v) {
            if (param)
                param->setValueNotifyingHost(param->getNormalisableRange().convertTo0to1(v));
        };
    };

    // Create XY Pad
    auto xyPadOwned = std::make_unique<XYPadFrame>();
    xyPad_ = xyPadOwned.get();
    auto* xParam = processorRef.parameters.getParameter("x");
    auto* yParam = processorRef.parameters.getParameter("y");
    xyPad_->setX(*processorRef.parameters.getRawParameterValue("x"));
    xyPad_->setY(*processorRef.parameters.getRawParameterValue("y"));
    xyPad_->onGestureStart = [this, xParam, yParam]() {
        beginParameterGesture(xParam);
        beginParameterGesture(yParam);
    };
    xyPad_->onValueChange = [xParam, yParam](float x, float y) {
        if (xParam)
            xParam->setValueNotifyingHost(xParam->getNormalisableRange().convertTo0to1(x));
        if (yParam)
            yParam->setValueNotifyingHost(yParam->getNormalisableRange().convertTo0to1(y));
    };
    xyPad_->onGestureEnd = [this, xParam, yParam]() {
        endParameterGesture(xParam);
        endParameterGesture(yParam);
    };

    // Create Chaos knob
    auto chaosOwned = std::make_unique<RotaryKnobFrame>("Chaos", 0xffff8833);
    chaosKnob_ = chaosOwned.get();
    auto* chaosParam = processorRef.parameters.getParameter("chaos");
    chaosKnob_->setValue(*processorRef.parameters.getRawParameterValue("chaos"));
    bindContinuousParameter(chaosKnob_, chaosParam);

    // Create Swing knob
    auto swingOwned = std::make_unique<RotaryKnobFrame>("Swing", 0xff33aaff);
    swingKnob_ = swingOwned.get();
    auto* swingParam = processorRef.parameters.getParameter("swing");
    swingKnob_->setValue(*processorRef.parameters.getRawParameterValue("swing"));
    bindContinuousParameter(swingKnob_, swingParam);

    // Create Reset button
    auto resetOwned = std::make_unique<ResetButtonFrame>();
    resetButton_ = resetOwned.get();
    auto* resetParam = processorRef.parameters.getParameter("reset");
    resetButton_->onPress = [this, resetParam]() {
        performDiscreteParameterChange(resetParam, 1.0f);
    };

    // Create Density sliders (BD=red, SD=green, HH=yellow)
    auto bdOwned = std::make_unique<DensitySliderFrame>("BD", 0xffff4444);
    bdDensity_ = bdOwned.get();
    auto* bdDensityParam = processorRef.parameters.getParameter("density_1_bd");
    bdDensity_->setValue(*processorRef.parameters.getRawParameterValue("density_1_bd"));
    bindContinuousParameter(bdDensity_, bdDensityParam);

    auto sdOwned = std::make_unique<DensitySliderFrame>("SD", 0xff44ff44);
    sdDensity_ = sdOwned.get();
    auto* sdDensityParam = processorRef.parameters.getParameter("density_2_sd");
    sdDensity_->setValue(*processorRef.parameters.getRawParameterValue("density_2_sd"));
    bindContinuousParameter(sdDensity_, sdDensityParam);

    auto hhOwned = std::make_unique<DensitySliderFrame>("HH", 0xffffff44);
    hhDensity_ = hhOwned.get();
    auto* hhDensityParam = processorRef.parameters.getParameter("density_3_hh");
    hhDensity_->setValue(*processorRef.parameters.getRawParameterValue("density_3_hh"));
    bindContinuousParameter(hhDensity_, hhDensityParam);

    // Create Velocity knobs (below density sliders)
    auto bdVelOwned = std::make_unique<RotaryKnobFrame>("BD", 0xffff4444);
    bdVelKnob_ = bdVelOwned.get();
    auto* bdVelocityParam = processorRef.parameters.getParameter("velocity_1_bd");
    bdVelKnob_->setValue(*processorRef.parameters.getRawParameterValue("velocity_1_bd"));
    bindContinuousParameter(bdVelKnob_, bdVelocityParam);

    auto sdVelOwned = std::make_unique<RotaryKnobFrame>("SD", 0xff44ff44);
    sdVelKnob_ = sdVelOwned.get();
    auto* sdVelocityParam = processorRef.parameters.getParameter("velocity_2_sd");
    sdVelKnob_->setValue(*processorRef.parameters.getRawParameterValue("velocity_2_sd"));
    bindContinuousParameter(sdVelKnob_, sdVelocityParam);

    auto hhVelOwned = std::make_unique<RotaryKnobFrame>("HH", 0xffffff44);
    hhVelKnob_ = hhVelOwned.get();
    auto* hhVelocityParam = processorRef.parameters.getParameter("velocity_3_hh");
    hhVelKnob_->setValue(*processorRef.parameters.getRawParameterValue("velocity_3_hh"));
    bindContinuousParameter(hhVelKnob_, hhVelocityParam);

    // Create Settings button
    auto settingsOwned = std::make_unique<SettingsButtonFrame>();
    settingsButton_ = settingsOwned.get();

    // Create Settings panel overlay
    auto settingsPanelOwned = std::make_unique<SettingsPanelFrame>();
    settingsPanel_ = settingsPanelOwned.get();
    settingsPanel_->setVisible(false);

    // Initialize settings panel from current parameter values
    settingsPanel_->setMidiChannel(static_cast<int>(*processorRef.parameters.getRawParameterValue("midi_channel")));
    settingsPanel_->setBDNote(static_cast<int>(*processorRef.parameters.getRawParameterValue("note_1_bd")));
    settingsPanel_->setSDNote(static_cast<int>(*processorRef.parameters.getRawParameterValue("note_2_sd")));
    settingsPanel_->setHHNote(static_cast<int>(*processorRef.parameters.getRawParameterValue("note_3_hh")));
    settingsPanel_->setMidiThru(*processorRef.parameters.getRawParameterValue("midi_thru") > 0.5f);
    settingsPanel_->setLiveMode(*processorRef.parameters.getRawParameterValue("live_mode") > 0.5f);
    settingsPanel_->setResetMode(static_cast<int>(*processorRef.parameters.getRawParameterValue("reset_mode")));
    syncSettingsPanelFromProcessor();

    // Wire settings panel callbacks to processor parameters
    auto* midiChannelParam = processorRef.parameters.getParameter("midi_channel");
    auto* bdNoteParam = processorRef.parameters.getParameter("note_1_bd");
    auto* sdNoteParam = processorRef.parameters.getParameter("note_2_sd");
    auto* hhNoteParam = processorRef.parameters.getParameter("note_3_hh");
    auto* midiThruParam = processorRef.parameters.getParameter("midi_thru");
    auto* liveModeParam = processorRef.parameters.getParameter("live_mode");
    auto* resetModeParam = processorRef.parameters.getParameter("reset_mode");
    settingsPanel_->onMidiChannelChange = [this, midiChannelParam](int ch) {
        if (midiChannelParam)
            performDiscreteParameterChange(
                midiChannelParam,
                midiChannelParam->getNormalisableRange().convertTo0to1(static_cast<float>(ch)));
    };
    settingsPanel_->onBDNoteChange = [this, bdNoteParam](int note) {
        if (bdNoteParam)
            performDiscreteParameterChange(
                bdNoteParam,
                bdNoteParam->getNormalisableRange().convertTo0to1(static_cast<float>(note)));
    };
    settingsPanel_->onSDNoteChange = [this, sdNoteParam](int note) {
        if (sdNoteParam)
            performDiscreteParameterChange(
                sdNoteParam,
                sdNoteParam->getNormalisableRange().convertTo0to1(static_cast<float>(note)));
    };
    settingsPanel_->onHHNoteChange = [this, hhNoteParam](int note) {
        if (hhNoteParam)
            performDiscreteParameterChange(
                hhNoteParam,
                hhNoteParam->getNormalisableRange().convertTo0to1(static_cast<float>(note)));
    };
    settingsPanel_->onMidiThruChange = [this, midiThruParam](bool v) {
        performDiscreteParameterChange(midiThruParam, v ? 1.0f : 0.0f);
    };
    settingsPanel_->onLiveModeChange = [this, liveModeParam](bool v) {
        performDiscreteParameterChange(liveModeParam, v ? 1.0f : 0.0f);
    };
    settingsPanel_->onResetModeChange = [this, resetModeParam](int mode) {
        if (resetModeParam)
            performDiscreteParameterChange(
                resetModeParam,
                resetModeParam->getNormalisableRange().convertTo0to1(static_cast<float>(mode)));
    };
    settingsPanel_->onOpenAcknowledgements = [this]() { launchAcknowledgements(); };
    settingsPanel_->onMidiLearnStart = [this]() {
        processorRef.startMidiLearnForReset();
        syncSettingsPanelFromProcessor();
    };
    settingsPanel_->onMidiLearnStop = [this]() {
        processorRef.stopMidiLearn();
        syncSettingsPanelFromProcessor();
    };
#ifdef ENABLE_MODULATION_MATRIX
    settingsPanel_->onLFOEnableChange = [this](int lfoIdx, bool enabled) {
        auto& lfo = processorRef.getModulationMatrix().getLFO(lfoIdx);
        lfo.setEnabled(enabled);
        syncSettingsPanelFromProcessor();
    };
    settingsPanel_->onLFOShapeChange = [this](int lfoIdx, int shape) {
        auto& lfo = processorRef.getModulationMatrix().getLFO(lfoIdx);
        lfo.setShape(static_cast<LFO::Shape>(shape));
        syncSettingsPanelFromProcessor();
    };
    settingsPanel_->onLFORateChange = [this](int lfoIdx, float rate) {
        auto& lfo = processorRef.getModulationMatrix().getLFO(lfoIdx);
        lfo.setRate(rate);
        syncSettingsPanelFromProcessor();
    };
    settingsPanel_->onLFODepthChange = [this](int lfoIdx, float depth) {
        auto& lfo = processorRef.getModulationMatrix().getLFO(lfoIdx);
        lfo.setDepth(depth);
        syncSettingsPanelFromProcessor();
    };
    settingsPanel_->onLFODestChange = [this](int lfoIdx, int destIdx, bool enabled) {
        auto dest = destinationForIndex(destIdx);
        if (!dest.has_value())
            return;

        auto& modulationMatrix = processorRef.getModulationMatrix();
        if (enabled) {
            modulationMatrix.setRouting(lfoIdx, *dest, 1.0f, true);
        } else {
            const auto& routing = modulationMatrix.getRouting(*dest);
            if (routing.enabled && routing.sourceId == lfoIdx)
                modulationMatrix.clearRouting(*dest);
        }
        syncSettingsPanelFromProcessor();
    };
#endif

    // Wire settings button to toggle the panel
    settingsButton_->onPress = [this]() {
        if (settingsPanel_)
            settingsPanel_->toggleVisible();
    };

    // Create LED Matrix
    auto ledMatrixOwned = std::make_unique<LEDMatrixFrame>();
    ledMatrix_ = ledMatrixOwned.get();

    // Add children to root frame (settings panel last so it draws on top)
    rootFrame_->addChild(xyPadOwned.release());
    rootFrame_->addChild(chaosOwned.release());
    rootFrame_->addChild(swingOwned.release());
    rootFrame_->addChild(resetOwned.release());
    rootFrame_->addChild(bdOwned.release());
    rootFrame_->addChild(sdOwned.release());
    rootFrame_->addChild(hhOwned.release());
    rootFrame_->addChild(bdVelOwned.release());
    rootFrame_->addChild(sdVelOwned.release());
    rootFrame_->addChild(hhVelOwned.release());
    rootFrame_->addChild(settingsOwned.release());
    rootFrame_->addChild(ledMatrixOwned.release());
    rootFrame_->addChild(settingsPanelOwned.release());

    // Native macOS title bar for standalone mode.
    // CRITICAL: setUsingNativeTitleBar() removes JUCE's drawn title bar border
    // (27px top + 1px sides) but the window stays the same native size. The content
    // area expands to fill the full window, making the editor larger than requested.
    // Fix: re-assert setSize() after the switch to force correct dimensions.
    if (auto* window = findParentComponentOfClass<juce::DocumentWindow>()) {
        window->setUsingNativeTitleBar(true);
        setSize(580, 330);

#if JUCE_MAC
        // Add "Settings..." to macOS app menu via JUCE's MenuBarModel
        settingsMenuModel_ = std::make_unique<SettingsMenuBarModel>();
        settingsMenuModel_->onSettings = [this]() { toggleSettings(); };

        settingsMenuPopup_ = std::make_unique<juce::PopupMenu>();
        settingsMenuPopup_->addItem(1, "Settings...");
        settingsMenuPopup_->addSeparator();

        juce::MenuBarModel::setMacMainMenu(settingsMenuModel_.get(), settingsMenuPopup_.get());
#endif
    }

    // Register key listener on top-level window to catch Cmd+, and ESC
    if (auto* topLevel = getTopLevelComponent())
        topLevel->addKeyListener(this);

    // Set up bridge
    bridge_ = std::make_unique<JuceVisageBridge>();
    addAndMakeVisible(*bridge_);
    bridge_->setBounds(getLocalBounds());
    bridge_->setRootFrame(rootFrame_.get());
    bridge_->startTimer(10);

    uiCreated_ = true;

    // The editor is often already at its final size before the Visage tree exists,
    // so no later resized() callback is guaranteed from the host.
    rootFrame_->setBounds(0, 0, static_cast<float>(getWidth()), static_cast<float>(getHeight()));
    layoutChildren();

    // Initial update
    updateUIFromProcessor();
}

void GriddyAudioProcessorEditor::updateUIFromProcessor() {
    auto& engine = processorRef.getGridsEngine();

    float paramX = *processorRef.parameters.getRawParameterValue("x");
    float paramY = *processorRef.parameters.getRawParameterValue("y");
    float bdDensity = *processorRef.parameters.getRawParameterValue("density_1_bd");
    float sdDensity = *processorRef.parameters.getRawParameterValue("density_2_sd");
    float hhDensity = *processorRef.parameters.getRawParameterValue("density_3_hh");
    float bdVel = *processorRef.parameters.getRawParameterValue("velocity_1_bd");
    float sdVel = *processorRef.parameters.getRawParameterValue("velocity_2_sd");
    float hhVel = *processorRef.parameters.getRawParameterValue("velocity_3_hh");
    float chaos = *processorRef.parameters.getRawParameterValue("chaos");
    float swing = *processorRef.parameters.getRawParameterValue("swing");

#ifdef ENABLE_MODULATION_MATRIX
    const auto previewGeneration = processorRef.getRealtimeModulationPreviewGeneration();
    if (previewGeneration != lastRealtimePreviewGeneration_) {
        lastRealtimePreviewGeneration_ = previewGeneration;
        staleRealtimePreviewFrames_ = 0;
    } else if (staleRealtimePreviewFrames_ < 1000) {
        ++staleRealtimePreviewFrames_;
    }

    const bool useRealtimePreview =
        processorRef.shouldUseRealtimeModulationPreview() && staleRealtimePreviewFrames_ <= 2;

    if (!processorRef.shouldUseRealtimeModulationPreview())
        staleRealtimePreviewFrames_ = 0;

    if (useRealtimePreview) {
        paramX = processorRef.getModulatedX();
        paramY = processorRef.getModulatedY();
        bdDensity = processorRef.getModulatedBDDensity();
        sdDensity = processorRef.getModulatedSDDensity();
        hhDensity = processorRef.getModulatedHHDensity();
        chaos = processorRef.getModulatedChaos();
        swing = processorRef.getModulatedSwing();
#ifdef ENABLE_VELOCITY_SYSTEM
        bdVel = processorRef.getModulatedBDVelocity();
        sdVel = processorRef.getModulatedSDVelocity();
        hhVel = processorRef.getModulatedHHVelocity();
#endif
    }
#endif

    // Ensure engine has latest values for pattern generation (UI thread sync)
    engine.setX(paramX);
    engine.setY(paramY);
    engine.setBDDensity(bdDensity);
    engine.setSDDensity(sdDensity);
    engine.setHHDensity(hhDensity);

    // Update XY pad
    if (xyPad_) {
        xyPad_->setX(paramX);
        xyPad_->setY(paramY);
    }

    if (bdDensity_) bdDensity_->setValue(bdDensity);
    if (sdDensity_) sdDensity_->setValue(sdDensity);
    if (hhDensity_) hhDensity_->setValue(hhDensity);

    // Update knobs
    if (chaosKnob_)
        chaosKnob_->setValue(chaos);
    if (swingKnob_)
        swingKnob_->setValue(swing);

    // Update velocity knobs
    if (bdVelKnob_)
        bdVelKnob_->setValue(bdVel);
    if (sdVelKnob_)
        sdVelKnob_->setValue(sdVel);
    if (hhVelKnob_)
        hhVelKnob_->setValue(hhVel);

    // Update settings panel MIDI learn state
    if (settingsPanel_) {
        settingsPanel_->setMidiLearnActive(processorRef.isMidiLearning());
        settingsPanel_->setResetMidiCC(processorRef.getResetMidiCC());
    }

    // Update reset button glow and LED animation
    if (processorRef.hasResetOccurred()) {
        bool retrigger = processorRef.wasResetRetrigger();
        resetGlowFramesRemaining_ = 6;
        if (resetButton_) resetButton_->setGlow(true);
        if (ledMatrix_) ledMatrix_->triggerResetAnimation(retrigger);
    } else if (resetGlowFramesRemaining_ > 0) {
        --resetGlowFramesRemaining_;
        if (resetGlowFramesRemaining_ == 0 && resetButton_)
            resetButton_->setGlow(false);
    }

    // Update LED matrix with patterns and velocity ranges
    if (ledMatrix_) {
        ledMatrix_->setDensities(bdDensity, sdDensity, hhDensity);
        ledMatrix_->setVelocityRanges(bdVel, sdVel, hhVel);
        ledMatrix_->setPatterns(engine.getBDPattern(), engine.getSDPattern(), engine.getHHPattern());
        ledMatrix_->setCurrentStep(engine.getCurrentStep());
    }
}
