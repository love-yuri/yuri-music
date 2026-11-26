export module components:player_bar_slider;

import std;
import yuri.core;
import yuri.ui;
import yuri.skia;
import :player_bar_style;

using namespace skia;
using namespace ui::widgets;
using ui::render::PaintDesc;

export namespace controls::player_bar {

/** 播放进度与音量共用的滑条控件。 */
class PlayerSlider : public Widget {
public:
  /** 滑条方向及更新方式。 */
  enum class Mode {
    Seek,   // 横向，松开后提交进度
    Volume, // 纵向，拖动和滚轮实时更新音量
  };

  Signal<float> value_changed; // 用户操作后的归一化值

  /**
   * 创建滑条。
   * @param mode 滑条用途
   * @param parent 父控件
   */
  PlayerSlider(Mode mode, Widget *parent);

  /**
   * 同步滑条的当前值。
   * @param value 归一化值
   */
  void set_value(float value) noexcept;

  /** 获取滑条当前的归一化值。 */
  [[nodiscard]] float value() const noexcept {
    return value_;
  }

  /**
   * 按步长调整当前值。
   * @param step 有符号的归一化步长
   */
  void adjust(float step);

  /**
   * 在本地区域绘制轨道、填充和音量滑块。
   * @param canvas 目标画布
   */
  void paint(SkCanvas *canvas) override;

protected:
  /**
   * 拖动时更新音量。
   * @param x 本地横坐标
   * @param y 本地纵坐标
   */
  void on_mouse_move(float x, float y) override;

  /**
   * 在音量滑条上按下时立即更新。
   * @param x 本地横坐标
   * @param y 本地纵坐标
   */
  void on_mouse_left_pressed(float x, float y) override;

  /**
   * 松开时提交进度或音量。
   * @param x 本地横坐标
   * @param y 本地纵坐标
   */
  void on_mouse_left_released(float x, float y) override;

private:
  /** 获取轨道首尾两端共用的留白。 */
  [[nodiscard]] float track_inset() const noexcept;

  /**
   * 将指针位置换算成滑条值。
   * @param x 本地横坐标
   * @param y 本地纵坐标
   */
  void update_from_point(float x, float y);

  /**
   * 应用用户的操作并发送信号。
   * @param value 归一化值
   */
  void commit(float value);

  Mode mode_;          // 滑条用途
  float value_ = 0.0f; // 当前值
};

PlayerSlider::PlayerSlider(const Mode mode, Widget *parent) : Widget(parent), mode_(mode) {
}

void PlayerSlider::set_value(const float value) noexcept {
  value_ = std::clamp(value, 0.0f, 1.0f);
}

void PlayerSlider::adjust(const float step) {
  commit(value_ + step);
}

float PlayerSlider::track_inset() const noexcept {
  return mode_ == Mode::Volume ? 8.0f : std::min(4.0f, width() * 0.25f);
}

void PlayerSlider::paint(SkCanvas *canvas) {
  const bool vertical = mode_ == Mode::Volume;
  const float inset = track_inset();
  const float length = (vertical ? height() : width()) - 2.0f * inset;
  if (length <= 0.0f) return;

  // 以横向轨道为本地坐标系，音量轨道从底部向上增长。
  canvas->save();
  if (vertical) {
    canvas->translate(width() * 0.5f, height() - inset);
    canvas->rotate(-90.0f);
  } else {
    canvas->translate(inset, height() * 0.5f);
  }

  constexpr float kRadius = 2.0f;
  const auto track_color = ColorFromARGB(vertical ? 50 : 78, 54, 65, 82);
  const float end = std::round(length * value_);
  const auto track = SkRect::MakeXYWH(0.0f, -kRadius, length, 2.0f * kRadius);
  canvas->drawRoundRect(track, kRadius, kRadius, PaintDesc { .color = track_color });
  canvas->drawRoundRect(
    SkRect::MakeXYWH(0.0f, -kRadius, end, 2.0f * kRadius),
    kRadius,
    kRadius,
    PaintDesc { .color = kAccent }
  );
  if (vertical) {
    canvas->drawCircle(end, 0.0f, 7.5f, PaintDesc { .color = skia_colors::white });
    canvas->drawCircle(
      end,
      0.0f,
      7.0f,
      PaintDesc {
        .color = ColorFromARGB(42, 24, 31, 42),
        .strokeWidth = 1.0f,
        .style = SkPaint::kStroke_Style,
      }
    );
    canvas->drawCircle(end, 0.0f, 4.5f, PaintDesc { .color = kAccent });
  }
  canvas->restore();
}

void PlayerSlider::update_from_point(const float x, const float y) {
  const bool vertical = mode_ == Mode::Volume;
  const float inset = track_inset();
  const float length = (vertical ? height() : width()) - 2.0f * inset;
  if (length > 0.0f) commit(((vertical ? height() - y : x) - inset) / length);
}

void PlayerSlider::commit(const float value) {
  set_value(value);
  value_changed.emit(value_);
}

void PlayerSlider::on_mouse_move(const float x, const float y) {
  if (mode_ == Mode::Volume && pressed_) update_from_point(x, y);
}

void PlayerSlider::on_mouse_left_pressed(const float x, const float y) {
  if (mode_ == Mode::Volume) update_from_point(x, y);
}

void PlayerSlider::on_mouse_left_released(const float x, const float y) {
  update_from_point(x, y);
}

} // namespace controls::player_bar
