#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Grids/GridsEngine.h"

#ifdef ENABLE_MODULATION_MATRIX
#include "Modulation/ModulationMatrix.h"
#endif

// Reset quantization options
enum QuantizeValue {
    QUANTIZE_OFF = 0,     // Immediate (hardware behavior)
    QUANTIZE_2_BAR,       // 2 bars
    QUANTIZE_1_BAR,       // 1 bar
    QUANTIZE_1_2,         // 1/2 note
    QUANTIZE_1_4,         // 1/4 note (beat)
    QUANTIZE_1_8,         // 1/8 note
    QUANTIZE_1_16,        // 1/16 note
    QUANTIZE_1_32,        // 1/32 note
    QUANTIZE_1_4T,        // 1/4 triplet
    QUANTIZE_1_8T,        // 1/8 triplet
    QUANTIZE_1_16T        // 1/16 triplet
};

class GridsAudioProcessor : public juce::AudioProcessor
{
public:
    GridsAudioProcessor();
    ~GridsAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    bool supportsDoublePrecisionProcessing() const override { return false; }

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    // Parameters
    juce::AudioProcessorValueTreeState parameters;
    
    // Get the Grids engine for UI access
    GridsEngine& getGridsEngine() { return gridsEngine; }
    
#ifdef ENABLE_MODULATION_MATRIX
    // Get the modulation matrix for UI access
    ModulationMatrix& getModulationMatrix() { return modulationMatrix; }
#endif
    
    // Check if reset occurred (for UI feedback)
    bool hasResetOccurred() { 
        bool occurred = resetOccurred;
        resetOccurred = false;
        return occurred;
    }
    bool wasResetRetrigger() { return wasRetrigger; }
    
    // Notify reset occurred (for UI feedback)
    void notifyReset(bool isRetrigger) { resetOccurred = true; wasRetrigger = isRetrigger; }
    
    // Set/get reset quantization
    void setResetQuantize(QuantizeValue value) { resetQuantize = value; }
    QuantizeValue getResetQuantize() const { return resetQuantize; }
    
    // MIDI learn for reset
    void startMidiLearnForReset() { midiLearnActive = true; }
    void stopMidiLearn() { midiLearnActive = false; }
    bool isMidiLearning() const { return midiLearnActive; }
    int getResetMidiCC() const { return resetMidiCC; }
    void setResetMidiCC(int cc) { resetMidiCC = cc; }
    
#ifdef ENABLE_MODULATION_MATRIX
    // Get modulated parameter values for UI display
    float getModulatedBDDensity();
    float getModulatedSDDensity();
    float getModulatedHHDensity();
    float getModulatedChaos();
    float getModulatedSwing();
    float getModulatedX();
    float getModulatedY();
    bool isResetModulated();
#ifdef ENABLE_VELOCITY_SYSTEM
    float getModulatedBDVelocity();
    float getModulatedSDVelocity();
    float getModulatedHHVelocity();
#endif
#endif

private:
    // Create parameter layout
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // Grids pattern engine
    GridsEngine gridsEngine;
    
#ifdef ENABLE_MODULATION_MATRIX
    // Modulation matrix for LFO routing
    ModulationMatrix modulationMatrix;
#endif
    
    // Timing
    double currentSampleRate = 44100.0;
    int samplesPerClock = 0;
    int sampleCounter = 0;
    double lastPpqPosition = 0.0;
    bool isPlaying = false;
    
    // Reset handling
    float lastResetValue = 0.0f;
    bool shouldRetrigger = false;
    bool resetOccurred = false;
    bool wasRetrigger = false;
    bool resetArmed = false;
    QuantizeValue resetQuantize = QUANTIZE_OFF;
    double quantizePhase = 0.0;
    
    // MIDI note numbers
    int bdNote = 36;  // C1
    int sdNote = 38;  // D1
    int hhNote = 42;  // F#1
    int midiChannel = 1;
    
    // MIDI learn
    bool midiLearnActive = false;
    int resetMidiCC = -1;  // -1 means no CC assigned
    float lastCCValue = 0.0f;
    
    // Count-in and sync tracking
    bool wasInCountIn = false;
    int currentPatternStep = 0;  // Track which step we're on (0-31)
    double ppqOffsetAtReset = 0.0;  // PPQ position when reset was triggered
    bool hasResetOffset = false;  // Whether we have an active reset offset
    
    // Calculate samples per clock based on tempo
    void updateTiming(const juce::AudioPlayHead::PositionInfo& posInfo);
    
    // Check if we're at a quantize point
    bool isQuantizePoint(const juce::AudioPlayHead::PositionInfo& posInfo, QuantizeValue quantize);
    
    // Execute the reset
    void executeReset();
    
    // Generate MIDI note
    void addMidiNote(juce::MidiBuffer& midiMessages, int sampleOffset, 
                     int noteNumber, bool noteOn, int velocity);
    
    // Calculate velocity based on accent and velocity range settings
    int calculateVelocity(bool isBD, bool isAccent, const juce::String& voice);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GridsAudioProcessor)
};