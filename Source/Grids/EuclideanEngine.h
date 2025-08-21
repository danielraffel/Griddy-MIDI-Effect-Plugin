#pragma once

#include <JuceHeader.h>
#include "EuclideanTables.h"
#include "../Settings/SettingsManager.h"

#ifdef ENABLE_EUCLIDEAN_MODE

/**
 * EuclideanEngine - Generates Euclidean rhythms for each drum voice
 * 
 * Uses pre-computed lookup tables matching Grids' implementation.
 * Each instrument can have different cycle lengths for polyrhythmic patterns.
 */
class EuclideanEngine
{
public:
    EuclideanEngine() {
        loadDefaults();
    }
    
    // Reset all step positions to beginning
    void reset() {
        step_[0] = step_[1] = step_[2] = 0;
    }
    
    // Advance all sequences by one step
    void tick() {
        for (int i = 0; i < 3; ++i) {
            if (++step_[i] >= length_[i]) {
                step_[i] = 0;
            }
        }
    }
    
    // Check if an instrument should trigger based on density
    bool shouldTrigger(int instrument, float density) {
        if (instrument < 0 || instrument >= 3) return false;
        
        // Convert density (0-1) to number of hits in the pattern
        int hits = static_cast<int>(density * length_[instrument] + 0.5f);
        
        // Get the pattern for this length/hits combination
        uint32_t pattern = EuclideanTables::getPattern(length_[instrument], hits);
        
        // Check if current step should trigger
        return EuclideanTables::shouldTrigger(pattern, step_[instrument]);
    }
    
    // Set the cycle length for an instrument
    void setLength(int instrument, uint8_t length) {
        if (instrument < 0 || instrument >= 3) return;
        
        length_[instrument] = juce::jlimit(uint8_t(1), uint8_t(32), length);
        
        // Reset step position if it's beyond new length
        if (step_[instrument] >= length_[instrument]) {
            step_[instrument] = 0;
        }
    }
    
    // Get the cycle length for an instrument
    uint8_t getLength(int instrument) const {
        if (instrument < 0 || instrument >= 3) return 16;
        return length_[instrument];
    }
    
    // Get current step position for an instrument
    uint8_t getStep(int instrument) const {
        if (instrument < 0 || instrument >= 3) return 0;
        return step_[instrument];
    }
    
    // Load default lengths from global settings
    void loadDefaults() {
        auto& settings = SettingsManager::getInstance();
        length_[0] = static_cast<uint8_t>(settings.getInt(SettingsManager::Keys::euclideanBDLength, 16));
        length_[1] = static_cast<uint8_t>(settings.getInt(SettingsManager::Keys::euclideanSDLength, 12));
        length_[2] = static_cast<uint8_t>(settings.getInt(SettingsManager::Keys::euclideanHHLength, 8));
    }
    
    // Save current lengths to global settings
    void saveToSettings() {
        auto& settings = SettingsManager::getInstance();
        settings.setInt(SettingsManager::Keys::euclideanBDLength, length_[0]);
        settings.setInt(SettingsManager::Keys::euclideanSDLength, length_[1]);
        settings.setInt(SettingsManager::Keys::euclideanHHLength, length_[2]);
    }
    
    // Get pattern visualization for UI
    uint32_t getPattern(int instrument, float density) const {
        if (instrument < 0 || instrument >= 3) return 0;
        
        int hits = static_cast<int>(density * length_[instrument] + 0.5f);
        return EuclideanTables::getPattern(length_[instrument], hits);
    }
    
private:
    uint8_t step_[3] = {0, 0, 0};         // Current step for each instrument
    uint8_t length_[3] = {16, 12, 8};     // Cycle length for each instrument
};

#endif // ENABLE_EUCLIDEAN_MODE