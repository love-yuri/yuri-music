export module components:player_bar_actions;

import yuri.skia;
import yuri.ui;
import :player_bar_style;

using namespace skia;
using namespace ui::layout;
using namespace ui::widgets;

namespace controls::player_bar {

/** 可在播放器栏切换选中状态的歌词按钮。 */
class LyricsButton : public Button {
public:
  /**
   * 创建歌词按钮。
   * @param parent 父控件
   */
  explicit LyricsButton(Widget *parent);

  static constexpr float kSize = 32.0f; // 歌词按钮边长

private:
  /** 切换选中状态并更新按钮样式。 */
  void toggle();

  /** 根据当前选中状态应用样式。 */
  void update_style();

  bool selected_ = false; // 是否选中歌词按钮
};

} // namespace controls::player_bar

export namespace controls::player_bar {

/** 播放栏右侧的附加操作。 */
class PlayerActions : public Widget {
public:
  /**
   * 创建靠右对齐的操作区域。
   * @param parent 父控件
   */
  explicit PlayerActions(Widget *parent);

  /** 根据可用空间显示歌词按钮。 */
  void layout_children() override;

private:
  LyricsButton *lyrics_; // 歌词按钮
};

} // namespace controls::player_bar

using namespace controls::player_bar;

LyricsButton::LyricsButton(Widget *parent) : Button("词", parent) {
  set_fixed_size(kSize, kSize);
  set_vertical_alignment(VerticalAlignment::Center);
  render_text_.set_font(font::bold_font);
  clicked.connect<&LyricsButton::toggle>(this);

  update_style();
}

void LyricsButton::toggle() {
  selected_ = !selected_;
  update_style();
}

void LyricsButton::update_style() {
  const ButtonStyle normal{
    .background = selected_ ? kAccent : skia_colors::transparent,
    .border = selected_ ? kAccent : kTextMuted,
    .foreground = selected_ ? skia_colors::white : kTextPrimary,
    .padding = Insets(0.0f),
    .border_width = 1.0f,
    .radius = 7.0f,
    .font_size = 19.0f,
  };
  auto hover = normal;
  hover.background = selected_ ? kAccentHot : ColorFromARGB(18, 54, 65, 82);
  hover.border = selected_ ? kAccentHot : kTextSecondary;
  set_style({ .normal = normal, .hovered = hover, .pressed = hover, .disabled = normal });
}

PlayerActions::PlayerActions(Widget *parent) : Widget(parent), lyrics_(new LyricsButton(this)) {
  set_layout(new HBoxLayout(HorizontalAlignment::Right));
}

void PlayerActions::layout_children() {
  lyrics_->set_visible(width() >= LyricsButton::kSize);
  Widget::layout_children();
}
