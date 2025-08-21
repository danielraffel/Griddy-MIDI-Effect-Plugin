#pragma once

#include <JuceHeader.h>

/**
 * SettingsManager - Singleton for managing global user preferences
 * 
 * Handles persistent global settings that apply to all sessions,
 * separate from session-specific state saved with DAW projects.
 * 
 * Settings are stored in:
 * - macOS: ~/Library/Application Support/Griddy/settings.json
 * - Windows: %APPDATA%/Griddy/settings.json
 * - Linux: ~/.config/Griddy/settings.json
 */
class SettingsManager
{
public:
    //==============================================================================
    // Singleton access
    static SettingsManager& getInstance()
    {
        static SettingsManager instance;
        return instance;
    }
    
    //==============================================================================
    // Initialization
    void initialise()
    {
        if (!initialized)
        {
            ensureSettingsFile();
            loadSettings();
            initialized = true;
        }
    }
    
    //==============================================================================
    // Global preference getters (with defaults)
    bool getBool(const juce::String& key, bool defaultValue) const
    {
        juce::ScopedLock lock(settingsLock);
        if (settings && settings->hasProperty(key))
            return settings->getProperty(key);
        return defaultValue;
    }
    
    int getInt(const juce::String& key, int defaultValue) const
    {
        juce::ScopedLock lock(settingsLock);
        if (settings && settings->hasProperty(key))
            return settings->getProperty(key);
        return defaultValue;
    }
    
    float getFloat(const juce::String& key, float defaultValue) const
    {
        juce::ScopedLock lock(settingsLock);
        if (settings && settings->hasProperty(key))
            return static_cast<float>(static_cast<double>(settings->getProperty(key)));
        return defaultValue;
    }
    
    juce::String getString(const juce::String& key, const juce::String& defaultValue) const
    {
        juce::ScopedLock lock(settingsLock);
        if (settings && settings->hasProperty(key))
            return settings->getProperty(key).toString();
        return defaultValue;
    }
    
    //==============================================================================
    // Global preference setters
    void setBool(const juce::String& key, bool value)
    {
        juce::ScopedLock lock(settingsLock);
        if (settings)
        {
            settings->setProperty(key, value);
            saveSettingsAsync();
        }
    }
    
    void setInt(const juce::String& key, int value)
    {
        juce::ScopedLock lock(settingsLock);
        if (settings)
        {
            settings->setProperty(key, value);
            saveSettingsAsync();
        }
    }
    
    void setFloat(const juce::String& key, float value)
    {
        juce::ScopedLock lock(settingsLock);
        if (settings)
        {
            settings->setProperty(key, value);
            saveSettingsAsync();
        }
    }
    
    void setString(const juce::String& key, const juce::String& value)
    {
        juce::ScopedLock lock(settingsLock);
        if (settings)
        {
            settings->setProperty(key, value);
            saveSettingsAsync();
        }
    }
    
    //==============================================================================
    // Reset all global settings to defaults
    void resetToDefaults()
    {
        juce::ScopedLock lock(settingsLock);
        createDefaultSettings();
        saveSettings();
    }
    
    // Save immediately (on app shutdown)
    void flush()
    {
        saveTimer.stopTimer();
        saveSettings();
    }
    
    //==============================================================================
    // Settings keys as constants for consistency
    struct Keys
    {
        // UI Preferences
        static constexpr const char* defaultResetMode = "defaultResetMode";
        static constexpr const char* defaultResetQuantize = "defaultResetQuantize";
        static constexpr const char* preferEuclideanMode = "preferEuclideanMode";
        static constexpr const char* showTooltips = "showTooltips";
        
        // MIDI Defaults
        static constexpr const char* defaultMidiChannel = "defaultMidiChannel";
        static constexpr const char* defaultBDNote = "defaultBDNote";
        static constexpr const char* defaultSDNote = "defaultSDNote";
        static constexpr const char* defaultHHNote = "defaultHHNote";
        static constexpr const char* midiThruDefault = "midiThruDefault";
        static constexpr const char* liveModeDefault = "liveModeDefault";
        
        // Advanced Defaults
        static constexpr const char* defaultGateMode = "defaultGateMode";
        static constexpr const char* euclideanBDLength = "euclideanBDLength";
        static constexpr const char* euclideanSDLength = "euclideanSDLength";
        static constexpr const char* euclideanHHLength = "euclideanHHLength";
        
        // Pattern Chain Defaults
        static constexpr const char* defaultTransitionMode = "defaultTransitionMode";
        static constexpr const char* defaultBarsPerPattern = "defaultBarsPerPattern";
    };
    
private:
    //==============================================================================
    SettingsManager() = default;
    ~SettingsManager() { flush(); }
    
    //==============================================================================
    juce::File getSettingsFile() const
    {
        #ifdef JUCE_MAC
            auto dir = juce::File::getSpecialLocation(
                juce::File::userApplicationDataDirectory)
                .getChildFile("Application Support/Griddy");
        #elif JUCE_WINDOWS
            auto dir = juce::File::getSpecialLocation(
                juce::File::userApplicationDataDirectory)
                .getChildFile("Griddy");
        #else  // Linux
            auto dir = juce::File::getSpecialLocation(
                juce::File::userHomeDirectory)
                .getChildFile(".config/Griddy");
        #endif
        
        dir.createDirectory();
        return dir.getChildFile("settings.json");
    }
    
    //==============================================================================
    void ensureSettingsFile()
    {
        auto file = getSettingsFile();
        if (!file.existsAsFile())
        {
            createDefaultSettings();
            saveSettings();
        }
    }
    
    void createDefaultSettings()
    {
        settings = new juce::DynamicObject();
        
        // UI Preferences (global)
        settings->setProperty(Keys::defaultResetMode, "transparent");
        settings->setProperty(Keys::defaultResetQuantize, "off");
        settings->setProperty(Keys::preferEuclideanMode, false);
        settings->setProperty(Keys::showTooltips, true);
        
        // MIDI Defaults (global)
        settings->setProperty(Keys::defaultMidiChannel, 1);
        settings->setProperty(Keys::defaultBDNote, 36);  // C1
        settings->setProperty(Keys::defaultSDNote, 38);  // D1
        settings->setProperty(Keys::defaultHHNote, 42);  // F#1
        settings->setProperty(Keys::midiThruDefault, true);
        settings->setProperty(Keys::liveModeDefault, false);
        
        // Advanced Defaults (global)
        settings->setProperty(Keys::defaultGateMode, false);
        settings->setProperty(Keys::euclideanBDLength, 16);
        settings->setProperty(Keys::euclideanSDLength, 12);
        settings->setProperty(Keys::euclideanHHLength, 8);
        
        // Pattern Chain Defaults
        settings->setProperty(Keys::defaultTransitionMode, "smooth");
        settings->setProperty(Keys::defaultBarsPerPattern, 4);
    }
    
    //==============================================================================
    void loadSettings()
    {
        auto file = getSettingsFile();
        if (file.existsAsFile())
        {
            auto json = file.loadFileAsString();
            auto parsed = juce::JSON::parse(json);
            if (parsed.isObject())
            {
                settings = parsed.getDynamicObject();
            }
            else
            {
                // File exists but is invalid, recreate
                createDefaultSettings();
                saveSettings();
            }
        }
        else
        {
            createDefaultSettings();
        }
    }
    
    void saveSettings()
    {
        if (!settings) return;
        
        auto file = getSettingsFile();
        auto json = juce::JSON::toString(settings.get(), true); // Pretty print
        
        if (!file.replaceWithText(json))
        {
            DBG("Failed to save settings to: " << file.getFullPathName());
        }
    }
    
    void saveSettingsAsync()
    {
        // Debounce saves using a timer to avoid excessive disk writes
        saveTimer.startTimer(500);
    }
    
    //==============================================================================
    // Timer for debounced saving
    class SaveTimer : public juce::Timer
    {
    public:
        SaveTimer(SettingsManager& owner) : manager(owner) {}
        
        void timerCallback() override
        {
            stopTimer();
            manager.saveSettings();
        }
        
    private:
        SettingsManager& manager;
    };
    
    //==============================================================================
    juce::DynamicObject::Ptr settings;
    bool initialized = false;
    mutable juce::CriticalSection settingsLock;
    SaveTimer saveTimer{*this};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsManager)
};