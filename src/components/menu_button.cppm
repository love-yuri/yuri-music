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

using namespace ui::render;
using namespace ui::layout;
using namespace skia;
using namespace skia::skia_colors;

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
  void setOpacity(float alpha);

  RenderText render_text; // 渲染字体
};

MenuButton::MenuButton(const std::string_view text, Widget* parent)
  : Box(parent), render_text(text, {}) {
  render_text.setTextAndAlignment(text, Alignment::CenterLeft);
  render_bg.setColor(light_pink);
  render_bg.setOpacity(0);
  render_border.setColor(light_gray);
  render_border.setWidth(2);
  radius = 4;

  const auto& textBound = render_text.textBound();
  resize(textBound.width() + 48, textBound.height() + 12);
  setPadding(Insets::fromLeft(6));
}

void MenuButton::layoutChildren() {
  render_text.update(contentRect());
  render_bg.update(borderRect());
  render_border.update(borderRect());
}

void MenuButton::paint(SkCanvas* canvas) {
  render_bg.render(canvas);
  render_border.render(canvas);
  render_text.render(canvas);
}

void MenuButton::onMouseEnter(float x, float y) {
  animation_manager->start<&MenuButton::setOpacity>(0, 1, 200, this);
}

void MenuButton::onMouseLeave(float x, float y) {
  animation_manager->start<&MenuButton::setOpacity>(1, 0, 200, this);
}

void MenuButton::setOpacity(const float alpha) {
  render_bg.setOpacity(alpha);
}

} // namespace ui::widgets
