//
// Created by yuri on 2026/2/1.
//

export module main_window;

import components;
import profiling;
import yuri_log;
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
  Splitter* splitter_ = nullptr;
public:
  MainWindow(): Window(800, 800) {
    setPadding(80);
    // setLayout<VBoxLayout<Widget>>();

    auto button = new Button("恬淡晴天", this);
    button->clicked.connect<&MainWindow::onClicked>(this);
    button->background().setColor(skia_colors::pink);
    button->border().visible = false;

    auto menu_button = new MenuButton("哈哈哈", this);
    menu_button->setGeometry(100, 50, 200, 50);

  }

  void onClicked() {

  }

protected:
  void paint(SkCanvas *canvas) final;
  void render(SkCanvas *canvas) final;

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