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
using namespace ui::animation;
using namespace skia;

constexpr SkColor default_bg_color = ColorFromARGB(0, 255, 255, 255);
constexpr SkColor hover_bg_color = ColorFromARGB(62, 255, 255, 255);
constexpr SkColor active_bg_color = ColorFromARGB(182, 255, 255, 255);
constexpr SkColor active_border_color = ColorFromARGB(108, 255, 255, 255);
constexpr SkColor default_text_color = ColorFromARGB(176, 24, 31, 42);
constexpr SkColor hover_text_color = ColorFromARGB(238, 18, 24, 34);
constexpr SkColor active_text_color = ColorFromARGB(255, 204, 45, 86);
constexpr SkColor active_mark_color = ColorFromARGB(255, 255, 76, 119);
constexpr SkColor pressed_bg_color = ColorFromARGB(214, 255, 255, 255);

export namespace ui::widgets {

class MenuButton : public Box {
public:
  Signal<const std::string &> clicked{}; // 点击信号

  // 创建菜单按钮
  explicit MenuButton(std::string_view text, std::string_view icon_path, Widget *parent = nullptr);
  // 更新图标和文字布局
  void layoutChildren() override;
  // 绘制按钮背景、图标和文字
  void paint(SkCanvas *canvas) override;

  // 设置当前菜单项是否激活
  void setActive(bool active);

  // 判断菜单项是否激活
  [[nodiscard]] bool isActive() const noexcept {
    return is_active;
  }

  // 设置菜单项ID
  void setId(const std::string_view mid) {
    id = mid;
  }

protected:
  // 鼠标进入时启动悬浮动效
  void onMouseEnter(float x, float y) override;
  // 鼠标离开时还原悬浮动效
  void onMouseLeave(float x, float y) override;
  // 鼠标按下时启动按压动效
  void onMouseLeftPressed(float x, float y) override;
  // 鼠标释放时触发点击信号
  void onMouseLeftReleased(float x, float y) override;

private:
  // 设置背景颜色
  void setBackgroundColor(SkColor color);
  // 设置文字颜色
  void setTextColor(SkColor color);
  // 设置悬浮动画进度
  void setHoverT(float t) noexcept { hover_t = t; }
  // 设置按压动画进度
  void setPressT(float t) noexcept { press_t = t; }
  // 设置激活动画进度
  void setActiveT(float t) noexcept { active_t = t; }

  SkColor current_bg = default_bg_color;           // 当前背景颜色
  SkColor current_text_color = default_text_color; // 当前字体颜色
  RenderText render_text;                          // 字体节点
  RenderSvg render_svg;                            // svg节点
  std::string id = utils::uuid::generate();        // menu_button的uuid
  bool is_active = false;                          // 是否是活跃状态
  float hover_t = 0.0f;                            // 悬浮动画进度
  float press_t = 0.0f;                            // 按压动画进度
  float active_t = 0.0f;                           // 激活动画进度
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

  radius = 12;

  const auto &textBound = render_text.textBound();
  resize(textBound.width() + 60, textBound.height() + 18);
  render_text.setPadding(Insets::fromLeft(48)); // 文字距左 48px
}

void MenuButton::layoutChildren() {
  render_bg.update(borderRect());
  render_border.update(borderRect());
  render_svg.update(borderRect());
  render_text.update(contentRect());
}

void MenuButton::paint(SkCanvas *canvas) {
  canvas->save();
  const float scale = 1.0f - press_t * 0.018f;
  canvas->translate(width_ * 0.5f, height_ * 0.5f);
  canvas->scale(scale, scale);
  canvas->translate(-width_ * 0.5f, -height_ * 0.5f);

  if (hover_t > 0.01f || active_t > 0.01f) {
    constexpr float sigma = 7.0f;
    auto glow_rect = borderRect().makeInset(5.0f, 5.0f);
    auto layer_bounds = glow_rect.makeOutset(sigma * 3.0f, sigma * 3.0f);
    SkPaint layer;
    layer.setImageFilter(SkImageFilters::Blur(sigma, sigma, nullptr));
    SkPaint glow;
    glow.setAntiAlias(true);
    glow.setColor(ColorFromARGB(static_cast<U8CPU>(18.0f * std::max(hover_t, active_t)), 255, 255, 255));
    canvas->saveLayer(&layer_bounds, &layer);
    canvas->drawRoundRect(glow_rect, radius, radius, glow);
    canvas->restore();
  }

  render_bg.render(canvas);
  if (active_t > 0.01f) {
    SkPaint mark;
    mark.setAntiAlias(true);
    mark.setColor(active_mark_color);
    mark.setAlphaf(active_t);
    canvas->drawRoundRect(SkRect::MakeXYWH(7.0f, height_ * 0.5f - 10.0f, 3.0f, 20.0f), 1.5f, 1.5f, mark);
    render_border.render(canvas);
  }
  render_text.render(canvas);
  render_svg.render(canvas);
  canvas->restore();
}

void MenuButton::setActive(const bool active) {
  if (is_active == active) {
    return;
  }

  is_active = active;
  if (is_active) {
    startAnimation<&MenuButton::setBackgroundColor>(current_bg, active_bg_color, 220, CubicBezier::EaseOut());
    startAnimation<&MenuButton::setTextColor>(current_text_color, active_text_color, 180, CubicBezier::EaseOut());
    startAnimation<&MenuButton::setActiveT>(active_t, 1.0f, 240, CubicBezier::EaseOut());
  } else {
    startAnimation<&MenuButton::setBackgroundColor>(current_bg, default_bg_color, 220, CubicBezier::EaseOut());
    startAnimation<&MenuButton::setTextColor>(current_text_color, default_text_color, 180, CubicBezier::EaseOut());
    startAnimation<&MenuButton::setActiveT>(active_t, 0.0f, 220, CubicBezier::EaseOut());
  }
}

void MenuButton::onMouseEnter(float, float) {
  window()->setCursor(glfw::CursorType::Hand);
  startAnimation<&MenuButton::setHoverT>(hover_t, 1.0f, 160.0f, CubicBezier::EaseOut());
  if (!is_active) {
    startAnimation<&MenuButton::setBackgroundColor>(current_bg, hover_bg_color, 180, CubicBezier::EaseOut());
    startAnimation<&MenuButton::setTextColor>(current_text_color, hover_text_color, 150, CubicBezier::EaseOut());
  }
}

void MenuButton::onMouseLeave(float, float) {
  window()->setCursor(glfw::CursorType::Arrow);
  startAnimation<&MenuButton::setHoverT>(hover_t, 0.0f, 220.0f, CubicBezier::EaseOut());
  startAnimation<&MenuButton::setPressT>(press_t, 0.0f, 140.0f, CubicBezier::EaseOut());
  if (!is_active) {
    startAnimation<&MenuButton::setBackgroundColor>(current_bg, default_bg_color, 220, CubicBezier::EaseOut());
    startAnimation<&MenuButton::setTextColor>(current_text_color, default_text_color, 180, CubicBezier::EaseOut());
  }
}

void MenuButton::onMouseLeftPressed(float, float) {
  startAnimation<&MenuButton::setPressT>(press_t, 1.0f, 65.0f, CubicBezier::EaseOut());
  if (!is_active) {
    startAnimation<&MenuButton::setBackgroundColor>(current_bg, pressed_bg_color, 90.0f, CubicBezier::EaseOut());
  }
}

void MenuButton::onMouseLeftReleased(float, float) {
  startAnimation<&MenuButton::setPressT>(press_t, 0.0f, 180.0f, CubicBezier::EaseOut());
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
