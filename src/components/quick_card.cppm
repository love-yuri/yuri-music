//
// Created by yuri on 2026/4/8.
//
export module components:quick_card;

import std;
import ui;
import core;
import skia;
import glfw.api;

using namespace ui::render;
using namespace ui::layout;
using namespace ui::widgets;
using namespace ui::animation;
using namespace skia;

// 卡片背景色
constexpr SkColor kCardBgColor = ColorFromARGB(255, 248, 248, 248);
// 悬浮背景色
constexpr SkColor kCardHoverColor = ColorFromARGB(255, 238, 238, 248);
// 按下背景色
constexpr SkColor kCardPressColor = ColorFromARGB(255, 226, 226, 240);
// 副标题颜色
constexpr SkColor kSubtitleColor = ColorFromARGB(255, 140, 140, 140);

// 图标尺寸
static constexpr float kIconSize = 46.0f;
// 图标圆角
static constexpr float kIconRadius = 8.0f;

export namespace components {

class QuickCard : public Box {
public:
  Signal<> clicked; // 点击事件

  QuickCard(std::string_view title,
            std::string_view subtitle,
            SkColor icon_color,
            Widget *parent = nullptr);

  void layoutChildren() override;
  void paint(SkCanvas *canvas) override;

protected:
  void onMouseEnter(float x, float y) override;
  void onMouseLeave(float x, float y) override;
  void onMouseLeftPressed(float x, float y) override;
  void onMouseLeftReleased(float x, float y) override;

private:
  /**
   * 设置背景色
   */
  void setBackgroundColor(SkColor color) noexcept;

  /**
   * 设置视觉Y偏移（用于上浮动画）
   */
  void setOffsetY(float offset) noexcept;

  RenderBackground icon_bg;        // icon 背景
  float icon_radius = kIconRadius; // icon 圆角大小
  RenderText title_text;           // 标题
  RenderText subtitle_text;        // 子标题
  SkColor bg_color = kCardBgColor; // 当前背景色
  float offset_y_ = 0.f;             // 视觉Y偏移（上浮动画用）
  bool is_pressed = false;         // 是否正在被按下
};

QuickCard::QuickCard(const std::string_view title,
                     const std::string_view subtitle,
                     const SkColor icon_color,
                     Widget *parent) :

  Box(parent), title_text(title), subtitle_text(subtitle) {

  // 卡片背景
  render_bg.setColor(kCardBgColor);
  render_border.setWidth(1.4);
  render_border.setColor(skia_colors::light_gray);
  radius = 8;

  // 图标色块
  icon_bg.setColor(icon_color);
  icon_bg.radius = &icon_radius;

  // 标题
  title_text.setFontSize(14);
  title_text.setColor(skia_colors::black);
  title_text.setAlignment(Alignment::CenterLeft);

  // 副标题
  subtitle_text.setFontSize(12);
  subtitle_text.setColor(kSubtitleColor);
  subtitle_text.setAlignment(Alignment::CenterLeft);

  // 固定高度
  setMaxHeight(72);
  setMinHeight(72);
  setPadding(Insets::fromLeft(16));
}

void QuickCard::onMouseEnter(float, float) {
  is_pressed = false;
  render_border.setColor(skia_colors::pink);
  window()->setCursor(glfw::CursorType::Hand);
  startAnimation<&QuickCard::setBackgroundColor>(bg_color, kCardHoverColor, 150.0f);
  startAnimation<&QuickCard::setOffsetY>(offset_y_, -4.f, 150.0f);
}

void QuickCard::onMouseLeave(float, float) {
  is_pressed = false;
  render_border.setColor(skia_colors::light_gray);
  window()->setCursor(glfw::CursorType::Arrow);
  startAnimation<&QuickCard::setBackgroundColor>(bg_color, kCardBgColor, 150.0f, CubicBezier::EaseOut());
  startAnimation<&QuickCard::setOffsetY>(offset_y_, 0.f, 150.0f);
}

void QuickCard::onMouseLeftPressed(float, float) {
  is_pressed = true;
  startAnimation<&QuickCard::setBackgroundColor>(bg_color, kCardPressColor, 100.0f, CubicBezier::EaseOut());
}

void QuickCard::onMouseLeftReleased(float, float) {
  is_pressed = false;
  clicked.emit();
  startAnimation<&QuickCard::setBackgroundColor>(bg_color, kCardHoverColor, 100.0f);
}

void QuickCard::setBackgroundColor(const SkColor color) noexcept {
  bg_color = color;
  render_bg.setColor(color);
}

void QuickCard::setOffsetY(const float offset) noexcept {
  offset_y_ = offset;
}

void QuickCard::layoutChildren() {
  const auto rect = contentRect();
  const float pad_left = padding_.left;
  const float pad_top = padding_.top;

  // 背景
  render_bg.setColor(bg_color);
  render_bg.update(borderRect());
  render_border.update(borderRect());

  // 图标：左侧垂直居中
  const float icon_x = pad_left;
  const float icon_y = pad_top + (rect.height() - kIconSize) * 0.5f;
  icon_bg.update(SkRect::MakeXYWH(icon_x, icon_y, kIconSize, kIconSize));

  // 文字区：图标右边间隔 16px
  const float text_x = icon_x + kIconSize + 16.0f;
  const float text_w = rect.width() - text_x;
  const float center_y = pad_top + (rect.height() - 36.0f) * 0.5f; // 标题+副标题总高约36

  title_text.update(SkRect::MakeXYWH(text_x, center_y, text_w, 18.0f));
  subtitle_text.update(SkRect::MakeXYWH(text_x, center_y + 20.0f, text_w, 16.0f));
}

void QuickCard::paint(SkCanvas *canvas) {
  canvas->save();
  canvas->translate(0, offset_y_);
  render_bg.render(canvas);
  icon_bg.render(canvas);
  title_text.render(canvas);
  subtitle_text.render(canvas);
  render_border.render(canvas);
  canvas->restore();
}

} // namespace components
