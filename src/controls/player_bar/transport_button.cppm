export module components:player_bar_transport_button;

import std;
import yuri.core;
import yuri.skia;
import yuri.ui;
import :player_bar_style;

using namespace skia;
using namespace ui::widgets;
using ui::render::PaintDesc;

export namespace controls::player_bar {

/** 播放器按钮的图标，坐标以按钮中心为原点。 */
enum class TransportIcon {
  Repeat,   // 顺序播放
  Shuffle,  // 随机播放
  Previous, // 上一首
  Next,     // 下一首
  Play,     // 播放
  Pause,    // 暂停
  Loading,  // 加载中
  Volume,   // 普通音量
  Muted,    // 静音
};

} // namespace controls::player_bar

using controls::player_bar::TransportIcon;

/**
 * 绘制顺序或随机播放图标。
 * @param canvas 原点为图标中心的画布
 * @param shuffle 是否为随机播放
 * @param color 图标颜色
 */
void draw_mode(SkCanvas *canvas, bool shuffle, SkColor color);

/**
 * 绘制上一首或下一首图标。
 * @param canvas 原点为图标中心的画布
 * @param previous 是否绘制上一首
 * @param paint 图标画笔
 */
void draw_skip(SkCanvas *canvas, bool previous, const SkPaint &paint);

/**
 * 绘制播放、暂停或加载中的图标。
 * @param canvas 原点为图标中心的画布
 * @param icon 要绘制的图标
 */
void draw_playback(SkCanvas *canvas, TransportIcon icon);

export namespace controls::player_bar {

/** 复用 Button 事件和捕获逻辑的播放栏图标按钮。 */
class TransportButton : public Button {
public:
  /**
   * 创建播放控制按钮。
   * @param icon 初始图标
   * @param primary 是否为强调显示的播放键
   * @param parent 父控件
   */
  TransportButton(TransportIcon icon, bool primary, Widget *parent);

  /**
   * 同步当前图标和选中样式。
   * @param icon 要显示的图标
   * @param active 是否使用强调色
   */
  void set_icon(TransportIcon icon, bool active = false);

  /**
   * 绘制按钮自身的图标和交互样式。
   * @param canvas 目标画布
   */
  void paint(SkCanvas *canvas) override;

private:
  static constexpr float kPrimarySize = 48.0f; // 播放按钮边长
  static constexpr float kSideSize = 42.0f;    // 其他按钮边长

  TransportIcon icon_;  // 当前图标
  bool primary_;        // 是否为播放主按钮
  bool active_ = false; // 是否使用强调色
};

TransportButton::TransportButton(const TransportIcon icon, const bool primary, Widget *parent) :
  Button("", parent), icon_(icon), primary_(primary) {
  const float size = primary ? kPrimarySize : kSideSize;
  set_min_width(0.0f);
  set_preferred_width(size);
  set_fixed_height(size);
  set_vertical_alignment(ui::layout::VerticalAlignment::Center);
}

void TransportButton::set_icon(const TransportIcon icon, const bool active) {
  icon_ = icon;
  active_ = active;
  set_enabled(icon != TransportIcon::Loading);
}

void TransportButton::paint(SkCanvas *canvas) {
  const float nominal_width = primary_ ? kPrimarySize : kSideSize;
  const auto button_state = state();
  const bool highlighted =
    button_state == ButtonState::Hovered || button_state == ButtonState::Pressed;
  const float press_scale = button_state == ButtonState::Pressed ? 0.92f : 1.0f;
  const float scale = std::min(1.0f, width() / nominal_width) * press_scale;
  if (scale <= 0.0f) return;

  canvas->save();
  canvas->translate(width() * 0.5f, height() * 0.5f);
  canvas->scale(scale, scale);
  if (primary_) {
    const auto fill = highlighted ? kAccentHot : kAccent;
    canvas->drawCircle(0.0f, 0.0f, 24.0f, PaintDesc { .color = fill });
    canvas->drawCircle(-5.0f, -5.5f, 4.0f, PaintDesc { .color = ColorFromARGB(22, 255, 255, 255) });
    draw_playback(canvas, icon_);
  } else {
    if (highlighted) {
      canvas->drawCircle(
        0.0f, 0.0f, 19.0f, PaintDesc { .color = ColorFromARGB(66, 255, 255, 255) }
      );
    }
    const auto color = active_ ? kAccent : (highlighted ? kTextPrimary : kTextSecondary);
    switch (icon_) {
      case TransportIcon::Repeat:
      case TransportIcon::Shuffle: draw_mode(canvas, icon_ == TransportIcon::Shuffle, color); break;
      case TransportIcon::Previous:
      case TransportIcon::Next:
        draw_skip(canvas, icon_ == TransportIcon::Previous, PaintDesc { .color = color });
        break;
      default: draw_speaker(canvas, { 0.0f, 0.0f }, icon_ == TransportIcon::Muted, color); break;
    }
  }
  canvas->restore();
}

} // namespace controls::player_bar

void draw_mode(SkCanvas *canvas, const bool shuffle, const SkColor color) {
  SkPaint line = PaintDesc { .color = color, .strokeWidth = 1.8f, .style = SkPaint::kStroke_Style };
  line.setStrokeCap(SkPaint::kRound_Cap);
  line.setStrokeJoin(SkPaint::kRound_Join);

  SkPathBuilder arrows;
  if (shuffle) {
    arrows.moveTo(-7.0f, -5.0f).lineTo(-3.0f, -5.0f).lineTo(3.0f, 5.0f).lineTo(7.0f, 5.0f);
    arrows.moveTo(-7.0f, 5.0f).lineTo(-3.0f, 5.0f).lineTo(3.0f, -5.0f).lineTo(7.0f, -5.0f);
    arrows.moveTo(4.0f, 2.0f).lineTo(7.0f, 5.0f).lineTo(4.0f, 8.0f);
    arrows.moveTo(4.0f, -8.0f).lineTo(7.0f, -5.0f).lineTo(4.0f, -2.0f);
  } else {
    arrows.moveTo(-7.0f, -4.0f).lineTo(7.0f, -4.0f);
    arrows.moveTo(4.0f, -7.0f).lineTo(7.0f, -4.0f).lineTo(4.0f, -1.0f);
    arrows.moveTo(7.0f, 4.0f).lineTo(-7.0f, 4.0f);
    arrows.moveTo(-4.0f, 1.0f).lineTo(-7.0f, 4.0f).lineTo(-4.0f, 7.0f);
  }
  canvas->drawPath(arrows.detach(), line);
}

void draw_skip(SkCanvas *canvas, const bool previous, const SkPaint &paint) {
  if (previous) canvas->scale(-1.0f, 1.0f);
  SkPathBuilder triangle;
  triangle.moveTo(-5.5f, -6.5f).lineTo(2.5f, 0.0f).lineTo(-5.5f, 6.5f).close();
  canvas->drawPath(triangle.detach(), paint);
  canvas->drawRect(SkRect::MakeXYWH(4.0f, -6.5f, 2.0f, 13.0f), paint);
}

void draw_playback(SkCanvas *canvas, const TransportIcon icon) {
  SkPaint paint = PaintDesc { .color = skia_colors::white };
  if (icon == TransportIcon::Loading) {
    constexpr int kDots = 8;
    const float time = static_cast<float>(profiling::frame_clock.now % 1000000ULL) / 1000000.0f;
    for (int i = 0; i < kDots; ++i) {
      const float phase = static_cast<float>(i) / kDots;
      const float angle = (phase + time) * 2.0f * std::numbers::pi_v<float>;
      paint.setAlphaf(0.3f + phase * 0.7f);
      canvas->drawCircle(std::cos(angle) * 6.2f, std::sin(angle) * 6.2f, 1.55f, paint);
    }
  } else if (icon == TransportIcon::Pause) {
    canvas->drawRoundRect(SkRect::MakeXYWH(-5.4f, -6.2f, 3.2f, 12.4f), 1.3f, 1.3f, paint);
    canvas->drawRoundRect(SkRect::MakeXYWH(2.2f, -6.2f, 3.2f, 12.4f), 1.3f, 1.3f, paint);
  } else {
    constexpr float kBaseX = -16.0f / 3.0f;
    SkPathBuilder triangle;
    triangle.moveTo(kBaseX + 16.0f, 0.0f).lineTo(kBaseX, -10.0f).lineTo(kBaseX, 10.0f).close();
    canvas->drawPath(triangle.detach(), paint);
  }
}
