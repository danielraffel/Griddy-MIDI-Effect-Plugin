#pragma once

#include <visage_ui/frame.h>
#include <visage_graphics/canvas.h>
#include <visage_graphics/font.h>
#include <functional>
#include <string>

namespace visage::fonts { extern ::visage::EmbeddedFile Lato_Regular_ttf; }

class EuclideanOverlayFrame : public visage::Frame {
public:
    EuclideanOverlayFrame() : Frame("EuclideanOverlay") {}

    std::function<void(int instrument, int length)> onLengthChange;

    void setActive(bool active) {
        if (active_ != active) { active_ = active; redraw(); }
    }
    bool isActive() const { return active_; }

    void setLengths(int bd, int sd, int hh) {
        lengths_[0] = bd; lengths_[1] = sd; lengths_[2] = hh;
        redraw();
    }

    void draw(visage::Canvas& canvas) override {
        if (!active_) return;

        float w = width();
        float h = height();

        // Semi-transparent dark overlay
        canvas.setColor(0xdd0a0a0a);
        canvas.roundedRectangle(0, 0, w, h, 8.0f);

        visage::Font titleFont(12.0f, visage::fonts::Lato_Regular_ttf);
        visage::Font font(11.0f, visage::fonts::Lato_Regular_ttf);
        visage::Font smallFont(9.0f, visage::fonts::Lato_Regular_ttf);

        // Title
        canvas.setColor(0xffff8833);
        canvas.text("Euclidean Lengths", titleFont, visage::Font::kCenter, 0, 12, w, 18);

        // Stepper rows inside a container box
        const char* labels[] = { "BD", "SD", "HH" };
        unsigned int colors[] = { 0xffff4444, 0xff44ff44, 0xffffff44 };
        float rowH = 28.0f;
        float labelW = 28.0f;
        float stepperW = 90.0f;
        float innerGroupW = labelW + 4.0f + stepperW;
        float boxPadH = 16.0f;  // horizontal padding inside box
        float boxPadV = 10.0f;  // vertical padding inside box
        float boxW = innerGroupW + boxPadH * 2;
        float boxH = rowH * 3 + boxPadV * 2;
        float boxX = (w - boxW) / 2.0f;
        float boxY = (h - boxH) / 2.0f + 8;  // slight offset for title

        // Container background
        canvas.setColor(0xff161616);
        canvas.roundedRectangle(boxX, boxY, boxW, boxH, 6.0f);
        canvas.setColor(0xff2a2a2a);
        canvas.roundedRectangleBorder(boxX, boxY, boxW, boxH, 6.0f, 1.0f);

        float labelX = boxX + boxPadH;
        float stepperX = labelX + labelW + 4.0f;
        float startY = boxY + boxPadV;

        for (int i = 0; i < 3; i++) {
            float ry = startY + i * rowH;
            canvas.setColor(colors[i]);
            canvas.text(labels[i], font, visage::Font::kLeft, labelX, ry, labelW, rowH);
            drawCompactStepper(canvas, font, stepperX, ry, stepperW, rowH,
                               std::to_string(lengths_[i]).c_str(),
                               lengths_[i] > 1, lengths_[i] < 32);
        }

        // "?" help icon — bottom-right corner
        float helpSize = 18.0f;
        float helpX = 8;
        float helpY = h - helpSize - 8;
        bool helpHovered = isHoveredRect(helpX, helpY, helpSize, helpSize);
        canvas.setColor(helpHovered ? 0xff666666 : 0xff333333);
        canvas.circle(helpX, helpY, helpSize);
        canvas.setColor(helpHovered ? 0xffffffff : 0xff888888);
        canvas.text("?", font, visage::Font::kCenter, helpX, helpY, helpSize, helpSize);

        if (helpHovered) {
            float tipW = 200.0f, tipH = 36.0f;
            float tipX = helpX + helpSize + 4;
            float tipY2 = helpY - tipH - 4;  // above the icon
            if (tipX < 4) tipX = 4;
            if (tipY2 < 4) tipY2 = helpY + helpSize + 4;
            canvas.setColor(0xf0222222);
            canvas.roundedRectangle(tipX, tipY2, tipW, tipH, 4.0f);
            canvas.setColor(0xff666666);
            canvas.roundedRectangleBorder(tipX, tipY2, tipW, tipH, 4.0f, 1.0f);
            canvas.setColor(0xffcccccc);
            canvas.text("XY pad is inactive in Euclidean", smallFont, visage::Font::kCenter, tipX + 4, tipY2 + 2, tipW - 8, 16);
            canvas.text("mode \xe2\x80\x94 uses lengths + density", smallFont, visage::Font::kCenter, tipX + 4, tipY2 + 16, tipW - 8, 16);
        }
    }

    void mouseDown(const visage::MouseEvent& e) override {
        if (!active_) return;
        mouseInside_ = true;
        mouseX_ = e.position.x; mouseY_ = e.position.y;
        mouseDownX_ = e.position.x; mouseDownY_ = e.position.y;
        mouseIsDown_ = true;
        dragInstrument_ = -1;
        dragAccum_ = 0.0f;

        float w = width(), h = height();
        float rowH = 28.0f, labelW = 28.0f, stepperW = 90.0f;
        float innerGroupW = labelW + 4.0f + stepperW;
        float boxPadH = 16.0f, boxPadV = 10.0f;
        float boxW = innerGroupW + boxPadH * 2;
        float boxH = rowH * 3 + boxPadV * 2;
        float boxX = (w - boxW) / 2.0f;
        float boxY = (h - boxH) / 2.0f + 8;
        float stepperX = boxX + boxPadH + labelW + 4.0f;
        float startY = boxY + boxPadV;
        float chevW = 20.0f;
        float mx = e.position.x, my = e.position.y;

        for (int i = 0; i < 3; i++) {
            float ry = startY + i * rowH;
            if (my < ry || my >= ry + rowH) continue;
            float sH = rowH - 4.0f, sY = ry + 2.0f;

            // Down chevron (left)
            if (mx >= stepperX && mx <= stepperX + chevW && my >= sY && my <= sY + sH) {
                if (lengths_[i] > 1) { lengths_[i]--; if (onLengthChange) onLengthChange(i, lengths_[i]); redraw(); }
                return;
            }
            // Up chevron (right)
            float upX = stepperX + stepperW - chevW;
            if (mx >= upX && mx <= upX + chevW && my >= sY && my <= sY + sH) {
                if (lengths_[i] < 32) { lengths_[i]++; if (onLengthChange) onLengthChange(i, lengths_[i]); redraw(); }
                return;
            }
            // Value area — start drag
            float vX = stepperX + chevW, vW = stepperW - chevW * 2;
            if (mx >= vX && mx <= vX + vW && my >= sY && my <= sY + sH) {
                dragInstrument_ = i;
                dragStartValue_ = lengths_[i];
                return;
            }
        }
        redraw();
    }

    void mouseMove(const visage::MouseEvent& e) override {
        if (!active_) return;
        mouseInside_ = true; mouseX_ = e.position.x; mouseY_ = e.position.y;
        updateCursorForPosition(e.position.x, e.position.y);
        redraw();
    }
    void mouseDrag(const visage::MouseEvent& e) override {
        if (!active_) return;
        mouseInside_ = true; mouseX_ = e.position.x; mouseY_ = e.position.y;
        if (dragInstrument_ >= 0) {
            float dy = mouseDownY_ - e.position.y;  // up = positive
            float sensitivity = 4.0f;  // pixels per step
            int delta = static_cast<int>(dy / sensitivity);
            int newVal = std::max(1, std::min(32, dragStartValue_ + delta));
            if (newVal != lengths_[dragInstrument_]) {
                lengths_[dragInstrument_] = newVal;
                if (onLengthChange) onLengthChange(dragInstrument_, newVal);
            }
        }
        redraw();
    }
    void mouseUp(const visage::MouseEvent& e) override {
        if (!active_) return;
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
    void drawCompactStepper(visage::Canvas& canvas, const visage::Font& font,
                            float x, float y, float w, float h,
                            const char* valueText, bool canDec, bool canInc) {
        float sH = h - 4.0f, sY = y + 2.0f;
        float chevW = 20.0f;

        // Background pill
        canvas.setColor(0xff2a2a2a);
        canvas.roundedRectangle(x, sY, w, sH, 5.0f);
        canvas.setColor(0xff444444);
        canvas.roundedRectangleBorder(x, sY, w, sH, 5.0f, 0.5f);

        // Down chevron (decrease, left side)
        bool downHov = isHoveredRect(x, sY, chevW, sH);
        bool downPress = isPressedRect(x, sY, chevW, sH);
        canvas.setColor(canDec ? (downPress ? 0xffff8833 : (downHov ? 0xffcccccc : 0xff888888)) : 0xff444444);
        canvas.text("\xe2\x96\xbe", font, visage::Font::kCenter, x, y, chevW, h);

        // Up chevron (increase, right side)
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

    void updateCursorForPosition(float mx, float my) {
        float w = width(), h = height();
        float rowH = 28.0f, labelW = 28.0f, stepperW = 90.0f;
        float innerGroupW = labelW + 4.0f + stepperW;
        float boxPadH = 16.0f, boxPadV = 10.0f;
        float boxW = innerGroupW + boxPadH * 2;
        float boxH = rowH * 3 + boxPadV * 2;
        float boxX = (w - boxW) / 2.0f;
        float boxY = (h - boxH) / 2.0f + 8;
        float stepperX = boxX + boxPadH + labelW + 4.0f;
        float startY = boxY + boxPadV;
        float chevW = 20.0f;
        float vX = stepperX + chevW;
        float vW = stepperW - chevW * 2;

        for (int i = 0; i < 3; i++) {
            float ry = startY + i * rowH;
            float sH = rowH - 4.0f, sY = ry + 2.0f;
            if (mx >= vX && mx <= vX + vW && my >= sY && my <= sY + sH) {
                setCursorStyle(visage::MouseCursor::VerticalResize);
                return;
            }
        }
        setCursorStyle(visage::MouseCursor::Arrow);
    }

    bool active_ = false;
    int lengths_[3] = { 16, 12, 8 };
    float mouseX_ = 0, mouseY_ = 0;
    float mouseDownX_ = 0, mouseDownY_ = 0;
    bool mouseInside_ = false;
    bool mouseIsDown_ = false;
    int dragInstrument_ = -1;
    int dragStartValue_ = 0;
    float dragAccum_ = 0.0f;
};
