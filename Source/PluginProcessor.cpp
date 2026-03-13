#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Settings/SettingsManager.h"

GriddyAudioProcessor::GriddyAudioProcessor()
     : AudioProcessor (BusesProperties()
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                      ),
       parameters(*this, nullptr, "GridsParameters", createParameterLayout())
{
    // Initialize engine with parameter values
    float x = parameters.getRawParameterValue("x")->load();
    float y = parameters.getRawParameterValue("y")->load();
    float bdDensity = parameters.getRawParameterValue("density_1_bd")->load();
    float sdDensity = parameters.getRawParameterValue("density_2_sd")->load();
    float hhDensity = parameters.getRawParameterValue("density_3_hh")->load();
    float chaos = parameters.getRawParameterValue("chaos")->load();
    float swing = parameters.getRawParameterValue("swing")->load();

    gridsEngine.setX(x);
    gridsEngine.setY(y);
    gridsEngine.setBDDensity(bdDensity);
    gridsEngine.setSDDensity(sdDensity);
    gridsEngine.setHHDensity(hhDensity);
    gridsEngine.setChaos(chaos);
    gridsEngine.setSwing(swing);

#ifdef ENABLE_MODULATION_MATRIX
    modulatedX_.store(x);
    modulatedY_.store(y);
    modulatedBDDensity_.store(bdDensity);
    modulatedSDDensity_.store(sdDensity);
    modulatedHHDensity_.store(hhDensity);
    modulatedChaos_.store(chaos);
    modulatedSwing_.store(swing);
    resetModulated_.store(false);
#ifdef ENABLE_VELOCITY_SYSTEM
    modulatedBDVelocity_.store(parameters.getRawParameterValue("velocity_1_bd")->load());
    modulatedSDVelocity_.store(parameters.getRawParameterValue("velocity_2_sd")->load());
    modulatedHHVelocity_.store(parameters.getRawParameterValue("velocity_3_hh")->load());
#endif
#endif
}

GriddyAudioProcessor::~GriddyAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout GriddyAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Check if user has custom MIDI note defaults
    auto& settings = SettingsManager::getInstance();
    settings.initialise();
    bool useCustom = settings.getBool(SettingsManager::Keys::useCustomMidiDefaults, false);

    int defaultBDNote = useCustom ? settings.getInt(SettingsManager::Keys::defaultBDNote, 36) : 36;
    int defaultSDNote = useCustom ? settings.getInt(SettingsManager::Keys::defaultSDNote, 38) : 38;
    int defaultHHNote = useCustom ? settings.getInt(SettingsManager::Keys::defaultHHNote, 42) : 42;

    // Pattern position
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("x", 1), "Pattern X",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("y", 1), "Pattern Y",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

    // Modulation
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("chaos", 1), "Chaos",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("swing", 1), "Swing",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

    // Playback and MIDI settings
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("midi_thru", 1), "MIDI Thru", true));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("live_mode", 1), "Live Mode", false));
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("midi_channel", 1), "MIDI Channel",
        1, 16, 1));

    // Pattern control
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("reset", 1), "Pattern Reset",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("reset_mode", 1), "Reset Mode",
        juce::StringArray{"Transparent", "Retrigger"}, 0));

    // Density controls
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("density_1_bd", 1), "BD Density",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("density_2_sd", 1), "SD Density",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("density_3_hh", 1), "HH Density",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

#ifdef ENABLE_VELOCITY_SYSTEM
    // Velocity controls
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("velocity_1_bd", 1), "BD Vel",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("velocity_2_sd", 1), "SD Vel",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("velocity_3_hh", 1), "HH Vel",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
#endif

    // MIDI Note assignments
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("note_1_bd", 1), "BD Note",
        0, 127, defaultBDNote));
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("note_2_sd", 1), "SD Note",
        0, 127, defaultSDNote));
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("note_3_hh", 1), "HH Note",
        0, 127, defaultHHNote));

    return layout;
}

const juce::String GriddyAudioProcessor::getName() const
{
    return "Griddy";
}

bool GriddyAudioProcessor::acceptsMidi() const
{
    return true;
}

bool GriddyAudioProcessor::producesMidi() const
{
    return true;
}

bool GriddyAudioProcessor::isMidiEffect() const
{
    return true;
}

double GriddyAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int GriddyAudioProcessor::getNumPrograms()
{
    return 1;
}

int GriddyAudioProcessor::getCurrentProgram()
{
    return 0;
}

void GriddyAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String GriddyAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void GriddyAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

void GriddyAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    currentSampleRate = sampleRate;
    gridsEngine.reset();
    sampleCounter = 0;
}

void GriddyAudioProcessor::releaseResources()
{
}

bool GriddyAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::disabled()
        && layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void GriddyAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer& midiMessages)
{
    // Clear audio output — this is a MIDI-only plugin with silent audio buses
    buffer.clear();

#ifdef ENABLE_MODULATION_MATRIX
    realtimeModulationPreviewGeneration_.fetch_add(1, std::memory_order_relaxed);
#endif

    // Process incoming MIDI for MIDI learn and CC control
    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();

        if (msg.isController())
        {
            int cc = msg.getControllerNumber();
            float value = msg.getControllerValue() / 127.0f;

            if (midiLearnActive)
            {
                resetMidiCC = cc;
                midiLearnActive = false;
            }
            else if (cc == resetMidiCC && resetMidiCC >= 0)
            {
                if (auto* resetParam = parameters.getParameter("reset"))
                    resetParam->setValueNotifyingHost(value);
            }
        }
    }

    // Get playhead info
    auto playHead = getPlayHead();
    if (!playHead) {
#ifdef ENABLE_MODULATION_MATRIX
        realtimeModulationPreviewActive_.store(false);
#endif
        return;
    }

    auto posInfo = playHead->getPosition();
    if (!posInfo.hasValue()) {
#ifdef ENABLE_MODULATION_MATRIX
        realtimeModulationPreviewActive_.store(false);
#endif
        return;
    }

    auto pos = *posInfo;
    auto ppq = pos.getPpqPosition();

    bool inCountIn = ppq.hasValue() && *ppq < 0.0;
    bool playing = pos.getIsPlaying();
    bool recording = pos.getIsRecording();
    bool liveMode = *parameters.getRawParameterValue("live_mode") > 0.5f;

    // Don't generate MIDI during count-in (unless in live mode)
    if (inCountIn && !liveMode) {
#ifdef ENABLE_MODULATION_MATRIX
        realtimeModulationPreviewActive_.store(false);
#endif
        if (wasInCountIn != inCountIn) {
            gridsEngine.reset();
            sampleCounter = 0;
        }
        wasInCountIn = true;
        return;
    }

    // Check for transition from count-in to playing
    bool justExitedCountIn = false;
    if (wasInCountIn && !inCountIn) {
        gridsEngine.reset();
        sampleCounter = 0;
        currentPatternStep = 0;
        justExitedCountIn = true;
    }
    wasInCountIn = inCountIn;

    // Check for reset trigger
    float currentResetValue = *parameters.getRawParameterValue("reset");

#ifdef ENABLE_MODULATION_MATRIX
    if (modulationMatrix.getModulation(ModulationMatrix::PATTERN_RESET) > 0.0f) {
        currentResetValue = std::max(currentResetValue,
                                    modulationMatrix.getModulation(ModulationMatrix::PATTERN_RESET));
    }
#endif

    // Trigger on rising edge crossing 0.5 threshold
    if (lastResetValue < 0.5f && currentResetValue >= 0.5f) {
        if (resetQuantize != QUANTIZE_OFF) {
            resetArmed = true;
        } else {
            executeReset();
        }
    }

    // Check for quantized reset
    if (resetArmed && isQuantizePoint(pos, resetQuantize)) {
        executeReset();
        resetArmed = false;
    }

    lastResetValue = currentResetValue;

    // Auto-reset parameter for button behavior
    if (currentResetValue > 0.5f) {
        if (auto* resetParam = parameters.getParameter("reset")) {
            resetParam->beginChangeGesture();
            resetParam->setValueNotifyingHost(0.0f);
            resetParam->endChangeGesture();
        }
    }

    // Generate MIDI when playing, recording, OR in live mode
    if (!playing && !recording && !liveMode) {
        isPlaying = false;
#ifdef ENABLE_MODULATION_MATRIX
        realtimeModulationPreviewActive_.store(false);
#endif
        return;
    }

    // Reset on transport start or loop
    if (!justExitedCountIn && (!isPlaying || (ppq.hasValue() && *ppq < lastPpqPosition))) {
        gridsEngine.reset();
        sampleCounter = 0;
        currentPatternStep = 0;
        hasResetOffset = false;
        ppqOffsetAtReset = 0.0;
    }
    isPlaying = playing || recording || liveMode;

#ifdef ENABLE_MODULATION_MATRIX
    realtimeModulationPreviewActive_.store(isPlaying);
#endif

    if (ppq.hasValue())
        lastPpqPosition = *ppq;

    // Update timing
    updateTiming(pos);

#ifdef ENABLE_MODULATION_MATRIX
    if (pos.getBpm().hasValue() && *pos.getBpm() > 0)
    {
        double bpm = *pos.getBpm();
        double samplesPerBeat = (currentSampleRate * 60.0) / bpm;
        modulationMatrix.processBlock(samplesPerBeat, buffer.getNumSamples());
    }
#endif

    // Handle MIDI thru mode
    bool midiThru = *parameters.getRawParameterValue("midi_thru") > 0.5f;
    if (!midiThru) {
        midiMessages.clear();
    }

    // Update engine parameters
#ifdef ENABLE_MODULATION_MATRIX
    float xValue = modulationMatrix.applyModulation(ModulationMatrix::PATTERN_X,
                                                    *parameters.getRawParameterValue("x"));
    float yValue = modulationMatrix.applyModulation(ModulationMatrix::PATTERN_Y,
                                                    *parameters.getRawParameterValue("y"));
    float bdDensity = modulationMatrix.applyModulation(ModulationMatrix::BD_DENSITY,
                                                       *parameters.getRawParameterValue("density_1_bd"));
    float sdDensity = modulationMatrix.applyModulation(ModulationMatrix::SD_DENSITY,
                                                       *parameters.getRawParameterValue("density_2_sd"));
    float hhDensity = modulationMatrix.applyModulation(ModulationMatrix::HH_DENSITY,
                                                       *parameters.getRawParameterValue("density_3_hh"));
    float chaos = modulationMatrix.applyModulation(ModulationMatrix::CHAOS,
                                                   *parameters.getRawParameterValue("chaos"));
    float swing = modulationMatrix.applyModulation(ModulationMatrix::SWING,
                                                   *parameters.getRawParameterValue("swing"));
    float swingForTiming = swing;
#ifdef ENABLE_VELOCITY_SYSTEM
    float bdVelocity = modulationMatrix.applyModulation(ModulationMatrix::BD_VELOCITY,
                                                        *parameters.getRawParameterValue("velocity_1_bd"));
    float sdVelocity = modulationMatrix.applyModulation(ModulationMatrix::SD_VELOCITY,
                                                        *parameters.getRawParameterValue("velocity_2_sd"));
    float hhVelocity = modulationMatrix.applyModulation(ModulationMatrix::HH_VELOCITY,
                                                        *parameters.getRawParameterValue("velocity_3_hh"));
#endif

    gridsEngine.setX(xValue);
    gridsEngine.setY(yValue);
    gridsEngine.setBDDensity(bdDensity);
    gridsEngine.setSDDensity(sdDensity);
    gridsEngine.setHHDensity(hhDensity);
    gridsEngine.setChaos(chaos);
    gridsEngine.setSwing(swing);

    modulatedX_.store(xValue);
    modulatedY_.store(yValue);
    modulatedBDDensity_.store(bdDensity);
    modulatedSDDensity_.store(sdDensity);
    modulatedHHDensity_.store(hhDensity);
    modulatedChaos_.store(chaos);
    modulatedSwing_.store(swing);
    resetModulated_.store(std::abs(modulationMatrix.getModulation(ModulationMatrix::PATTERN_RESET)) > 0.5f);
#ifdef ENABLE_VELOCITY_SYSTEM
    modulatedBDVelocity_.store(bdVelocity);
    modulatedSDVelocity_.store(sdVelocity);
    modulatedHHVelocity_.store(hhVelocity);
#endif
#else
    float xValue = *parameters.getRawParameterValue("x");
    float yValue = *parameters.getRawParameterValue("y");
    float bdDensity = *parameters.getRawParameterValue("density_1_bd");
    float sdDensity = *parameters.getRawParameterValue("density_2_sd");
    float hhDensity = *parameters.getRawParameterValue("density_3_hh");
    float chaos = *parameters.getRawParameterValue("chaos");
    float swing = *parameters.getRawParameterValue("swing");
    float swingForTiming = swing;

    gridsEngine.setX(xValue);
    gridsEngine.setY(yValue);
    gridsEngine.setBDDensity(bdDensity);
    gridsEngine.setSDDensity(sdDensity);
    gridsEngine.setHHDensity(hhDensity);
    gridsEngine.setChaos(chaos);
    gridsEngine.setSwing(swing);
#endif

    // Get MIDI settings
    bdNote = *parameters.getRawParameterValue("note_1_bd");
    sdNote = *parameters.getRawParameterValue("note_2_sd");
    hhNote = *parameters.getRawParameterValue("note_3_hh");

#ifdef ENABLE_MODULATION_MATRIX
    float bdNoteModulation = modulationMatrix.getModulation(ModulationMatrix::BD_MIDI_NOTE);
    float sdNoteModulation = modulationMatrix.getModulation(ModulationMatrix::SD_MIDI_NOTE);
    float hhNoteModulation = modulationMatrix.getModulation(ModulationMatrix::HH_MIDI_NOTE);

    bdNote = juce::jlimit(0.0f, 127.0f, bdNote + (bdNoteModulation * 12.0f));
    sdNote = juce::jlimit(0.0f, 127.0f, sdNote + (sdNoteModulation * 12.0f));
    hhNote = juce::jlimit(0.0f, 127.0f, hhNote + (hhNoteModulation * 12.0f));
#endif

    midiChannel = *parameters.getRawParameterValue("midi_channel");

    int numSamples = buffer.getNumSamples();

    // Handle retrigger at the beginning of the buffer
    if (shouldRetrigger) {
        gridsEngine.evaluateDrums();
        if (gridsEngine.getBDTrigger()) {
            int velocity = calculateVelocity(true, gridsEngine.getBDAccent(), "bd");
            addMidiNote(midiMessages, 0, bdNote, true, velocity);
        }
        if (gridsEngine.getSDTrigger()) {
            int velocity = calculateVelocity(false, gridsEngine.getSDAccent(), "sd");
            addMidiNote(midiMessages, 0, sdNote, true, velocity);
        }
        if (gridsEngine.getHHTrigger()) {
            int velocity = calculateVelocity(false, gridsEngine.getHHAccent(), "hh");
            addMidiNote(midiMessages, 0, hhNote, true, velocity);
        }
        shouldRetrigger = false;
    }

    // PPQ-based synchronization
    if (ppq.hasValue() && pos.getBpm().hasValue() && *pos.getBpm() > 0) {
        double currentPpq = *ppq;
        double bpm = *pos.getBpm();
        double ppqPerSample = (bpm / 60.0) / currentSampleRate;

        for (int sample = 0; sample < numSamples; ++sample) {
            double samplePpq = currentPpq + (sample * ppqPerSample);

            // Apply swing adjustment
            double swingValue = swingForTiming;
            double adjustedPpq = samplePpq;

            if (swingValue != 0.5f) {
                double beatPosition = std::fmod(samplePpq, 1.0);
                double swingOffset = (swingValue - 0.5f) * 0.1;

                if ((beatPosition > 0.20 && beatPosition < 0.30) ||
                    (beatPosition > 0.70 && beatPosition < 0.80)) {
                    adjustedPpq += swingOffset;
                }
            }

            double ppqForStep = adjustedPpq;
            if (hasResetOffset) {
                ppqForStep = adjustedPpq - ppqOffsetAtReset;
                while (ppqForStep < 0) ppqForStep += 8.0;
            }

            int targetStep = static_cast<int>(ppqForStep * 4.0) % 32;
            if (targetStep < 0) targetStep = 0;

            if (targetStep != currentPatternStep || (justExitedCountIn && targetStep == 0 && sample == 0)) {
                bool prevBD = gridsEngine.getBDTrigger();
                bool prevSD = gridsEngine.getSDTrigger();
                bool prevHH = gridsEngine.getHHTrigger();

                gridsEngine.setCurrentStep(targetStep);
                gridsEngine.syncEuclideanSteps(targetStep);
                gridsEngine.evaluateDrums();
                currentPatternStep = targetStep;

                if (prevBD) addMidiNote(midiMessages, sample, bdNote, false, 0);
                if (prevSD) addMidiNote(midiMessages, sample, sdNote, false, 0);
                if (prevHH) addMidiNote(midiMessages, sample, hhNote, false, 0);

                if (gridsEngine.getBDTrigger()) {
                    int velocity = calculateVelocity(true, gridsEngine.getBDAccent(), "bd");
                    addMidiNote(midiMessages, sample, bdNote, true, velocity);
                }
                if (gridsEngine.getSDTrigger()) {
                    int velocity = calculateVelocity(false, gridsEngine.getSDAccent(), "sd");
                    addMidiNote(midiMessages, sample, sdNote, true, velocity);
                }
                if (gridsEngine.getHHTrigger()) {
                    int velocity = calculateVelocity(false, gridsEngine.getHHAccent(), "hh");
                    addMidiNote(midiMessages, sample, hhNote, true, velocity);
                }
            }
        }
    } else {
        // Fallback to sample counting
        for (int sample = 0; sample < numSamples; ++sample) {
            if (samplesPerClock > 0 && ++sampleCounter >= samplesPerClock) {
                sampleCounter = 0;

                bool prevBD = gridsEngine.getBDTrigger();
                bool prevSD = gridsEngine.getSDTrigger();
                bool prevHH = gridsEngine.getHHTrigger();

                gridsEngine.tick();
                currentPatternStep = (currentPatternStep + 1) % 32;

                if (prevBD) addMidiNote(midiMessages, sample, bdNote, false, 0);
                if (prevSD) addMidiNote(midiMessages, sample, sdNote, false, 0);
                if (prevHH) addMidiNote(midiMessages, sample, hhNote, false, 0);

                if (gridsEngine.getBDTrigger()) {
                    int velocity = calculateVelocity(true, gridsEngine.getBDAccent(), "bd");
                    addMidiNote(midiMessages, sample, bdNote, true, velocity);
                }
                if (gridsEngine.getSDTrigger()) {
                    int velocity = calculateVelocity(false, gridsEngine.getSDAccent(), "sd");
                    addMidiNote(midiMessages, sample, sdNote, true, velocity);
                }
                if (gridsEngine.getHHTrigger()) {
                    int velocity = calculateVelocity(false, gridsEngine.getHHAccent(), "hh");
                    addMidiNote(midiMessages, sample, hhNote, true, velocity);
                }
            }
        }
    }
}

void GriddyAudioProcessor::updateTiming(const juce::AudioPlayHead::PositionInfo& posInfo)
{
    if (posInfo.getBpm().hasValue() && *posInfo.getBpm() > 0) {
        double bpm = *posInfo.getBpm();
        double beatsPerSecond = bpm / 60.0;
        double sixteenthsPerSecond = beatsPerSecond * 4.0;
        double samplesPerSixteenth = currentSampleRate / sixteenthsPerSecond;
        samplesPerClock = static_cast<int>(samplesPerSixteenth);
    }
}

void GriddyAudioProcessor::addMidiNote(juce::MidiBuffer& midiMessages, int sampleOffset,
                                       int noteNumber, bool noteOn, int velocity)
{
    int channel = juce::jlimit(1, 16, (int)midiChannel);
    juce::MidiMessage msg = noteOn
        ? juce::MidiMessage::noteOn(channel, noteNumber, (juce::uint8)velocity)
        : juce::MidiMessage::noteOff(channel, noteNumber);

    midiMessages.addEvent(msg, sampleOffset);
}

int GriddyAudioProcessor::calculateVelocity(bool isBD, bool isAccent, const juce::String& voice)
{
#ifdef ENABLE_VELOCITY_SYSTEM
    float velocityRange = 0.5f;
    if (voice == "bd")
        velocityRange = *parameters.getRawParameterValue("velocity_1_bd");
    else if (voice == "sd")
        velocityRange = *parameters.getRawParameterValue("velocity_2_sd");
    else if (voice == "hh")
        velocityRange = *parameters.getRawParameterValue("velocity_3_hh");

#ifdef ENABLE_MODULATION_MATRIX
    float velocityModulation = 0.0f;
    if (voice == "bd")
        velocityModulation = modulationMatrix.getModulation(ModulationMatrix::BD_VELOCITY);
    else if (voice == "sd")
        velocityModulation = modulationMatrix.getModulation(ModulationMatrix::SD_VELOCITY);
    else if (voice == "hh")
        velocityModulation = modulationMatrix.getModulation(ModulationMatrix::HH_VELOCITY);

    velocityRange = juce::jlimit(0.0f, 1.0f, velocityRange + velocityModulation);
#endif

    int minVel = static_cast<int>(80 - (velocityRange * 40));
    int maxVel = static_cast<int>(100 + (velocityRange * 27));
    int normalVel = (minVel + maxVel) / 2;

    if (isAccent)
        return maxVel;

    float chaos = *parameters.getRawParameterValue("chaos");
    if (chaos > 0.0f) {
        int range = maxVel - minVel;
        int variation = static_cast<int>((juce::Random::getSystemRandom().nextFloat() - 0.5f) * range * chaos * 0.3f);
        normalVel = juce::jlimit(minVel, maxVel, normalVel + variation);
    }

    if (isBD)
        normalVel = juce::jmin(127, normalVel + 10);

    return normalVel;
#else
    if (isBD)
        return isAccent ? 127 : 100;
    else if (voice == "sd")
        return isAccent ? 127 : 90;
    else
        return isAccent ? 127 : 80;
#endif
}

bool GriddyAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* GriddyAudioProcessor::createEditor()
{
    return new GriddyAudioProcessorEditor(*this);
}

void GriddyAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();

#ifdef ENABLE_MODULATION_MATRIX
    auto modTree = state.getOrCreateChildWithName("ModulationMatrix", nullptr);
    modulationMatrix.saveToValueTree(modTree);
#endif

    // Save UI toggle states
    auto uiTree = state.getOrCreateChildWithName("UIToggles", nullptr);
    uiTree.setProperty("showNotesOnMain", showNotesOnMain_, nullptr);
    uiTree.setProperty("showQuantizeOnMain", showQuantizeOnMain_, nullptr);

    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void GriddyAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName (parameters.state.getType()))
        {
            auto newState = juce::ValueTree::fromXml (*xmlState);
            parameters.replaceState (newState);

#ifdef ENABLE_MODULATION_MATRIX
            auto modTree = newState.getChildWithName("ModulationMatrix");
            if (modTree.isValid())
                modulationMatrix.loadFromValueTree(modTree);
#endif

            // Restore UI toggle states
            auto uiTree = newState.getChildWithName("UIToggles");
            if (uiTree.isValid()) {
                showNotesOnMain_ = static_cast<bool>(uiTree.getProperty("showNotesOnMain", false));
                showQuantizeOnMain_ = static_cast<bool>(uiTree.getProperty("showQuantizeOnMain", false));
            }
        }
    }
}

#ifdef ENABLE_MODULATION_MATRIX
float GriddyAudioProcessor::getModulatedBDDensity() const { return modulatedBDDensity_.load(); }
float GriddyAudioProcessor::getModulatedSDDensity() const { return modulatedSDDensity_.load(); }
float GriddyAudioProcessor::getModulatedHHDensity() const { return modulatedHHDensity_.load(); }
float GriddyAudioProcessor::getModulatedChaos() const { return modulatedChaos_.load(); }
float GriddyAudioProcessor::getModulatedSwing() const { return modulatedSwing_.load(); }
float GriddyAudioProcessor::getModulatedX() const { return modulatedX_.load(); }
float GriddyAudioProcessor::getModulatedY() const { return modulatedY_.load(); }
bool GriddyAudioProcessor::shouldUseRealtimeModulationPreview() const { return realtimeModulationPreviewActive_.load(); }
uint64_t GriddyAudioProcessor::getRealtimeModulationPreviewGeneration() const {
    return realtimeModulationPreviewGeneration_.load(std::memory_order_relaxed);
}
bool GriddyAudioProcessor::isResetModulated() const { return resetModulated_.load(); }
#ifdef ENABLE_VELOCITY_SYSTEM
float GriddyAudioProcessor::getModulatedBDVelocity() const { return modulatedBDVelocity_.load(); }
float GriddyAudioProcessor::getModulatedSDVelocity() const { return modulatedSDVelocity_.load(); }
float GriddyAudioProcessor::getModulatedHHVelocity() const { return modulatedHHVelocity_.load(); }
#endif
#endif

void GriddyAudioProcessor::executeReset()
{
    gridsEngine.reset();
    sampleCounter = 0;
    currentPatternStep = 0;

    if (auto* playHead = getPlayHead()) {
        auto posOptional = playHead->getPosition();
        if (posOptional.hasValue()) {
            auto pos = *posOptional;
            if (pos.getPpqPosition().hasValue()) {
                ppqOffsetAtReset = *pos.getPpqPosition();
                hasResetOffset = true;
            }
        }
    }

    int resetMode = *parameters.getRawParameterValue("reset_mode");
    bool isRetrigger = (resetMode == 1);

    if (isRetrigger)
        shouldRetrigger = true;

    notifyReset(isRetrigger);
}

bool GriddyAudioProcessor::isQuantizePoint(const juce::AudioPlayHead::PositionInfo& posInfo, QuantizeValue quantize)
{
    if (quantize == QUANTIZE_OFF) return true;

    auto ppq = posInfo.getPpqPosition();
    if (!ppq.hasValue()) return false;

    double ppqPos = *ppq;
    double quantum = 0.0;

    switch (quantize) {
        case QUANTIZE_2_BAR:  quantum = 8.0; break;
        case QUANTIZE_1_BAR:  quantum = 4.0; break;
        case QUANTIZE_1_2:    quantum = 2.0; break;
        case QUANTIZE_1_4:    quantum = 1.0; break;
        case QUANTIZE_1_8:    quantum = 0.5; break;
        case QUANTIZE_1_16:   quantum = 0.25; break;
        case QUANTIZE_1_32:   quantum = 0.125; break;
        case QUANTIZE_1_4T:   quantum = 2.0/3.0; break;
        case QUANTIZE_1_8T:   quantum = 1.0/3.0; break;
        case QUANTIZE_1_16T:  quantum = 0.5/3.0; break;
        default: return false;
    }

    double remainder = std::fmod(ppqPos, quantum);
    double prevRemainder = std::fmod(ppqPos - quantizePhase, quantum);

    quantizePhase = ppqPos;

    return (prevRemainder > remainder) || (remainder < 0.01);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GriddyAudioProcessor();
}
