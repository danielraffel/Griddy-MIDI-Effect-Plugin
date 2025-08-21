#pragma once

#include <JuceHeader.h>
#include "LFO.h"
#include <vector>

#ifdef ENABLE_MODULATION_MATRIX

/**
 * ModulationMatrix - Routes LFO sources to parameter destinations
 * 
 * Manages multiple LFOs and their routing to various parameters.
 * Supports bipolar and unipolar modulation with adjustable amounts.
 */
class ModulationMatrix
{
public:
    // Available modulation destinations
    enum Destination
    {
        PATTERN_X,
        PATTERN_Y,
        CHAOS,
        SWING,
        PATTERN_RESET,
        BD_DENSITY,
        SD_DENSITY,
        HH_DENSITY,
        BD_VELOCITY,
        SD_VELOCITY,
        HH_VELOCITY,
        BD_MIDI_NOTE,
        SD_MIDI_NOTE,
        HH_MIDI_NOTE,
        NUM_DESTINATIONS
    };
    
    // Modulation routing configuration
    struct Routing
    {
        int sourceId;           // LFO index (0 or 1)
        Destination dest;       // Target parameter
        float amount;          // Modulation amount (-1 to +1)
        bool bipolar;          // True for bipolar (-1 to +1), false for unipolar (0 to 1)
        bool enabled;          // Is this routing active?
        
        Routing() : sourceId(0), dest(PATTERN_X), amount(0.0f), bipolar(true), enabled(false) {}
        
        Routing(int source, Destination d, float amt, bool bi = true)
            : sourceId(source), dest(d), amount(amt), bipolar(bi), enabled(true) {}
    };
    
    ModulationMatrix()
    {
        // Initialize with empty routings for each destination
        routings_.resize(NUM_DESTINATIONS);
    }
    
    // Update LFOs (called from audio thread)
    void processBlock(double samplesPerBeat, int numSamples)
    {
        lfos_[0].advance(samplesPerBeat, numSamples);
        lfos_[1].advance(samplesPerBeat, numSamples);
    }
    
    // Add or update a routing
    void setRouting(int lfoId, Destination dest, float amount, bool bipolar = true)
    {
        if (lfoId < 0 || lfoId >= 2) return;
        if (dest < 0 || dest >= NUM_DESTINATIONS) return;
        
        routings_[dest] = Routing(lfoId, dest, amount, bipolar);
    }
    
    // Remove a routing
    void clearRouting(Destination dest)
    {
        if (dest < 0 || dest >= NUM_DESTINATIONS) return;
        routings_[dest].enabled = false;
    }
    
    // Get modulation value for a destination
    float getModulation(Destination dest) const
    {
        if (dest < 0 || dest >= NUM_DESTINATIONS) return 0.0f;
        
        const auto& routing = routings_[dest];
        if (!routing.enabled) return 0.0f;
        
        if (routing.sourceId < 0 || routing.sourceId >= 2) return 0.0f;
        
        const auto& lfo = lfos_[routing.sourceId];
        if (!lfo.isEnabled()) return 0.0f;
        
        float value = routing.bipolar ? lfo.getValue() : lfo.getUnipolarValue();
        return value * routing.amount;
    }
    
    // Apply modulation to a parameter value
    float applyModulation(Destination dest, float baseValue) const
    {
        float modulation = getModulation(dest);
        return juce::jlimit(0.0f, 1.0f, baseValue + modulation);
    }
    
    // Get LFO references for configuration
    LFO& getLFO(int index)
    {
        jassert(index >= 0 && index < 2);
        return lfos_[index];
    }
    
    const LFO& getLFO(int index) const
    {
        jassert(index >= 0 && index < 2);
        return lfos_[index];
    }
    
    // Get routing for a destination
    const Routing& getRouting(Destination dest) const
    {
        static Routing emptyRouting;
        if (dest < 0 || dest >= NUM_DESTINATIONS) return emptyRouting;
        return routings_[dest];
    }
    
    // Reset all LFOs to phase 0
    void reset()
    {
        lfos_[0].reset();
        lfos_[1].reset();
    }
    
    // Clear all routings
    void clearAllRoutings()
    {
        for (auto& routing : routings_)
        {
            routing.enabled = false;
        }
    }
    
    // Save/restore state
    void saveToValueTree(juce::ValueTree& tree) const
    {
        // Save LFO states
        auto lfo1Tree = tree.getOrCreateChildWithName("LFO1", nullptr);
        lfos_[0].saveToValueTree(lfo1Tree);
        
        auto lfo2Tree = tree.getOrCreateChildWithName("LFO2", nullptr);
        lfos_[1].saveToValueTree(lfo2Tree);
        
        // Save routings
        auto routingsTree = tree.getOrCreateChildWithName("Routings", nullptr);
        routingsTree.removeAllChildren(nullptr);
        
        for (int i = 0; i < NUM_DESTINATIONS; ++i)
        {
            const auto& routing = routings_[i];
            if (routing.enabled)
            {
                auto routingTree = juce::ValueTree("Routing");
                routingsTree.appendChild(routingTree, nullptr);
                routingTree.setProperty("sourceId", routing.sourceId, nullptr);
                routingTree.setProperty("destination", i, nullptr);
                routingTree.setProperty("amount", routing.amount, nullptr);
                routingTree.setProperty("bipolar", routing.bipolar, nullptr);
                routingTree.setProperty("enabled", routing.enabled, nullptr);
            }
        }
    }
    
    void loadFromValueTree(const juce::ValueTree& tree)
    {
        // Load LFO states
        auto lfo1Tree = tree.getChildWithName("LFO1");
        if (lfo1Tree.isValid())
            lfos_[0].loadFromValueTree(lfo1Tree);
        
        auto lfo2Tree = tree.getChildWithName("LFO2");
        if (lfo2Tree.isValid())
            lfos_[1].loadFromValueTree(lfo2Tree);
        
        // Clear and load routings
        clearAllRoutings();
        
        auto routingsTree = tree.getChildWithName("Routings");
        if (routingsTree.isValid())
        {
            for (int i = 0; i < routingsTree.getNumChildren(); ++i)
            {
                auto routingTree = routingsTree.getChild(i);
                int destIndex = routingTree.getProperty("destination", -1);
                
                if (destIndex >= 0 && destIndex < NUM_DESTINATIONS)
                {
                    routings_[destIndex].sourceId = routingTree.getProperty("sourceId", 0);
                    routings_[destIndex].dest = static_cast<Destination>(destIndex);
                    routings_[destIndex].amount = routingTree.getProperty("amount", 0.0f);
                    routings_[destIndex].bipolar = routingTree.getProperty("bipolar", true);
                    routings_[destIndex].enabled = routingTree.getProperty("enabled", false);
                }
            }
        }
    }
    
    // Get destination name for UI
    static juce::String getDestinationName(Destination dest)
    {
        switch (dest)
        {
            case PATTERN_X:      return "Pattern X";
            case PATTERN_Y:      return "Pattern Y";
            case CHAOS:          return "Chaos";
            case SWING:          return "Swing";
            case PATTERN_RESET:  return "Reset";
            case BD_DENSITY:     return "BD Density";
            case SD_DENSITY:     return "SD Density";
            case HH_DENSITY:     return "HH Density";
            case BD_VELOCITY:    return "BD Velocity";
            case SD_VELOCITY:    return "SD Velocity";
            case HH_VELOCITY:    return "HH Velocity";
            case BD_MIDI_NOTE:   return "BD MIDI Note";
            case SD_MIDI_NOTE:   return "SD MIDI Note";
            case HH_MIDI_NOTE:   return "HH MIDI Note";
            default:             return "Unknown";
        }
    }
    
private:
    LFO lfos_[2];                              // Two LFOs
    std::vector<Routing> routings_;           // Routing configuration per destination
};

#endif // ENABLE_MODULATION_MATRIX