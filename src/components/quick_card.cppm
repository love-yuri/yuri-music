//
// Created by yuri on 2026/4/8.
//
export module components:quick_card;

import std;
import ui;
import core;
import skia;

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

// 快捷操作卡片：图标色块 + 标题 + 副标题
class QuickCard : public Box {
public:
  QuickCard(std::string_view title,
            std::string_view subtitle,
            SkColor icon_color,
            Widget *parent = nullptr);

  void layoutChildren() override;
  void paint(SkCanvas *canvas) override;

protected:
  // 鼠标悬浮进入
  void onMouseEnter(float x, float y) override;
  // 鼠标悬浮离开
  void onMouseLeave(float x, float y) override;
  // 鼠标按下
  void onMouseLeftPressed(float x, float y) override;
  // 鼠标松开
  void onMouseLeftReleased(float x, float y) override;

private:
  /**
   * 设置背景色
   */
  void setBackgroundColor(SkColor color) noexcept;


  RenderBackground icon_bg_;
  float icon_radius_ = kIconRadius;
  RenderText title_text_;
  RenderText subtitle_text_;
  // 动画状态
  SkColor bg_color_ = kCardBgColor;
  bool is_pressed_ = false;
};

QuickCard::QuickCard(const std::string_view title,
                     const std::string_view subtitle,
                     const SkColor icon_color,
                     Widget *parent) :
  Box(parent), title_text_(title), subtitle_text_(subtitle) {
  // 卡片背景
  render_bg.setColor(kCardBgColor);
  radius = 10;

  // 图标色块
  icon_bg_.setColor(icon_color);
  icon_bg_.radius = &icon_radius_;

  // 标题
  title_text_.setFontSize(14);
  title_text_.setColor(skia_colors::black);
  title_text_.setAlignment(Alignment::CenterLeft);

  // 副标题
  subtitle_text_.setFontSize(12);
  subtitle_text_.setColor(kSubtitleColor);
  subtitle_text_.setAlignment(Alignment::CenterLeft);

  // 固定高度
  setMaxHeight(82);
  setMinHeight(82);
}

void QuickCard::onMouseEnter(float, float) {
  is_pressed_ = false;
  startAnimation<&QuickCard::setBackgroundColor>(bg_color_, kCardHoverColor, 150.0f);
}

void QuickCard::onMouseLeave(float, float) {
  is_pressed_ = false;
  startAnimation<&QuickCard::setBackgroundColor>(bg_color_, kCardBgColor, 150.0f, CubicBezier::EaseOut());
}

void QuickCard::onMouseLeftPressed(float, float) {
  is_pressed_ = true;
  startAnimation<&QuickCard::setBackgroundColor>(bg_color_, kCardPressColor, 100.0f, CubicBezier::EaseOut());
}

void QuickCard::onMouseLeftReleased(float, float) {
  is_pressed_ = false;
  startAnimation<&QuickCard::setBackgroundColor>(bg_color_, kCardHoverColor, 100.0f);
}

void QuickCard::setBackgroundColor(const SkColor color) noexcept {
  bg_color_ = color;
  render_bg.setColor(color);
}

void QuickCard::layoutChildren() {
  const auto rect = contentRect();
  const float pad_left = padding_.left;
  const float pad_top = padding_.top;

  // 背景
  render_bg.setColor(bg_color_);
  render_bg.update(borderRect());

  // 图标：左侧垂直居中
  const float icon_x = pad_left;
  const float icon_y = pad_top + (rect.height() - kIconSize) * 0.5f;
  icon_bg_.update(SkRect::MakeXYWH(icon_x, icon_y, kIconSize, kIconSize));

  // 文字区：图标右边间隔 16px
  const float text_x = icon_x + kIconSize + 16.0f;
  const float text_w = rect.width() - text_x;
  const float center_y = pad_top + (rect.height() - 36.0f) * 0.5f; // 标题+副标题总高约36

  title_text_.update(SkRect::MakeXYWH(text_x, center_y, text_w, 18.0f));
  subtitle_text_.update(SkRect::MakeXYWH(text_x, center_y + 20.0f, text_w, 16.0f));
}

void QuickCard::paint(SkCanvas *canvas) {
  render_bg.render(canvas);
  icon_bg_.render(canvas);
  title_text_.render(canvas);
  subtitle_text_.render(canvas);
}

} // namespace components
