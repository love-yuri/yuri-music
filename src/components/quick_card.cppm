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
using namespace ui::algorithm;
using namespace skia;

constexpr SkColor kCardBgColor = ColorFromARGB(154, 255, 255, 255);
constexpr SkColor kCardHoverColor = ColorFromARGB(212, 255, 255, 255);
constexpr SkColor kCardPressColor = ColorFromARGB(235, 248, 250, 252);
constexpr SkColor kCardBorder = ColorFromARGB(120, 255, 255, 255);
constexpr SkColor kQuickTitleColor = ColorFromARGB(255, 23, 31, 44);
constexpr SkColor kQuickSubtitleColor = ColorFromARGB(172, 58, 70, 88);
constexpr SkColor kShadowColor = ColorFromARGB(26, 25, 36, 52);

// 图标尺寸
static constexpr float kIconSize = 46.0f;
// 图标圆角
static constexpr float kIconRadius = 12.0f;

export namespace components {

class QuickCard : public Box {
public:
  Signal<> clicked; // 点击事件

  // 创建快捷入口卡片
  QuickCard(std::string_view title,
            std::string_view subtitle,
            SkColor icon_color,
            Widget *parent = nullptr);

  // 更新内部绘制节点布局
  void layoutChildren() override;
  // 绘制卡片背景、图标和文本
  void paint(SkCanvas *canvas) override;

protected:
  // 鼠标进入时启动悬浮动效
  void onMouseEnter(float x, float y) override;
  // 鼠标离开时还原悬浮动效
  void onMouseLeave(float x, float y) override;
  // 鼠标按下时启动按压动效
  void onMouseLeftPressed(float x, float y) override;
  // 鼠标释放时触发点击事件
  void onMouseLeftReleased(float x, float y) override;

private:
  // 设置背景色
  void setBackgroundColor(SkColor color) noexcept;
  // 设置悬浮动画进度
  void setHoverT(float t) noexcept { hover_t = t; }
  // 设置按压动画进度
  void setPressT(float t) noexcept { press_t = t; }

  RenderBackground icon_bg;        // icon 背景
  float icon_radius = kIconRadius; // icon 圆角大小
  RenderText title_text;           // 标题
  RenderText subtitle_text;        // 子标题
  SkColor bg_color = kCardBgColor;        // 当前背景色
  float offset_y = 0.f;                   // 视觉Y偏移（上浮动画用）
  float hover_t = 0.f;                    // 悬浮动画进度
  float press_t = 0.f;                    // 按压动画进度
  SkColor icon_color_ = skia_colors::black; // 图标主色
  bool is_pressed = false;                // 是否正在被按下
};

QuickCard::QuickCard(const std::string_view title,
                     const std::string_view subtitle,
                     const SkColor icon_color,
                     Widget *parent) :
  Box(parent), title_text(title), subtitle_text(subtitle), icon_color_(icon_color) {

  // 卡片背景
  render_bg.setColor(kCardBgColor);
  render_border.setWidth(1.0);
  render_border.setColor(kCardBorder);
  radius = 12;

  // 图标色块
  icon_bg.setColor(icon_color);
  icon_bg.radius = &icon_radius;

  // 标题
  title_text.setFontSize(14);
  title_text.setColor(kQuickTitleColor);
  title_text.setAlignment(Alignment::CenterLeft);

  // 副标题
  subtitle_text.setFontSize(12);
  subtitle_text.setColor(kQuickSubtitleColor);
  subtitle_text.setAlignment(Alignment::CenterLeft);

  // 固定高度
  setMaxHeight(78);
  setMinHeight(78);
  setPadding(Insets::fromLeft(16));
}

void QuickCard::onMouseEnter(float, float) {
  is_pressed = false;
  render_border.setColor(ColorFromARGB(160, 255, 255, 255));
  window()->setCursor(glfw::CursorType::Hand);
  startAnimation<&QuickCard::setBackgroundColor>(bg_color, kCardHoverColor, 180.0f, CubicBezier::EaseOut());
  startAnimation<&QuickCard::setHoverT>(hover_t, 1.0f, 180.0f, CubicBezier::EaseOut());
  startAnimation(offset_y, -5.f, 180.f, &offset_y, CubicBezier::EaseOut());
}

void QuickCard::onMouseLeave(float, float) {
  is_pressed = false;
  render_border.setColor(kCardBorder);
  window()->setCursor(glfw::CursorType::Arrow);
  startAnimation<&QuickCard::setBackgroundColor>(bg_color, kCardBgColor, 220.0f, CubicBezier::EaseOut());
  startAnimation<&QuickCard::setHoverT>(hover_t, 0.0f, 220.0f, CubicBezier::EaseOut());
  startAnimation<&QuickCard::setPressT>(press_t, 0.0f, 140.0f, CubicBezier::EaseOut());
  startAnimation(offset_y, 0.f, 220.0f, &offset_y, CubicBezier::EaseOut());
}

void QuickCard::onMouseLeftPressed(float, float) {
  is_pressed = true;
  startAnimation<&QuickCard::setBackgroundColor>(bg_color, kCardPressColor, 90.0f, CubicBezier::EaseOut());
  startAnimation<&QuickCard::setPressT>(press_t, 1.0f, 70.0f, CubicBezier::EaseOut());
}

void QuickCard::onMouseLeftReleased(float, float) {
  is_pressed = false;
  clicked.emit();
  startAnimation<&QuickCard::setBackgroundColor>(bg_color, kCardHoverColor, 120.0f, CubicBezier::EaseOut());
  startAnimation<&QuickCard::setPressT>(press_t, 0.0f, 170.0f, CubicBezier::EaseOut());
}

void QuickCard::setBackgroundColor(const SkColor color) noexcept {
  bg_color = color;
  render_bg.setColor(color);
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
  canvas->translate(0, offset_y);

  const float scale = 1.0f - press_t * 0.018f;
  canvas->translate(width_ * 0.5f, height_ * 0.5f);
  canvas->scale(scale, scale);
  canvas->translate(-width_ * 0.5f, -height_ * 0.5f);

  const float sigma = 8.0f + hover_t * 4.0f;
  auto shadow_rect = borderRect().makeOffset(0, 6.0f + hover_t * 1.5f).makeInset(7.0f, 7.0f);
  auto layer_bounds = shadow_rect.makeOutset(sigma * 3.0f, sigma * 3.0f);
  SkPaint shadow_layer;
  shadow_layer.setImageFilter(SkImageFilters::Blur(sigma, sigma, nullptr));
  SkPaint shadow;
  shadow.setAntiAlias(true);
  shadow.setColor(ColorFromARGB(static_cast<U8CPU>(14.0f + hover_t * 8.0f), 25, 36, 52));
  canvas->saveLayer(&layer_bounds, &shadow_layer);
  canvas->drawRoundRect(shadow_rect, radius, radius, shadow);
  canvas->restore();

  render_bg.render(canvas);

  SkPaint shine;
  shine.setAntiAlias(true);
  shine.setColor(ColorFromARGB(static_cast<U8CPU>(34.0f + hover_t * 28.0f), 255, 255, 255));
  canvas->drawRoundRect(SkRect::MakeXYWH(1.0f, 1.0f, width_ - 2.0f, height_ * 0.45f), radius, radius, shine);

  icon_bg.render(canvas);

  SkPaint iconGlow;
  iconGlow.setAntiAlias(true);
  iconGlow.setColor(lerp(icon_color_, ColorFromARGB(255, 255, 255, 255), 0.78f));
  iconGlow.setAlphaf(0.42f + hover_t * 0.18f);
  canvas->drawCircle(16.0f + kIconSize * 0.62f, height_ * 0.5f - 7.0f, 10.0f + hover_t * 2.0f, iconGlow);

  title_text.render(canvas);
  subtitle_text.render(canvas);
  render_border.render(canvas);
  canvas->restore();
}

} // namespace components
