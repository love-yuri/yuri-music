//
// Created by yuri on 2026/4/8.
//
export module components:quick_card;

import std;
import yuri.ui;
import yuri.skia;
import yuri.glfw.api;

using namespace ui::render;
using namespace ui::layout;
using namespace ui::widgets;
using namespace ui::animation;
using namespace ui::algorithm;
using namespace skia;

constexpr float kCardRadius = 12.0f;                                    // 卡片圆角
constexpr float kIconSize = 46.0f;                                      // 图标尺寸
constexpr float kIconInset = 16.0f;                                     // 图标左侧间距
constexpr SkColor kQuickTitleColor = ColorFromARGB(255, 23, 31, 44);    // 标题色
constexpr SkColor kQuickSubtitleColor = ColorFromARGB(172, 58, 70, 88); // 副标题色

export namespace components {

/** 使用按钮交互和样式绘制带图标的推荐卡片。 */
class QuickCard : public Button {
public:
  /**
   * 创建快捷入口卡片。
   * @param title 标题
   * @param subtitle 副标题
   * @param icon_color 图标主色
   * @param parent 父控件
   */
  QuickCard(std::string_view title, std::string_view subtitle, SkColor icon_color, Widget *parent);

  /** 更新内部绘制节点的位置。 */
  void layout_children() override;

  /**
   * 绘制卡片背景、图标和文本。
   * @param canvas 目标画布
   */
  void paint(SkCanvas *canvas) override;

protected:
  /**
   * 更新悬停样式并抬升卡片。
   * @param x 鼠标本地横坐标
   * @param y 鼠标本地纵坐标
   */
  void on_mouse_enter(float x, float y) override;

  /**
   * 还原样式和卡片位置。
   * @param x 鼠标本地横坐标
   * @param y 鼠标本地纵坐标
   */
  void on_mouse_leave(float x, float y) override;

private:
  RenderBackground icon_bg{};      // 图标背景
  float icon_radius = kCardRadius; // 图标圆角
  RenderText title_text{};         // 标题
  RenderText subtitle_text{};      // 副标题
  SkColor icon_color_;             // 图标主色
  float hover_t = 0.0f;            // 悬浮动画进度
};

QuickCard::QuickCard(
  const std::string_view title,
  const std::string_view subtitle,
  const SkColor icon_color,
  Widget *parent
) : Button("", parent), title_text(title), subtitle_text(subtitle), icon_color_(icon_color) {
  constexpr ButtonStyle base{
    .background = 0x9affffff,
    .border = 0x78ffffff,
    .radius = kCardRadius,
  };
  ButtonStyles styles{ base, base, base, base };
  styles.hovered.background = 0xd4ffffff;
  styles.hovered.border = 0xa0ffffff;
  styles.pressed.background = 0xebf8fafc;
  styles.pressed.border = styles.hovered.border;
  styles.disabled.background = 0x62ffffff;
  set_style(styles);

  icon_bg.set_color(icon_color);
  icon_bg.radius = &icon_radius;
  title_text.set_font_size(14.0f);
  title_text.set_color(kQuickTitleColor);
  title_text.set_alignment(Alignment::CenterLeft);
  subtitle_text.set_font_size(12.0f);
  subtitle_text.set_color(kQuickSubtitleColor);
  subtitle_text.set_alignment(Alignment::CenterLeft);
  set_fixed_height(78.0f);
}

void QuickCard::on_mouse_enter(const float x, const float y) {
  Button::on_mouse_enter(x, y);
  if (enabled()) window()->setCursor(glfw::CursorType::Hand);
  start_animation(hover_t, 1.0f, 180.0f, &hover_t, CubicBezier::EaseOut());
}

void QuickCard::on_mouse_leave(const float x, const float y) {
  Button::on_mouse_leave(x, y);
  window()->setCursor(glfw::CursorType::Arrow);
  start_animation(hover_t, 0.0f, 220.0f, &hover_t, CubicBezier::EaseOut());
}

void QuickCard::layout_children() {
  const float icon_y = (height_ - kIconSize) * 0.5f;
  icon_bg.update(SkRect::MakeXYWH(kIconInset, icon_y, kIconSize, kIconSize));

  constexpr float kTextX = kIconInset + kIconSize + 16.0f;
  const float text_w = std::max(0.0f, width_ - kTextX - kIconInset);
  const float text_y = (height_ - 36.0f) * 0.5f;
  title_text.update(SkRect::MakeXYWH(kTextX, text_y, text_w, 18.0f));
  subtitle_text.update(SkRect::MakeXYWH(kTextX, text_y + 20.0f, text_w, 16.0f));
}

void QuickCard::paint(SkCanvas *canvas) {
  canvas->save();
  canvas->translate(0.0f, -5.0f * hover_t);
  if (state() == ButtonState::Pressed) {
    canvas->translate(width_ * 0.5f, height_ * 0.5f);
    canvas->scale(0.982f, 0.982f);
    canvas->translate(-width_ * 0.5f, -height_ * 0.5f);
  }

  const float sigma = 8.0f + hover_t * 4.0f;
  const auto shadow_rect =
    local_bounds().makeOffset(0, 6.0f + hover_t * 1.5f).makeInset(7.0f, 7.0f);
  const auto layer_bounds = shadow_rect.makeOutset(sigma * 3.0f, sigma * 3.0f);
  SkPaint shadow_layer;
  shadow_layer.setImageFilter(SkImageFilters::Blur(sigma, sigma, nullptr));
  SkPaint shadow;
  shadow.setAntiAlias(true);
  shadow.setColor(ColorFromARGB(static_cast<U8CPU>(14.0f + hover_t * 8.0f), 25, 36, 52));
  canvas->saveLayer(&layer_bounds, &shadow_layer);
  canvas->drawRoundRect(shadow_rect, kCardRadius, kCardRadius, shadow);
  canvas->restore();

  Button::paint(canvas);

  SkPaint shine;
  shine.setAntiAlias(true);
  shine.setColor(ColorFromARGB(static_cast<U8CPU>(34.0f + hover_t * 28.0f), 255, 255, 255));
  canvas->drawRoundRect(
    SkRect::MakeXYWH(1.0f, 1.0f, std::max(0.0f, width_ - 2.0f), height_ * 0.45f),
    kCardRadius,
    kCardRadius,
    shine
  );

  icon_bg.draw(canvas);

  SkPaint icon_glow;
  icon_glow.setAntiAlias(true);
  icon_glow.setColor(lerp(icon_color_, skia_colors::white, 0.78f));
  icon_glow.setAlphaf(0.42f + hover_t * 0.18f);
  canvas->drawCircle(
    kIconInset + kIconSize * 0.62f, height_ * 0.5f - 7.0f, 10.0f + hover_t * 2.0f, icon_glow
  );

  title_text.draw(canvas);
  subtitle_text.draw(canvas);
  canvas->restore();
}

} // namespace components
