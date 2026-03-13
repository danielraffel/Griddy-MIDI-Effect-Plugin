#pragma once

#include <visage_ui/frame.h>
#include <visage_graphics/canvas.h>
#include <visage_graphics/font.h>
#include <functional>
#include <string>
#include <cmath>
#include <array>

namespace visage::fonts {
    extern ::visage::EmbeddedFile Lato_Regular_ttf;
}

class SettingsPanelFrame : public visage::Frame {
public:
    SettingsPanelFrame() : Frame("SettingsPanel") {}

    // --- Callbacks ---
    std::function<void(int)> onMidiChannelChange;
    std::function<void(int)> onBDNoteChange;
    std::function<void(int)> onSDNoteChange;
    std::function<void(int)> onHHNoteChange;
    std::function<void(bool)> onMidiThruChange;
    std::function<void(bool)> onLiveModeChange;
    std::function<void(int)> onResetModeChange;
    std::function<void()> onOpenAcknowledgements;
    std::function<void()> onMidiLearnStart;
    std::function<void()> onMidiLearnStop;
    // Display toggles
    std::function<void(bool)> onShowNotesOnMainChange;
    std::function<void(bool)> onShowQuantizeOnMainChange;
    // Advanced tab
    std::function<void(int)> onResetQuantizeChange;
    std::function<void(bool)> onEuclideanEnableChange;
    std::function<void(int, int)> onEuclideanLengthChange; // (instrument, length)
    // Modulation tab
    std::function<void(int, bool)> onLFOEnableChange;       // (lfoIdx, enabled)
    std::function<void(int, int)> onLFOShapeChange;          // (lfoIdx, shape)
    std::function<void(int, float)> onLFORateChange;         // (lfoIdx, rate)
    std::function<void(int, float)> onLFODepthChange;        // (lfoIdx, depth)
    std::function<void(int, int, bool)> onLFODestChange;     // (lfoIdx, destIdx, enabled)
    std::function<void(int, int, bool)> onLFODestBipolarChange; // (lfoIdx, destIdx, bipolar)

    // --- Setters ---
    void setMidiChannel(int ch) { midiChannel_ = std::max(1, std::min(16, ch)); redraw(); }
    void setBDNote(int n) { bdNote_ = std::max(0, std::min(127, n)); redraw(); }
    void setSDNote(int n) { sdNote_ = std::max(0, std::min(127, n)); redraw(); }
    void setHHNote(int n) { hhNote_ = std::max(0, std::min(127, n)); redraw(); }
    void setMidiThru(bool v) { midiThru_ = v; redraw(); }
    void setLiveMode(bool v) { liveMode_ = v; redraw(); }
    void setResetMode(int m) { resetMode_ = m; redraw(); }
    void setMidiLearnActive(bool v) { midiLearnActive_ = v; redraw(); }
    void setResetMidiCC(int cc) { resetMidiCC_ = cc; redraw(); }
    void setShowNotesOnMain(bool v) { showNotesOnMain_ = v; redraw(); }
    bool getShowNotesOnMain() const { return showNotesOnMain_; }
    void setShowQuantizeOnMain(bool v) { showQuantizeOnMain_ = v; redraw(); }
    bool getShowQuantizeOnMain() const { return showQuantizeOnMain_; }
    void setResetQuantize(int q) { resetQuantize_ = q; redraw(); }
    void setEuclideanEnabled(bool v) { euclideanEnabled_ = v; redraw(); }
    void setEuclideanLength(int inst, int len) {
        if (inst >= 0 && inst < 3) euclideanLengths_[inst] = std::max(1, std::min(32, len));
        redraw();
    }
    void setLFOEnabled(int idx, bool v) { if (idx >= 0 && idx < 2) lfoEnabled_[idx] = v; redraw(); }
    void setLFOShape(int idx, int s) { if (idx >= 0 && idx < 2) lfoShape_[idx] = s; redraw(); }
    void setLFORate(int idx, float r) { if (idx >= 0 && idx < 2) lfoRate_[idx] = r; redraw(); }
    void setLFODepth(int idx, float d) { if (idx >= 0 && idx < 2) lfoDepth_[idx] = d; redraw(); }
    void setLFODest(int lfo, int dest, bool on) {
        if (lfo >= 0 && lfo < 2 && dest >= 0 && dest < 14)
            lfoDest_[lfo][dest] = on;
        redraw();
    }

    void show() { Frame::setVisible(true); redraw(); }
    void hide() { Frame::setVisible(false); redraw(); }
    void toggleVisible() { if (Frame::isVisible()) hide(); else show(); }

    enum Tab { TAB_GENERAL = 0, TAB_ADVANCED, TAB_MIDI, TAB_MODULATION, TAB_COUNT };

    void draw(visage::Canvas& canvas) override {
        if (!Frame::isVisible()) return;

        float w = width();
        float h = height();
        visage::Font font(11.0f, visage::fonts::Lato_Regular_ttf);
        visage::Font fontSmall(9.0f, visage::fonts::Lato_Regular_ttf);
        visage::Font fontTitle(13.0f, visage::fonts::Lato_Regular_ttf);
        visage::Font fontHeader(16.0f, visage::fonts::Lato_Regular_ttf);

        // Backdrop
        canvas.setColor(0xcc000000);
        canvas.fill(0, 0, w, h);

        // Panel — clamp to fit within available frame height
        float panelW = 540.0f;
        float panelH = std::min(360.0f, h - 10.0f);
        float panelX = (w - panelW) / 2.0f;
        float panelY = std::max(5.0f, (h - panelH) / 2.0f);

        canvas.setColor(0xff2a2a2a);
        canvas.roundedRectangle(panelX, panelY, panelW, panelH, 8.0f);
        canvas.setColor(0xff444444);
        canvas.roundedRectangleBorder(panelX, panelY, panelW, panelH, 8.0f, 1.0f);

        // Header
        float headerH = 32.0f;
        canvas.setColor(0xffff8833);
        canvas.text("Settings", fontHeader, visage::Font::kCenter,
                    panelX, panelY + 8, panelW, headerH - 8);

        // Close button
        float closeBtnSize = 22.0f;
        float closeBtnX = panelX + panelW - closeBtnSize - 10;
        float closeBtnY = panelY + (headerH - closeBtnSize) / 2.0f;
        bool closeHovered = isHoveredRect(closeBtnX, closeBtnY, closeBtnSize, closeBtnSize);
        bool closePressed = isPressedRect(closeBtnX, closeBtnY, closeBtnSize, closeBtnSize);
        if (closeHovered || closePressed) {
            canvas.setColor(closePressed ? 0xff3a3a3a : 0xff333333);
            canvas.roundedRectangle(closeBtnX, closeBtnY, closeBtnSize, closeBtnSize, 4.0f);
        }
        canvas.setColor(closePressed ? 0xffff8833 : (closeHovered ? 0xffdddddd : 0xff888888));
        canvas.text("X", font, visage::Font::kCenter,
                    closeBtnX, closeBtnY, closeBtnSize, closeBtnSize);

        // Tab bar
        float tabBarY = panelY + headerH;
        float tabH = 28.0f;
        float tabW = panelW / TAB_COUNT;
        const char* tabNames[] = { "General", "Advanced", "MIDI", "Modulation" };
        for (int i = 0; i < TAB_COUNT; i++) {
            float tx = panelX + i * tabW;
            bool tabHovered = isHoveredRect(tx, tabBarY, tabW, tabH);
            bool tabPressed = isPressedRect(tx, tabBarY, tabW, tabH);
            if (i == activeTab_) {
                canvas.setColor(tabPressed ? 0xff4a4a4a : (tabHovered ? 0xff444444 : 0xff3a3a3a));
                canvas.fill(tx, tabBarY, tabW, tabH);
                canvas.setColor(0xffff8833);
                canvas.fill(tx, tabBarY + tabH - 2, tabW, 2);
            } else if (tabHovered || tabPressed) {
                canvas.setColor(tabPressed ? 0xff3a3a3a : 0xff303030);
                canvas.fill(tx, tabBarY, tabW, tabH);
            }
            canvas.setColor(i == activeTab_
                                ? 0xffffffff
                                : (tabHovered ? 0xffcccccc : 0xff888888));
            canvas.text(tabNames[i], font, visage::Font::kCenter, tx, tabBarY + 2, tabW, tabH - 4);
        }

        float contentX = panelX + 16.0f;
        float contentY = tabBarY + tabH + 8.0f;
        float contentW = panelW - 32.0f;
        float rowH = 24.0f;

        switch (activeTab_) {
            case TAB_GENERAL:    drawGeneralTab(canvas, font, fontSmall, fontTitle, contentX, contentY, contentW, rowH); break;
            case TAB_ADVANCED:   drawAdvancedTab(canvas, font, fontSmall, fontTitle, contentX, contentY, contentW, rowH); break;
            case TAB_MIDI:       drawMidiTab(canvas, font, fontSmall, fontTitle, contentX, contentY, contentW, rowH); break;
            case TAB_MODULATION: drawModulationTab(canvas, font, fontSmall, fontTitle, contentX, contentY, contentW, rowH); break;
            default: break;
        }
    }

    void mouseDown(const visage::MouseEvent& e) override {
        if (!Frame::isVisible()) return;

        mouseInside_ = true;
        mouseX_ = e.position.x;
        mouseY_ = e.position.y;
        mouseDownX_ = e.position.x;
        mouseDownY_ = e.position.y;
        mouseIsDown_ = true;
        redraw();

        float w = width(), h = height();
        float panelW = 540.0f, panelH = std::min(360.0f, h - 10.0f);
        float panelX = (w - panelW) / 2.0f, panelY = std::max(5.0f, (h - panelH) / 2.0f);
        float mx = e.position.x, my = e.position.y;

        // Close button
        float headerH = 32.0f;
        float closeBtnSize = 22.0f;
        float closeBtnX = panelX + panelW - closeBtnSize - 10;
        float closeBtnY = panelY + (headerH - closeBtnSize) / 2.0f;
        if (mx >= closeBtnX && mx <= closeBtnX + closeBtnSize &&
            my >= closeBtnY && my <= closeBtnY + closeBtnSize) { hide(); return; }

        // Click outside
        if (mx < panelX || mx > panelX + panelW || my < panelY || my > panelY + panelH) { hide(); return; }

        // Tab clicks
        float tabBarY = panelY + headerH;
        float tabH = 28.0f;
        float tabW = panelW / TAB_COUNT;
        if (my >= tabBarY && my <= tabBarY + tabH) {
            int tab = static_cast<int>((mx - panelX) / tabW);
            if (tab >= 0 && tab < TAB_COUNT) { activeTab_ = static_cast<Tab>(tab); redraw(); return; }
        }

        float contentX = panelX + 16.0f;
        float contentY = tabBarY + tabH + 8.0f;
        float contentW = panelW - 32.0f;
        float rowH = 24.0f;

        switch (activeTab_) {
            case TAB_GENERAL:    handleGeneralClick(mx, my, contentX, contentY, contentW, rowH); break;
            case TAB_ADVANCED:   handleAdvancedClick(mx, my, contentX, contentY, contentW, rowH); break;
            case TAB_MIDI:       handleMidiClick(mx, my, contentX, contentY, contentW, rowH); break;
            case TAB_MODULATION: handleModulationClick(mx, my, contentX, contentY, contentW, rowH); break;
            default: break;
        }
    }

    void mouseMove(const visage::MouseEvent& e) override {
        mouseInside_ = true;
        mouseX_ = e.position.x;
        mouseY_ = e.position.y;
        redraw();
    }

    void mouseDrag(const visage::MouseEvent& e) override {
        mouseInside_ = true;
        mouseX_ = e.position.x;
        mouseY_ = e.position.y;
        redraw();
    }

    void mouseUp(const visage::MouseEvent& e) override {
        mouseInside_ = true;
        mouseX_ = e.position.x;
        mouseY_ = e.position.y;
        mouseIsDown_ = false;
        redraw();
    }

    void mouseExit(const visage::MouseEvent&) override {
        mouseInside_ = false;
        mouseIsDown_ = false;
        redraw();
    }

private:
    // ==================== GENERAL TAB ====================
    void drawGeneralTab(visage::Canvas& canvas, const visage::Font& font,
                        const visage::Font& fontSmall, const visage::Font& fontTitle,
                        float x, float y, float w, float rowH) {
        float cy = y;
        canvas.setColor(0xffff8833);
        canvas.text("Reset Mode", fontTitle, visage::Font::kLeft, x, cy, w, rowH); cy += rowH;
        drawRadio(canvas, font, x, cy, w, rowH, "Transparent (silent)", resetMode_ == 0); cy += rowH;
        drawRadio(canvas, font, x, cy, w, rowH, "Retrigger (instant fire)", resetMode_ == 1); cy += rowH + 8;

        canvas.setColor(0xffff8833);
        canvas.text("Options", fontTitle, visage::Font::kLeft, x, cy, w, rowH); cy += rowH;
        drawToggle(canvas, font, x, cy, w, rowH, "MIDI Thru", midiThru_); cy += rowH;
        drawToggle(canvas, font, x, cy, w, rowH, "Live Mode", liveMode_); cy += rowH + 12.0f;

        canvas.setColor(0xffff8833);
        canvas.text("About", fontTitle, visage::Font::kLeft, x, cy, w, rowH); cy += rowH;
        drawActionButton(canvas, font, x, cy, 128.0f, rowH, "Acknowledgements");
    }

    void handleGeneralClick(float mx, float my, float x, float y, float w, float rowH) {
        float cy = y + rowH;
        if (my >= cy && my < cy + rowH) { resetMode_ = 0; if (onResetModeChange) onResetModeChange(0); redraw(); return; } cy += rowH;
        if (my >= cy && my < cy + rowH) { resetMode_ = 1; if (onResetModeChange) onResetModeChange(1); redraw(); return; } cy += rowH + 8 + rowH;
        if (my >= cy && my < cy + rowH && mx >= x + w - 40) { midiThru_ = !midiThru_; if (onMidiThruChange) onMidiThruChange(midiThru_); redraw(); return; } cy += rowH;
        if (my >= cy && my < cy + rowH && mx >= x + w - 40) { liveMode_ = !liveMode_; if (onLiveModeChange) onLiveModeChange(liveMode_); redraw(); return; }
        cy += rowH + 12.0f + rowH;

        float buttonW = 128.0f;
        if (mx >= x && mx <= x + buttonW && my >= cy && my <= cy + rowH) {
            if (onOpenAcknowledgements)
                onOpenAcknowledgements();
            return;
        }
    }

    // ==================== ADVANCED TAB ====================
    void drawAdvancedTab(visage::Canvas& canvas, const visage::Font& font,
                         const visage::Font& fontSmall, const visage::Font& fontTitle,
                         float x, float y, float w, float rowH) {
        float cy = y;
        // Reset Quantization header with "Show and adjust on main screen" toggle
        canvas.setColor(0xffff8833);
        canvas.text("Reset Quantization", fontTitle, visage::Font::kLeft, x, cy, w * 0.5f, rowH);
        {
            float toggleW = 36.0f, toggleH = 16.0f;
            float toggleX = x + w - toggleW - 4;
            float labelRightEdge = toggleX - 6.0f;
            float labelStartX = labelRightEdge - 160.0f;
            canvas.setColor(0xffaaaaaa);
            canvas.text("Show and adjust on main screen", fontSmall, visage::Font::kRight, labelStartX, cy, labelRightEdge - labelStartX, rowH);
            float ty = cy + (rowH - toggleH) / 2.0f;
            bool hovered = isHoveredRect(toggleX, ty, toggleW, toggleH);
            bool pressed = isPressedRect(toggleX, ty, toggleW, toggleH);
            canvas.setColor(showQuantizeOnMain_
                                ? (pressed ? 0xffffaa55 : (hovered ? 0xffff9944 : 0xffff8833))
                                : (pressed ? 0xff5a5a5a : (hovered ? 0xff505050 : 0xff444444)));
            canvas.roundedRectangle(toggleX, ty, toggleW, toggleH, toggleH / 2.0f);
            if (hovered || pressed) {
                canvas.setColor(pressed ? 0xffff8833 : 0xff888888);
                canvas.roundedRectangleBorder(toggleX, ty, toggleW, toggleH, toggleH / 2.0f, 1.0f);
            }
            float thumbSize = toggleH - 4;
            float thumbX = showQuantizeOnMain_ ? toggleX + toggleW - thumbSize - 2 : toggleX + 2;
            canvas.setColor(pressed ? 0xfff5f5f5 : 0xffffffff);
            canvas.circle(thumbX, ty + 2, thumbSize);
        }
        cy += rowH;

        const char* quantNames[] = { "Off", "2 Bar", "1 Bar", "1/2", "1/4", "1/8", "1/16", "1/32", "1/4T", "1/8T", "1/16T" };
        float btnW = 42.0f, btnH = 20.0f, gap = 3.0f;
        float bx = x;
        for (int i = 0; i < 11; i++) {
            if (bx + btnW > x + w) { bx = x; cy += btnH + gap; }
            bool hovered = isHoveredRect(bx, cy, btnW, btnH);
            bool pressed = isPressedRect(bx, cy, btnW, btnH);
            canvas.setColor(resetQuantize_ == i
                                ? (pressed ? 0xffffaa55 : (hovered ? 0xffff9944 : 0xffff8833))
                                : (pressed ? 0xff555555 : (hovered ? 0xff4d4d4d : 0xff444444)));
            canvas.roundedRectangle(bx, cy, btnW, btnH, 3.0f);
            canvas.setColor(resetQuantize_ == i ? 0xff000000 : 0xffcccccc);
            canvas.text(quantNames[i], fontSmall, visage::Font::kCenter, bx, cy, btnW, btnH);
            bx += btnW + gap;
        }
        cy += btnH + 12;

        // Euclidean Mode
        canvas.setColor(0xffff8833);
        canvas.text("Euclidean Mode", fontTitle, visage::Font::kLeft, x, cy, w, rowH); cy += rowH;
        drawToggle(canvas, font, x, cy, w, rowH, "Enable Euclidean", euclideanEnabled_); cy += rowH;

        if (euclideanEnabled_) {
            const char* instNames[] = { "BD Length", "SD Length", "HH Length" };
            for (int i = 0; i < 3; i++) {
                drawNumberValue(canvas, font, x, cy, w, rowH, instNames[i], euclideanLengths_[i], 1, 32);
                cy += rowH;
            }
        }
    }

    void handleAdvancedClick(float mx, float my, float x, float y, float w, float rowH) {
        // "Show and adjust on main screen" toggle on header row
        if (my >= y && my < y + rowH && mx >= x + w - 40) {
            showQuantizeOnMain_ = !showQuantizeOnMain_;
            if (onShowQuantizeOnMainChange) onShowQuantizeOnMainChange(showQuantizeOnMain_);
            redraw(); return;
        }
        float cy = y + rowH; // skip header
        // Quantize buttons
        float btnW = 42.0f, btnH = 20.0f, gap = 3.0f;
        float bx = x;
        for (int i = 0; i < 11; i++) {
            if (bx + btnW > x + w) { bx = x; cy += btnH + gap; }
            if (mx >= bx && mx <= bx + btnW && my >= cy && my <= cy + btnH) {
                resetQuantize_ = i;
                if (onResetQuantizeChange) onResetQuantizeChange(i);
                redraw(); return;
            }
            bx += btnW + gap;
        }
        cy += btnH + 12;

        cy += rowH; // skip Euclidean header
        // Euclidean toggle
        if (my >= cy && my < cy + rowH && mx >= x + w - 40) {
            euclideanEnabled_ = !euclideanEnabled_;
            if (onEuclideanEnableChange) onEuclideanEnableChange(euclideanEnabled_);
            redraw(); return;
        }
        cy += rowH;

        if (euclideanEnabled_) {
            float btnSize = 20.0f, valueW = 40.0f;
            float rightX = x + w - btnSize * 2 - valueW - 4;
            for (int i = 0; i < 3; i++) {
                handleEuclideanLengthClick(mx, my, cy, rowH, rightX, btnSize, valueW, i);
                cy += rowH;
            }
        }
    }

    void handleEuclideanLengthClick(float mx, float my, float cy, float rowH,
                                     float rightX, float btnSize, float valueW, int inst) {
        if (my < cy || my >= cy + rowH) return;
        float btnY = cy + (rowH - btnSize) / 2;
        if (mx >= rightX && mx <= rightX + btnSize && my >= btnY && my <= btnY + btnSize) {
            if (euclideanLengths_[inst] > 1) { euclideanLengths_[inst]--; if (onEuclideanLengthChange) onEuclideanLengthChange(inst, euclideanLengths_[inst]); redraw(); }
            return;
        }
        float plusX = rightX + btnSize + valueW;
        if (mx >= plusX && mx <= plusX + btnSize && my >= btnY && my <= btnY + btnSize) {
            if (euclideanLengths_[inst] < 32) { euclideanLengths_[inst]++; if (onEuclideanLengthChange) onEuclideanLengthChange(inst, euclideanLengths_[inst]); redraw(); }
            return;
        }
    }

    // ==================== MIDI TAB ====================
    void drawMidiTab(visage::Canvas& canvas, const visage::Font& font,
                     const visage::Font& fontSmall, const visage::Font& fontTitle,
                     float x, float y, float w, float rowH) {
        float cy = y;
        canvas.setColor(0xffff8833);
        canvas.text("MIDI Output", fontTitle, visage::Font::kLeft, x, cy, w, rowH); cy += rowH;
        drawNumberValue(canvas, font, x, cy, w, rowH, "Channel", midiChannel_, 1, 16); cy += rowH + 8;

        canvas.setColor(0xffff8833);
        canvas.text("Note Assignments", fontTitle, visage::Font::kLeft, x, cy, w * 0.5f, rowH);
        // Toggle with label right-adjacent to the switch
        {
            float toggleW = 36.0f, toggleH = 16.0f;
            float toggleX = x + w - toggleW - 4;
            float labelRightEdge = toggleX - 6.0f;
            float labelStartX = labelRightEdge - 160.0f;
            canvas.setColor(0xffaaaaaa);
            canvas.text("Show and adjust on main screen", fontSmall, visage::Font::kRight, labelStartX, cy, labelRightEdge - labelStartX, rowH);
            // Draw the toggle manually (same as drawToggle but with explicit position)
            float ty = cy + (rowH - toggleH) / 2.0f;
            bool hovered = isHoveredRect(toggleX, ty, toggleW, toggleH);
            bool pressed = isPressedRect(toggleX, ty, toggleW, toggleH);
            canvas.setColor(showNotesOnMain_
                                ? (pressed ? 0xffffaa55 : (hovered ? 0xffff9944 : 0xffff8833))
                                : (pressed ? 0xff5a5a5a : (hovered ? 0xff505050 : 0xff444444)));
            canvas.roundedRectangle(toggleX, ty, toggleW, toggleH, toggleH / 2.0f);
            if (hovered || pressed) {
                canvas.setColor(pressed ? 0xffff8833 : 0xff888888);
                canvas.roundedRectangleBorder(toggleX, ty, toggleW, toggleH, toggleH / 2.0f, 1.0f);
            }
            float thumbSize = toggleH - 4;
            float thumbX = showNotesOnMain_ ? toggleX + toggleW - thumbSize - 2 : toggleX + 2;
            canvas.setColor(pressed ? 0xfff5f5f5 : 0xffffffff);
            canvas.circle(thumbX, ty + 2, thumbSize);
        }
        cy += rowH;

        drawNoteValue(canvas, font, x, cy, w, rowH, "BD Note", bdNote_); cy += rowH;
        drawNoteValue(canvas, font, x, cy, w, rowH, "SD Note", sdNote_); cy += rowH;
        drawNoteValue(canvas, font, x, cy, w, rowH, "HH Note", hhNote_); cy += rowH + 8;

        canvas.setColor(0xffff8833);
        canvas.text("MIDI Learn", fontTitle, visage::Font::kLeft, x, cy, w, rowH); cy += rowH;

        canvas.setColor(0xffcccccc);
        std::string ccLabel = "Reset CC: ";
        ccLabel += (resetMidiCC_ >= 0) ? std::to_string(resetMidiCC_) : "None";
        canvas.text(ccLabel.c_str(), font, visage::Font::kLeft, x, cy, w * 0.5f, rowH);

        float btnW = 60.0f, btnH = 20.0f;
        float btnX = x + w - btnW - 4;
        float btnY = cy + (rowH - btnH) / 2;
        bool hovered = isHoveredRect(btnX, btnY, btnW, btnH);
        bool pressed = isPressedRect(btnX, btnY, btnW, btnH);
        canvas.setColor(midiLearnActive_
                            ? (pressed ? 0xffff6666 : (hovered ? 0xffff5555 : 0xffff4444))
                            : (pressed ? 0xff666666 : (hovered ? 0xff5c5c5c : 0xff555555)));
        canvas.roundedRectangle(btnX, btnY, btnW, btnH, 4.0f);
        if (hovered || pressed) {
            canvas.setColor(pressed ? 0xffff8833 : 0xff888888);
            canvas.roundedRectangleBorder(btnX, btnY, btnW, btnH, 4.0f, 1.0f);
        }
        canvas.setColor(0xffffffff);
        canvas.text(midiLearnActive_ ? "Stop" : "Learn", font, visage::Font::kCenter, btnX, cy, btnW, rowH);
    }

    void drawNoteValue(visage::Canvas& canvas, const visage::Font& font,
                       float x, float y, float w, float rowH,
                       const char* label, int noteNum) {
        canvas.setColor(0xffcccccc);
        canvas.text(label, font, visage::Font::kLeft, x, y, w * 0.35f, rowH);

        // Note name display
        std::string noteName = midiNoteName(noteNum);
        canvas.setColor(0xff888888);
        canvas.text(noteName.c_str(), font, visage::Font::kLeft, x + w * 0.35f, y, w * 0.2f, rowH);

        // +/- buttons
        float btnSize = 20.0f, valueW = 40.0f;
        float rightX = x + w - btnSize * 2 - valueW - 4;
        float btnY = y + (rowH - btnSize) / 2.0f;
        bool minusHovered = isHoveredRect(rightX, btnY, btnSize, btnSize);
        bool minusPressed = isPressedRect(rightX, btnY, btnSize, btnSize);
        bool plusHovered = isHoveredRect(rightX + btnSize + valueW, btnY, btnSize, btnSize);
        bool plusPressed = isPressedRect(rightX + btnSize + valueW, btnY, btnSize, btnSize);

        canvas.setColor(noteNum > 0
                            ? (minusPressed ? 0xff666666 : (minusHovered ? 0xff5c5c5c : 0xff555555))
                            : 0xff333333);
        canvas.roundedRectangle(rightX, btnY, btnSize, btnSize, 4.0f);
        canvas.setColor(noteNum > 0 ? 0xffcccccc : 0xff555555);
        canvas.text("-", font, visage::Font::kCenter, rightX, y, btnSize, rowH);

        canvas.setColor(0xffffffff);
        std::string valStr = std::to_string(noteNum);
        canvas.text(valStr.c_str(), font, visage::Font::kCenter, rightX + btnSize, y, valueW, rowH);

        canvas.setColor(noteNum < 127
                            ? (plusPressed ? 0xff666666 : (plusHovered ? 0xff5c5c5c : 0xff555555))
                            : 0xff333333);
        canvas.roundedRectangle(rightX + btnSize + valueW, btnY, btnSize, btnSize, 4.0f);
        canvas.setColor(noteNum < 127 ? 0xffcccccc : 0xff555555);
        canvas.text("+", font, visage::Font::kCenter, rightX + btnSize + valueW, y, btnSize, rowH);
    }

    void handleMidiClick(float mx, float my, float x, float y, float w, float rowH) {
        float cy = y + rowH; // skip MIDI Output header
        float btnSize = 20.0f, valueW = 40.0f;
        float rightX = x + w - btnSize * 2 - valueW - 4;

        handleNumberClick(mx, my, cy, rowH, rightX, btnSize, valueW, midiChannel_, 1, 16, onMidiChannelChange);
        cy += rowH + 8; // gap before Note Assignments row

        // "Show and adjust on main screen" toggle on Note Assignments header row
        if (my >= cy && my < cy + rowH && mx >= x + w - 40) {
            showNotesOnMain_ = !showNotesOnMain_;
            if (onShowNotesOnMainChange) onShowNotesOnMainChange(showNotesOnMain_);
            redraw(); return;
        }
        cy += rowH; // past the header

        handleNumberClick(mx, my, cy, rowH, rightX, btnSize, valueW, bdNote_, 0, 127, onBDNoteChange); cy += rowH;
        handleNumberClick(mx, my, cy, rowH, rightX, btnSize, valueW, sdNote_, 0, 127, onSDNoteChange); cy += rowH;
        handleNumberClick(mx, my, cy, rowH, rightX, btnSize, valueW, hhNote_, 0, 127, onHHNoteChange); cy += rowH + 8;

        cy += rowH; // skip MIDI Learn header

        // MIDI Learn button
        if (my >= cy && my < cy + rowH) {
            float learnBtnX = x + w - 64;
            if (mx >= learnBtnX) {
                midiLearnActive_ = !midiLearnActive_;
                if (midiLearnActive_) { if (onMidiLearnStart) onMidiLearnStart(); }
                else { if (onMidiLearnStop) onMidiLearnStop(); }
                redraw(); return;
            }
        }
    }

    // ==================== MODULATION TAB ====================
    void drawModulationTab(visage::Canvas& canvas, const visage::Font& font,
                           const visage::Font& fontSmall, const visage::Font& fontTitle,
                           float x, float y, float w, float rowH) {
        float cy = y;

        // LFO sub-tabs
        float subTabW = 60.0f;
        for (int i = 0; i < 2; i++) {
            float tx = x + i * (subTabW + 4);
            bool hovered = isHoveredRect(tx, cy, subTabW, 20.0f);
            bool pressed = isPressedRect(tx, cy, subTabW, 20.0f);
            canvas.setColor(activeLFO_ == i
                                ? (pressed ? 0xffffaa55 : (hovered ? 0xffff9944 : 0xffff8833))
                                : (pressed ? 0xff555555 : (hovered ? 0xff4d4d4d : 0xff444444)));
            canvas.roundedRectangle(tx, cy, subTabW, 20, 3.0f);
            canvas.setColor(activeLFO_ == i ? 0xff000000 : 0xffcccccc);
            const char* lfoNames[] = { "LFO 1", "LFO 2" };
            canvas.text(lfoNames[i], fontSmall, visage::Font::kCenter, tx, cy, subTabW, 20);
        }
        cy += 26;

        int lfo = activeLFO_;

        // Enable toggle
        drawToggle(canvas, font, x, cy, w, rowH, "Enabled", lfoEnabled_[lfo]); cy += rowH;

        if (lfoEnabled_[lfo]) {
            // Shape selector
            canvas.setColor(0xffcccccc);
            canvas.text("Shape", font, visage::Font::kLeft, x, cy, 50, rowH);
            const char* shapeNames[] = { "Sin", "Tri", "Sq", "Saw", "Rnd" };
            float shapeBtnW = 36.0f;
            for (int s = 0; s < 5; s++) {
                float sx = x + 55 + s * (shapeBtnW + 3);
                bool hovered = isHoveredRect(sx, cy + 2, shapeBtnW, rowH - 4);
                bool pressed = isPressedRect(sx, cy + 2, shapeBtnW, rowH - 4);
                canvas.setColor(lfoShape_[lfo] == s
                                    ? (pressed ? 0xffffaa55 : (hovered ? 0xffff9944 : 0xffff8833))
                                    : (pressed ? 0xff555555 : (hovered ? 0xff4d4d4d : 0xff444444)));
                canvas.roundedRectangle(sx, cy + 2, shapeBtnW, rowH - 4, 3.0f);
                canvas.setColor(lfoShape_[lfo] == s ? 0xff000000 : 0xffcccccc);
                canvas.text(shapeNames[s], fontSmall, visage::Font::kCenter, sx, cy, shapeBtnW, rowH);
            }
            cy += rowH;

            // Rate
            canvas.setColor(0xffcccccc);
            canvas.text("Rate", font, visage::Font::kLeft, x, cy, 50, rowH);
            drawSliderBar(canvas, font, x + 55, cy, w - 55, rowH, lfoRate_[lfo], 0.25f, 16.0f);
            std::string rateStr = formatFloat(lfoRate_[lfo], 2) + " beats";
            canvas.setColor(0xff888888);
            canvas.text(rateStr.c_str(), fontSmall, visage::Font::kRight, x + w - 80, cy, 80, rowH);
            cy += rowH;

            // Depth
            canvas.setColor(0xffcccccc);
            canvas.text("Depth", font, visage::Font::kLeft, x, cy, 50, rowH);
            drawSliderBar(canvas, font, x + 55, cy, w - 55, rowH, lfoDepth_[lfo], 0.0f, 1.0f);
            std::string depthStr = std::to_string(static_cast<int>(lfoDepth_[lfo] * 100)) + "%";
            canvas.setColor(0xff888888);
            canvas.text(depthStr.c_str(), fontSmall, visage::Font::kRight, x + w - 50, cy, 50, rowH);
            cy += rowH + 6;

            // Destinations header
            canvas.setColor(0xffff8833);
            canvas.text("Destinations", fontTitle, visage::Font::kLeft, x, cy, w, rowH); cy += rowH;

            // Destination checkboxes in 3-column grid
            const char* destNames[] = {
                "X", "Y", "Chaos", "Swing", "Reset",
                "BD Dens", "SD Dens", "HH Dens",
                "BD Vel", "SD Vel", "HH Vel",
                "BD Note", "SD Note", "HH Note"
            };
            float colW = w / 3.0f;
            float checkRowH = 20.0f;
            for (int d = 0; d < 14; d++) {
                int col = d % 3;
                float dx = x + col * colW;
                float dy = cy + (d / 3) * checkRowH;
                drawCheckbox(canvas, fontSmall, dx, dy, colW, checkRowH, destNames[d], lfoDest_[lfo][d]);
            }
        }
    }

    void handleModulationClick(float mx, float my, float x, float y, float w, float rowH) {
        float cy = y;

        // LFO sub-tab clicks
        float subTabW = 60.0f;
        if (my >= cy && my <= cy + 20) {
            for (int i = 0; i < 2; i++) {
                float tx = x + i * (subTabW + 4);
                if (mx >= tx && mx <= tx + subTabW) { activeLFO_ = i; redraw(); return; }
            }
        }
        cy += 26;
        int lfo = activeLFO_;

        // Enable toggle
        if (my >= cy && my < cy + rowH && mx >= x + w - 40) {
            lfoEnabled_[lfo] = !lfoEnabled_[lfo];
            if (onLFOEnableChange) onLFOEnableChange(lfo, lfoEnabled_[lfo]);
            redraw(); return;
        }
        cy += rowH;

        if (!lfoEnabled_[lfo]) return;

        // Shape buttons
        float shapeBtnW = 36.0f;
        if (my >= cy && my < cy + rowH) {
            for (int s = 0; s < 5; s++) {
                float sx = x + 55 + s * (shapeBtnW + 3);
                if (mx >= sx && mx <= sx + shapeBtnW) {
                    lfoShape_[lfo] = s;
                    if (onLFOShapeChange) onLFOShapeChange(lfo, s);
                    redraw(); return;
                }
            }
        }
        cy += rowH;

        // Rate slider click
        if (my >= cy && my < cy + rowH && mx >= x + 55) {
            float norm = (mx - (x + 55)) / (w - 55);
            norm = std::max(0.0f, std::min(1.0f, norm));
            lfoRate_[lfo] = 0.25f + norm * (16.0f - 0.25f);
            if (onLFORateChange) onLFORateChange(lfo, lfoRate_[lfo]);
            redraw(); return;
        }
        cy += rowH;

        // Depth slider click
        if (my >= cy && my < cy + rowH && mx >= x + 55) {
            float norm = (mx - (x + 55)) / (w - 55);
            lfoDepth_[lfo] = std::max(0.0f, std::min(1.0f, norm));
            if (onLFODepthChange) onLFODepthChange(lfo, lfoDepth_[lfo]);
            redraw(); return;
        }
        cy += rowH + 6 + rowH; // skip gap + destinations header

        // Destination checkboxes
        float colW = w / 3.0f;
        float checkRowH = 20.0f;
        for (int d = 0; d < 14; d++) {
            int col = d % 3;
            float dx = x + col * colW;
            float dy = cy + (d / 3) * checkRowH;
            if (mx >= dx && mx <= dx + colW && my >= dy && my <= dy + checkRowH) {
                lfoDest_[lfo][d] = !lfoDest_[lfo][d];
                if (onLFODestChange) onLFODestChange(lfo, d, lfoDest_[lfo][d]);
                redraw(); return;
            }
        }
    }

    // ==================== DRAWING HELPERS ====================
    void drawRadio(visage::Canvas& canvas, const visage::Font& font,
                   float x, float y, float w, float rowH, const char* label, bool selected) {
        float r = 5.0f;
        float rx = x + 4, ry = y + (rowH - r * 2) / 2.0f;
        bool hovered = isHoveredRect(x, y, w, rowH);
        bool pressed = isPressedRect(x, y, w, rowH);
        canvas.setColor(selected ? 0xffff8833 : (pressed ? 0xff888888 : (hovered ? 0xff777777 : 0xff555555)));
        canvas.ring(rx, ry, r * 2, 1.5f);
        if (selected) { canvas.setColor(0xffff8833); canvas.circle(rx + 3, ry + 3, r * 2 - 6); }
        canvas.setColor(pressed ? 0xffffffff : (hovered ? 0xffe0e0e0 : 0xffcccccc));
        canvas.text(label, font, visage::Font::kLeft, x + r * 2 + 12, y, w - r * 2 - 12, rowH);
    }

    void drawToggle(visage::Canvas& canvas, const visage::Font& font,
                    float x, float y, float w, float rowH, const char* label, bool on) {
        canvas.setColor(0xffcccccc);
        canvas.text(label, font, visage::Font::kLeft, x, y, w * 0.6f, rowH);
        float toggleW = 36.0f, toggleH = 16.0f;
        float tx = x + w - toggleW - 4, ty = y + (rowH - toggleH) / 2.0f;
        bool hovered = isHoveredRect(tx, ty, toggleW, toggleH);
        bool pressed = isPressedRect(tx, ty, toggleW, toggleH);
        canvas.setColor(on
                            ? (pressed ? 0xffffaa55 : (hovered ? 0xffff9944 : 0xffff8833))
                            : (pressed ? 0xff5a5a5a : (hovered ? 0xff505050 : 0xff444444)));
        canvas.roundedRectangle(tx, ty, toggleW, toggleH, toggleH / 2.0f);
        if (hovered || pressed) {
            canvas.setColor(pressed ? 0xffff8833 : 0xff888888);
            canvas.roundedRectangleBorder(tx, ty, toggleW, toggleH, toggleH / 2.0f, 1.0f);
        }
        float thumbSize = toggleH - 4;
        float thumbX = on ? tx + toggleW - thumbSize - 2 : tx + 2;
        canvas.setColor(pressed ? 0xfff5f5f5 : 0xffffffff);
        canvas.circle(thumbX, ty + 2, thumbSize);
    }

    void drawActionButton(visage::Canvas& canvas, const visage::Font& font,
                          float x, float y, float width, float rowH, const char* label) {
        float buttonH = rowH;
        bool hovered = isHoveredRect(x, y, width, buttonH);
        bool pressed = isPressedRect(x, y, width, buttonH);
        canvas.setColor(pressed ? 0xff2d2d2d : (hovered ? 0xff3a3a3a : 0xff333333));
        canvas.roundedRectangle(x, y, width, buttonH, 5.0f);
        canvas.setColor(pressed ? 0xffff8833 : (hovered ? 0xff888888 : 0xff666666));
        canvas.roundedRectangleBorder(x, y, width, buttonH, 5.0f, 1.0f);
        canvas.setColor(pressed ? 0xffffffff : 0xffdddddd);
        canvas.text(label, font, visage::Font::kCenter, x, y, width, buttonH);
    }

    void drawNumberValue(visage::Canvas& canvas, const visage::Font& font,
                         float x, float y, float w, float rowH,
                         const char* label, int value, int min, int max) {
        canvas.setColor(0xffcccccc);
        canvas.text(label, font, visage::Font::kLeft, x, y, w * 0.5f, rowH);
        float btnSize = 20.0f, valueW = 40.0f;
        float rightX = x + w - btnSize * 2 - valueW - 4;
        float btnY = y + (rowH - btnSize) / 2;
        bool minusHovered = isHoveredRect(rightX, btnY, btnSize, btnSize);
        bool minusPressed = isPressedRect(rightX, btnY, btnSize, btnSize);
        bool plusHovered = isHoveredRect(rightX + btnSize + valueW, btnY, btnSize, btnSize);
        bool plusPressed = isPressedRect(rightX + btnSize + valueW, btnY, btnSize, btnSize);

        canvas.setColor(value > min
                            ? (minusPressed ? 0xff666666 : (minusHovered ? 0xff5c5c5c : 0xff555555))
                            : 0xff333333);
        canvas.roundedRectangle(rightX, btnY, btnSize, btnSize, 4.0f);
        canvas.setColor(value > min ? 0xffcccccc : 0xff555555);
        canvas.text("-", font, visage::Font::kCenter, rightX, y, btnSize, rowH);

        canvas.setColor(0xffffffff);
        canvas.text(std::to_string(value).c_str(), font, visage::Font::kCenter, rightX + btnSize, y, valueW, rowH);

        canvas.setColor(value < max
                            ? (plusPressed ? 0xff666666 : (plusHovered ? 0xff5c5c5c : 0xff555555))
                            : 0xff333333);
        canvas.roundedRectangle(rightX + btnSize + valueW, btnY, btnSize, btnSize, 4.0f);
        canvas.setColor(value < max ? 0xffcccccc : 0xff555555);
        canvas.text("+", font, visage::Font::kCenter, rightX + btnSize + valueW, y, btnSize, rowH);
    }

    void drawSliderBar(visage::Canvas& canvas, const visage::Font& font,
                       float x, float y, float w, float rowH,
                       float value, float min, float max) {
        float barH = 6.0f;
        float by = y + (rowH - barH) / 2.0f;
        float barW = w - 90; // leave room for value text
        canvas.setColor(0xff444444);
        canvas.roundedRectangle(x, by, barW, barH, 3.0f);
        float norm = (value - min) / (max - min);
        if (norm > 0.01f) {
            canvas.setColor(0xffff8833);
            canvas.roundedRectangle(x, by, barW * norm, barH, 3.0f);
        }
        // Thumb
        float thumbX = x + barW * norm - 4;
        canvas.setColor(0xffffffff);
        canvas.circle(thumbX, by - 2, 10);
    }

    void drawCheckbox(visage::Canvas& canvas, const visage::Font& font,
                      float x, float y, float w, float h,
                      const char* label, bool checked) {
        float boxSize = 12.0f;
        float bx = x + 2, by = y + (h - boxSize) / 2.0f;
        bool hovered = isHoveredRect(x, y, w, h);
        bool pressed = isPressedRect(x, y, w, h);
        canvas.setColor(checked
                            ? (pressed ? 0xffffaa55 : (hovered ? 0xffff9944 : 0xffff8833))
                            : (pressed ? 0xff555555 : (hovered ? 0xff4d4d4d : 0xff444444)));
        canvas.roundedRectangle(bx, by, boxSize, boxSize, 2.0f);
        if (checked) {
            canvas.setColor(0xff000000);
            canvas.text("v", font, visage::Font::kCenter, bx, by - 1, boxSize, boxSize + 2);
        }
        canvas.setColor(pressed ? 0xffffffff : (hovered ? 0xffe0e0e0 : 0xffcccccc));
        canvas.text(label, font, visage::Font::kLeft, x + boxSize + 6, y, w - boxSize - 8, h);
    }

    static bool pointInRect(float px, float py, float x, float y, float w, float h) {
        return px >= x && px <= x + w && py >= y && py <= y + h;
    }

    bool isHoveredRect(float x, float y, float w, float h) const {
        return mouseInside_ && pointInRect(mouseX_, mouseY_, x, y, w, h);
    }

    bool isPressedRect(float x, float y, float w, float h) const {
        return mouseIsDown_ && pointInRect(mouseDownX_, mouseDownY_, x, y, w, h);
    }

    void handleNumberClick(float mx, float my, float cy, float rowH,
                           float rightX, float btnSize, float valueW,
                           int& value, int min, int max,
                           std::function<void(int)>& callback) {
        if (my < cy || my >= cy + rowH) return;
        float btnY = cy + (rowH - btnSize) / 2;
        if (mx >= rightX && mx <= rightX + btnSize && my >= btnY && my <= btnY + btnSize) {
            if (value > min) { value--; if (callback) callback(value); redraw(); } return;
        }
        float plusX = rightX + btnSize + valueW;
        if (mx >= plusX && mx <= plusX + btnSize && my >= btnY && my <= btnY + btnSize) {
            if (value < max) { value++; if (callback) callback(value); redraw(); } return;
        }
    }

    // MIDI note name helper
    static std::string midiNoteName(int note) {
        const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        int octave = (note / 12) - 1;
        return std::string(names[note % 12]) + std::to_string(octave) + " (" + std::to_string(note) + ")";
    }

    static std::string formatFloat(float v, int decimals) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.*f", decimals, v);
        return std::string(buf);
    }

    // --- State ---
    Tab activeTab_ = TAB_GENERAL;
    int activeLFO_ = 0;
    int midiChannel_ = 1;
    int bdNote_ = 36;
    int sdNote_ = 38;
    int hhNote_ = 42;
    bool midiThru_ = true;
    bool liveMode_ = false;
    int resetMode_ = 0;
    bool midiLearnActive_ = false;
    int resetMidiCC_ = -1;
    int resetQuantize_ = 0;
    bool showNotesOnMain_ = false;
    bool showQuantizeOnMain_ = false;
    bool euclideanEnabled_ = false;
    int euclideanLengths_[3] = { 16, 12, 8 };
    bool lfoEnabled_[2] = { false, false };
    int lfoShape_[2] = { 0, 0 };
    float lfoRate_[2] = { 4.0f, 4.0f };
    float lfoDepth_[2] = { 0.5f, 0.5f };
    bool lfoDest_[2][14] = {};
    float mouseX_ = 0.0f;
    float mouseY_ = 0.0f;
    float mouseDownX_ = 0.0f;
    float mouseDownY_ = 0.0f;
    bool mouseInside_ = false;
    bool mouseIsDown_ = false;
};
