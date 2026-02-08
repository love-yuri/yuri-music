//
// Created by yuri on 2026/2/7.
//

export module components:menu_button;

import std;
import ui;
import glfw;
import skia;


using namespace skia;
using namespace glfw;
using namespace ui::animation;
using namespace ui::widgets;
using namespace ui::layout;
using namespace ui::render;

export class MenuButton : public Widget {
public:
  explicit MenuButton(std::string_view label, Widget *parent);

protected:
  void paint(SkCanvas *canvas) override;
  void onMouseEnter(float x, float y) override;
  void onMouseLeave(float x, float y) override;
  void layoutChildren() override;

private:
  RenderBorder render_border;         // border
  RenderBackground render_background; // background
  RenderText render_text;             // text
  RenderSvg render_svg;               // svg
  float radius_ = 8;                  // radis
};

MenuButton::MenuButton(const std::string_view label, Widget *parent) :
  Widget(parent), render_border(&self_box),
  render_background(&self_box),
  render_text(label, &self_box),
  render_svg("/home/yuri/Downloads/file.svg") {
  render_text.setColor(SkColorSetARGB(255, 114, 119, 131));
  render_background.setColor(skia_colors::light_pink);
  render_background.setOpacity(0.1f);
  render_background.radius = &radius_;
  render_border.radius = &radius_;
  render_text.setAlignment(Alignment::Center);
}

void MenuButton::paint(SkCanvas *canvas) {
  render_background.render(canvas);
  render_text.render(canvas);
  render_svg.render(canvas);
}

void MenuButton::onMouseEnter(const float x, const float y) {
  window()->setCursor(CursorType::Hand);
  render_text.setColor(SkColorSetARGB(255, 238, 10, 36));
  const auto func = &memberThunk<RenderBackground, float, &RenderBackground::setOpacity>;
  animation_manager->start(0.f, 1.f, 200, &render_background, func);
}

void MenuButton::onMouseLeave(float x, float y) {
  window()->setCursor(CursorType::Arrow);
  render_text.setColor(SkColorSetARGB(255, 114, 119, 131));
  const auto func = &memberThunk<RenderBackground, float, &RenderBackground::setOpacity>;
  animation_manager->start(1.f, 0.1f, 200, &render_background, func);
}

void MenuButton::layoutChildren() {
  render_text.update();
}
