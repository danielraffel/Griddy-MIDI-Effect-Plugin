// Non-macOS implementation — no-op since there's no macOS app menu
#if !JUCE_MAC

namespace SettingsMenuHelper {
    void setSettingsKeyEquivalent() {}
}

#endif
