#include "PluginProcessor.h"
#include "Visage/GridsPluginEditor.h"

GridsAudioProcessor::GridsAudioProcessor()
     : AudioProcessor (BusesProperties()
                      #if ! JucePlugin_IsMidiEffect
                      .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       parameters(*this, nullptr, "GridsParameters", createParameterLayout())
{
    
    // Initialize engine with parameter values
    gridsEngine.setX(*parameters.getRawParameterValue("x"));
    gridsEngine.setY(*parameters.getRawParameterValue("y"));
    gridsEngine.setBDDensity(*parameters.getRawParameterValue("density_1_bd"));
    gridsEngine.setSDDensity(*parameters.getRawParameterValue("density_2_sd"));
    gridsEngine.setHHDensity(*parameters.getRawParameterValue("density_3_hh"));
    gridsEngine.setChaos(*parameters.getRawParameterValue("chaos"));
    gridsEngine.setSwing(*parameters.getRawParameterValue("swing"));
}

GridsAudioProcessor::~GridsAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout GridsAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
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
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));  // Momentary trigger
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("reset_mode", 1), "Reset Mode",
        juce::StringArray{"Transparent", "Retrigger"}, 0));
    
    // Density controls - using numeric IDs to force order
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
    // Velocity controls - mini knobs below density sliders
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("velocity_1_bd", 1), "BD Vel", 
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));  // 0 = narrow range, 1 = wide range
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("velocity_2_sd", 1), "SD Vel", 
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("velocity_3_hh", 1), "HH Vel", 
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
#endif
    
    // MIDI Note assignments - using numeric IDs to force order
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("note_1_bd", 1), "BD Note", 
        0, 127, 36));
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("note_2_sd", 1), "SD Note", 
        0, 127, 38));
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("note_3_hh", 1), "HH Note", 
        0, 127, 42));
    
    return layout;
}

const juce::String GridsAudioProcessor::getName() const
{
    return "Griddy";
}

bool GridsAudioProcessor::acceptsMidi() const
{
    return true;
}

bool GridsAudioProcessor::producesMidi() const
{
    return true;
}

bool GridsAudioProcessor::isMidiEffect() const
{
    return true;
}

double GridsAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int GridsAudioProcessor::getNumPrograms()
{
    return 1;
}

int GridsAudioProcessor::getCurrentProgram()
{
    return 0;
}

void GridsAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String GridsAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void GridsAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

void GridsAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    currentSampleRate = sampleRate;
    gridsEngine.reset();
    sampleCounter = 0;
}

void GridsAudioProcessor::releaseResources()
{
}

bool GridsAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // For AU MIDI effects in Logic Pro, we need stereo in/out
    // Logic Pro requires audio buses even for MIDI effects
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // If there's an input bus, it should also be stereo
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::disabled()
        && layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void GridsAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (buffer);
    
    // Process incoming MIDI for MIDI learn and CC control
    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();
        
        if (msg.isController())
        {
            int cc = msg.getControllerNumber();
            float value = msg.getControllerValue() / 127.0f;
            
            // MIDI learn mode - learn the CC
            if (midiLearnActive)
            {
                resetMidiCC = cc;
                midiLearnActive = false;
                DBG("Learned MIDI CC " + juce::String(cc) + " for reset");
            }
            // Check if this CC controls reset
            else if (cc == resetMidiCC && resetMidiCC >= 0)
            {
                // Use CC value to control reset (>0.5 triggers)
                if (auto* resetParam = parameters.getParameter("reset"))
                {
                    resetParam->setValueNotifyingHost(value);
                }
            }
        }
    }
    
    // Get playhead info
    auto playHead = getPlayHead();
    if (!playHead) return;
    
    auto posInfo = playHead->getPosition();
    if (!posInfo.hasValue()) return;
    
    auto pos = *posInfo;
    
    // Get PPQ position for sync
    auto ppq = pos.getPpqPosition();
    
    // Check for count-in (negative PPQ position)
    bool inCountIn = ppq.hasValue() && *ppq < 0.0;
    
    // Check if transport is playing or recording
    bool playing = pos.getIsPlaying();
    bool recording = pos.getIsRecording();
    bool liveMode = *parameters.getRawParameterValue("live_mode") > 0.5f;
    
    // Don't generate MIDI during count-in (unless in live mode)
    if (inCountIn && !liveMode) {
        // During count-in: keep pattern at step 0, don't generate MIDI
        if (wasInCountIn != inCountIn) {
            gridsEngine.reset();
            sampleCounter = 0;
        }
        wasInCountIn = true;
        return;
    }
    
    // Check for transition from count-in to recording/playing
    bool justExitedCountIn = false;
    if (wasInCountIn && !inCountIn) {
        // Just exited count-in, reset pattern to start
        gridsEngine.reset();
        sampleCounter = 0;
        currentPatternStep = 0;
        justExitedCountIn = true;
    }
    wasInCountIn = inCountIn;
    
    // Check for reset trigger BEFORE early return so it works when stopped
    float currentResetValue = *parameters.getRawParameterValue("reset");
    
    // Combine with internal modulation if present
#ifdef ENABLE_MODULATION_MATRIX
    if (modulationMatrix.getModulation(ModulationMatrix::PATTERN_RESET) > 0.0f) {
        currentResetValue = std::max(currentResetValue, 
                                    modulationMatrix.getModulation(ModulationMatrix::PATTERN_RESET));
    }
#endif
    
    // Debug logging for reset
    if (currentResetValue != lastResetValue) {
        DBG("Reset value changed from " << lastResetValue << " to " << currentResetValue);
    }
    
    // Trigger on rising edge crossing 0.5 threshold
    if (lastResetValue < 0.5f && currentResetValue >= 0.5f) {
        DBG("Reset triggered! Quantize mode: " << resetQuantize);
        if (resetQuantize != QUANTIZE_OFF) {
            resetArmed = true;  // Wait for quantize point
            DBG("Reset armed, waiting for quantize point");
        } else {
            DBG("Executing immediate reset");
            executeReset();     // Immediate
        }
    }
    
    // Check for quantized reset
    if (resetArmed && isQuantizePoint(pos, resetQuantize)) {
        executeReset();
        resetArmed = false;
    }
    
    // Store last value before auto-reset
    lastResetValue = currentResetValue;
    
    // Auto-reset parameter for button behavior (after processing the trigger)
    // This keeps it momentary like a hardware button
    if (currentResetValue > 0.5f) {
        if (auto* resetParam = parameters.getParameter("reset")) {
            // Use beginChangeGesture/endChangeGesture for proper automation
            resetParam->beginChangeGesture();
            resetParam->setValueNotifyingHost(0.0f);
            resetParam->endChangeGesture();
        }
    }
    
    // Generate MIDI when playing, recording, OR in live mode
    if (!playing && !recording && !liveMode) {
        isPlaying = false;
        return;
    }
    
    // Reset on transport start or loop (but NOT if we just exited count-in)
    if (!justExitedCountIn && (!isPlaying || (ppq.hasValue() && *ppq < lastPpqPosition))) {
        gridsEngine.reset();
        sampleCounter = 0;
        currentPatternStep = 0;
        hasResetOffset = false;  // Clear any reset offset on transport restart
        ppqOffsetAtReset = 0.0;
    }
    isPlaying = playing || recording || liveMode;
    
    if (ppq.hasValue())
        lastPpqPosition = *ppq;
    
    // Update timing
    updateTiming(pos);
    
#ifdef ENABLE_MODULATION_MATRIX
    // Update modulation LFOs
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
        midiMessages.clear();  // Clear input if MIDI thru is disabled
    }
    // Otherwise preserve input MIDI and add our generated MIDI to it
    
    // Get current parameter values and apply modulation
#ifdef ENABLE_MODULATION_MATRIX
    // Apply modulation to parameters
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
    
    gridsEngine.setX(xValue);
    gridsEngine.setY(yValue);
    gridsEngine.setBDDensity(bdDensity);
    gridsEngine.setSDDensity(sdDensity);
    gridsEngine.setHHDensity(hhDensity);
    gridsEngine.setChaos(chaos);
    gridsEngine.setSwing(swing);
#else
    // Get current parameter values without modulation
    gridsEngine.setX(*parameters.getRawParameterValue("x"));
    gridsEngine.setY(*parameters.getRawParameterValue("y"));
    gridsEngine.setBDDensity(*parameters.getRawParameterValue("density_1_bd"));
    gridsEngine.setSDDensity(*parameters.getRawParameterValue("density_2_sd"));
    gridsEngine.setHHDensity(*parameters.getRawParameterValue("density_3_hh"));
    gridsEngine.setChaos(*parameters.getRawParameterValue("chaos"));
    gridsEngine.setSwing(*parameters.getRawParameterValue("swing"));
#endif
    
    // Get MIDI settings
    bdNote = *parameters.getRawParameterValue("note_1_bd");
    sdNote = *parameters.getRawParameterValue("note_2_sd");
    hhNote = *parameters.getRawParameterValue("note_3_hh");
    
#ifdef ENABLE_MODULATION_MATRIX
    // Apply MIDI note modulation
    float bdNoteModulation = modulationMatrix.getModulation(ModulationMatrix::BD_MIDI_NOTE);
    float sdNoteModulation = modulationMatrix.getModulation(ModulationMatrix::SD_MIDI_NOTE);
    float hhNoteModulation = modulationMatrix.getModulation(ModulationMatrix::HH_MIDI_NOTE);
    
    // Apply modulation to MIDI notes (± 12 semitones range)
    bdNote = juce::jlimit(0.0f, 127.0f, bdNote + (bdNoteModulation * 12.0f));
    sdNote = juce::jlimit(0.0f, 127.0f, sdNote + (sdNoteModulation * 12.0f));
    hhNote = juce::jlimit(0.0f, 127.0f, hhNote + (hhNoteModulation * 12.0f));
#endif
    
    midiChannel = *parameters.getRawParameterValue("midi_channel");
    
    // Process each sample in the buffer - PPQ-based sync
    int numSamples = buffer.getNumSamples();
    
    // Handle retrigger at the beginning of the buffer if needed
    if (shouldRetrigger) {
        // Evaluate drums at step 0 and trigger immediately
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
    
    // Use PPQ-based synchronization if available
    if (ppq.hasValue() && pos.getBpm().hasValue() && *pos.getBpm() > 0) {
        double currentPpq = *ppq;
        double bpm = *pos.getBpm();
        double samplesPerBeat = (currentSampleRate * 60.0) / bpm;
        double samplesPerSixteenth = samplesPerBeat / 4.0;
        
        // Calculate samples per PPQ unit
        double ppqPerSample = (bpm / 60.0) / currentSampleRate;
        
        for (int sample = 0; sample < numSamples; ++sample) {
            // Calculate the PPQ position for this sample
            double samplePpq = currentPpq + (sample * ppqPerSample);
            
            // Apply swing adjustment
            double swingValue = *parameters.getRawParameterValue("swing");
            double adjustedPpq = samplePpq;
            
            if (swingValue != 0.5f) {
                // Calculate position within a beat (0-1)
                double beatPosition = std::fmod(samplePpq, 1.0);
                
                // Apply swing to 2nd and 4th sixteenth notes (0.25 and 0.75 within beat)
                // Swing shifts odd 16th notes later (>0.5) or earlier (<0.5)
                double swingOffset = (swingValue - 0.5f) * 0.1;  // Max ±10% of a 16th note
                
                // Check if we're near an odd 16th note
                if ((beatPosition > 0.20 && beatPosition < 0.30) ||  // 2nd 16th
                    (beatPosition > 0.70 && beatPosition < 0.80)) {  // 4th 16th
                    adjustedPpq += swingOffset;
                }
            }
            
            // Calculate which 16th note step we should be on (32 steps = 2 bars = 8 beats)
            // If we have a reset offset, calculate relative to that point
            double ppqForStep = adjustedPpq;
            if (hasResetOffset) {
                ppqForStep = adjustedPpq - ppqOffsetAtReset;
                // Ensure positive value for step calculation
                while (ppqForStep < 0) ppqForStep += 8.0; // Add 2 bars worth of PPQ
            }
            
            // PPQ 0 = bar 1, beat 1 should map to step 0
            // PPQ 8 = bar 3, beat 1 should map to step 0 (wraps around after 2 bars)
            int targetStep = static_cast<int>(ppqForStep * 4.0) % 32;
            if (targetStep < 0) targetStep = 0;  // Handle negative PPQ during count-in
            
            
            // Check if we need to advance to the next step
            // Also force evaluation if we just exited count-in and are at step 0
            if (targetStep != currentPatternStep || (justExitedCountIn && targetStep == 0 && sample == 0)) {
                // Store previous trigger states
                bool prevBD = gridsEngine.getBDTrigger();
                bool prevSD = gridsEngine.getSDTrigger();
                bool prevHH = gridsEngine.getHHTrigger();
                
                // Advance to the target step
                // Set the GridsEngine to the correct step and evaluate
                gridsEngine.setCurrentStep(targetStep);
                gridsEngine.evaluateDrums();
                currentPatternStep = targetStep;
                
                // Generate note offs for previous triggers
                if (prevBD) addMidiNote(midiMessages, sample, bdNote, false, 0);
                if (prevSD) addMidiNote(midiMessages, sample, sdNote, false, 0);
                if (prevHH) addMidiNote(midiMessages, sample, hhNote, false, 0);
                
                // Generate note ons for new triggers
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
        // Fallback to sample counting if no PPQ available
        for (int sample = 0; sample < numSamples; ++sample) {
            // Check if we should tick the pattern
            if (samplesPerClock > 0 && ++sampleCounter >= samplesPerClock) {
                sampleCounter = 0;
                
                // Store previous trigger states
                bool prevBD = gridsEngine.getBDTrigger();
                bool prevSD = gridsEngine.getSDTrigger();
                bool prevHH = gridsEngine.getHHTrigger();
                
                // Advance pattern
                gridsEngine.tick();
                currentPatternStep = (currentPatternStep + 1) % 32;
                
                // Generate note offs for previous triggers
                if (prevBD) addMidiNote(midiMessages, sample, bdNote, false, 0);
                if (prevSD) addMidiNote(midiMessages, sample, sdNote, false, 0);
                if (prevHH) addMidiNote(midiMessages, sample, hhNote, false, 0);
                
                // Generate note ons for new triggers
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

void GridsAudioProcessor::updateTiming(const juce::AudioPlayHead::PositionInfo& posInfo)
{
    // Calculate samples per 16th note (Grids uses 32 steps = 2 bars)
    if (posInfo.getBpm().hasValue() && *posInfo.getBpm() > 0) {
        double bpm = *posInfo.getBpm();
        double beatsPerSecond = bpm / 60.0;
        double sixteenthsPerSecond = beatsPerSecond * 4.0;  // 4 sixteenths per beat
        double samplesPerSixteenth = currentSampleRate / sixteenthsPerSecond;
        
        // Grids advances once per 16th note
        samplesPerClock = static_cast<int>(samplesPerSixteenth);
    }
}

void GridsAudioProcessor::addMidiNote(juce::MidiBuffer& midiMessages, int sampleOffset,
                                      int noteNumber, bool noteOn, int velocity)
{
    juce::MidiMessage msg = noteOn 
        ? juce::MidiMessage::noteOn(midiChannel, noteNumber, (juce::uint8)velocity)
        : juce::MidiMessage::noteOff(midiChannel, noteNumber);
    
    midiMessages.addEvent(msg, sampleOffset);
}

int GridsAudioProcessor::calculateVelocity(bool isBD, bool isAccent, const juce::String& voice)
{
#ifdef ENABLE_VELOCITY_SYSTEM
    // Get velocity range parameter (0.0 = narrow range, 1.0 = wide range)
    float velocityRange = 0.5f;
    if (voice == "bd")
        velocityRange = *parameters.getRawParameterValue("velocity_1_bd");
    else if (voice == "sd")
        velocityRange = *parameters.getRawParameterValue("velocity_2_sd");
    else if (voice == "hh")
        velocityRange = *parameters.getRawParameterValue("velocity_3_hh");
    
#ifdef ENABLE_MODULATION_MATRIX
    // Apply velocity modulation
    float velocityModulation = 0.0f;
    if (voice == "bd")
        velocityModulation = modulationMatrix.getModulation(ModulationMatrix::BD_VELOCITY);
    else if (voice == "sd")
        velocityModulation = modulationMatrix.getModulation(ModulationMatrix::SD_VELOCITY);
    else if (voice == "hh")
        velocityModulation = modulationMatrix.getModulation(ModulationMatrix::HH_VELOCITY);
    
    // Apply modulation to velocity range
    velocityRange = juce::jlimit(0.0f, 1.0f, velocityRange + velocityModulation);
#endif
    
    // Calculate base velocities based on range
    // Range goes from narrow (80-100) to wide (40-127)
    int minVel = static_cast<int>(80 - (velocityRange * 40));  // 80 -> 40
    int maxVel = static_cast<int>(100 + (velocityRange * 27)); // 100 -> 127
    int normalVel = (minVel + maxVel) / 2;
    
    // Apply accent
    if (isAccent) {
        return maxVel;
    }
    
    // Add slight variation based on chaos parameter
    float chaos = *parameters.getRawParameterValue("chaos");
    if (chaos > 0.0f) {
        // Add random variation based on chaos (up to ±15% of range)
        int range = maxVel - minVel;
        int variation = static_cast<int>((juce::Random::getSystemRandom().nextFloat() - 0.5f) * range * chaos * 0.3f);
        normalVel = juce::jlimit(minVel, maxVel, normalVel + variation);
    }
    
    // BD typically louder than other drums
    if (isBD) {
        normalVel = juce::jmin(127, normalVel + 10);
    }
    
    return normalVel;
#else
    // Fallback to original fixed velocities
    if (isBD)
        return isAccent ? 127 : 100;
    else if (voice == "sd")
        return isAccent ? 127 : 90;
    else
        return isAccent ? 127 : 80;
#endif
}

bool GridsAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* GridsAudioProcessor::createEditor()
{
    DBG("createEditor() called - creating GridsPluginEditor");
    return new GridsPluginEditor(*this);
}

void GridsAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    
#ifdef ENABLE_MODULATION_MATRIX
    // Save modulation matrix state
    auto modTree = state.getOrCreateChildWithName("ModulationMatrix", nullptr);
    modulationMatrix.saveToValueTree(modTree);
#endif
    
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void GridsAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    
    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName (parameters.state.getType()))
        {
            auto newState = juce::ValueTree::fromXml (*xmlState);
            parameters.replaceState (newState);
            
#ifdef ENABLE_MODULATION_MATRIX
            // Restore modulation matrix state
            auto modTree = newState.getChildWithName("ModulationMatrix");
            if (modTree.isValid())
                modulationMatrix.loadFromValueTree(modTree);
#endif
        }
    }
}

#ifdef ENABLE_MODULATION_MATRIX
// Get modulated parameter values for UI display
float GridsAudioProcessor::getModulatedBDDensity()
{
    float baseValue = parameters.getRawParameterValue("density_1_bd")->load();
    return modulationMatrix.applyModulation(ModulationMatrix::BD_DENSITY, baseValue);
}

float GridsAudioProcessor::getModulatedSDDensity()
{
    float baseValue = parameters.getRawParameterValue("density_2_sd")->load();
    return modulationMatrix.applyModulation(ModulationMatrix::SD_DENSITY, baseValue);
}

float GridsAudioProcessor::getModulatedHHDensity()
{
    float baseValue = parameters.getRawParameterValue("density_3_hh")->load();
    return modulationMatrix.applyModulation(ModulationMatrix::HH_DENSITY, baseValue);
}

float GridsAudioProcessor::getModulatedChaos()
{
    float baseValue = parameters.getRawParameterValue("chaos")->load();
    return modulationMatrix.applyModulation(ModulationMatrix::CHAOS, baseValue);
}

float GridsAudioProcessor::getModulatedSwing()
{
    float baseValue = parameters.getRawParameterValue("swing")->load();
    return modulationMatrix.applyModulation(ModulationMatrix::SWING, baseValue);
}

float GridsAudioProcessor::getModulatedX()
{
    float baseValue = parameters.getRawParameterValue("x")->load();
    return modulationMatrix.applyModulation(ModulationMatrix::PATTERN_X, baseValue);
}

float GridsAudioProcessor::getModulatedY()
{
    float baseValue = parameters.getRawParameterValue("y")->load();
    return modulationMatrix.applyModulation(ModulationMatrix::PATTERN_Y, baseValue);
}

bool GridsAudioProcessor::isResetModulated()
{
    // Check if reset has modulation applied
    float modulation = modulationMatrix.getModulation(ModulationMatrix::PATTERN_RESET);
    return std::abs(modulation) > 0.5f; // Trigger when modulation crosses threshold
}

#ifdef ENABLE_VELOCITY_SYSTEM
float GridsAudioProcessor::getModulatedBDVelocity()
{
    float baseValue = parameters.getRawParameterValue("velocity_1_bd")->load();
    return modulationMatrix.applyModulation(ModulationMatrix::BD_VELOCITY, baseValue);
}

float GridsAudioProcessor::getModulatedSDVelocity()
{
    float baseValue = parameters.getRawParameterValue("velocity_2_sd")->load();
    return modulationMatrix.applyModulation(ModulationMatrix::SD_VELOCITY, baseValue);
}

float GridsAudioProcessor::getModulatedHHVelocity()
{
    float baseValue = parameters.getRawParameterValue("velocity_3_hh")->load();
    return modulationMatrix.applyModulation(ModulationMatrix::HH_VELOCITY, baseValue);
}
#endif
#endif

// This creates new instances of the plugin
void GridsAudioProcessor::executeReset()
{
    DBG("executeReset() called");
    gridsEngine.reset();  // Always reset position
    sampleCounter = 0;
    currentPatternStep = 0;  // Reset pattern step tracking
    
    // Store the current PPQ position as offset for proper pattern restart
    if (auto* playHead = getPlayHead()) {
        auto posOptional = playHead->getPosition();
        if (posOptional.hasValue()) {
            auto pos = *posOptional;
            if (pos.getPpqPosition().hasValue()) {
                ppqOffsetAtReset = *pos.getPpqPosition();
                hasResetOffset = true;
                DBG("Stored PPQ offset at reset: " << ppqOffsetAtReset);
            }
        }
    }
    
    // Check reset mode
    int resetMode = *parameters.getRawParameterValue("reset_mode");
    bool isRetrigger = (resetMode == 1);
    DBG("Reset mode: " << (isRetrigger ? "Retrigger" : "Transparent"));
    
    if (isRetrigger) {
        // Drill'n'bass mode - evaluate and fire triggers immediately
        shouldRetrigger = true;
    }
    
    // Notify UI of reset
    notifyReset(isRetrigger);
    DBG("Reset executed, pattern step: " << currentPatternStep);
}

bool GridsAudioProcessor::isQuantizePoint(const juce::AudioPlayHead::PositionInfo& posInfo, QuantizeValue quantize)
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
    
    // Check if we're at a quantize boundary
    double remainder = std::fmod(ppqPos, quantum);
    double prevRemainder = std::fmod(ppqPos - quantizePhase, quantum);
    
    quantizePhase = ppqPos;
    
    // Detect crossing the boundary
    return (prevRemainder > remainder) || (remainder < 0.01);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GridsAudioProcessor();
}