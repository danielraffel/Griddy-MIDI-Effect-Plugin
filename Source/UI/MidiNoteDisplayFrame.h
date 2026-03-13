#pragma once

#include <visage_ui/frame.h>
#include <visage_graphics/canvas.h>
#include <visage_graphics/font.h>
#include <functional>
#include <string>

namespace visage::fonts { extern ::visage::EmbeddedFile Lato_Regular_ttf; }

class MidiNoteDisplayFrame : public visage::Frame {
public:
    MidiNoteDisplayFrame() : Frame("MidiNoteDisplay") {}

    std::function<void(int)> onBDNoteChange;
    std::function<void(int)> onSDNoteChange;
    std::function<void(int)> onHHNoteChange;

    void setNotes(int bd, int sd, int hh) {
        if (notes_[0] != bd || notes_[1] != sd || notes_[2] != hh) {
            notes_[0] = bd; notes_[1] = sd; notes_[2] = hh;
            redraw();
        }
    }

    void setVisible(bool v) { visible_ = v; redraw(); }
    bool getVisible() const { return visible_; }

    void draw(visage::Canvas& canvas) override {
        if (!visible_) return;

        visage::Font titleFont(10.0f, visage::fonts::Lato_Regular_ttf);
        visage::Font font(10.0f, visage::fonts::Lato_Regular_ttf);
        float w = width();
        float h = height();

        // "MIDI Notes" title
        canvas.setColor(0xffaaaaaa);
        canvas.text("MIDI Notes", titleFont, visage::Font::kLeft, 0, 0, w, 14);

        const char* labels[] = { "BD", "SD", "HH" };
        unsigned int colors[] = { 0xffff4444, 0xff44ff44, 0xffffff44 };
        float rowH = (h - 16.0f) / 3.0f;
        float startY = 16.0f;
        float labelW = 24.0f;
        float stepperW = w - labelW - 4.0f;

        for (int i = 0; i < 3; i++) {
            float ry = startY + i * rowH;

            canvas.setColor(colors[i]);
            canvas.text(labels[i], font, visage::Font::kLeft, 0, ry, labelW, rowH);

            drawCompactStepper(canvas, font, labelW + 4, ry, stepperW, rowH,
                               midiNoteName(notes_[i]).c_str(),
                               notes_[i] > 0, notes_[i] < 127);
        }
    }

    void mouseDown(const visage::MouseEvent& e) override {
        if (!visible_) return;
        mouseInside_ = true;
        mouseX_ = e.position.x; mouseY_ = e.position.y;
        mouseDownX_ = e.position.x; mouseDownY_ = e.position.y;
        mouseIsDown_ = true;
        dragInstrument_ = -1;

        float h = height();
        float rowH = (h - 16.0f) / 3.0f;
        float startY = 16.0f;
        float labelW = 24.0f;
        float stepperW = width() - labelW - 4.0f;
        float stepperX = labelW + 4;
        float chevW = 20.0f;
        float mx = e.position.x, my = e.position.y;

        for (int i = 0; i < 3; i++) {
            float ry = startY + i * rowH;
            if (my < ry || my >= ry + rowH) continue;
            float sH = rowH - 3.0f, sY = ry + 1.5f;

            // Down chevron
            if (mx >= stepperX && mx <= stepperX + chevW && my >= sY && my <= sY + sH) {
                if (notes_[i] > 0) { notes_[i]--; fireNoteChange(i); redraw(); }
                return;
            }
            // Up chevron
            float upX = stepperX + stepperW - chevW;
            if (mx >= upX && mx <= upX + chevW && my >= sY && my <= sY + sH) {
                if (notes_[i] < 127) { notes_[i]++; fireNoteChange(i); redraw(); }
                return;
            }
            // Value area — start drag
            float vX = stepperX + chevW, vW = stepperW - chevW * 2;
            if (mx >= vX && mx <= vX + vW && my >= sY && my <= sY + sH) {
                dragInstrument_ = i;
                dragStartValue_ = notes_[i];
                return;
            }
        }
        redraw();
    }

    void mouseMove(const visage::MouseEvent& e) override {
        if (!visible_) return;
        mouseInside_ = true; mouseX_ = e.position.x; mouseY_ = e.position.y;
        updateCursorForPosition(e.position.x, e.position.y);
        redraw();
    }
    void mouseDrag(const visage::MouseEvent& e) override {
        if (!visible_) return;
        mouseInside_ = true; mouseX_ = e.position.x; mouseY_ = e.position.y;
        if (dragInstrument_ >= 0) {
            float dy = mouseDownY_ - e.position.y;  // up = positive
            float sensitivity = 3.0f;  // pixels per step
            int delta = static_cast<int>(dy / sensitivity);
            int newVal = std::max(0, std::min(127, dragStartValue_ + delta));
            if (newVal != notes_[dragInstrument_]) {
                notes_[dragInstrument_] = newVal;
                fireNoteChange(dragInstrument_);
            }
        }
        redraw();
    }
    void mouseUp(const visage::MouseEvent& e) override {
        if (!visible_) return;
        mouseInside_ = true; mouseX_ = e.position.x; mouseY_ = e.position.y;
        mouseIsDown_ = false;
        dragInstrument_ = -1;
        updateCursorForPosition(e.position.x, e.position.y);
        redraw();
    }
    void mouseExit(const visage::MouseEvent&) override {
        mouseInside_ = false; mouseIsDown_ = false;
        dragInstrument_ = -1;
        setCursorStyle(visage::MouseCursor::Arrow);
        redraw();
    }

private:
    void fireNoteChange(int i) {
        if (i == 0 && onBDNoteChange) onBDNoteChange(notes_[0]);
        if (i == 1 && onSDNoteChange) onSDNoteChange(notes_[1]);
        if (i == 2 && onHHNoteChange) onHHNoteChange(notes_[2]);
    }

    void drawCompactStepper(visage::Canvas& canvas, const visage::Font& font,
                            float x, float y, float w, float h,
                            const char* valueText, bool canDec, bool canInc) {
        float sH = h - 3.0f, sY = y + 1.5f;
        float chevW = 20.0f;

        // Background pill
        canvas.setColor(0xff2a2a2a);
        canvas.roundedRectangle(x, sY, w, sH, 5.0f);
        canvas.setColor(0xff444444);
        canvas.roundedRectangleBorder(x, sY, w, sH, 5.0f, 0.5f);

        // Down chevron (decrease, left)
        bool downHov = isHoveredRect(x, sY, chevW, sH);
        bool downPress = isPressedRect(x, sY, chevW, sH);
        canvas.setColor(canDec ? (downPress ? 0xffff8833 : (downHov ? 0xffcccccc : 0xff888888)) : 0xff444444);
        canvas.text("\xe2\x96\xbe", font, visage::Font::kCenter, x, y, chevW, h);

        // Up chevron (increase, right)
        float upX = x + w - chevW;
        bool upHov = isHoveredRect(upX, sY, chevW, sH);
        bool upPress = isPressedRect(upX, sY, chevW, sH);
        canvas.setColor(canInc ? (upPress ? 0xffff8833 : (upHov ? 0xffcccccc : 0xff888888)) : 0xff444444);
        canvas.text("\xe2\x96\xb4", font, visage::Font::kCenter, upX, y, chevW, h);

        // Value centered
        float vX = x + chevW, vW = w - chevW * 2;
        canvas.setColor(0xffeeeeee);
        canvas.text(valueText, font, visage::Font::kCenter, vX, y, vW, h);
    }

    static bool pointInRect(float px, float py, float rx, float ry, float rw, float rh) {
        return px >= rx && px <= rx + rw && py >= ry && py <= ry + rh;
    }
    bool isHoveredRect(float rx, float ry, float rw, float rh) const {
        return mouseInside_ && pointInRect(mouseX_, mouseY_, rx, ry, rw, rh);
    }
    bool isPressedRect(float rx, float ry, float rw, float rh) const {
        return mouseIsDown_ && pointInRect(mouseDownX_, mouseDownY_, rx, ry, rw, rh);
    }

    static std::string midiNoteName(int note) {
        const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        int octave = (note / 12) - 1;
        return std::string(names[note % 12]) + std::to_string(octave);
    }

    void updateCursorForPosition(float mx, float my) {
        float h = height();
        float rowH = (h - 16.0f) / 3.0f;
        float startY = 16.0f;
        float labelW = 24.0f;
        float stepperW = width() - labelW - 4.0f;
        float stepperX = labelW + 4;
        float chevW = 20.0f;
        float vX = stepperX + chevW;
        float vW = stepperW - chevW * 2;

        for (int i = 0; i < 3; i++) {
            float ry = startY + i * rowH;
            float sH = rowH - 3.0f, sY = ry + 1.5f;
            if (mx >= vX && mx <= vX + vW && my >= sY && my <= sY + sH) {
                setCursorStyle(visage::MouseCursor::VerticalResize);
                return;
            }
        }
        setCursorStyle(visage::MouseCursor::Arrow);
    }

    int notes_[3] = { 36, 38, 42 };
    bool visible_ = false;
    float mouseX_ = 0, mouseY_ = 0;
    float mouseDownX_ = 0, mouseDownY_ = 0;
    bool mouseInside_ = false;
    bool mouseIsDown_ = false;
    int dragInstrument_ = -1;
    int dragStartValue_ = 0;
};
