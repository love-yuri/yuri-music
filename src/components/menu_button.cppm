//
// Created by yuri on 2026/2/7.
//
module;
#if defined(_WIN32)
// ReSharper disable once CppUnusedIncludeDirective
#include "include/private/base/SkFloatingPoint.h"
#endif
export module components:menu_button;

import std;
import ui;
import skia;
import signal;
import yuri_log;
import glfw.api;

using namespace ui::render;
using namespace ui::layout;
using namespace skia;

export namespace ui::widgets {

class MenuButton : public Box {

public:
  Signal<std::string&> itemClicked;
  explicit MenuButton(std::string_view text, Widget* parent = nullptr);
  void layoutChildren() override;

protected:
  void paint(SkCanvas *canvas) override;
  void onMouseEnter(float x, float y) override;
  void onMouseLeave(float x, float y) override;

private:
  void setBackground(SkColor color);

  SkColor current_bg = skia_colors::white;   // 当前背景色
  RenderText render_text; // 渲染字体
  RenderSvg render_svg;   // 渲染svg
};

MenuButton::MenuButton(const std::string_view text, Widget* parent)
  : Box(parent), render_text(text), render_svg(R"(E:\love-yuri\pixel-journey\test\yuri-music\resources\svg\home.svg)") {
  render_text.setTextAndAlignment(text, Alignment::CenterLeft);
  render_bg.setColor(current_bg);
  render_border.setColor(skia_colors::light_gray);
  render_border.setWidth(2);
  radius = 8;

  const auto& textBound = render_text.textBound();
  resize(textBound.width() + 48, textBound.height() + 12);
  setPadding(Insets::fromLeft(66));
}

void MenuButton::layoutChildren() {
  render_text.update(contentRect());
  render_bg.update(borderRect());
  render_border.update(borderRect());
  render_svg.move(20, (contentHeight() - render_svg.height()) / 2);
}

void MenuButton::paint(SkCanvas* canvas) {
  render_bg.render(canvas);
  render_border.render(canvas);
  render_text.render(canvas);
  render_svg.render(canvas);
}

void MenuButton::onMouseEnter(float x, float y) {
  window()->setCursor(glfw::CursorType::Hand);
  startAnimation<&MenuButton::setBackground>(current_bg, skia_colors::light_pink, 200);
}

void MenuButton::onMouseLeave(float x, float y) {
  window()->setCursor(glfw::CursorType::Arrow);
  startAnimation<&MenuButton::setBackground>(current_bg, skia_colors::white, 200);
}

void MenuButton::setBackground(const SkColor color) {
  current_bg = color;
  render_bg.setColor(color);
}

} // namespace ui::widgets
