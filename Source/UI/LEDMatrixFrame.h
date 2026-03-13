#pragma once

#include <visage_ui/frame.h>
#include <visage_graphics/canvas.h>
#include <visage_graphics/font.h>
#include <array>
#include <cstdint>
#include <cmath>

namespace visage::fonts { extern ::visage::EmbeddedFile Lato_Regular_ttf; }

class LEDMatrixFrame : public visage::Frame {
public:
    LEDMatrixFrame() : Frame("LEDMatrix") {}

    void setPatterns(const std::array<uint8_t, 32>& bd,
                     const std::array<uint8_t, 32>& sd,
                     const std::array<uint8_t, 32>& hh) {
        bdPattern_ = bd;
        sdPattern_ = sd;
        hhPattern_ = hh;
        redraw();
    }

    void setDensities(float bd, float sd, float hh) {
        bdDensity_ = bd;
        sdDensity_ = sd;
        hhDensity_ = hh;
        redraw();
    }

    void setVelocityRanges(float bd, float sd, float hh) {
        bdVelocity_ = bd;
        sdVelocity_ = sd;
        hhVelocity_ = hh;
        redraw();
    }

    void setCurrentStep(int step) {
        if (currentStep_ != step) {
            currentStep_ = step;
            redraw();
        }
    }

    // Euclidean mode: per-instrument playhead steps and cycle lengths
    void setEuclideanMode(bool enabled, int bdStep, int sdStep, int hhStep,
                          int bdLength, int sdLength, int hhLength) {
        euclideanMode_ = enabled;
        eucBdStep_ = bdStep;
        eucSdStep_ = sdStep;
        eucHhStep_ = hhStep;
        eucBdLength_ = bdLength;
        eucSdLength_ = sdLength;
        eucHhLength_ = hhLength;
        redraw();
    }

    void triggerResetAnimation(bool retrigger) {
        resetAnimProgress_ = 1.0f;
        resetIsRetrigger_ = retrigger;
        redraw();
    }

    void draw(visage::Canvas& canvas) override {
        float w = width();
        float h = height();
        visage::Font font(9.0f, visage::fonts::Lato_Regular_ttf);

        // Background
        canvas.setColor(0xff0a0a0a);
        canvas.roundedRectangle(0, 0, w, h, 6.0f);

        // Border
        canvas.setColor(0xff202020);
        canvas.roundedRectangleBorder(0, 0, w, h, 6.0f, 1.0f);

        // Layout constants
        float labelWidth = 25.0f;
        float startX = labelWidth + 10.0f;
        float rightPad = 10.0f;
        float groupGap = 8.0f;
        float totalWidth = w - startX - rightPad;
        float usableWidth = totalWidth - 3.0f * groupGap;
        float stepSpacing = usableWidth / 32.0f;
        float ledSize = std::min(stepSpacing * 0.8f, 10.0f);
        float barNumHeight = 14.0f;
        float topY = barNumHeight + ledSize / 2.0f;
        float bottomY = h - ledSize / 2.0f - 6.0f;
        float midY = (topY + bottomY) / 2.0f;

        // Density thresholds
        uint8_t bdThresh = static_cast<uint8_t>(255 * (1.0f - bdDensity_));
        uint8_t sdThresh = static_cast<uint8_t>(255 * (1.0f - sdDensity_));
        uint8_t hhThresh = static_cast<uint8_t>(255 * (1.0f - hhDensity_));

        // Row text labels
        canvas.setColor(0xffcc2222);
        canvas.text("BD", font, visage::Font::kLeft, 4, topY - 6, labelWidth, 12);
        canvas.setColor(0xff22cc22);
        canvas.text("SD", font, visage::Font::kLeft, 4, midY - 6, labelWidth, 12);
        canvas.setColor(0xffcccc22);
        canvas.text("HH", font, visage::Font::kLeft, 4, bottomY - 6, labelWidth, 12);

        // Group separators and bar numbers
        visage::Font barFont(8.0f, visage::fonts::Lato_Regular_ttf);
        for (int g = 0; g < 4; ++g) {
            float groupStartX = startX + g * (8 * stepSpacing + groupGap);

            canvas.setColor(0xff555555);
            const char* barNums[] = { "1", "2", "3", "4" };
            canvas.text(barNums[g], barFont, visage::Font::kCenter,
                        groupStartX, 2, 8 * stepSpacing, 10);

            if (g > 0) {
                float sepX = groupStartX - groupGap / 2.0f;
                canvas.setColor(0x30ffffff);
                canvas.segment(sepX, topY - ledSize,
                               sepX, bottomY + ledSize,
                               0.5f, false);
            }
        }

        // Draw LEDs
        for (int step = 0; step < 32; ++step) {
            int group = step / 8;
            int stepInGroup = step % 8;
            float lx = startX + group * (8 * stepSpacing + groupGap)
                      + stepInGroup * stepSpacing + (stepSpacing - ledSize) / 2;

            // Determine if each instrument is in the disabled zone (beyond cycle length)
            bool bdDisabled = euclideanMode_ && step >= eucBdLength_;
            bool sdDisabled = euclideanMode_ && step >= eucSdLength_;
            bool hhDisabled = euclideanMode_ && step >= eucHhLength_;

            bool bdOn = !bdDisabled && bdPattern_[step] > bdThresh;
            bool sdOn = !sdDisabled && sdPattern_[step] > sdThresh;
            bool hhOn = !hhDisabled && hhPattern_[step] > hhThresh;

            bool bdAccent = bdPattern_[step] > 200 && bdOn;
            bool sdAccent = sdPattern_[step] > 200 && sdOn;
            bool hhAccent = hhPattern_[step] > 200 && hhOn;

            // Per-instrument current step: in Euclidean mode each has its own playhead
            bool bdCurrent, sdCurrent, hhCurrent;
            if (euclideanMode_) {
                bdCurrent = !bdDisabled && (step == eucBdStep_);
                sdCurrent = !sdDisabled && (step == eucSdStep_);
                hhCurrent = !hhDisabled && (step == eucHhStep_);
            } else {
                bdCurrent = (step == currentStep_);
                sdCurrent = (step == currentStep_);
                hhCurrent = (step == currentStep_);
            }

            // Reset animation glow
            float resetGlow = 0.0f;
            if (resetAnimProgress_ > 0.0f) {
                if (resetIsRetrigger_) {
                    float sweepPos = (1.0f - resetAnimProgress_) * 32.0f;
                    float dist = std::abs(static_cast<float>(step) - sweepPos);
                    if (dist < 4.0f)
                        resetGlow = (1.0f - dist / 4.0f) * resetAnimProgress_;
                } else {
                    resetGlow = resetAnimProgress_ * 0.5f;
                }
            }

            // Velocity brightness
            float bdVelBright = velocityBrightness(bdAccent, bdVelocity_);
            float sdVelBright = velocityBrightness(sdAccent, sdVelocity_);
            float hhVelBright = velocityBrightness(hhAccent, hhVelocity_);

            // BD LED
            if (bdDisabled)
                drawDisabledLED(canvas, lx, topY - ledSize / 2, ledSize);
            else
                drawLED(canvas, lx, topY - ledSize / 2, ledSize,
                        bdOn, bdAccent, bdCurrent, resetGlow, bdVelBright,
                        0xff331111, 0xffcc2222, 0xffff4444);

            // SD LED
            if (sdDisabled)
                drawDisabledLED(canvas, lx, midY - ledSize / 2, ledSize);
            else
                drawLED(canvas, lx, midY - ledSize / 2, ledSize,
                        sdOn, sdAccent, sdCurrent, resetGlow, sdVelBright,
                        0xff113311, 0xff22cc22, 0xff44ff44);

            // HH LED
            if (hhDisabled)
                drawDisabledLED(canvas, lx, bottomY - ledSize / 2, ledSize);
            else
                drawLED(canvas, lx, bottomY - ledSize / 2, ledSize,
                        hhOn, hhAccent, hhCurrent, resetGlow, hhVelBright,
                        0xff333311, 0xffcccc22, 0xffffff44);
        }

        // Decay reset animation
        if (resetAnimProgress_ > 0.0f) {
            resetAnimProgress_ -= 0.08f;
            if (resetAnimProgress_ < 0.0f)
                resetAnimProgress_ = 0.0f;
        }
    }

private:
    float velocityBrightness(bool isAccent, float velocityRange) {
        float minVel = 80.0f - (velocityRange * 40.0f);
        float maxVel = 100.0f + (velocityRange * 27.0f);
        float normalVel = (minVel + maxVel) / 2.0f;
        float vel = isAccent ? maxVel : normalVel;
        return 0.55f + 0.45f * (vel / 127.0f);
    }

    unsigned int scaleColor(unsigned int color, float brightness) {
        unsigned int r = static_cast<unsigned int>(((color >> 16) & 0xff) * brightness);
        unsigned int g = static_cast<unsigned int>(((color >> 8) & 0xff) * brightness);
        unsigned int b = static_cast<unsigned int>((color & 0xff) * brightness);
        r = std::min(r, 255u);
        g = std::min(g, 255u);
        b = std::min(b, 255u);
        return (0xff << 24) | (r << 16) | (g << 8) | b;
    }

    // Disabled/inactive zone LED — same grey as stepper buttons to clearly show "beyond cycle length"
    void drawDisabledLED(visage::Canvas& canvas, float x, float y, float size) {
        canvas.setColor(0xff444444);
        canvas.circle(x, y, size);
    }

    void drawLED(visage::Canvas& canvas, float x, float y, float size,
                 bool on, bool accent, bool current, float resetGlow,
                 float velBrightness,
                 unsigned int offColor, unsigned int onColor, unsigned int accentColor) {
        if (on) {
            canvas.setColor(0x30000000);
            canvas.circle(x + 1, y + 1, size);
        }

        unsigned int fillColor = offColor;
        if (accent)
            fillColor = scaleColor(accentColor, velBrightness);
        else if (on)
            fillColor = scaleColor(onColor, velBrightness);
        canvas.setColor(fillColor);
        canvas.circle(x, y, size);

        if (on) {
            unsigned int hlAlpha = static_cast<unsigned int>(0x40 * velBrightness);
            canvas.setColor((hlAlpha << 24) | 0x00ffffff);
            canvas.circle(x + 2, y + 1, size * 0.5f);
        }

        if (resetGlow > 0.0f) {
            unsigned int alpha = static_cast<unsigned int>(resetGlow * 120);
            canvas.setColor((alpha << 24) | 0x00ffffff);
            canvas.circle(x - 1, y - 1, size + 2);
        }

        if (current) {
            if (on) {
                unsigned int glowColor = (accentColor & 0x00ffffff) | 0x30000000;
                canvas.setColor(glowColor);
                canvas.circle(x - 2, y - 2, size + 4);
            }

            canvas.setColor(0xb0ffffff);
            float outerInset = -1.5f;
            canvas.circle(x + outerInset, y + outerInset, size - outerInset * 2);

            canvas.setColor(fillColor);
            canvas.circle(x, y, size);

            if (on) {
                unsigned int hlAlpha = static_cast<unsigned int>(0x40 * velBrightness);
                canvas.setColor((hlAlpha << 24) | 0x00ffffff);
                canvas.circle(x + 2, y + 1, size * 0.5f);
            }
        }
    }

    std::array<uint8_t, 32> bdPattern_{};
    std::array<uint8_t, 32> sdPattern_{};
    std::array<uint8_t, 32> hhPattern_{};
    float bdDensity_ = 0.5f;
    float sdDensity_ = 0.5f;
    float hhDensity_ = 0.5f;
    float bdVelocity_ = 0.5f;
    float sdVelocity_ = 0.5f;
    float hhVelocity_ = 0.5f;
    int currentStep_ = -1;

    // Euclidean mode state
    bool euclideanMode_ = false;
    int eucBdStep_ = -1;
    int eucSdStep_ = -1;
    int eucHhStep_ = -1;
    int eucBdLength_ = 32;
    int eucSdLength_ = 32;
    int eucHhLength_ = 32;

    float resetAnimProgress_ = 0.0f;
    bool resetIsRetrigger_ = false;
};
