//
// Created by yuri on 2026/4/8.
//
export module components:song_item;

import std;
import ui;
import core;
import skia;
import glfw.api;

using namespace ui::render;
using namespace ui::layout;
using namespace ui::widgets;
using namespace ui::animation;
using namespace skia;

// 序号/时长文字颜色
constexpr SkColor kMutedColor = ColorFromARGB(160, 0, 0, 0);
// 歌手文字颜色
constexpr SkColor kArtistColor = ColorFromARGB(140, 0, 0, 0);
// 封面色块颜色
constexpr SkColor kCoverColor = ColorFromARGB(255, 99, 102, 241);
// 悬浮背景色
constexpr SkColor kHoverBgColor = ColorFromARGB(255, 245, 245, 252);
// 按下背景色
constexpr SkColor kPressBgColor = ColorFromARGB(255, 235, 235, 248);
// 播放中背景色
constexpr SkColor kPlayingBgColor = ColorFromARGB(20, 139, 92, 246);
// 播放中序号颜色
constexpr SkColor kPlayingAccentColor = ColorFromARGB(255, 139, 92, 246);
// 操作按钮颜色
constexpr SkColor kActionBtnColor = ColorFromARGB(140, 0, 0, 0);
// 喜欢按钮颜色
constexpr SkColor kLikedColor = ColorFromARGB(255, 244, 114, 182);
// 选中背景色
constexpr SkColor kSelectedBgColor = ColorFromARGB(255, 235, 235, 240);

// 封面尺寸
static constexpr float kCoverSize = 44.0f;
// 封面圆角
static constexpr float kCoverRadius = 8.0f;
// 行高
static constexpr float kRowHeight = 60.0f;
// 左右内边距
static constexpr float kPadH = 14.0f;
// 序号区宽度
static constexpr float kIndexWidth = 20.0f;
// 元素间距
static constexpr float kGap = 14.0f;
// 时长区宽度
static constexpr float kDurationWidth = 40.0f;
// 操作按钮区宽度
static constexpr float kActionWidth = 74.0f;
// 双击阈值
static constexpr std::uint64_t kDoubleClickThresholdUs = 400'000; // 400ms

export namespace components {

/**
 * 歌曲行组件：序号 + 封面色块 + 歌曲名/歌手 + 时长 + 操作按钮
 */
class SongItem : public Widget {
public:
  SongItem(int index,
           std::string_view title,
           std::string_view artist,
           std::string_view duration,
           bool is_playing = false,
           Widget *parent = nullptr);

  void layoutChildren() override;
  void paint(SkCanvas *canvas) override;

  /** 设置是否选中 */
  void setSelected(bool value);

  Signal<> doubleClicked; // 双击信号

protected:
  /** 鼠标悬浮进入 */
  void onMouseEnter(float x, float y) override;
  /** 鼠标悬浮离开 */
  void onMouseLeave(float x, float y) override;
  /** 鼠标移动 */
  void onMouseMove(float x, float y) override;
  /** 鼠标按下 */
  void onMouseLeftPressed(float x, float y) override;
  /** 鼠标松开 */
  void onMouseLeftReleased(float x, float y) override;

private:
  /** 设置悬浮背景色 */
  void setHoverBackgroundColor(SkColor color) noexcept;
  /** 判断鼠标是否悬浮在操作按钮上 */
  [[nodiscard]] bool isOverHeart(float x, float y) const;
  [[nodiscard]] bool isOverMore(float x, float y) const;

  RenderText index_text;                       // 序号
  RenderBackground cover_bg;                   // 封面背景
  RenderSvg cover_svg;                         // 封面 SVG
  float cover_radius = kCoverRadius;           // 封面圆角
  RenderText title_text;                       // 标题
  RenderText artist_text;                      // 歌手
  RenderText duration_text;                    // 时长
  RenderBackground hover_bg;                   // 悬浮背景
  float hover_radius = 8.0f;                   // 悬浮圆角
  SkColor bg_color = skia_colors::transparent; // 当前背景色
  bool is_pressed = false;                     // 是否正在被按下
  bool is_playing = false;                     // 是否播放中
  bool liked = false;                          // 是否已喜欢
  bool is_hovering = false;                    // 是否悬浮中
  bool selected = false;                       // 是否被选中
  std::uint64_t last_click_time = 0;           // 上次点击时间
};

SongItem::SongItem(const int index,
                   const std::string_view title,
                   const std::string_view artist,
                   const std::string_view duration,
                   const bool is_playing,
                   Widget *parent) :
  Widget(parent), index_text(std::to_string(index + 1)), cover_bg(),
  cover_svg("resources/svg/play.svg"), title_text(title), artist_text(artist),
  duration_text(duration), is_playing(is_playing) {
  // 序号
  index_text.setFontSize(12);
  index_text.setColor(is_playing ? kPlayingAccentColor : kMutedColor);
  index_text.setAlignment(Alignment::Center);

  // 封面色块
  cover_bg.setColor(kCoverColor);
  cover_bg.radius = &cover_radius;

  // 封面播放图标
  cover_svg.setAlignment(Alignment::Center);

  // 标题
  title_text.setFontSize(14);
  title_text.setColor(is_playing ? kPlayingAccentColor : skia_colors::black);
  title_text.setAlignment(Alignment::TopLeft);

  // 歌手
  artist_text.setFontSize(12);
  artist_text.setColor(kArtistColor);
  artist_text.setAlignment(Alignment::TopLeft);

  // 时长
  duration_text.setFontSize(12);
  duration_text.setColor(kMutedColor);
  duration_text.setAlignment(Alignment::CenterRight);

  // 悬浮背景
  hover_bg.setColor(is_playing ? kPlayingBgColor : bg_color);
  hover_bg.radius = &hover_radius;

  // 固定高度
  setMaxHeight(kRowHeight);
  setMinHeight(kRowHeight);
}

void SongItem::onMouseEnter(float, float) {
  is_pressed = false;
  is_hovering = true;
  startAnimation<&SongItem::setHoverBackgroundColor>(bg_color, kHoverBgColor, 150.0f);
}

void SongItem::onMouseLeave(float, float) {
  is_pressed = false;
  is_hovering = false;
  window()->setCursor(glfw::CursorType::Arrow);
  const auto target =
    is_playing ? kPlayingBgColor : (selected ? kSelectedBgColor : skia_colors::transparent);
  startAnimation<&SongItem::setHoverBackgroundColor>(bg_color, target, 150.0f,
                                                     CubicBezier::EaseOut());
}

void SongItem::onMouseLeftPressed(float, float) {
  is_pressed = true;
  startAnimation<&SongItem::setHoverBackgroundColor>(bg_color, kPressBgColor, 100.0f,
                                                     CubicBezier::EaseOut());
}

void SongItem::onMouseLeftReleased(float x, float y) {
  is_pressed = false;
  startAnimation<&SongItem::setHoverBackgroundColor>(bg_color, kHoverBgColor, 100.0f);

  if (isOverHeart(x, y)) {
    liked = !liked;
    return;
  }
  if (isOverMore(x, y)) {
    return;
  }

  const auto now = profiling::frame_clock.now;
  const auto elapsed = now - last_click_time;
  last_click_time = now;

  if (elapsed > 0 && elapsed < kDoubleClickThresholdUs) {
    // 双击：选中当前项并发射信号
    last_click_time = 0;
    if (!selected) {
      setSelected(true);
      if (parent_) {
        for (auto *sibling : parent_->children()) {
          if (sibling != this) {
            if (auto *other = dynamic_cast<SongItem *>(sibling)) {
              other->setSelected(false);
            }
          }
        }
      }
    }
    doubleClicked.emit();
  }
}

void SongItem::setHoverBackgroundColor(const SkColor color) noexcept {
  bg_color = color;
  hover_bg.setColor(color);
}

void SongItem::onMouseMove(float x, float y) {
  if (isOverHeart(x, y) || isOverMore(x, y)) {
    window()->setCursor(glfw::CursorType::Hand);
  } else {
    window()->setCursor(glfw::CursorType::Arrow);
  }
}

void SongItem::setSelected(const bool value) {
  if (selected == value) return;
  selected = value;
  if (!is_hovering && !is_pressed) {
    const auto target =
      is_playing ? kPlayingBgColor : (selected ? kSelectedBgColor : skia_colors::transparent);
    bg_color = target;
    hover_bg.setColor(target);
  }
}

bool SongItem::isOverHeart(float x, float y) const {
  const float heart_cx = contentWidth() - 56.0f;
  constexpr float btn_cy = kRowHeight * 0.5f;
  const float dx = x - heart_cx;
  const float dy = y - btn_cy;
  return dx * dx + dy * dy < 18.0f * 18.0f;
}

bool SongItem::isOverMore(float x, float y) const {
  const float dots_cx = contentWidth() - 24.0f;
  constexpr float btn_cy = kRowHeight * 0.5f;
  const float dx = x - dots_cx;
  const float dy = y - btn_cy;
  return dx * dx + dy * dy < 18.0f * 18.0f;
}

void SongItem::layoutChildren() {
  const auto rect = contentRect();

  // 悬浮背景
  hover_bg.update(borderRect());

  // 序号区
  index_text.update(SkRect::MakeXYWH(kPadH, 0, kIndexWidth, rect.height()));

  // 封面：序号右边间隔 kGap，垂直居中
  constexpr float cover_x = kPadH + kIndexWidth + kGap;
  constexpr float cover_y = (kRowHeight - kCoverSize) * 0.5f;
  constexpr auto cover_rect = SkRect::MakeXYWH(cover_x, cover_y, kCoverSize, kCoverSize);
  cover_bg.update(cover_rect);
  cover_svg.update(cover_rect);

  // 操作按钮区
  const float action_x = rect.width() - kActionWidth;

  // 文字区：封面右边间隔 kGap
  constexpr float text_x = cover_x + kCoverSize + kGap;
  const float text_w = action_x - kGap - text_x;

  // 标题：垂直偏上
  constexpr float title_y = 14.0f;
  title_text.update(SkRect::MakeXYWH(text_x, title_y, text_w, 18.0f));

  // 歌手：标题下方
  constexpr float artist_y = 36.0f;
  artist_text.update(SkRect::MakeXYWH(text_x, artist_y, text_w, 16.0f));

  // 时长：操作按钮区左侧
  constexpr float duration_w = kDurationWidth;
  const float duration_x = action_x - duration_w;
  duration_text.update(SkRect::MakeXYWH(duration_x, 0, duration_w, rect.height()));
}

void SongItem::paint(SkCanvas *canvas) {
  hover_bg.render(canvas);
  index_text.render(canvas);
  cover_bg.render(canvas);
  cover_svg.render(canvas);
  title_text.render(canvas);
  artist_text.render(canvas);
  duration_text.render(canvas);

  // 悬停时绘制操作按钮
  if (!title_text.text().empty()) {
    const float content_w = contentWidth();
    constexpr float btn_cy = kRowHeight * 0.5f;

    // 喜欢按钮（心形）
    const float heart_cx = content_w - 56.0f;
    SkPaint heart_paint;
    heart_paint.setAntiAlias(true);
    heart_paint.setStyle(SkPaint::kFill_Style);
    heart_paint.setColor(liked ? kLikedColor : kActionBtnColor);
    heart_paint.setAlphaf(is_hovering || liked ? 1.0f : 0.0f);
    canvas->drawPath(MakeHeartPath(heart_cx, btn_cy, 14.0f), heart_paint);

    // 更多按钮（省略号）
    const float dots_cx = content_w - 24.0f;
    SkPaint dots_paint;
    dots_paint.setAntiAlias(true);
    dots_paint.setColor(kActionBtnColor);
    dots_paint.setAlphaf(is_hovering ? 1.0f : 0.0f);
    canvas->drawCircle(dots_cx - 7.0f, btn_cy, 2.0f, dots_paint);
    canvas->drawCircle(dots_cx, btn_cy, 2.0f, dots_paint);
    canvas->drawCircle(dots_cx + 7.0f, btn_cy, 2.0f, dots_paint);
  }
}

} // namespace components
