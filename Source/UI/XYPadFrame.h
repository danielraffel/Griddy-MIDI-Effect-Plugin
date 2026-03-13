#pragma once

#include <visage_ui/frame.h>
#include <visage_graphics/canvas.h>
#include <visage_graphics/font.h>
#include <functional>

namespace visage::fonts { extern ::visage::EmbeddedFile Lato_Regular_ttf; }

class XYPadFrame : public visage::Frame {
public:
    XYPadFrame() : Frame("XYPad") {}

    // Disabled state (for Euclidean mode)
    void setDisabled(bool d) { if (disabled_ != d) { disabled_ = d; redraw(); } }
    bool isDisabled() const { return disabled_; }

    // Current X/Y values (0.0 to 1.0)
    float getX() const { return xValue_; }
    float getY() const { return yValue_; }

    void setX(float x) {
        x = std::max(0.0f, std::min(1.0f, x));
        if (std::abs(xValue_ - x) > 0.001f) {
            xValue_ = x;
            redraw();
        }
    }

    void setY(float y) {
        y = std::max(0.0f, std::min(1.0f, y));
        if (std::abs(yValue_ - y) > 0.001f) {
            yValue_ = y;
            redraw();
        }
    }

    // Callback when user drags
    std::function<void()> onGestureStart;
    std::function<void()> onGestureEnd;
    std::function<void(float x, float y)> onValueChange;

    void draw(visage::Canvas& canvas) override {
        float w = width();
        float h = height();
        float pad = 15.0f;

        // Background
        canvas.setColor(0xff0a0a0a);
        canvas.roundedRectangle(0, 0, w, h, 8.0f);

        // Border
        canvas.setColor(0xff303030);
        canvas.roundedRectangleBorder(0, 0, w, h, 8.0f, 1.0f);

        // Active area
        float ax = pad;
        float ay = pad;
        float aw = w - pad * 2;
        float ah = h - pad * 2;

        // 5x5 grid lines
        canvas.setColor(0x20ffffff);
        for (int i = 1; i < 5; ++i) {
            float gx = ax + (aw * i / 5.0f);
            float gy = ay + (ah * i / 5.0f);
            canvas.segment(gx, ay, gx, ay + ah, 0.5f, false);
            canvas.segment(ax, gy, ax + aw, gy, 0.5f, false);
        }

        // Thumb position (Y inverted: 0=bottom, 1=top)
        float thumbX = ax + xValue_ * aw;
        float thumbY = ay + (1.0f - yValue_) * ah;

        if (!disabled_) {
            // Crosshair lines (brighter when dragging)
            canvas.setColor(dragging_ ? 0x60ff8833 : 0x30ffffff);
            canvas.segment(thumbX, ay, thumbX, ay + ah, 0.5f, false);
            canvas.segment(ax, thumbY, ax + aw, thumbY, 0.5f, false);

            float thumbR = 10.0f;

            // Outer glow when dragging (GPU-rendered radial effect)
            if (dragging_) {
                canvas.setColor(0x20ff8833);
                canvas.circle(thumbX - thumbR * 2.5f, thumbY - thumbR * 2.5f, thumbR * 5);
                canvas.setColor(0x30ff8833);
                canvas.circle(thumbX - thumbR * 1.5f, thumbY - thumbR * 1.5f, thumbR * 3);
            }

            // Thumb shadow
            canvas.setColor(0x80000000);
            canvas.circle(thumbX - thumbR + 1, thumbY - thumbR + 2, thumbR * 2);

            // Thumb (orange, brighter when dragging)
            canvas.setColor(dragging_ ? 0xffffaa44 : 0xffff8833);
            canvas.circle(thumbX - thumbR, thumbY - thumbR, thumbR * 2);

            // Inner glow
            canvas.setColor(0x40ffaa00);
            canvas.circle(thumbX - thumbR + 2, thumbY - thumbR + 2, thumbR * 2 - 4);

            // Thumb highlight
            canvas.setColor(0x60ffffff);
            canvas.circle(thumbX - thumbR * 0.5f, thumbY - thumbR * 0.6f, thumbR * 0.6f);
        }

        // Axis labels (hidden when disabled — overlay replaces them)
        if (!disabled_) {
        visage::Font font(9.0f, visage::fonts::Lato_Regular_ttf);
        canvas.setColor(0x60ffffff);
        canvas.text("X", font, visage::Font::kCenter, w - pad + 2, h / 2 - 6, pad - 2, 12);
        canvas.text("Y", font, visage::Font::kCenter, w / 2 - 6, 1, 12, pad - 2);
        }
    }

    void mouseDown(const visage::MouseEvent& e) override {
        if (disabled_ || dragging_)
            return;
        dragging_ = true;
        if (onGestureStart)
            onGestureStart();
        updateFromMouse(e);
    }

    void mouseUp(const visage::MouseEvent&) override {
        if (disabled_ || !dragging_)
            return;
        dragging_ = false;
        if (onGestureEnd)
            onGestureEnd();
        redraw();
    }

    void mouseDrag(const visage::MouseEvent& e) override {
        if (disabled_) return;
        updateFromMouse(e);
    }

private:
    void updateFromMouse(const visage::MouseEvent& e) {
        float pad = 15.0f;
        float aw = width() - pad * 2;
        float ah = height() - pad * 2;

        float newX = (e.position.x - pad) / aw;
        float newY = 1.0f - (e.position.y - pad) / ah;

        newX = std::max(0.0f, std::min(1.0f, newX));
        newY = std::max(0.0f, std::min(1.0f, newY));

        xValue_ = newX;
        yValue_ = newY;
        redraw();

        if (onValueChange)
            onValueChange(xValue_, yValue_);
    }

    float xValue_ = 0.5f;
    float yValue_ = 0.5f;
    bool dragging_ = false;
    bool disabled_ = false;
};
