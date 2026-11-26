export module components:player_bar_volume_popup;

import std;
import player;
import yuri.skia;
import yuri.ui;
import :player_bar_slider;
import :player_bar_style;

using namespace skia;
using namespace ui::layout;
using namespace ui::widgets;
using ui::render::PaintDesc;
using controls::player_bar::PlayerSlider;

export namespace components {

/** 窗口层上的竖向音量浮层。 */
class VolumeSliderPopup : public Widget {
public:
  static constexpr float kPreferredWidth = 52.0f;   // 浮层宽度
  static constexpr float kPreferredHeight = 142.0f; // 浮层高度

  /**
   * 创建音量浮层及滑条。
   * @param parent 窗口控件
   */
  explicit VolumeSliderPopup(Widget *parent = nullptr);

  /** 断开播放器的音量订阅。 */
  ~VolumeSliderPopup() override;

  /**
   * 绘制浮层面板和扬声器状态。
   * @param canvas 目标画布
   */
  void paint(SkCanvas *canvas) override;

protected:
  /**
   * 在浮层范围内滚动时调节音量。
   * @param delta_x 横向滚动量
   * @param delta_y 纵向滚动量
   */
  void on_mouse_wheel(float delta_x, float delta_y) override;

private:
  PlayerSlider *slider_; // 竖向音量滑条
};

VolumeSliderPopup::VolumeSliderPopup(Widget *parent) :
  Widget(parent), slider_(new PlayerSlider(PlayerSlider::Mode::Volume, this)) {
  set_fixed_size(kPreferredWidth, kPreferredHeight);
  set_visible(false);
  set_layout(new VBoxLayout);
  slider_->set_fixed_size(30.0f, 100.0f);
  slider_->set_horizontal_alignment(HorizontalAlignment::Center);
  slider_->set_margin(Insets::from_top(12.0f));
  slider_->set_value(1.0f);
  auto &audio = player::bass24_player();
  slider_->value_changed.connect<&player::Bass24Player::set_volume>(&audio);
  audio.volume_changed.connect<&PlayerSlider::set_value>(slider_);
}

VolumeSliderPopup::~VolumeSliderPopup() {
  player::bass24_player().volume_changed.disconnect(slider_);
}

void VolumeSliderPopup::paint(SkCanvas *canvas) {
  constexpr float kRadius = 18.0f;
  const auto panel = local_bounds().makeInset(4.0f, 4.0f);
  using namespace controls::player_bar;
  draw_shadow(
    canvas,
    panel.makeOffset(0.0f, 5.0f).makeInset(3.0f, 3.0f),
    kRadius,
    10.0f,
    ColorFromARGB(34, 24, 31, 42)
  );
  canvas->drawRoundRect(
    panel, kRadius, kRadius, PaintDesc { .color = ColorFromARGB(242, 255, 255, 255) }
  );
  canvas->drawRoundRect(
    panel.makeInset(0.5f, 0.5f),
    kRadius - 0.5f,
    kRadius - 0.5f,
    PaintDesc {
      .color = ColorFromARGB(136, 255, 255, 255),
      .strokeWidth = 1.0f,
      .style = SkPaint::kStroke_Style,
    }
  );
  draw_speaker(
    canvas, { width() * 0.5f, height() - 21.0f }, slider_->value() <= 0.001f, kTextSecondary
  );
}

void VolumeSliderPopup::on_mouse_wheel(const float, const float delta_y) {
  if (std::abs(delta_y) > 0.001f) slider_->adjust(std::copysign(0.05f, delta_y));
}

} // namespace components
