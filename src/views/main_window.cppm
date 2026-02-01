//
// Created by yuri on 2026/2/1.
//

export module main_window;

import profiling;
import glfw;
import skia;
import std;
import ui;

using namespace skia;
using namespace glfw;
using namespace ui::widgets;
using namespace ui::layout;

export class MainWindow: public Window {
  using Window::Window;
public:
  MainWindow(): Window(800, 800) {
    setPadding(80);
    setLayout<VBoxLayout<Widget>>();

    const auto splitter = new Splitter(this);
    auto button = new Button("左侧", splitter);
    button = new Button("右侧", splitter);
    button->background().setColor(skia_colors::light_pink);
  }


protected:
  void paint(SkCanvas *canvas) override;
  void render(SkCanvas *canvas) override;

private:
  profiling::FpsCounter fpsCounter;   // fps统计
  SkPaint sk_paint = SkPaintBuilder() // paint
    .setColor(skia_colors::gray)
    .setAntiAlias(true)
    .setStrokeWidth(2)
    .build();
};

void MainWindow::paint(SkCanvas *canvas) {
  canvas->drawString(std::format("FPS: {:.1f}", fpsCounter.getFPS()).c_str(), 20, 30, font::default_font, sk_paint);
  canvas->drawString(std::format("current: {:.1f} {:.1f}", cursor_x, cursor_y).c_str(), 320, 30, font::default_font, sk_paint);
}

void MainWindow::render(SkCanvas *canvas) {
  fpsCounter.update();
  canvas->clear(skia_colors::white);
  Widget::render(canvas);
}