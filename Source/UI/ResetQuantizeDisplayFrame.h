#pragma once

#include <visage_ui/frame.h>
#include <visage_graphics/canvas.h>
#include <visage_graphics/font.h>
#include <functional>
#include <string>
#include <algorithm>

namespace visage::fonts { extern ::visage::EmbeddedFile Lato_Regular_ttf; }

class ResetQuantizeDisplayFrame : public visage::Frame {
public:
    ResetQuantizeDisplayFrame() : Frame("ResetQuantizeDisplay") {}

    std::function<void(int)> onQuantizeChange;

    void setQuantize(int q) {
        if (quantize_ != q) { quantize_ = std::max(0, std::min(10, q)); redraw(); }
    }
    int getQuantize() const { return quantize_; }

    void setVisible(bool v) { visible_ = v; redraw(); }
    bool getVisible() const { return visible_; }

    void draw(visage::Canvas& canvas) override {
        if (!visible_) return;

        visage::Font titleFont(10.0f, visage::fonts::Lato_Regular_ttf);
        visage::Font font(10.0f, visage::fonts::Lato_Regular_ttf);
        float w = width();
        float h = height();

        // "Quantize" title
        canvas.setColor(0xffaaaaaa);
        canvas.text("Quantize", titleFont, visage::Font::kCenter, 0, 0, w, 14);

        // Single stepper row
        float stepperY = 16.0f;
        float stepperH = h - stepperY;
        drawCompactStepper(canvas, font, 0, stepperY, w, stepperH,
                           quantizeNames_[quantize_],
                           quantize_ > 0, quantize_ < 10);
    }

    void mouseDown(const visage::MouseEvent& e) override {
        if (!visible_) return;
        mouseInside_ = true;
        mouseX_ = e.position.x; mouseY_ = e.position.y;
        mouseDownX_ = e.position.x; mouseDownY_ = e.position.y;
        mouseIsDown_ = true;
        dragActive_ = false;

        float w = width();
        float stepperY = 16.0f, stepperH = height() - stepperY;
        float sH = stepperH - 3.0f, sY = stepperY + 1.5f;
        float chevW = 14.0f;
        float mx = e.position.x, my = e.position.y;

        if (my < stepperY) return;

        // Down chevron (left)
        if (mx >= 0 && mx <= chevW && my >= sY && my <= sY + sH) {
            if (quantize_ > 0) { quantize_--; if (onQuantizeChange) onQuantizeChange(quantize_); redraw(); }
            return;
        }
        // Up chevron (right)
        float upX = w - chevW;
        if (mx >= upX && mx <= w && my >= sY && my <= sY + sH) {
            if (quantize_ < 10) { quantize_++; if (onQuantizeChange) onQuantizeChange(quantize_); redraw(); }
            return;
        }
        // Value area — start drag
        float vX = chevW, vW = w - chevW * 2;
        if (mx >= vX && mx <= vX + vW && my >= sY && my <= sY + sH) {
            dragActive_ = true;
            dragStartValue_ = quantize_;
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
        if (dragActive_) {
            float dy = mouseDownY_ - e.position.y;
            float sensitivity = 6.0f;
            int delta = static_cast<int>(dy / sensitivity);
            int newVal = std::max(0, std::min(10, dragStartValue_ + delta));
            if (newVal != quantize_) {
                quantize_ = newVal;
                if (onQuantizeChange) onQuantizeChange(quantize_);
            }
        }
        redraw();
    }
    void mouseUp(const visage::MouseEvent& e) override {
        if (!visible_) return;
        mouseInside_ = true; mouseX_ = e.position.x; mouseY_ = e.position.y;
        mouseIsDown_ = false;
        dragActive_ = false;
        updateCursorForPosition(e.position.x, e.position.y);
        redraw();
    }
    void mouseExit(const visage::MouseEvent&) override {
        mouseInside_ = false; mouseIsDown_ = false;
        dragActive_ = false;
        setCursorStyle(visage::MouseCursor::Arrow);
        redraw();
    }

private:
    void drawCompactStepper(visage::Canvas& canvas, const visage::Font& font,
                            float x, float y, float w, float h,
                            const char* valueText, bool canDec, bool canInc) {
        float sH = h - 3.0f, sY = y + 1.5f;
        float chevW = 14.0f;

        canvas.setColor(0xff2a2a2a);
        canvas.roundedRectangle(x, sY, w, sH, 5.0f);
        canvas.setColor(0xff444444);
        canvas.roundedRectangleBorder(x, sY, w, sH, 5.0f, 0.5f);

        bool downHov = isHoveredRect(x, sY, chevW, sH);
        bool downPress = isPressedRect(x, sY, chevW, sH);
        canvas.setColor(canDec ? (downPress ? 0xffff8833 : (downHov ? 0xffcccccc : 0xff888888)) : 0xff444444);
        canvas.text("\xe2\x96\xbe", font, visage::Font::kCenter, x, y, chevW, h);

        float upX = x + w - chevW;
        bool upHov = isHoveredRect(upX, sY, chevW, sH);
        bool upPress = isPressedRect(upX, sY, chevW, sH);
        canvas.setColor(canInc ? (upPress ? 0xffff8833 : (upHov ? 0xffcccccc : 0xff888888)) : 0xff444444);
        canvas.text("\xe2\x96\xb4", font, visage::Font::kCenter, upX, y, chevW, h);

        float vX = x + chevW, vW = w - chevW * 2;
        canvas.setColor(0xffeeeeee);
        canvas.text(valueText, font, visage::Font::kCenter, vX, y, vW, h);
    }

    void updateCursorForPosition(float mx, float my) {
        float stepperY = 16.0f, stepperH = height() - stepperY;
        float sH = stepperH - 3.0f, sY = stepperY + 1.5f;
        float chevW = 14.0f;
        float vX = chevW, vW = width() - chevW * 2;
        if (mx >= vX && mx <= vX + vW && my >= sY && my <= sY + sH) {
            setCursorStyle(visage::MouseCursor::VerticalResize);
            return;
        }
        setCursorStyle(visage::MouseCursor::Arrow);
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

    static constexpr const char* quantizeNames_[] = {
        "Off", "2 Bar", "1 Bar", "1/2", "1/4", "1/8", "1/16", "1/32", "1/4T", "1/8T", "1/16T"
    };

    int quantize_ = 0;
    bool visible_ = false;
    float mouseX_ = 0, mouseY_ = 0;
    float mouseDownX_ = 0, mouseDownY_ = 0;
    bool mouseInside_ = false;
    bool mouseIsDown_ = false;
    bool dragActive_ = false;
    int dragStartValue_ = 0;
};
