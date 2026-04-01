//
// Created by yuri on 2026/2/7.
//
export module components:menu_button;

import std;
import ui;
import skia;
import core;
import yuri_log;
import glfw.api;

using namespace ui::render;
using namespace ui::layout;
using namespace skia;

// 浅色主题颜色常量
constexpr SkColor default_bg_color = ColorFromARGB(0, 255, 255, 255);    // 透明
constexpr SkColor hover_bg_color = ColorFromARGB(20, 0, 0, 0);           // rgba(0,0,0,0.08)
constexpr SkColor active_bg_color = ColorFromARGB(26, 139, 92, 246);     // rgba(139, 92, 246, 0.1)
constexpr SkColor active_border_color = ColorFromARGB(77, 139, 92, 246); // rgba(139, 92, 246, 0.3)
constexpr SkColor default_text_color = ColorFromARGB(166, 0, 0, 0);      // rgba(0,0,0,0.65)
constexpr SkColor hover_text_color = ColorFromARGB(217, 0, 0, 0);        // rgba(0,0,0,0.85)
constexpr SkColor active_text_color = ColorFromARGB(255, 139, 92, 246);  // #8b5cf6

export namespace ui::widgets {

class MenuButton : public Box {
public:
  Signal<const std::string &> clicked{};
  explicit MenuButton(std::string_view text, std::string_view icon_path, Widget *parent = nullptr);
  void layoutChildren() override;
  void paint(SkCanvas *canvas) override;

  void setActive(bool active);

  [[nodiscard]] bool isActive() const noexcept {
    return is_active;
  }

  void setId(const std::string_view mid) {
    id = mid;
  }

protected:
  void onMouseEnter(float x, float y) override;
  void onMouseLeave(float x, float y) override;
  void onMouseLeftReleased(float x, float y) override;

private:
  void setBackgroundColor(SkColor color);
  void setTextColor(SkColor color);

  SkColor current_bg = default_bg_color;           // 当前背景颜色
  SkColor current_text_color = default_text_color; // 当前字体颜色
  RenderText render_text;                          // 字体节点
  RenderSvg render_svg;                            // svg节点
  std::string id = utils::uuid::generate();        // menu_button的uuid
  bool is_active = false;                          // 是否是活跃状态
};

MenuButton::MenuButton(const std::string_view text,
                       const std::string_view icon_path,
                       Widget *parent) : Box(parent), render_text(text), render_svg(icon_path) {
  render_text.setTextAndAlignment(text, Alignment::CenterLeft);
  render_text.setColor(current_text_color);
  render_bg.setColor(current_bg);
  render_border.setColor(active_border_color);
  render_border.setWidth(1);

  render_svg.setAlignment(Alignment::CenterLeft);
  render_svg.setPadding(Insets::fromLeft(14)); // 图标距左 14px

  radius = 8;

  const auto &textBound = render_text.textBound();
  resize(textBound.width() + 60, textBound.height() + 18);
  setPadding(Insets::fromLeft(48)); // 文字距左 48px
}

void MenuButton::layoutChildren() {
  render_bg.update(borderRect());
  render_border.update(borderRect());
  render_svg.update(borderRect());
  render_text.update(contentRect());
}

void MenuButton::paint(SkCanvas *canvas) {
  render_bg.render(canvas);
  if (is_active) {
    render_border.render(canvas);
  }
  render_text.render(canvas);
  render_svg.render(canvas);
}

void MenuButton::setActive(const bool active) {
  if (is_active == active) return;
  is_active = active;
  if (is_active) {
    current_bg = active_bg_color;
    current_text_color = active_text_color;
  } else {
    current_bg = default_bg_color;
    current_text_color = default_text_color;
  }
  render_bg.setColor(current_bg);
  render_text.setColor(current_text_color);
}

void MenuButton::onMouseEnter(float x, float y) {
  window()->setCursor(glfw::CursorType::Hand);
  if (!is_active) {
    setBackgroundColor(hover_bg_color);
    setTextColor(hover_text_color);
  }
}

void MenuButton::onMouseLeave(float x, float y) {
  window()->setCursor(glfw::CursorType::Arrow);
  if (!is_active) {
    setBackgroundColor(default_bg_color);
    setTextColor(default_text_color);
  }
}

void MenuButton::onMouseLeftReleased(float x, float y) {
  setActive(!is_active);
  clicked.emit(id);
}

void MenuButton::setBackgroundColor(const SkColor color) {
  current_bg = color;
  render_bg.setColor(color);
}

void MenuButton::setTextColor(const SkColor color) {
  current_text_color = color;
  render_text.setColor(color);
}

} // namespace ui::widgets
