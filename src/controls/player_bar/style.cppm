export module components:player_bar_style;

import yuri.skia;
import yuri.ui;

using namespace skia;
using ui::render::PaintDesc;

export namespace controls::player_bar {

inline constexpr SkColor kAccent = ColorFromARGB(255, 218, 52, 92);       // 播放强调色
inline constexpr SkColor kAccentHot = ColorFromARGB(255, 255, 76, 119);   // 悬停强调色
inline constexpr SkColor kTextPrimary = ColorFromARGB(255, 20, 26, 36);   // 标题与歌词颜色
inline constexpr SkColor kTextSecondary = ColorFromARGB(176, 56, 68, 86); // 歌手与按钮颜色
inline constexpr SkColor kTextMuted = ColorFromARGB(122, 54, 65, 82);     // 时长与次要边框颜色

/**
 * 绘制播放器面板、按钮底座和封面共用的圆角阴影。
 * @param canvas 目标画布
 * @param rect 阴影形状的边界
 * @param radius 圆角半径
 * @param sigma 模糊半径
 * @param color 阴影颜色
 */
void draw_shadow(SkCanvas *canvas, const SkRect &rect, float radius, float sigma, SkColor color);

/**
 * 绘制扬声器图标。
 * @param canvas 目标画布
 * @param center 图标中心
 * @param muted 是否静音
 * @param color 图标颜色
 */
void draw_speaker(SkCanvas *canvas, SkPoint center, bool muted, SkColor color);

} // namespace controls::player_bar

void controls::player_bar::draw_shadow(
  SkCanvas *canvas,
  const SkRect &rect,
  const float radius,
  const float sigma,
  const SkColor color
) {
  SkPaint blur;
  blur.setImageFilter(SkImageFilters::Blur(sigma, sigma, nullptr));
  const auto bounds = rect.makeOutset(3.0f * sigma, 3.0f * sigma);
  canvas->saveLayer(&bounds, &blur);
  canvas->drawRoundRect(rect, radius, radius, PaintDesc{ .color = color });
  canvas->restore();
}

void controls::player_bar::draw_speaker(
  SkCanvas *canvas,
  const SkPoint center,
  const bool muted,
  const SkColor color
) {
  canvas->save();
  canvas->translate(center.x(), center.y());

  SkPathBuilder body;
  body.moveTo(-7.0f, -4.0f);
  body.lineTo(-4.0f, -4.0f);
  body.lineTo(0.0f, -7.5f);
  body.lineTo(0.0f, 7.5f);
  body.lineTo(-4.0f, 4.0f);
  body.lineTo(-7.0f, 4.0f);
  body.close();
  canvas->drawPath(body.detach(), PaintDesc{ .color = color });

  SkPaint line = PaintDesc{ .color = color, .strokeWidth = 1.4f, .style = SkPaint::kStroke_Style };
  line.setStrokeCap(SkPaint::kRound_Cap);
  if (muted) {
    canvas->drawLine(2.0f, -3.5f, 7.0f, 3.5f, line);
    canvas->drawLine(7.0f, -3.5f, 2.0f, 3.5f, line);
  } else {
    SkPathBuilder wave;
    wave.moveTo(2.0f, -3.5f);
    wave.quadTo(5.0f, 0.0f, 2.0f, 3.5f);
    canvas->drawPath(wave.detach(), line);
  }
  canvas->restore();
}
