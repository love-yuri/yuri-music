//
// Created by yuri on 2026/4/8.
//
export module components:song_item;

import std;
import yuri.ui;
import yuri.core;
import yuri.skia;
import yuri.glfw.api;
import qq_music_api;
import yuri.vulkan.context;
import models;

using namespace ui::render;
using namespace ui::layout;
using namespace ui::widgets;
using namespace ui::animation;
using namespace ui::algorithm;
using namespace skia;

// 颜色常量
constexpr SkColor kMutedColor = ColorFromARGB(132, 54, 65, 82);          // 次要文字（序号、时长）
constexpr SkColor kArtistColor = ColorFromARGB(154, 65, 78, 96);         // 歌手名
constexpr SkColor kCoverColor = ColorFromARGB(255, 224, 231, 238);       // 封面占位色
constexpr SkColor kHoverBgColor = ColorFromARGB(96, 255, 255, 255);      // 悬浮背景
constexpr SkColor kPressBgColor = ColorFromARGB(154, 248, 250, 252);     // 按下背景
constexpr SkColor kPlayingBgColor = ColorFromARGB(70, 255, 236, 242);    // 播放中背景
constexpr SkColor kPlayingAccentColor = ColorFromARGB(255, 218, 52, 92); // 播放中强调色
constexpr SkColor kActionBtnColor = ColorFromARGB(132, 42, 52, 68);      // 操作按钮（心形、更多）
constexpr SkColor kLikedColor = ColorFromARGB(255, 226, 47, 88);         // 已喜欢（心形填充）
constexpr SkColor kSelectedBgColor = ColorFromARGB(118, 255, 255, 255);  // 选中背景
constexpr SkColor kSongTitleColor = ColorFromARGB(255, 24, 31, 42);      // 歌曲标题

// --- 布局常量 ---
static constexpr int kCoverGetSize = 300;         // 封面请求尺寸
static constexpr float kCoverSize = 50.0f;        // 封面尺寸
static constexpr float kCoverRadius = 10.0f;      // 封面圆角
static constexpr float kRowHeight = 68.0f;        // 行高
static constexpr float kPadH = 10.0f;             // 左右内边距
static constexpr float kIndexWidth = 28.0f;       // 序号区宽度
static constexpr float kGap = 12.0f;              // 元素间距
static constexpr float kColumnGap = 28.0f;        // 歌曲信息与专辑间距
static constexpr float kSongInfoRatio = 0.52f;    // 歌曲信息区占比
static constexpr float kDurationWidth = 48.0f;    // 时长区宽度
static constexpr float kActionWidth = 72.0f;      // 操作按钮区宽度
static constexpr float kActionRightInset = 32.0f; // 操作区距离滚动条的安全间距
static constexpr float kHoverRightInset = 18.0f;  // 行 hover 背景右侧留白，避免贴到滚动条
static constexpr std::uint64_t kDoubleClickThresholdUs = 400'000; // 400ms 双击阈值

export namespace components {

/**
 * 歌曲行组件：序号 + 封面色块 + 歌曲名/歌手 + 时长 + 操作按钮
 *
 * 动画模型：每个交互状态拥有独立的 float 进度通道 [0,1]，
 * 互不干扰，在 paint() 中按层叠加合成最终背景色。
 *
 *   base(playing/transparent) → select 层 → hover 层 → press 层
 */
class SongItem : public Widget {
public:
  /** 创建歌曲行组件 */
  SongItem(int index, const SongInfo &info, bool is_playing = false, Widget *parent = nullptr);

  /**
   * 布局child
   */
  void layout_children() override;

  /**
   * 绘制自身
   */
  void paint(SkCanvas *canvas) override;

  /** 设置是否选中 */
  void setSelected(bool value);

  /** 是否选中 */
  [[nodiscard]] bool isSelected() const {
    return selected_;
  }

  /** 获取歌曲信息 */
  [[nodiscard]] const SongInfo &info() const {
    return info_;
  }

  Signal<SongItem *> doubleClicked;        // 双击信号
  Signal<SongItem *> contextMenuRequested; // 上下文菜单请求信号

protected:
  /** 鼠标悬浮进入 */
  void on_mouse_enter(float x, float y) override;

  /** 鼠标悬浮离开 */
  void on_mouse_leave(float x, float y) override;

  /** 鼠标移动 */
  void on_mouse_move(float x, float y) override;

  /** 鼠标按下 */
  void on_mouse_left_pressed(float x, float y) override;

  /** 鼠标松开 */
  void on_mouse_left_released(float x, float y) override;

  /** 鼠标右键按下 */
  void on_mouse_right_pressed(float x, float y) override;

private:
  /** 根据各动画通道合成最终背景色 */
  [[nodiscard]] SkColor computeBackgroundColor() const noexcept;

  /** 判断鼠标是否悬浮在心形按钮上 */
  [[nodiscard]] bool isOverHeart(float x, float y) const;

  /** 判断鼠标是否悬浮在更多按钮上 */
  [[nodiscard]] bool isOverMore(float x, float y) const;

  /**
   * 设置喜欢按钮弹性动画进度。
   * @param t 动画进度
   */
  void setLikeT(float t) noexcept;

  /** 异步加载专辑封面 */
  void loadCoverImage();

  /** 绘制专辑封面 */
  bool drawCoverImage(SkCanvas *canvas) const;

  // --- 渲染节点 ---
  RenderText index_text;                            // 序号
  RenderBackground cover_bg {};                     // 封面背景
  RenderSvg cover_svg { "resources/svg/play.svg" }; // 封面 SVG
  float cover_radius = kCoverRadius;                // 封面圆角
  RenderText title_text;                            // 标题
  RenderText artist_text;                           // 歌手
  RenderText album_text;                            // 专辑名
  RenderText duration_text;                         // 时长
  RenderBackground hover_bg {};                     // 行背景（覆盖整行）
  float hover_radius = 12.0f;                       // 行背景圆角
  SongInfo info_;                                   // 歌曲信息
  SkColor cover_color = kCoverColor;                // 封面主色
  sk_sp<SkImage> cover_image;                       // 专辑封面图片

  // --- 交互状态 ---
  bool is_playing = false; // 是否播放中

  bool pressed_heart = false;        // 本次左键是否在收藏按钮上按下
  bool pressed_more = false;         // 本次左键是否在更多按钮上按下
  bool selected_ = false;            // 是否被选中
  std::uint64_t last_click_time = 0; // 上次点击时间

  // --- 动画进度（独立通道，互不干扰） ---
  float hover_t = 0.0f;  // 悬浮渐入 [0,1]
  float press_t = 0.0f;  // 按下效果 [0,1]
  float select_t = 0.0f; // 选中渐入 [0,1]
  float action_t = 0.0f; // 操作按钮透明度 [0,1]
  float like_t = 0.0f;   // 喜欢按钮弹性反馈
};

SongItem::SongItem(const int index, const SongInfo &info, const bool is_playing, Widget *parent) :
  Widget(parent), index_text(std::to_string(index + 1)), title_text(info.title),
  artist_text(info.artist), album_text(info.album_name), duration_text(info.duration_text()),
  info_(std::move(info)), is_playing(is_playing) {
  constexpr SkColor palette[] = {
    ColorFromARGB(255, 255, 99, 132),  ColorFromARGB(255, 42, 140, 233),
    ColorFromARGB(255, 16, 178, 180),  ColorFromARGB(255, 245, 155, 54),
    ColorFromARGB(255, 128, 111, 232),
  };
  cover_color = palette[static_cast<std::size_t>(index) % std::size(palette)];

  // 序号
  index_text.set_font_size(12.5f);
  index_text.set_color(is_playing ? kPlayingAccentColor : kMutedColor);
  index_text.set_alignment(Alignment::CenterRight);

  // 封面色块
  cover_bg.set_color(cover_color);
  cover_bg.radius = &cover_radius;

  // 封面播放图标
  cover_svg.set_alignment(Alignment::Center);

  // 标题
  title_text.set_font_size(14);
  title_text.set_color(is_playing ? kPlayingAccentColor : kSongTitleColor);
  title_text.set_alignment(Alignment::TopLeft);

  // 歌手
  artist_text.set_font_size(12.5f);
  artist_text.set_color(kArtistColor);
  artist_text.set_alignment(Alignment::TopLeft);

  // 专辑
  album_text.set_font_size(12.5f);
  album_text.set_color(kMutedColor);
  album_text.set_alignment(Alignment::CenterLeft);

  // 时长
  duration_text.set_font_size(12.5f);
  duration_text.set_color(kMutedColor);
  duration_text.set_alignment(Alignment::CenterRight);

  // 行背景
  hover_bg.radius = &hover_radius;

  // 固定高度
  set_fixed_height(kRowHeight);

  // 加载歌曲图片
  loadCoverImage();
}

void SongItem::loadCoverImage() {
  if (info_.album_mid.empty()) {
    return;
  }

  const auto album_mid = info_.album_mid;
  thread_manager->addTask([this, album_mid] {
    const auto image_url = qqmusic_api::album::get_album_cover(kCoverGetSize, album_mid);
    const auto image_data = curl::get(image_url).value();
    cover_image = decodeImage(image_data);
  });
}

bool SongItem::drawCoverImage(SkCanvas *canvas) const {
  if (!cover_image) {
    return false;
  }

  constexpr float cover_x = kPadH + kIndexWidth + kGap;
  constexpr float cover_y = (kRowHeight - kCoverSize) * 0.5f;
  constexpr auto cover_rect = SkRect::MakeXYWH(cover_x, cover_y, kCoverSize, kCoverSize);
  constexpr float radius = kCoverRadius;

  canvas->save();
  canvas->clipRRect(SkRRect::MakeRectXY(cover_rect, radius, radius), true);
  canvas->drawImageRect(
    cover_image,
    cover_rect,
    SkSamplingOptions(SkFilterMode::kLinear, SkMipmapMode::kLinear),
    nullptr
  );
  canvas->restore();
  return true;
}

SkColor SongItem::computeBackgroundColor() const noexcept {
  // 底层：播放中背景，否则透明
  SkColor bg = is_playing ? kPlayingBgColor : skia_colors::transparent;
  // 选中层
  if (select_t > 0.0f) {
    bg = lerp(bg, kSelectedBgColor, select_t);
  }
  // 悬浮层
  if (hover_t > 0.0f) {
    bg = lerp(bg, kHoverBgColor, hover_t);
  }
  // 按下层（最顶层）
  if (press_t > 0.0f) {
    bg = lerp(bg, kPressBgColor, press_t);
  }
  return bg;
}

void SongItem::on_mouse_enter(float x, float y) {
  // 悬浮背景渐入
  start_animation(hover_t, 1.0f, 200.0f, &hover_t, CubicBezier::EaseOut());
  // 操作按钮淡入
  start_animation(action_t, 1.0f, 200.0f, &action_t, CubicBezier::EaseOut());
}

void SongItem::on_mouse_leave(float x, float y) {
  window()->setCursor(glfw::CursorType::Arrow);
  // 悬浮背景渐出（比渐入稍慢，体感更柔和）
  start_animation(hover_t, 0.0f, 280.0f, &hover_t, CubicBezier::EaseOut());
  // 按下效果归零
  if (press_t > 0.0f) {
    start_animation(press_t, 0.0f, 150.0f, &press_t, CubicBezier::EaseOut());
  }
  // 操作按钮淡出
  start_animation(action_t, 0.0f, 200.0f, &action_t, CubicBezier::EaseOut());
}

void SongItem::on_mouse_left_pressed(const float x, const float y) {
  pressed_heart = isOverHeart(x, y);
  pressed_more = isOverMore(x, y);
  // 按下效果快速响应
  start_animation(press_t, 1.0f, 80.0f, &press_t, CubicBezier::EaseOut());
}

void SongItem::on_mouse_right_pressed(float x, float y) {
  contextMenuRequested.emit(this);
}

void SongItem::on_mouse_left_released(const float x, const float y) {
  // 按下效果释放
  start_animation(press_t, 0.0f, 160.0f, &press_t, CubicBezier::EaseOut());

  const bool over_heart = isOverHeart(x, y);
  const bool over_more = isOverMore(x, y);
  if (
    !hovered_ || !visible() || !local_bounds().contains(x, y) || over_heart != pressed_heart
    || over_more != pressed_more
  ) {
    last_click_time = 0;
    return;
  }

  if (over_heart) {
    last_click_time = 0;
    info_.liked = !info_.liked;
    like_t = 1.0f;
    start_animation<&SongItem::setLikeT>(like_t, 0.0f, 260.0f, CubicBezier::EaseOut());
    return;
  }
  if (over_more) {
    last_click_time = 0;
    return;
  }

  const auto now = profiling::frame_clock.now;
  const auto previous = std::exchange(last_click_time, now);
  if (previous != 0 && now - previous < kDoubleClickThresholdUs) {
    // 双击：选中当前项并发射信号
    last_click_time = 0;
    if (!selected_) {
      doubleClicked.emit(this);
    }
  }
}

void SongItem::on_mouse_move(const float x, const float y) {
  if (!hovered_) return;
  if (isOverHeart(x, y) || isOverMore(x, y)) {
    window()->setCursor(glfw::CursorType::Hand);
  } else {
    window()->setCursor(glfw::CursorType::Arrow);
  }
}

void SongItem::setSelected(const bool value) {
  if (selected_ == value) return;
  selected_ = value;
  start_animation(select_t, value ? 1.0f : 0.0f, 250.0f, &select_t, CubicBezier::EaseOut());
}

void SongItem::setLikeT(const float t) noexcept {
  like_t = t;
}

bool SongItem::isOverHeart(const float x, const float y) const {
  const float heart_cx = width() - kActionRightInset - 52.0f;
  constexpr float btn_cy = kRowHeight * 0.5f;
  const float dx = x - heart_cx;
  const float dy = y - btn_cy;
  return dx * dx + dy * dy < 16.0f * 16.0f;
}

bool SongItem::isOverMore(float x, float y) const {
  const float dots_cx = width() - kActionRightInset - 22.0f;
  constexpr float btn_cy = kRowHeight * 0.5f;
  const float dx = x - dots_cx;
  const float dy = y - btn_cy;
  return dx * dx + dy * dy < 16.0f * 16.0f;
}

void SongItem::layout_children() {
  const auto rect = local_bounds();

  // 行背景
  hover_bg.update(
    SkRect::MakeXYWH(0.0f, 0.0f, std::max(0.0f, rect.width() - kHoverRightInset), rect.height())
  );

  // 序号区
  index_text.update(SkRect::MakeXYWH(kPadH, 0, kIndexWidth, rect.height()));

  // 封面：序号右边间隔 kGap，垂直居中
  constexpr float cover_x = kPadH + kIndexWidth + kGap;
  constexpr float cover_y = (kRowHeight - kCoverSize) * 0.5f;
  constexpr auto cover_rect = SkRect::MakeXYWH(cover_x, cover_y, kCoverSize, kCoverSize);
  cover_bg.update(cover_rect);
  cover_svg.update(cover_rect);

  // 操作按钮区
  const float action_x = rect.width() - kActionRightInset - kActionWidth;

  // 文字区：封面右边间隔 kGap
  constexpr float text_x = cover_x + kCoverSize + kGap;

  // 时长：操作按钮区左侧
  constexpr float duration_w = kDurationWidth;
  const float duration_x = action_x - duration_w;
  const float content_w = std::max(0.0f, duration_x - kGap - text_x);
  const float song_info_w = std::max(0.0f, content_w * kSongInfoRatio - kColumnGap);
  const float album_x = text_x + song_info_w + kColumnGap;
  const float album_w = std::max(0.0f, duration_x - kGap - album_x);

  // 标题/歌手：围绕行高中心分布
  constexpr float kTitleHeight = 19.0f;
  constexpr float kArtistHeight = 17.0f;
  constexpr float kTextGap = 5.0f;
  constexpr float kTextBlockHeight = kTitleHeight + kTextGap + kArtistHeight;
  constexpr float kTitleY = (kRowHeight - kTextBlockHeight) * 0.5f;
  title_text.update(SkRect::MakeXYWH(text_x, kTitleY, song_info_w, kTitleHeight));
  album_text.update(SkRect::MakeXYWH(album_x, 0.0f, album_w, rect.height()));

  // 歌手：标题下方
  constexpr float kArtistY = kTitleY + kTitleHeight + kTextGap;
  artist_text.update(SkRect::MakeXYWH(text_x, kArtistY, song_info_w, kArtistHeight));
  duration_text.update(SkRect::MakeXYWH(duration_x, 0, duration_w, rect.height()));
}

void SongItem::paint(SkCanvas *canvas) {
  canvas->save();
  const float press_scale = 1.0f - press_t * 0.01f;

  auto save_pressed_scale = [&] {
    canvas->save();
    canvas->translate(width() * 0.5f, kRowHeight * 0.5f);
    canvas->scale(press_scale, press_scale);
    canvas->translate(-width() * 0.5f, -kRowHeight * 0.5f);
  };

  save_pressed_scale();

  // 合成背景色并渲染
  hover_bg.set_color(computeBackgroundColor());
  hover_bg.draw(canvas);

  if (hover_t > 0.01f || select_t > 0.01f) {
    constexpr float sigma = 6.0f;
    auto glow_rect = local_bounds().makeInset(8.0f, 8.0f).makeOffset(0, 3.0f);
    auto layer_bounds = glow_rect.makeOutset(sigma * 3.0f, sigma * 3.0f);
    SkPaint glow_layer;
    glow_layer.setImageFilter(SkImageFilters::Blur(sigma, sigma, nullptr));
    SkPaint glow;
    glow.setAntiAlias(true);
    glow.setColor(
      ColorFromARGB(static_cast<U8CPU>(10.0f * std::max(hover_t, select_t)), 18, 28, 44)
    );
    canvas->saveLayer(&layer_bounds, &glow_layer);
    canvas->drawRoundRect(glow_rect, hover_radius, hover_radius, glow);
    canvas->restore();
  }

  cover_bg.draw(canvas);

  if (!drawCoverImage(canvas)) {
    SkPaint coverShine;
    coverShine.setAntiAlias(true);
    coverShine.setColor(ColorFromARGB(52, 255, 255, 255));
    constexpr float cover_x = kPadH + kIndexWidth + kGap;
    constexpr float cover_y = (kRowHeight - kCoverSize) * 0.5f;
    canvas->drawRoundRect(
      SkRect::MakeXYWH(cover_x + 2.0f, cover_y + 2.0f, kCoverSize - 4.0f, kCoverSize * 0.42f),
      kCoverRadius - 2.0f,
      kCoverRadius - 2.0f,
      coverShine
    );
    cover_svg.draw(canvas);
  }
  canvas->restore();

  index_text.draw(canvas);
  title_text.draw(canvas);
  album_text.draw(canvas);
  artist_text.draw(canvas);
  duration_text.draw(canvas);

  // 操作按钮：基于 action_t 控制透明度
  if (!title_text.text().empty()) {
    const float content_w = width();
    constexpr float btn_cy = kRowHeight * 0.5f;

    // 喜欢按钮（心形）：liked 时始终可见，否则跟随 action_t
    const float heart_cx = content_w - kActionRightInset - 52.0f;
    SkPaint heart_paint;
    heart_paint.setAntiAlias(true);
    heart_paint.setStyle(SkPaint::kFill_Style);
    heart_paint.setColor(info_.liked ? kLikedColor : kActionBtnColor);
    heart_paint.setAlphaf(info_.liked ? 1.0f : action_t);
    canvas->save();
    const float heart_scale = 1.0f + like_t * 0.16f;
    canvas->translate(heart_cx, btn_cy);
    canvas->scale(heart_scale, heart_scale);
    canvas->translate(-heart_cx, -btn_cy);
    canvas->drawPath(path::makeHeartPath(heart_cx, btn_cy, 12.0f), heart_paint);
    canvas->restore();

    // 更多按钮（省略号）：仅 hover 时可见
    if (action_t > 0.01f) {
      const float dots_cx = content_w - kActionRightInset - 22.0f;
      SkPaint dots_paint;
      dots_paint.setAntiAlias(true);
      dots_paint.setColor(kActionBtnColor);
      dots_paint.setAlphaf(action_t);
      canvas->drawCircle(dots_cx - 7.0f, btn_cy, 2.0f, dots_paint);
      canvas->drawCircle(dots_cx, btn_cy, 2.0f, dots_paint);
      canvas->drawCircle(dots_cx + 7.0f, btn_cy, 2.0f, dots_paint);
    }
  }
  canvas->restore();
}

} // namespace components
