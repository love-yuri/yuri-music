//
// Created by yuri on 2026/5/4.
//
export module components:player_bar;

import std;
import ui;
import skia;
import core;
import qq_music_api;
import thread_pool;
import models;

using namespace ui::render;
using namespace ui::layout;
using namespace ui::widgets;
using namespace ui::animation;
using namespace ui::algorithm;
using namespace skia;

constexpr SkColor kBarBg = ColorFromARGB(186, 255, 255, 255);
constexpr SkColor kBarBorder = ColorFromARGB(126, 255, 255, 255);
constexpr SkColor kBarTopLine = ColorFromARGB(52, 42, 52, 68);
constexpr SkColor kAccent = ColorFromARGB(255, 218, 52, 92);
constexpr SkColor kAccentHot = ColorFromARGB(255, 255, 76, 119);
constexpr SkColor kInk = ColorFromARGB(255, 17, 24, 36);
constexpr SkColor kProgressTrack = ColorFromARGB(78, 54, 65, 82);
constexpr SkColor kProgressFill = ColorFromARGB(255, 218, 52, 92);
constexpr SkColor kTextPrimary = ColorFromARGB(255, 20, 26, 36);
constexpr SkColor kTextSecondary = ColorFromARGB(176, 56, 68, 86);
constexpr SkColor kTextMuted = ColorFromARGB(122, 54, 65, 82);
constexpr SkColor kPlayerCoverColor = ColorFromARGB(255, 255, 76, 119);
constexpr SkColor kVolTrack = ColorFromARGB(50, 54, 65, 82);
constexpr SkColor kVolFill = ColorFromARGB(188, 54, 65, 82);
constexpr SkColor kVolThumb = ColorFromARGB(255, 82, 94, 112);

// ─── 布局常量 ───
static constexpr float kBarHeight = 86.0f;
static constexpr float kPadH = 34.0f;
static constexpr float kCoverSize = 52.0f;
static constexpr float kCoverRadius = 9.0f;
static constexpr float kPlayBtnR = 14.0f;
static constexpr float kPlayBtnBgR = kPlayBtnR + 10.0f;
static constexpr float kSideBtnR = 13.0f;
static constexpr float kCtrlGap = 26.0f;
static constexpr float kControlIslandH = 42.0f;
static constexpr float kControlBlockOffsetY = 3.0f;
static constexpr float kVolWidth = 92.0f;
static constexpr float kVolTrackH = 4.0f;
static constexpr float kVolDotR = 5.5f;
static constexpr float kVolIconW = 16.0f;
static constexpr float kTimeW = 42.0f;
static constexpr float kProgressMaxW = 560.0f;
static constexpr float kProgressMinW = 180.0f;
static constexpr float kProgressH = 4.0f;
static constexpr float kLeftW = 292.0f;
static constexpr float kRightW = 214.0f;
static constexpr float kRowGap = 2.0f;
static constexpr float kPi = 3.14159265358979323846f;

/**
 * 将秒数格式化为 m:ss 形式。
 * @param seconds 秒数
 * @return 格式化后的时长字符串
 */
std::string formatTime(double seconds);

/**
 * 将 m:ss 格式的时长文本解析为秒数。
 * @param text 时长文本
 * @return 解析后的秒数；解析失败返回 -1
 */
int parseTimeText(std::string_view text);

/**
 * 绘制上一首图标（竖线 + 左三角）。
 * @param c Skia 画布
 * @param cx 图标中心 X 坐标
 * @param cy 图标中心 Y 坐标
 * @param p 画笔
 */
void drawPrevIcon(SkCanvas *c, float cx, float cy, const SkPaint &p);

/**
 * 绘制播放图标（居中三角）。
 * @param c Skia 画布
 * @param cx 图标中心 X 坐标
 * @param cy 图标中心 Y 坐标
 * @param sz 三角尺寸
 * @param p 画笔
 */
void drawPlayIcon(SkCanvas *c, float cx, float cy, float sz, const SkPaint &p);

/**
 * 绘制暂停图标（双竖线）。
 * @param c Skia 画布
 * @param cx 图标中心 X 坐标
 * @param cy 图标中心 Y 坐标
 * @param p 画笔
 */
void drawPauseIcon(SkCanvas *c, float cx, float cy, const SkPaint &p);

/**
 * 绘制加载旋转动画。
 * @param c Skia 画布
 * @param cx 动画中心 X 坐标
 * @param cy 动画中心 Y 坐标
 */
void drawLoadingSpinner(SkCanvas *c, float cx, float cy);

/**
 * 绘制下一首图标（右三角 + 竖线）。
 * @param c Skia 画布
 * @param cx 图标中心 X 坐标
 * @param cy 图标中心 Y 坐标
 * @param p 画笔
 */
void drawNextIcon(SkCanvas *c, float cx, float cy, const SkPaint &p);

/**
 * 绘制扬声器图标。
 * @param c Skia 画布
 * @param x 图标左上角 X 坐标
 * @param cy 图标垂直中心 Y 坐标
 * @param p 画笔
 */
void drawSpeaker(SkCanvas *c, float x, float cy, const SkPaint &p);

export namespace components {

/**
 * 播放器底栏 — macOS Music.app 风格
 *
 * 左：封面(52x52) + 歌曲信息
 * 中：控制按钮行 + 进度条行
 * 右：音量控制
 */
class PlayerBar : public Widget {
public:
  /**
   * 创建播放器底栏。
   * @param parent 父级控件
   */
  explicit PlayerBar(Widget *parent = nullptr);

  Signal<> previousClicked;      // 上一首点击信号
  Signal<> playPauseClicked;     // 播放/暂停点击信号
  Signal<> nextClicked;          // 下一首点击信号
  Signal<double> seekRequested;  // 进度跳转请求信号
  Signal<float> volumeChanged;   // 音量变化信号

  /** 布局子控件。 */
  void layoutChildren() override;

  /** 绘制播放器底栏。 */
  void paint(SkCanvas *canvas) override;

  /** 渲染播放器底栏及其子控件。 */
  void render(SkCanvas *canvas) override;

  /** 显示播放器底栏。 */
  void show();

  /** 隐藏播放器底栏。 */
  void hide();

  /**
   * 更新当前播放歌曲信息。
   * @param info 歌曲信息
   */
  void updateSong(const SongInfo& info);

  /**
   * 设置播放状态。
   * @param value 是否正在播放
   */
  void setPlaying(bool value) noexcept;

  /**
   * 设置加载状态。
   * @param value 是否正在加载
   */
  void setLoading(bool value) noexcept;

  /**
   * 更新播放进度。
   * @param position_seconds 当前播放位置（秒）
   * @param duration_seconds 歌曲总时长（秒）
   */
  void setPlaybackPosition(double position_seconds, double duration_seconds);

  /**
   * 设置音量。
   * @param value 音量值 [0, 1]
   */
  void setVolume(float value) noexcept;

  /**
   * 是否正在展示（含动画中）。
   * @return 是否展示中
   */
  [[nodiscard]] bool showing() const noexcept {
    return showing_;
  }

protected:
  /** 鼠标移动事件。 */
  void onMouseMove(float x, float y) override;

  /** 鼠标离开事件。 */
  void onMouseLeave(float x, float y) override;

  /** 鼠标左键按下事件。 */
  void onMouseLeftPressed(float x, float y) override;

  /** 鼠标左键释放事件。 */
  void onMouseLeftReleased(float x, float y) override;

private:
  /**
   * 检测鼠标是否悬浮在控制按钮上。
   * @param x 鼠标 X 坐标
   * @param y 鼠标 Y 坐标
   * @return 按钮索引（0: 无, 1: 上一首, 2: 播放, 3: 下一首）
   */
  [[nodiscard]] int hitTestButton(float x, float y) const;

  /**
   * 检测鼠标是否悬浮在进度条上。
   * @param x 鼠标 X 坐标
   * @param y 鼠标 Y 坐标
   * @return 是否在进度条范围内
   */
  [[nodiscard]] bool hitTestProgress(float x, float y) const;

  /**
   * 检测鼠标是否悬浮在音量条上。
   * @param x 鼠标 X 坐标
   * @param y 鼠标 Y 坐标
   * @return 是否在音量条范围内
   */
  [[nodiscard]] bool hitTestVolume(float x, float y) const;

  /**
   * 根据鼠标位置计算进度比例。
   * @param x 鼠标 X 坐标
   * @return 进度比例 [0, 1]
   */
  [[nodiscard]] double progressRatioFromPoint(float x) const;

  /**
   * 从鼠标位置请求进度跳转。
   * @param x 鼠标 X 坐标
   */
  void requestSeekFromPoint(float x);

  /**
   * 从鼠标位置更新音量。
   * @param x 鼠标 X 坐标
   */
  void updateVolumeFromPoint(float x);

  /**
   * 绘制控制按钮背景岛。
   * @param c Skia 画布
   */
  void drawControlIsland(SkCanvas *c) const;

  /**
   * 绘制侧边按钮悬浮光环。
   * @param c Skia 画布
   * @param cx 按钮中心 X 坐标
   * @param cy 按钮中心 Y 坐标
   * @param active 是否激活
   */
  void drawSideButtonHalo(SkCanvas *c, float cx, float cy, bool active) const;

  /**
   * 绘制进度条。
   * @param canvas Skia 画布
   * @param cx 进度条中心 X 坐标
   * @param cy 进度条中心 Y 坐标
   * @param max_w 进度条最大宽度
   */
  void drawProgressBar(SkCanvas *canvas, float cx, float cy, float max_w) const;

  /**
   * 绘制音量条。
   * @param canvas Skia 画布
   * @param x 音量条左上角 X 坐标
   * @param cy 音量条垂直中心 Y 坐标
   * @param w 音量条宽度
   */
  void drawVolumeBar(SkCanvas *canvas, float x, float cy, float w) const;

  /**
   * 异步加载专辑封面图片。
   * @param album_mid 专辑 MID
   */
  void loadCoverImage(std::string_view album_mid);

  // 渲染节点
  RenderBackground bg_;              // 背景
  RenderBackground cover_bg;         // 封面色块
  RenderSvg cover_svg;               // 封面图标
  float cover_radius = kCoverRadius; // 封面圆角
  sk_sp<SkImage> cover_image{};      // 专辑封面图片
  RenderText title_text;             // 歌曲标题
  RenderText artist_text;            // 歌手名
  RenderText time_left;              // 已播放时长
  RenderText time_right;             // 总时长

  // 动画
  float slide_t = 0.0f;      // 滑入进度
  float alpha_ = 0.0f;       // 整体透明度
  float btn_hover_t = 0.0f;  // 侧按钮悬浮
  float btn_press_t = 0.0f;  // 侧按钮按下
  float play_hover_t = 0.0f; // 播放按钮悬浮
  float play_press_t = 0.0f; // 播放按钮按下
  float text_alpha = 1.0f;   // 文字透明度

  // 按钮中心坐标（布局时计算）
  float btn_prev_cx = 0.0f;         // 上一首 X
  float btn_play_cx = 0.0f;         // 播放 X
  float btn_next_cx = 0.0f;         // 下一首 X
  float btn_cy = 0.0f;              // 按钮 Y
  float progress_cx = 0.0f;         // 进度条中心 X
  float progress_cy = 0.0f;         // 进度条中心 Y
  float progress_w = kProgressMaxW; // 进度条宽度
  float volume_x = 0.0f;            // 音量条 X
  float volume_cy = 0.0f;           // 音量条中心 Y

  int hovered_btn = 0;              // 当前悬浮按钮
  bool showing_ = false;            // 是否显示
  bool playing_ = false;            // 是否正在播放
  bool loading_ = false;            // 是否正在加载当前歌曲
  bool progress_dragging = false;   // 是否正在拖动进度
  bool volume_dragging = false;     // 是否正在拖动音量
  double pending_seek_ratio = 0.0;  // 拖动结束后应用的目标进度
  double duration_seconds = 0.0;    // 当前歌曲时长
  int elapsed_display_seconds = -1; // 当前显示的已播放秒数
  int duration_display_second = -1; // 当前显示的总时长秒数
  float progress_ = 0.0f;           // 当前播放进度 0..1
  float volume_ = 1.0f;             // 当前音量 0..1
};

PlayerBar::PlayerBar(Widget *parent) : Widget(parent), cover_svg("resources/svg/play.svg") {
  bg_.setColor(kBarBg);

  cover_bg.setColor(kPlayerCoverColor);
  cover_bg.radius = &cover_radius;
  cover_svg.setAlignment(Alignment::Center);

  title_text.setFont(font::bold_font);
  title_text.setFontSize(14.5f);
  title_text.setColor(kTextPrimary);
  title_text.setAlignment(Alignment::BottomLeft);

  artist_text.setFontSize(12.5f);
  artist_text.setColor(kTextSecondary);
  artist_text.setAlignment(Alignment::TopLeft);

  time_left.setFontSize(11.5f);
  time_left.setColor(kTextMuted);
  time_left.setAlignment(Alignment::CenterRight);
  time_left.setText("0:00");

  time_right.setFontSize(11.5f);
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
  startAnimation(slide_t, 1.0f, 350.0f, &slide_t, CubicBezier::EaseOut());
  startAnimation(alpha_, 1.0f, 350.0f, &alpha_, CubicBezier::EaseOut());
}

void PlayerBar::hide() {
  if (!showing_) return;
  showing_ = false;
  setVisible(false);
  markLayoutDirty();
}

void PlayerBar::updateSong(const SongInfo& info) {
  text_alpha = 0.0f;
  title_text.setText(info.title);
  artist_text.setText(info.artist);
  time_right.setText(info.duration);
  time_left.setText("0:00");
  progress_ = 0.0f;
  duration_display_second = parseTimeText(info.duration);
  duration_seconds = static_cast<double>(std::max(0, duration_display_second));
  elapsed_display_seconds = 0;
  loadCoverImage(info.album_mid);
  startAnimation(text_alpha, 1.0f, 250.0f, &text_alpha, CubicBezier::EaseOut());
  markLayoutDirty();
}

void PlayerBar::setPlaying(const bool value) noexcept {
  if (playing_ == value) return;
  playing_ = value;
  markLayoutDirty();
}

void PlayerBar::setLoading(const bool value) noexcept {
  if (loading_ == value) return;
  loading_ = value;
  markLayoutDirty();
}

void PlayerBar::setPlaybackPosition(const double position_seconds, double duration_seconds) {
  const double safe_position = std::max(0.0, position_seconds);
  const double safe_duration = std::max(0.0, duration_seconds);
  const double effective_duration = safe_duration > 0.001 ? safe_duration : this->duration_seconds;
  const float next_progress =
    effective_duration > 0.001 ?
      static_cast<float>(std::clamp(safe_position / effective_duration, 0.0, 1.0)) :
      progress_;

  bool changed = std::abs(progress_ - next_progress) > 0.001f;
  progress_ = next_progress;

  if (const int elapsed_seconds = static_cast<int>(safe_position);
      elapsed_seconds != elapsed_display_seconds) {
    elapsed_display_seconds = elapsed_seconds;
    time_left.setText(formatTime(elapsed_seconds));
    changed = true;
  }

  if (safe_duration > 0.001) {
    this->duration_seconds = safe_duration;
    duration_seconds = static_cast<int>(std::lround(safe_duration));
    if (duration_seconds != duration_display_second) {
      duration_display_second = static_cast<int>(duration_seconds);
      time_right.setText(formatTime(duration_seconds));
      changed = true;
    }
  }

  if (changed) {
    markLayoutDirty();
  }
}

void PlayerBar::setVolume(const float value) noexcept {
  const float next = std::clamp(value, 0.0f, 1.0f);
  if (std::abs(volume_ - next) <= 0.001f) return;
  volume_ = next;
  markLayoutDirty();
}

void PlayerBar::loadCoverImage(const std::string_view album_mid) {
  if (album_mid.empty()) {
    return;
  }

  const auto mid = std::string(album_mid);
  thread_manager->addTask([this, mid] {
    const auto image_url = qqmusic_api::album::get_album_cover(300, mid);
    const auto image_data = curl::get(image_url).value();
    cover_image = decodeImage(image_data);
  });
}

void PlayerBar::layoutChildren() {
  const float w = contentWidth();
  const float h = contentHeight();

  bg_.update(borderRect());

  // ─── 左列：封面 + 文字 ───
  const float cover_y = std::round((h - kCoverSize) * 0.5f);
  cover_bg.update(SkRect::MakeXYWH(kPadH, cover_y, kCoverSize, kCoverSize));
  cover_svg.update(SkRect::MakeXYWH(kPadH, cover_y, kCoverSize, kCoverSize));

  constexpr float kTextGapX = 14.0f;
  constexpr float kTitleHeight = 19.0f;
  constexpr float kArtistHeight = 17.0f;
  constexpr float kTextGapY = 5.0f;
  constexpr float kTextBlockHeight = kTitleHeight + kTextGapY + kArtistHeight;
  constexpr float kTextX = kPadH + kCoverSize + kTextGapX;
  constexpr float kTextWidth = kLeftW - kPadH - kCoverSize - kTextGapX;
  const float text_y = std::round((h - kTextBlockHeight) * 0.5f);
  title_text.update(SkRect::MakeXYWH(kTextX, text_y, kTextWidth, kTitleHeight));
  artist_text.update(SkRect::MakeXYWH(kTextX, text_y + kTitleHeight + kTextGapY, kTextWidth, kArtistHeight));

  // ─── 中列：控制按钮行 + 进度条行 ───
  constexpr float kCenterX = kLeftW;
  const float center_w = std::max(0.0f, w - kLeftW - kRightW);
  const float ctrl_cx = std::round(kCenterX + center_w * 0.5f);

  // 按钮行（上半部分）
  constexpr float kButtonRowHeight = kPlayBtnBgR * 2.0f;
  constexpr float kProgressRowHeight = kProgressH + 12.0f;
  constexpr float kTotalHeight = kButtonRowHeight + kRowGap + kProgressRowHeight;
  const float block_y = std::round((h - kTotalHeight) * 0.5f + kControlBlockOffsetY);

  btn_cy = std::round(block_y + kButtonRowHeight * 0.5f);
  btn_prev_cx = std::round(ctrl_cx - (kPlayBtnR + kSideBtnR + kCtrlGap));
  btn_play_cx = ctrl_cx;
  btn_next_cx = std::round(ctrl_cx + (kPlayBtnR + kSideBtnR + kCtrlGap));

  // 进度条行（下半部分）
  progress_cx = ctrl_cx;
  progress_cy = std::round(block_y + kButtonRowHeight + kRowGap + kProgressRowHeight * 0.5f);
  progress_w = std::round(std::clamp(center_w - kTimeW * 2.0f - 36.0f, kProgressMinW, kProgressMaxW));

  // 时长文字
  const float prog_half = progress_w * 0.5f;
  constexpr float kTimeGap = 10.0f;
  constexpr float kTimeHeight = 16.0f;
  const float time_y = std::round(progress_cy - kTimeHeight * 0.5f);
  time_left.update(SkRect::MakeXYWH(progress_cx - prog_half - kTimeW - kTimeGap, time_y, kTimeW, kTimeHeight));
  time_right.update(SkRect::MakeXYWH(progress_cx + prog_half + kTimeGap, time_y, kTimeW, kTimeHeight));

  // ─── 右列：音量 ───
  volume_x = std::round(w - kPadH - kVolWidth);
  volume_cy = std::round(h * 0.5f);
}

void PlayerBar::render(SkCanvas *canvas) {
  canvas->save();
  canvas->translate(x_, y_);
  canvas->translate(0, std::round((1.0f - slide_t) * kBarHeight));

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
  const auto panel = SkRect::MakeXYWH(14.0f, 6.0f, w - 28.0f, h - 12.0f);
  constexpr float kPanelRadius = 15.0f;

  // ─── 背景 ───
  {
    constexpr float sigma = 12.0f;
    auto shadow_rect = panel.makeOffset(0, 7.0f).makeInset(7.0f, 7.0f);
    auto layer_bounds = shadow_rect.makeOutset(sigma * 3.0f, sigma * 3.0f);
    SkPaint shadow_layer;
    shadow_layer.setImageFilter(SkImageFilters::Blur(sigma, sigma, nullptr));
    SkPaint shadow;
    shadow.setAntiAlias(true);
    shadow.setColor(ColorFromARGB(24, 25, 36, 52));
    canvas->saveLayer(&layer_bounds, &shadow_layer);
    canvas->drawRoundRect(shadow_rect, kPanelRadius, kPanelRadius, shadow);
    canvas->restore();

    SkPaint topLine;
    topLine.setAntiAlias(true);
    topLine.setColor(kBarTopLine);
    canvas->drawRect(SkRect::MakeXYWH(0.0f, 0.0f, w, 1.0f), topLine);

    SkPaint glass;
    glass.setAntiAlias(true);
    glass.setColor(kBarBg);
    canvas->drawRoundRect(panel, kPanelRadius, kPanelRadius, glass);

    SkPaint glow;
    glow.setAntiAlias(true);
    glow.setColor(ColorFromARGB(50, 255, 255, 255));
    canvas->drawRoundRect(
      SkRect::MakeXYWH(panel.fLeft + 1.0f, panel.fTop + 1.0f, panel.width() - 2.0f, 24.0f),
      kPanelRadius - 2.0f, kPanelRadius - 2.0f, glow);
  }

  // ─── 顶部分割线 ───
  {
    SkPaint border;
    border.setAntiAlias(true);
    border.setStyle(SkPaint::kStroke_Style);
    border.setStrokeWidth(1.0f);
    border.setColor(kBarBorder);
    canvas->drawRoundRect(panel.makeInset(0.5f, 0.5f), kPanelRadius, kPanelRadius, border);
  }

  // ─── 封面 ───
  {
    SkPaint coverShadowLayer;
    coverShadowLayer.setImageFilter(SkImageFilters::Blur(6.0f, 6.0f, nullptr));
    SkPaint coverShadow;
    coverShadow.setAntiAlias(true);
    coverShadow.setColor(ColorFromARGB(34, 25, 36, 52));
    const float sx = kPadH;
    const float sy = std::round((h - kCoverSize) * 0.5f) + 4.0f;
    auto cover_shadow_rect = SkRect::MakeXYWH(sx + 2.0f, sy, kCoverSize - 4.0f, kCoverSize);
    auto cover_bounds = cover_shadow_rect.makeOutset(18.0f, 18.0f);
    canvas->saveLayer(&cover_bounds, &coverShadowLayer);
    canvas->drawRoundRect(cover_shadow_rect, kCoverRadius, kCoverRadius, coverShadow);
    canvas->restore();

    if (cover_image) {
      const float cover_y = std::round((h - kCoverSize) * 0.5f);
      auto cover_rect = SkRect::MakeXYWH(kPadH, cover_y, kCoverSize, kCoverSize);
      canvas->save();
      canvas->clipRRect(SkRRect::MakeRectXY(cover_rect, kCoverRadius, kCoverRadius), true);
      canvas->drawImageRect(cover_image, cover_rect, SkSamplingOptions(SkFilterMode::kLinear, SkMipmapMode::kLinear), nullptr);
      canvas->restore();
    } else {
      cover_bg.render(canvas);
      cover_svg.render(canvas);
    }

    // 高光层
    SkPaint highlight;
    highlight.setAntiAlias(true);
    highlight.setColor(ColorFromARGB(44, 255, 255, 255));
    constexpr float hx = kPadH;
    const float hy = std::round((h - kCoverSize) * 0.5f);
    canvas->drawRoundRect(SkRect::MakeLTRB(hx, hy, hx + kCoverSize, hy + kCoverSize * 0.5f),
                          kCoverRadius, kCoverRadius, highlight);
  }

  // ─── 文字 ───
  if (text_alpha > 0.01f) {
    if (text_alpha < 0.999f) {
      canvas->saveLayerAlphaf(nullptr, text_alpha);
    }
    title_text.render(canvas);
    artist_text.render(canvas);
    if (text_alpha < 0.999f) {
      canvas->restore();
    }
  }

  // ─── 时长 ───
  time_left.render(canvas);
  time_right.render(canvas);

  // ─── 控制按钮 ───
  drawControlIsland(canvas);

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
    btn.setAlphaf(1.0f);
    btn.setColor(hovered_btn == 1 ? kInk : kTextSecondary);
    drawSideButtonHalo(canvas, btn_prev_cx, btn_cy, hovered_btn == 1);
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
    playBg.setColor(lerp(kAccent, kAccentHot, play_hover_t));
    canvas->drawCircle(btn_play_cx, btn_cy, kPlayBtnBgR + play_hover_t * 0.8f, playBg);

    SkPaint playShine;
    playShine.setAntiAlias(true);
    playShine.setColor(
      ColorFromARGB(static_cast<U8CPU>(22.0f + play_hover_t * 14.0f), 255, 255, 255));
    canvas->drawCircle(btn_play_cx - 5.0f, btn_cy - 5.5f, 4.0f, playShine);

    btn.setColor(skia_colors::white);
    btn.setAlphaf(lerp(0.92f, 1.0f, play_hover_t));
    if (loading_) {
      drawLoadingSpinner(canvas, btn_play_cx, btn_cy);
    } else if (playing_) {
      drawPauseIcon(canvas, btn_play_cx, btn_cy, btn);
    } else {
      drawPlayIcon(canvas, btn_play_cx, btn_cy, 14.0f, btn);
    }
    canvas->restore();
  }

  // 下一曲
  {
    canvas->save();
    const float s = 1.0f - btn_press_t * 0.08f;
    canvas->translate(btn_next_cx, btn_cy);
    canvas->scale(s, s);
    canvas->translate(-btn_next_cx, -btn_cy);
    btn.setAlphaf(1.0f);
    btn.setColor(hovered_btn == 3 ? kInk : kTextSecondary);
    drawSideButtonHalo(canvas, btn_next_cx, btn_cy, hovered_btn == 3);
    drawNextIcon(canvas, btn_next_cx, btn_cy, btn);
    canvas->restore();
  }

  // ─── 进度条 ───
  drawProgressBar(canvas, progress_cx, progress_cy, progress_w);

  // ─── 右侧：音量 ───
  btn.setAlphaf(1.0f);
  drawSpeaker(canvas, volume_x - kVolIconW - 4.0f, volume_cy, btn);
  drawVolumeBar(canvas, volume_x, volume_cy, kVolWidth);
}

void PlayerBar::drawControlIsland(SkCanvas *c) const {
  const auto island = SkRect::MakeXYWH(btn_prev_cx - 34.0f, btn_cy - kControlIslandH * 0.5f,
                                       btn_next_cx - btn_prev_cx + 68.0f, kControlIslandH);
  constexpr float island_r = kControlIslandH * 0.5f;

  SkPaint shadow_layer;
  shadow_layer.setImageFilter(SkImageFilters::Blur(10.0f, 10.0f, nullptr));
  SkPaint shadow;
  shadow.setAntiAlias(true);
  shadow.setColor(ColorFromARGB(24, 24, 31, 42));
  auto shadow_rect = island.makeOffset(0.0f, 6.0f).makeInset(6.0f, 6.0f);
  auto bounds = shadow_rect.makeOutset(30.0f, 30.0f);
  c->saveLayer(&bounds, &shadow_layer);
  c->drawRoundRect(shadow_rect, island_r - 4.0f, island_r - 4.0f, shadow);
  c->restore();

  SkPaint fill;
  fill.setAntiAlias(true);
  fill.setColor(ColorFromARGB(148, 255, 255, 255));
  c->drawRoundRect(island, island_r, island_r, fill);

  SkPaint tint;
  tint.setAntiAlias(true);
  tint.setColor(ColorFromARGB(static_cast<U8CPU>(14.0f + play_hover_t * 8.0f), 255, 76, 119));
  c->drawCircle(btn_play_cx, btn_cy, 24.0f + play_hover_t * 1.0f, tint);

  SkPaint halo;
  halo.setAntiAlias(true);
  halo.setColor(ColorFromARGB(static_cast<U8CPU>(15.0f + play_hover_t * 9.0f), 218, 52, 92));
  c->drawCircle(btn_play_cx, btn_cy, kPlayBtnBgR + 2.0f + play_hover_t * 1.0f, halo);

  SkPaint border;
  border.setAntiAlias(true);
  border.setStyle(SkPaint::kStroke_Style);
  border.setStrokeWidth(1.0f);
  border.setColor(ColorFromARGB(142, 255, 255, 255));
  c->drawRoundRect(island.makeInset(0.5f, 0.5f), island_r - 0.5f, island_r - 0.5f, border);
}

void PlayerBar::drawSideButtonHalo(SkCanvas *c, const float cx, const float cy, const bool active) const {
  const float t = active ? btn_hover_t : 0.0f;
  if (t <= 0.01f) return;

  SkPaint halo;
  halo.setAntiAlias(true);
  halo.setColor(ColorFromARGB(static_cast<U8CPU>(34.0f + t * 32.0f), 255, 255, 255));
  c->drawCircle(cx, cy, 18.0f + t * 1.5f, halo);

  SkPaint rim;
  rim.setAntiAlias(true);
  rim.setStyle(SkPaint::kStroke_Style);
  rim.setStrokeWidth(1.0f);
  rim.setColor(ColorFromARGB(static_cast<U8CPU>(42.0f * t), 218, 52, 92));
  c->drawCircle(cx, cy, 18.0f, rim);
}

// ─── 进度条 ───
void PlayerBar::drawProgressBar(SkCanvas *canvas, float cx, float cy, float max_w) const {
  const float x0 = std::round(cx - max_w * 0.5f);
  const float y0 = std::round(cy - kProgressH * 0.5f);
  const float w = std::round(max_w);
  constexpr float r = kProgressH * 0.5f;

  // 轨道
  SkPaint track;
  track.setAntiAlias(true);
  track.setColor(kProgressTrack);
  canvas->drawRoundRect(SkRect::MakeXYWH(x0, y0, w, kProgressH), r, r, track);

  // 填充
  const float fill_w = std::round(w * std::clamp(progress_, 0.0f, 1.0f));
  if (fill_w <= 0.0f) return;

  SkPaint fill;
  fill.setAntiAlias(true);
  fill.setColor(kProgressFill);
  canvas->drawRoundRect(SkRect::MakeXYWH(x0, y0, fill_w, kProgressH), r, r, fill);
}

// ─── 音量条 ───
void PlayerBar::drawVolumeBar(SkCanvas *canvas, const float x, const float cy, const float w) const {
  constexpr float kRadius = kVolTrackH * 0.5f;
  const float x0 = std::round(x);
  const float y0 = std::round(cy - kRadius);
  const float width = std::round(w);

  SkPaint track;
  track.setAntiAlias(true);
  track.setColor(kVolTrack);
  canvas->drawRoundRect(SkRect::MakeXYWH(x0, y0, width, kVolTrackH), kRadius, kRadius, track);

  const float fill_w = std::round(width * std::clamp(volume_, 0.0f, 1.0f));
  SkPaint fill;
  fill.setAntiAlias(true);
  fill.setColor(kVolFill);
  canvas->drawRoundRect(SkRect::MakeXYWH(x0, y0, fill_w, kVolTrackH), kRadius, kRadius, fill);

  // 圆点
  SkPaint dot;
  dot.setAntiAlias(true);
  dot.setColor(kVolThumb);
  canvas->drawCircle(x0 + fill_w, std::round(cy), kVolDotR, dot);
}

// ─── 命中检测 ───
int PlayerBar::hitTestButton(float x, float y) const {
  float dx, dy;

  dx = x - btn_prev_cx;
  dy = y - btn_cy;
  if (dx * dx + dy * dy < (kSideBtnR + 8.0f) * (kSideBtnR + 8.0f)) return 1;

  dx = x - btn_play_cx;
  dy = y - btn_cy;
  if (dx * dx + dy * dy < kPlayBtnBgR * kPlayBtnBgR) return 2;

  dx = x - btn_next_cx;
  dy = y - btn_cy;
  if (dx * dx + dy * dy < (kSideBtnR + 8.0f) * (kSideBtnR + 8.0f)) return 3;

  return 0;
}

bool PlayerBar::hitTestProgress(const float x, const float y) const {
  const float x0 = progress_cx - progress_w * 0.5f;
  return x >= x0 - 4.0f && x <= x0 + progress_w + 4.0f && std::abs(y - progress_cy) <= 8.0f;
}

bool PlayerBar::hitTestVolume(const float x, const float y) const {
  return x >= volume_x - 4.0f && x <= volume_x + kVolWidth + 4.0f
         && std::abs(y - volume_cy) <= 10.0f;
}

double PlayerBar::progressRatioFromPoint(const float x) const {
  const float x0 = progress_cx - progress_w * 0.5f;
  return progress_w > 0.0f ? std::clamp((x - x0) / progress_w, 0.0f, 1.0f) : 0.0;
}

void PlayerBar::requestSeekFromPoint(const float x) {
  const double ratio = progressRatioFromPoint(x);
  progress_ = static_cast<float>(ratio);
  if (duration_seconds > 0.001) {
    time_left.setText(formatTime(duration_seconds * ratio));
  }
  seekRequested.emit(ratio);
  markLayoutDirty();
}

void PlayerBar::updateVolumeFromPoint(const float x) {
  const float next = std::clamp((x - volume_x) / kVolWidth, 0.0f, 1.0f);
  volume_ = next;
  volumeChanged.emit(next);
  markLayoutDirty();
}

void PlayerBar::onMouseMove(float x, float y) {
  if (progress_dragging) {
    pending_seek_ratio = progressRatioFromPoint(x);
    return;
  }
  if (volume_dragging) {
    updateVolumeFromPoint(x);
    return;
  }

  const int btn = hitTestButton(x, y);
  if (btn != hovered_btn) {
    hovered_btn = btn;
    startAnimation(btn_hover_t, (btn == 1 || btn == 3) ? 1.0f : 0.0f, 120.0f, &btn_hover_t,
                   CubicBezier::EaseOut());
    startAnimation(play_hover_t, btn == 2 ? 1.0f : 0.0f, 120.0f, &play_hover_t,
                   CubicBezier::EaseOut());
  }
}

void PlayerBar::onMouseLeave(float, float) {
  hovered_btn = 0;
  startAnimation(btn_hover_t, 0.0f, 180.0f, &btn_hover_t, CubicBezier::EaseOut());
  startAnimation(play_hover_t, 0.0f, 180.0f, &play_hover_t, CubicBezier::EaseOut());
}

void PlayerBar::onMouseLeftPressed(float x, float y) {
  if (hitTestProgress(x, y)) {
    progress_dragging = true;
    is_dragging = true;
    pending_seek_ratio = progressRatioFromPoint(x);
    return;
  }
  if (hitTestVolume(x, y)) {
    volume_dragging = true;
    is_dragging = true;
    updateVolumeFromPoint(x);
    return;
  }

  if (hovered_btn == 1 || hovered_btn == 3) {
    startAnimation(btn_press_t, 1.0f, 60.0f, &btn_press_t, CubicBezier::EaseOut());
  } else if (hovered_btn == 2) {
    startAnimation(play_press_t, 1.0f, 60.0f, &play_press_t, CubicBezier::EaseOut());
  }
}

void PlayerBar::onMouseLeftReleased(float x, float) {
  const int released_btn = hovered_btn;

  if (progress_dragging || volume_dragging) {
    if (progress_dragging) {
      requestSeekFromPoint(x);
    } else {
      updateVolumeFromPoint(x);
    }
    progress_dragging = false;
    volume_dragging = false;
    is_dragging = false;
    return;
  }

  if (btn_press_t > 0.0f) {
    startAnimation(btn_press_t, 0.0f, 180.0f, &btn_press_t, CubicBezier::EaseOut());
  }
  if (play_press_t > 0.0f) {
    startAnimation(play_press_t, 0.0f, 180.0f, &play_press_t, CubicBezier::EaseOut());
  }

  if (released_btn == 1) {
    previousClicked.emit();
  } else if (released_btn == 2 && !loading_) {
    playPauseClicked.emit();
  } else if (released_btn == 3) {
    nextClicked.emit();
  }
}

} // namespace components

// ─── 独立函数 ───

// 上一首图标：竖线 + 左三角，整体水平居中
// 总宽 = barW(2) + gap(2.5) + triW(7.5) = 12, 中心在 cx-0.25 ≈ cx
void drawPrevIcon(SkCanvas *c, float cx, float cy, const SkPaint &p) {
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
void drawPlayIcon(SkCanvas *c, float cx, float cy, float sz, const SkPaint &p) {
  SkPathBuilder b;
  b.moveTo(SkPoint::Make(cx + sz * 0.42f, cy));
  b.lineTo(SkPoint::Make(cx - sz * 0.38f, cy - sz * 0.55f));
  b.lineTo(SkPoint::Make(cx - sz * 0.38f, cy + sz * 0.55f));
  b.close();
  c->drawPath(b.detach(), p);
}

void drawPauseIcon(SkCanvas *c, float cx, float cy, const SkPaint &p) {
  SkPaint bar = p;
  bar.setStyle(SkPaint::kFill_Style);
  c->drawRoundRect(SkRect::MakeXYWH(cx - 5.4f, cy - 6.2f, 3.2f, 12.4f), 1.3f, 1.3f, bar);
  c->drawRoundRect(SkRect::MakeXYWH(cx + 2.2f, cy - 6.2f, 3.2f, 12.4f), 1.3f, 1.3f, bar);
}

void drawLoadingSpinner(SkCanvas *c, const float cx, const float cy) {
  const float t = static_cast<float>(profiling::frame_clock.now % 1000000ULL) / 1000000.0f;
  constexpr int kDots = 8;

  for (int i = 0; i < kDots; ++i) {
    const float phase = static_cast<float>(i) / static_cast<float>(kDots);
    const float angle = (phase + t) * kPi * 2.0f;
    const float alpha = 0.30f + 0.70f * phase;

    SkPaint dot;
    dot.setAntiAlias(true);
    dot.setColor(skia_colors::white);
    dot.setAlphaf(alpha);
    c->drawCircle(cx + std::cos(angle) * 6.2f, cy + std::sin(angle) * 6.2f, 1.55f, dot);
  }
}

// 下一首图标：右三角 + 竖线，整体水平居中
void drawNextIcon(SkCanvas *c, float cx, float cy, const SkPaint &p) {
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

void drawSpeaker(SkCanvas *c, float x, float cy, const SkPaint &p) {
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

std::string formatTime(const double seconds) {
  const auto total = static_cast<int>(seconds);
  return std::format("{}:{:02}", total / 60, total % 60);
}

int parseTimeText(const std::string_view text) {
  const auto colon = text.find(':');
  if (colon == std::string_view::npos) {
    return -1;
  }

  int minutes = 0;
  int seconds = 0;
  const auto min_text = text.substr(0, colon);
  const auto sec_text = text.substr(colon + 1);
  const auto [min_ptr, min_ec] =
    std::from_chars(min_text.data(), min_text.data() + min_text.size(), minutes);
  const auto [sec_ptr, sec_ec] =
    std::from_chars(sec_text.data(), sec_text.data() + sec_text.size(), seconds);
  if (min_ec != std::errc{} || sec_ec != std::errc{}
      || min_ptr != min_text.data() + min_text.size()
      || sec_ptr != sec_text.data() + sec_text.size()) {
    return -1;
  }
  return minutes * 60 + seconds;
}
