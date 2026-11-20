//
// Created by yuri on 2026/2/7.
//
export module components:menu_button;

import std;
import yuri.ui;
import yuri.skia;

using namespace ui::layout;
using namespace ui::widgets;
using namespace skia;

constexpr float kMenuRadius = 12.0f;             // 导航按钮圆角
constexpr float kMenuFontSize = 24.0f;           // 导航字号
constexpr SkColor kActiveMarkColor = 0xffff4c77; // 选中标记颜色
constexpr float kActiveMarkLeft = 7.0f;          // 选中标记距左侧的距离
constexpr float kActiveMarkWidth = 3.0f;         // 选中标记宽度
constexpr float kActiveMarkHeight = 20.0f;       // 选中标记高度
constexpr int kSelectedBorderAlpha = 108;        // 选中时的边线透明度
constexpr int kHoveredBackgroundAlpha = 62;      // 悬停时的背景透明度

/**
 * 生成导航按钮各个交互状态的样式。
 * @param active 当前按钮是否对应可见页面
 * @return 默认、悬停、按下和禁用状态的样式
 */
ButtonStyles makeMenuStyles(bool active);

export namespace components {

/** 使用应用主题绘制图标并突出显示当前页面的导航按钮。 */
class MenuButton : public IconButton {
public:
  /**
   * 创建导航按钮。
   * @param text 按钮文字
   * @param icon_path 左侧 SVG 图标路径
   * @param parent 父控件
   */
  MenuButton(std::string_view text, std::string_view icon_path, Widget *parent);

  /**
   * 更新当前页面的选中样式。
   * @param active 当前按钮是否对应可见页面
   */
  void setActive(bool active);

  /**
   * 绘制导航按钮和当前页面的选中标记。
   * @param canvas 目标画布
   */
  void paint(SkCanvas *canvas) override;

private:
  bool active_ = false; // 是否选中
};

MenuButton::MenuButton(
  const std::string_view text,
  const std::string_view icon_path,
  Widget *parent
) : IconButton(text, parent) {
  set_style(makeMenuStyles(false));
  set_text_alignment(Alignment::CenterLeft);
  set_icon(icon_path, 10.0f);
  set_preferred_height(50.0f);
  set_max_height(50.0f);
}

void MenuButton::setActive(const bool active) {
  if (active_ == active) return;
  active_ = active;
  set_style(makeMenuStyles(active));
}

void MenuButton::paint(SkCanvas *canvas) {
  IconButton::paint(canvas);
  if (!active_) return;

  SkPaint mark;
  mark.setAntiAlias(true);
  mark.setColor(kActiveMarkColor);
  const auto rect = SkRect::MakeXYWH(
    kActiveMarkLeft, (height() - kActiveMarkHeight) * 0.5f, kActiveMarkWidth, kActiveMarkHeight
  );
  constexpr auto radius = kActiveMarkWidth * 0.5f;
  canvas->drawRoundRect(rect, radius, radius, mark);
}

} // namespace components

ButtonStyles makeMenuStyles(const bool active) {
  const ButtonStyle base{
    .background = active ? 0xb6ffffff : skia_colors::transparent,
    .border =
      active ? ColorFromARGB(kSelectedBorderAlpha, 255, 255, 255) : skia_colors::transparent,
    .foreground = active ? 0xffcc2d56 : 0xb0181f2a,
    .padding = Insets(13.0f, 6.0f, 11.0f, 6.0f),
    .radius = kMenuRadius,
    .font_size = kMenuFontSize,
  };
  ButtonStyles styles{ base, base, base, base };
  styles.hovered.background =
    active ? 0xc6ffffff : ColorFromARGB(kHoveredBackgroundAlpha, 255, 255, 255);
  styles.hovered.foreground = active ? base.foreground : 0xee121822;
  styles.pressed.background = 0xd6ffffff;
  styles.pressed.foreground = styles.hovered.foreground;
  styles.disabled.foreground = 0x60242b38;
  return styles;
}
