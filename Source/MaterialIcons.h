#pragma once

#include <JuceHeader.h>

namespace MaterialIcons {
    /**
     * Material Design Icons Unicode constants
     * These correspond to Material Symbols Outlined font
     */
    
    // Navigation & Controls
    static constexpr const char* PLAY_ARROW = "\ue037";
    static constexpr const char* PAUSE = "\ue034";
    static constexpr const char* STOP = "\ue047";
    static constexpr const char* REPLAY = "\ue042";
    static constexpr const char* SKIP_NEXT = "\ue044";
    static constexpr const char* SKIP_PREVIOUS = "\ue045";
    static constexpr const char* FAST_FORWARD = "\ue01f";
    static constexpr const char* FAST_REWIND = "\ue020";
    
    // Volume & Audio
    static constexpr const char* VOLUME_UP = "\ue050";
    static constexpr const char* VOLUME_DOWN = "\ue04d";
    static constexpr const char* VOLUME_MUTE = "\ue04e";
    static constexpr const char* VOLUME_OFF = "\ue04f";
    static constexpr const char* MIC = "\ue029";
    static constexpr const char* MIC_OFF = "\ue02a";
    static constexpr const char* HEADSET = "\ue026";
    static constexpr const char* SPEAKER = "\ue046";
    
    // Settings & Controls
    static constexpr const char* SETTINGS = "\ue8b8";
    static constexpr const char* TUNE = "\ue429";
    static constexpr const char* EQUALIZER = "\ue01d";
    static constexpr const char* SLIDERS = "\ue429"; // Alternative name for tune
    static constexpr const char* RADIO_BUTTON_UNCHECKED = "\ue836";
    static constexpr const char* RADIO_BUTTON_CHECKED = "\ue837";
    static constexpr const char* TOGGLE_ON = "\ue9f7";
    static constexpr const char* TOGGLE_OFF = "\ue9f8";
    
    // Navigation
    static constexpr const char* MENU = "\ue5d2";
    static constexpr const char* CLOSE = "\ue5cd";
    static constexpr const char* ARROW_BACK = "\ue5c4";
    static constexpr const char* ARROW_FORWARD = "\ue5c8";
    static constexpr const char* ARROW_UPWARD = "\ue5d8";
    static constexpr const char* ARROW_DOWNWARD = "\ue5db";
    static constexpr const char* EXPAND_MORE = "\ue5cf";
    static constexpr const char* EXPAND_LESS = "\ue5ce";
    static constexpr const char* CHEVRON_LEFT = "\ue5cb";
    static constexpr const char* CHEVRON_RIGHT = "\ue5cc";
    
    // File & Actions
    static constexpr const char* SAVE = "\ue161";
    static constexpr const char* FOLDER_OPEN = "\ue2c8";
    static constexpr const char* FILE_DOWNLOAD = "\ue2c4";
    static constexpr const char* FILE_UPLOAD = "\ue2c6";
    static constexpr const char* ADD = "\ue145";
    static constexpr const char* REMOVE = "\ue15b";
    static constexpr const char* EDIT = "\ue3c9";
    static constexpr const char* DELETE = "\ue872";
    static constexpr const char* COPY = "\ue14d";
    static constexpr const char* REFRESH = "\ue5d5";
    
    // Status & Feedback
    static constexpr const char* CHECK = "\ue5ca";
    static constexpr const char* CHECK_CIRCLE = "\ue2e0";
    static constexpr const char* ERROR = "\ue000";
    static constexpr const char* WARNING = "\ue002";
    static constexpr const char* INFO = "\ue88e";
    static constexpr const char* HELP = "\ue887";
    
    // Music & Media
    static constexpr const char* MUSIC_NOTE = "\ue405";
    static constexpr const char* ALBUM = "\ue019";
    static constexpr const char* PLAYLIST_ADD = "\ue03b";
    static constexpr const char* QUEUE_MUSIC = "\ue03d";
    static constexpr const char* SHUFFLE = "\ue043";
    static constexpr const char* REPEAT = "\ue040";
    static constexpr const char* REPEAT_ONE = "\ue041";
    
    // Grid & Layout (useful for Griddy)
    static constexpr const char* GRID_VIEW = "\ue8f0";
    static constexpr const char* GRID_ON = "\ue8e6";
    static constexpr const char* GRID_OFF = "\ue8e5";
    static constexpr const char* APPS = "\ue5c3";
    static constexpr const char* DASHBOARD = "\ue871";
    static constexpr const char* VIEW_MODULE = "\ue8f1";
    
    // Utility
    static constexpr const char* VISIBILITY = "\ue8f4";
    static constexpr const char* VISIBILITY_OFF = "\ue8f5";
    static constexpr const char* LOCK = "\ue897";
    static constexpr const char* LOCK_OPEN = "\ue898";
    static constexpr const char* SYNC = "\ue627";
    static constexpr const char* SYNC_DISABLED = "\ue628";
    
    /**
     * Helper function to create a JUCE Font object for Material Icons
     * This creates a font from the embedded Material Symbols font data
     * @param size Font size in points
     * @return JUCE Font configured for Material Symbols
     */
    inline juce::Font getJuceMaterialIconsFont(float size) {
        // Try to load the embedded font data (will be available after proper setup)
        static juce::Typeface::Ptr materialTypeface;
        
        if (!materialTypeface) {
            // For now, load the font from the copied TTF file in Resources
            // This will work until we get the Visage embedding working
            juce::File fontFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
                                      .getParentDirectory()
                                      .getChildFile("Resources")
                                      .getChildFile("Fonts")
                                      .getChildFile("Material_Symbols_Outlined")
                                      .getChildFile("static")
                                      .getChildFile("MaterialSymbolsOutlined-Regular.ttf");
            
            if (fontFile.existsAsFile()) {
                juce::MemoryBlock fontData;
                if (fontFile.loadFileAsData(fontData)) {
                    materialTypeface = juce::Typeface::createSystemTypefaceFor(
                        fontData.getData(), fontData.getSize());
                }
            }
            
            // Fallback to system font if Material Symbols not available
            if (!materialTypeface) {
                DBG("Material Symbols font not found, using system font");
                return juce::Font("Arial", size, juce::Font::plain);
            }
        }
        
        return juce::Font(materialTypeface).withHeight(size);
    }
    
    /**
     * Alternative approach: Create font by loading from file path
     * @param fontPath Path to the Material Symbols TTF file
     * @param size Font size in points
     * @return JUCE Font configured for Material Symbols
     */
    inline juce::Font createMaterialIconsFontFromFile(const juce::String& fontPath, float size) {
        juce::File fontFile(fontPath);
        if (!fontFile.existsAsFile()) {
            DBG("Font file not found: " << fontPath);
            return juce::Font("Arial", size, juce::Font::plain);
        }
        
        juce::MemoryBlock fontData;
        if (!fontFile.loadFileAsData(fontData)) {
            DBG("Failed to load font data from: " << fontPath);
            return juce::Font("Arial", size, juce::Font::plain);
        }
        
        auto typeface = juce::Typeface::createSystemTypefaceFor(fontData.getData(), fontData.getSize());
        if (!typeface) {
            DBG("Failed to create typeface from font data");
            return juce::Font("Arial", size, juce::Font::plain);
        }
        
        return juce::Font(typeface).withHeight(size);
    }
}