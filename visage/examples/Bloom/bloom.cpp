/* Copyright Vital Audio, LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <visage/app.h>
#include <visage_widgets/graph_line.h>

class AnimatedLine : public visage::Frame {
public:
  static constexpr int kNumPoints = 1200;
  static constexpr float kDotRadius = 4.0f;

  static inline float quickSin1(float phase) {
    phase = 0.5f - phase;
    return phase * (8.0f - 16.0f * fabsf(phase));
  }

  static inline float sin1(float phase) {
    float approx = quickSin1(phase - floorf(phase));
    return approx * (0.776f + 0.224f * fabsf(approx));
  }

  AnimatedLine() : graph_line_(kNumPoints) {
    addChild(&graph_line_);
    setIgnoresMouseEvents(true, false);
  }

  void resized() override { graph_line_.setBounds(0, 0, width(), height()); }

  void draw(visage::Canvas& canvas) override {
    static constexpr int kNumDots = 10;

    double render_time = canvas.time();
    int render_height = graph_line_.height();
    int render_width = graph_line_.width();
    int line_height = render_height * 0.3f;
    int offset = render_height * 0.5f;

    float position = 0.0f;
    float boost_time = render_time * 0.2f;
    double phase = render_time * 0.5;
    float boost_phase = (boost_time - floor(boost_time)) * 1.5f - 0.25f;

    auto compute_boost = [](float dist) { return std::max(0.0f, 1.0f - 8.0f * std::abs(dist)); };

    for (int i = 0; i < kNumPoints; ++i) {
      float t = 1.1f * i / (kNumPoints - 1.0f) - 0.05f;
      float delta = std::min(t, 1.0f - t);
      position += 0.02f * delta * delta + 0.003f;
      graph_line_.setXAt(i, t * render_width);
      graph_line_.setYAt(i, offset + sin1(phase + position) * 0.5f * line_height);
      graph_line_.setBoostAt(i, compute_boost(boost_phase - t));
    }

    float center_y = (render_height - line_height) * 0.25f;
    visage::Color color = 0xffaa88ff;
    for (int i = 0; i < kNumDots; ++i) {
      float t = (i + 1) / (kNumDots + 1.0f);
      float center_x = t * render_width;

      color.setHdr(1.0f + compute_boost(boost_phase - t));
      canvas.setColor(color);
      canvas.circle(center_x - kDotRadius, center_y - kDotRadius, kDotRadius * 2.0f);
      canvas.circle(center_x - kDotRadius, render_height - center_y - kDotRadius, kDotRadius * 2.0f);
    }

    redraw();
  }

private:
  visage::GraphLine graph_line_;
};

class ExampleEditor : public visage::ApplicationWindow {
public:
  ExampleEditor() {
    bloom_.setBloomSize(40.0f);
    bloom_.setBloomIntensity(1.0f);
    setPostEffect(&bloom_);
    addChild(&animated_line_);
    animated_line_.layout().setMargin(0);

    onDraw() = [this](visage::Canvas& canvas) {
      canvas.setColor(0xff22282d);
      canvas.fill(0, 0, width(), height());
    };

    setPalette(&palette_);
    visage::Brush rainbow = visage::Brush::horizontal(visage::Gradient(0xffff6666, 0xffffff66,
                                                                       0xff66ff66, 0xff66ffff, 0xff6666ff,
                                                                       0xffff66ff, 0xffff6666));
    palette_.setColor(visage::GraphLine::LineColor, rainbow);
    palette_.setValue(visage::GraphLine::LineWidth, 3.0f);
    palette_.setValue(visage::GraphLine::LineColorBoost, 0.8f);
  }

private:
  visage::Palette palette_;
  visage::BloomPostEffect bloom_;
  AnimatedLine animated_line_;
};

int runExample() {
  ExampleEditor editor;
  editor.setWindowDecoration(visage::Window::Decoration::Client);
  if (visage::isMobileDevice())
    editor.showMaximized();
  else
    editor.show(visage::Dimension::widthPercent(50.0f), visage::Dimension::widthPercent(14.0f));

  editor.runEventLoop();

  return 0;
}
