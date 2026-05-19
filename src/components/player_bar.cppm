//
// Created by yuri on 2026/5/4.
//
export module components:player_bar;

import std;
import ui;
import skia;
import core;
import glfw.api;

using namespace ui::render;
using namespace ui::layout;
using namespace ui::widgets;
using namespace ui::animation;
using namespace ui::algorithm;
using namespace skia;

constexpr SkColor kBarBg = ColorFromARGB(182, 18, 24, 34);
constexpr SkColor kBarBorder = ColorFromARGB(78, 255, 255, 255);
constexpr SkColor kAccent = ColorFromARGB(255, 255, 76, 119);
constexpr SkColor kProgressTrack = ColorFromARGB(52, 255, 255, 255);
constexpr SkColor kProgressFill = ColorFromARGB(238, 255, 255, 255);
constexpr SkColor kTextPrimary = ColorFromARGB(255, 255, 255, 255);
constexpr SkColor kTextSecondary = ColorFromARGB(196, 255, 255, 255);
constexpr SkColor kTextMuted = ColorFromARGB(116, 255, 255, 255);
constexpr SkColor kCoverGrad1 = ColorFromARGB(255, 255, 76, 119);
constexpr SkColor kCoverGrad2 = ColorFromARGB(255, 42, 140, 233);
constexpr SkColor kVolTrack = ColorFromARGB(50, 255, 255, 255);
constexpr SkColor kVolFill = ColorFromARGB(210, 255, 255, 255);

// ─── 布局常量 ───

static constexpr float kBarHeight = 78.0f;
static constexpr float kPadH = 28.0f;
static constexpr float kCoverSize = 44.0f;
static constexpr float kCoverRadius = 12.0f;
static constexpr float kPlayBtnR = 13.0f;
static constexpr float kSideBtnR = 12.0f;
static constexpr float kCtrlGap = 20.0f;
static constexpr float kVolWidth = 80.0f;
static constexpr float kVolTrackH = 3.0f;
static constexpr float kVolDotR = 6.0f;
static constexpr float kVolIconW = 16.0f;
static constexpr float kTimeW = 36.0f;
static constexpr float kProgressMaxW = 520.0f;
static constexpr float kProgressH = 3.0f;
static constexpr float kLeftW = 220.0f;
static constexpr float kRightW = 180.0f;
static constexpr float kRowGap = 6.0f;

export namespace components {

/**
 * 播放器底栏 — macOS Music.app 风格
 *
 * 左：封面(44x44) + 歌曲信息
 * 中：控制按钮行 + 进度条行
 * 右：音量控制
 */
class PlayerBar : public Widget {
public:
  explicit PlayerBar(Widget *parent = nullptr);

  void layoutChildren() override;
  void paint(SkCanvas *canvas) override;
  void render(SkCanvas *canvas) override;

  void show();
  void hide();
  void updateSong(std::string_view title, std::string_view artist, std::string_view duration);

  // 是否正在展示（含动画中）
  [[nodiscard]] bool showing() const noexcept { return showing_; }

protected:
  void onMouseMove(float x, float y) override;
  void onMouseLeave(float x, float y) override;
  void onMouseLeftPressed(float x, float y) override;
  void onMouseLeftReleased(float x, float y) override;

private:
  void setSlideT(float t) noexcept { slide_t = t; }
  void setAlphaT(float t) noexcept { alpha_ = t; }
  void setBtnHoverT(float t) noexcept { btn_hover_t = t; }
  void setBtnPressT(float t) noexcept { btn_press_t = t; }
  void setPlayHoverT(float t) noexcept { play_hover_t = t; }
  void setPlayPressT(float t) noexcept { play_press_t = t; }
  void setTextAlphaT(float t) noexcept { text_alpha = t; }

  [[nodiscard]] int hitTestButton(float x, float y) const;
  void drawPrevIcon(SkCanvas *c, float cx, float cy, const SkPaint &p);
  void drawPlayIcon(SkCanvas *c, float cx, float cy, float sz, const SkPaint &p);
  void drawNextIcon(SkCanvas *c, float cx, float cy, const SkPaint &p);
  void drawSpeaker(SkCanvas *c, float x, float cy, const SkPaint &p);
  void drawProgressBar(SkCanvas *c, float cx, float cy, float max_w);
  void drawVolumeBar(SkCanvas *c, float x, float cy, float w);

  // 渲染节点
  RenderBackground bg_;                // 背景
  RenderBackground cover_bg;           // 封面色块
  RenderSvg cover_svg;                 // 封面图标
  float cover_radius = kCoverRadius;   // 封面圆角
  RenderText title_text;               // 歌曲标题
  RenderText artist_text;              // 歌手名
  RenderText time_left;                // 已播放时长
  RenderText time_right;               // 总时长

  // 动画
  float slide_t = 0.0f;                // 滑入进度
  float alpha_ = 0.0f;                 // 整体透明度
  float btn_hover_t = 0.0f;            // 侧按钮悬浮
  float btn_press_t = 0.0f;            // 侧按钮按下
  float play_hover_t = 0.0f;           // 播放按钮悬浮
  float play_press_t = 0.0f;           // 播放按钮按下
  float text_alpha = 1.0f;             // 文字透明度

  // 按钮中心坐标（布局时计算）
  float btn_prev_cx = 0.0f;            // 上一首 X
  float btn_play_cx = 0.0f;            // 播放 X
  float btn_next_cx = 0.0f;            // 下一首 X
  float btn_cy = 0.0f;                 // 按钮 Y

  int hovered_btn = 0;                 // 当前悬浮按钮
  bool showing_ = false;               // 是否显示
};

PlayerBar::PlayerBar(Widget *parent) : Widget(parent), cover_svg("resources/svg/play.svg") {
  bg_.setColor(kBarBg);

  cover_bg.setColor(kCoverGrad1);
  cover_bg.radius = &cover_radius;
  cover_svg.setAlignment(Alignment::Center);

  title_text.setFont(font::bold_font);
  title_text.setFontSize(13);
  title_text.setColor(kTextPrimary);
  title_text.setAlignment(Alignment::BottomLeft);

  artist_text.setFontSize(11);
  artist_text.setColor(kTextSecondary);
  artist_text.setAlignment(Alignment::TopLeft);

  time_left.setFontSize(10);
  time_left.setColor(kTextMuted);
  time_left.setAlignment(Alignment::CenterRight);

  time_right.setFontSize(10);
  time_right.setColor(kTextMuted);
  time_right.setAlignment(Alignment::CenterLeft);

  setMaxHeight(kBarHeight);
  setMinHeight(kBarHeight);
}

void PlayerBar::show() {
  if (showing_) return;
  showing_ = true;
  setVisible(true);
  markLayoutDirty();
  startAnimation<&PlayerBar::setSlideT>(slide_t, 1.0f, 350.0f, CubicBezier::EaseOut());
  startAnimation<&PlayerBar::setAlphaT>(alpha_, 1.0f, 350.0f, CubicBezier::EaseOut());
}

void PlayerBar::hide() {
  if (!showing_) return;
  showing_ = false;
  setVisible(false);
  markLayoutDirty();
}

void PlayerBar::updateSong(std::string_view title, std::string_view artist, std::string_view duration) {
  text_alpha = 0.0f;
  title_text.setText(title);
  artist_text.setText(artist);
  time_right.setText(duration);
  startAnimation<&PlayerBar::setTextAlphaT>(text_alpha, 1.0f, 250.0f, CubicBezier::EaseOut());
  markLayoutDirty();
}

void PlayerBar::layoutChildren() {
  const float w = contentWidth();
  const float h = contentHeight();

  bg_.update(borderRect());

  // ─── 左列：封面 + 文字 ───
  const float cover_y = (h - kCoverSize) * 0.5f;
  cover_bg.update(SkRect::MakeXYWH(kPadH, cover_y, kCoverSize, kCoverSize));
  cover_svg.update(SkRect::MakeXYWH(kPadH, cover_y, kCoverSize, kCoverSize));

  const float text_x = kPadH + kCoverSize + 12.0f;
  const float text_w = kLeftW - kPadH - kCoverSize - 12.0f;
  title_text.update(SkRect::MakeXYWH(text_x, h * 0.5f - 16.0f, text_w, 16.0f));
  artist_text.update(SkRect::MakeXYWH(text_x, h * 0.5f + 4.0f, text_w, 14.0f));

  // ─── 中列：控制按钮行 + 进度条行 ───
  const float center_x = kLeftW;
  const float center_w = w - kLeftW - kRightW;
  const float ctrl_cx = center_x + center_w * 0.5f;

  // 按钮行（上半部分）
  const float btn_row_h = kPlayBtnR * 2.0f;
  const float prog_row_h = kProgressH + 10.0f;
  const float total_h = btn_row_h + kRowGap + prog_row_h;
  const float block_y = (h - total_h) * 0.5f;

  btn_cy = block_y + btn_row_h * 0.5f;
  btn_prev_cx = ctrl_cx - (kPlayBtnR + kSideBtnR + kCtrlGap);
  btn_play_cx = ctrl_cx;
  btn_next_cx = ctrl_cx + (kPlayBtnR + kSideBtnR + kCtrlGap);

  // 进度条行（下半部分）
  const float prog_cy = block_y + btn_row_h + kRowGap + prog_row_h * 0.5f;
  // 进度条中心由 paint 中的 drawProgressBar 使用

  // 时长文字
  constexpr float prog_half = kProgressMaxW * 0.5f;
  time_left.update(SkRect::MakeXYWH(ctrl_cx - prog_half - kTimeW - 8.0f, prog_cy - 7.0f, kTimeW, 14.0f));
  time_right.update(SkRect::MakeXYWH(ctrl_cx + prog_half + 8.0f, prog_cy - 7.0f, kTimeW, 14.0f));

  // ─── 右列：音量 ───
  // 音量在 paint 中直接绘制
}

void PlayerBar::render(SkCanvas *canvas) {
  canvas->save();
  canvas->translate(x_, y_);
  canvas->translate(0, (1.0f - slide_t) * kBarHeight);

  if (alpha_ < 0.999f) {
    canvas->saveLayerAlphaf(nullptr, alpha_);
  }

  paint(canvas);

  canvas->translate(padding_.left, padding_.top);
  for (const auto child : children_) {
    if (child->visible()) child->render(canvas);
  }

  if (alpha_ < 0.999f) canvas->restore();
  canvas->restore();
}

void PlayerBar::paint(SkCanvas *canvas) {
  const float w = width_;
  const float h = height_;
  const auto panel = SkRect::MakeXYWH(12.0f, 8.0f, w - 24.0f, h - 14.0f);
  constexpr float panel_r = 14.0f;

  // ─── 背景 ───
  {
    constexpr float sigma = 15.0f;
    auto shadow_rect = panel.makeOffset(0, 7.0f).makeInset(4.0f, 4.0f);
    auto layer_bounds = shadow_rect.makeOutset(sigma * 3.0f, sigma * 3.0f);
    SkPaint shadow_layer;
    shadow_layer.setImageFilter(SkImageFilters::Blur(sigma, sigma, nullptr));
    SkPaint shadow;
    shadow.setAntiAlias(true);
    shadow.setColor(ColorFromARGB(54, 10, 16, 26));
    canvas->saveLayer(&layer_bounds, &shadow_layer);
    canvas->drawRoundRect(shadow_rect, panel_r, panel_r, shadow);
    canvas->restore();

    SkPaint glass;
    glass.setAntiAlias(true);
    glass.setColor(kBarBg);
    canvas->drawRoundRect(panel, panel_r, panel_r, glass);

    SkPaint glow;
    glow.setAntiAlias(true);
    glow.setColor(ColorFromARGB(42, 255, 255, 255));
    canvas->drawRoundRect(SkRect::MakeXYWH(panel.fLeft + 1.0f, panel.fTop + 1.0f, panel.width() - 2.0f, 24.0f),
                          panel_r - 2.0f, panel_r - 2.0f, glow);
  }

  // ─── 顶部分割线 ───
  {
    SkPaint border;
    border.setAntiAlias(true);
    border.setStyle(SkPaint::kStroke_Style);
    border.setStrokeWidth(1.0f);
    border.setColor(kBarBorder);
    canvas->drawRoundRect(panel.makeInset(0.5f, 0.5f), panel_r, panel_r, border);
  }

  // ─── 封面 ───
  {
    SkPaint coverShadowLayer;
    coverShadowLayer.setImageFilter(SkImageFilters::Blur(6.0f, 6.0f, nullptr));
    SkPaint coverShadow;
    coverShadow.setAntiAlias(true);
    coverShadow.setColor(ColorFromARGB(42, 0, 0, 0));
    const float sx = kPadH;
    const float sy = (h - kCoverSize) * 0.5f + 4.0f;
    auto cover_shadow_rect = SkRect::MakeXYWH(sx + 2.0f, sy, kCoverSize - 4.0f, kCoverSize);
    auto cover_bounds = cover_shadow_rect.makeOutset(18.0f, 18.0f);
    canvas->saveLayer(&cover_bounds, &coverShadowLayer);
    canvas->drawRoundRect(cover_shadow_rect, kCoverRadius, kCoverRadius, coverShadow);
    canvas->restore();

    cover_bg.render(canvas);
    cover_svg.render(canvas);
    // 高光层
    SkPaint highlight;
    highlight.setAntiAlias(true);
    highlight.setColor(ColorFromARGB(44, 255, 255, 255));
    const float hx = kPadH;
    const float hy = (h - kCoverSize) * 0.5f;
    canvas->drawRoundRect(
      SkRect::MakeLTRB(hx, hy, hx + kCoverSize, hy + kCoverSize * 0.5f),
      kCoverRadius, kCoverRadius, highlight);
  }

  // ─── 文字 ───
  if (text_alpha > 0.01f) {
    canvas->saveLayerAlphaf(nullptr, text_alpha);
    title_text.render(canvas);
    artist_text.render(canvas);
    canvas->restore();
  }

  // ─── 时长 ───
  time_left.render(canvas);
  time_right.render(canvas);

  // ─── 控制按钮 ───
  SkPaint btn;
  btn.setAntiAlias(true);


  // 上一曲
  {
    canvas->save();
    const float s = 1.0f - btn_press_t * 0.08f;
    canvas->translate(btn_prev_cx, btn_cy);
    canvas->scale(s, s);
    canvas->translate(-btn_prev_cx, -btn_cy);
    btn.setStyle(SkPaint::kFill_Style);
    btn.setColor(hovered_btn == 1 ? kTextPrimary : kTextSecondary);
    drawPrevIcon(canvas, btn_prev_cx, btn_cy, btn);
    canvas->restore();
  }

  // 播放按钮
  {
    canvas->save();
    const float press_scale = 1.0f - play_press_t * 0.1f;
    canvas->translate(btn_play_cx, btn_cy);
    canvas->scale(press_scale, press_scale);
    canvas->translate(-btn_play_cx, -btn_cy);

    SkPaint playBg;
    playBg.setAntiAlias(true);
    playBg.setColor(ColorFromARGB(static_cast<U8CPU>(34.0f + play_hover_t * 44.0f), 255, 255, 255));
    canvas->drawCircle(btn_play_cx, btn_cy, kPlayBtnR + 7.0f + play_hover_t * 1.5f, playBg);

    btn.setColor(skia_colors::white);
    btn.setAlphaf(lerp(0.85f, 1.0f, play_hover_t));
    drawPlayIcon(canvas, btn_play_cx, btn_cy, 14.0f, btn);
    canvas->restore();
  }

  // 下一曲
  {
    canvas->save();
    const float s = 1.0f - btn_press_t * 0.08f;
    canvas->translate(btn_next_cx, btn_cy);
    canvas->scale(s, s);
    canvas->translate(-btn_next_cx, -btn_cy);
    btn.setColor(hovered_btn == 3 ? kTextPrimary : kTextSecondary);
    drawNextIcon(canvas, btn_next_cx, btn_cy, btn);
    canvas->restore();
  }

  // ─── 进度条 ───
  const float prog_cy = btn_cy + kPlayBtnR + kRowGap + kProgressH * 0.5f;
  drawProgressBar(canvas, w * 0.5f, prog_cy, kProgressMaxW);

  // ─── 右侧：音量 ───
  const float vol_x = w - kPadH - kVolWidth;
  drawSpeaker(canvas, vol_x - kVolIconW - 4.0f, h * 0.5f, btn);
  drawVolumeBar(canvas, vol_x, h * 0.5f, kVolWidth);
}

// ─── 进度条 ───
void PlayerBar::drawProgressBar(SkCanvas *canvas, float cx, float cy, float max_w) {
  const float x0 = cx - max_w * 0.5f;
  const float r = kProgressH * 0.5f;

  // 轨道
  SkPaint track;
  track.setAntiAlias(true);
  track.setColor(kProgressTrack);
  canvas->drawRoundRect(SkRect::MakeXYWH(x0, cy - r, max_w, kProgressH), r, r, track);

  // 填充
  const float fill_w = max_w * 0.3f;
  SkPaint fill;
  fill.setAntiAlias(true);
  fill.setColor(kProgressFill);
  canvas->drawRoundRect(SkRect::MakeXYWH(x0, cy - r, fill_w, kProgressH), r, r, fill);

  // 圆点滑块
  SkPaint dot;
  dot.setAntiAlias(true);
  dot.setColor(skia_colors::white);
  canvas->drawCircle(x0 + fill_w, cy, 4.0f, dot);
}

// ─── 音量条 ───
void PlayerBar::drawVolumeBar(SkCanvas *canvas, float x, float cy, float w) {
  const float r = kVolTrackH * 0.5f;

  SkPaint track;
  track.setAntiAlias(true);
  track.setColor(kVolTrack);
  canvas->drawRoundRect(SkRect::MakeXYWH(x, cy - r, w, kVolTrackH), r, r, track);

  const float fill_w = w * 0.7f;
  SkPaint fill;
  fill.setAntiAlias(true);
  fill.setColor(kVolFill);
  canvas->drawRoundRect(SkRect::MakeXYWH(x, cy - r, fill_w, kVolTrackH), r, r, fill);

  // 圆点
  SkPaint dot;
  dot.setAntiAlias(true);
  dot.setColor(skia_colors::white);
  canvas->drawCircle(x + fill_w, cy, kVolDotR, dot);
}

// ─── 命中检测 ───
int PlayerBar::hitTestButton(float x, float y) const {
  float dx, dy;

  dx = x - btn_prev_cx; dy = y - btn_cy;
  if (dx * dx + dy * dy < kSideBtnR * kSideBtnR) return 1;

  dx = x - btn_play_cx; dy = y - btn_cy;
  if (dx * dx + dy * dy < kPlayBtnR * kPlayBtnR) return 2;

  dx = x - btn_next_cx; dy = y - btn_cy;
  if (dx * dx + dy * dy < kSideBtnR * kSideBtnR) return 3;

  return 0;
}

void PlayerBar::onMouseMove(float x, float y) {
  const int btn = hitTestButton(x, y);
  if (btn != hovered_btn) {
    hovered_btn = btn;
    window()->setCursor(btn > 0 ? glfw::CursorType::Hand : glfw::CursorType::Arrow);
    startAnimation<&PlayerBar::setBtnHoverT>(btn_hover_t, (btn == 1 || btn == 3) ? 1.0f : 0.0f, 120.0f, CubicBezier::EaseOut());
    startAnimation<&PlayerBar::setPlayHoverT>(play_hover_t, btn == 2 ? 1.0f : 0.0f, 120.0f, CubicBezier::EaseOut());
  }
}

void PlayerBar::onMouseLeave(float, float) {
  hovered_btn = 0;
  window()->setCursor(glfw::CursorType::Arrow);
  startAnimation<&PlayerBar::setBtnHoverT>(btn_hover_t, 0.0f, 180.0f, CubicBezier::EaseOut());
  startAnimation<&PlayerBar::setPlayHoverT>(play_hover_t, 0.0f, 180.0f, CubicBezier::EaseOut());
}

void PlayerBar::onMouseLeftPressed(float, float) {
  if (hovered_btn == 1 || hovered_btn == 3) {
    startAnimation<&PlayerBar::setBtnPressT>(btn_press_t, 1.0f, 60.0f, CubicBezier::EaseOut());
  } else if (hovered_btn == 2) {
    startAnimation<&PlayerBar::setPlayPressT>(play_press_t, 1.0f, 60.0f, CubicBezier::EaseOut());
  }
}

void PlayerBar::onMouseLeftReleased(float, float) {
  if (btn_press_t > 0.0f) {
    startAnimation<&PlayerBar::setBtnPressT>(btn_press_t, 0.0f, 180.0f, CubicBezier::EaseOut());
  }
  if (play_press_t > 0.0f) {
    startAnimation<&PlayerBar::setPlayPressT>(play_press_t, 0.0f, 180.0f, CubicBezier::EaseOut());
  }
}

// ─── 图标 ───

// 上一首图标：竖线 + 左三角，整体水平居中
// 总宽 = barW(2) + gap(2.5) + triW(7.5) = 12, 中心在 cx-0.25 ≈ cx
void PlayerBar::drawPrevIcon(SkCanvas *c, float cx, float cy, const SkPaint &p) {
  // 竖线（偏左）
  c->drawRect(SkRect::MakeXYWH(cx - 6.0f, cy - 6.5f, 2.0f, 13.0f), p);
  // 三角（偏左）
  SkPathBuilder b;
  b.moveTo(SkPoint::Make(cx - 2.5f, cy - 6.5f));
  b.lineTo(SkPoint::Make(cx + 5.5f, cy));
  b.lineTo(SkPoint::Make(cx - 2.5f, cy + 6.5f));
  b.close();
  c->drawPath(b.detach(), p);
}

// 播放图标：居中三角
void PlayerBar::drawPlayIcon(SkCanvas *c, float cx, float cy, float sz, const SkPaint &p) {
  SkPathBuilder b;
  b.moveTo(SkPoint::Make(cx + sz * 0.42f, cy));
  b.lineTo(SkPoint::Make(cx - sz * 0.38f, cy - sz * 0.55f));
  b.lineTo(SkPoint::Make(cx - sz * 0.38f, cy + sz * 0.55f));
  b.close();
  c->drawPath(b.detach(), p);
}

// 下一首图标：右三角 + 竖线，整体水平居中
void PlayerBar::drawNextIcon(SkCanvas *c, float cx, float cy, const SkPaint &p) {
  // 三角（偏右）
  SkPathBuilder b;
  b.moveTo(SkPoint::Make(cx - 5.5f, cy - 6.5f));
  b.lineTo(SkPoint::Make(cx + 2.5f, cy));
  b.lineTo(SkPoint::Make(cx - 5.5f, cy + 6.5f));
  b.close();
  c->drawPath(b.detach(), p);
  // 竖线（偏右）
  c->drawRect(SkRect::MakeXYWH(cx + 4.0f, cy - 6.5f, 2.0f, 13.0f), p);
}

void PlayerBar::drawSpeaker(SkCanvas *c, float x, float cy, const SkPaint &p) {
  SkPaint sp = p;
  sp.setColor(kTextMuted);
  sp.setStyle(SkPaint::kFill_Style);

  SkPathBuilder body;
  body.moveTo(SkPoint::Make(x + 2, cy - 4));
  body.lineTo(SkPoint::Make(x + 5, cy - 4));
  body.lineTo(SkPoint::Make(x + 9, cy - 7.5f));
  body.lineTo(SkPoint::Make(x + 9, cy + 7.5f));
  body.lineTo(SkPoint::Make(x + 5, cy + 4));
  body.lineTo(SkPoint::Make(x + 2, cy + 4));
  body.close();
  c->drawPath(body.detach(), sp);

  sp.setStyle(SkPaint::kStroke_Style);
  sp.setStrokeWidth(1.2f);
  SkPathBuilder wave;
  wave.moveTo(SkPoint::Make(x + 11, cy - 3.5f));
  wave.quadTo(SkPoint::Make(x + 13.5f, cy), SkPoint::Make(x + 11, cy + 3.5f));
  c->drawPath(wave.detach(), sp);
}

} // namespace components
