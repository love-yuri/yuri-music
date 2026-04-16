//
// Created by yuri on 2026/4/8.
//
export module components:song_item;

import std;
import ui;
import core;
import skia;

using namespace ui::render;
using namespace ui::layout;
using namespace ui::widgets;
using namespace ui::animation;
using namespace skia;

// 序号/时长文字颜色
constexpr SkColor kMutedColor = ColorFromARGB(255, 160, 160, 160);
// 歌手文字颜色
constexpr SkColor kArtistColor = ColorFromARGB(255, 140, 140, 140);
// 封面色块颜色
constexpr SkColor kCoverColor = ColorFromARGB(255, 99, 102, 241);
// 悬浮背景色
constexpr SkColor kHoverBgColor = ColorFromARGB(255, 245, 245, 252);
// 按下背景色
constexpr SkColor kPressBgColor = ColorFromARGB(255, 235, 235, 248);

// 封面尺寸
static constexpr float kCoverSize = 44.0f;
// 封面圆角
static constexpr float kCoverRadius = 8.0f;

export namespace components {

// 歌曲行组件：序号 + 封面色块 + 歌曲名/歌手 + 时长
class SongItem : public Widget {
public:
  SongItem(int index,
           std::string_view title,
           std::string_view artist,
           std::string_view duration,
           Widget *parent = nullptr);

  void layoutChildren() override;
  void paint(SkCanvas *canvas) override;

protected:
  // 鼠标悬浮进入
  void onMouseEnter(float x, float y) override;
  // 鼠标悬浮离开
  void onMouseLeave(float x, float y) override;
  // 鼠标按下
  void onMouseLeftPressed(float x, float y) override;
  // 鼠标松开
  void onMouseLeftReleased(float x, float y) override;

private:
  /**
   * 设置悬浮背景色
   */
  void setHoverBackgroundColor(SkColor color) noexcept;

  RenderText index_text_;
  RenderBackground cover_bg_;
  float cover_radius_ = kCoverRadius;
  RenderText title_text_;
  RenderText artist_text_;
  RenderText duration_text_;
  RenderBackground hover_bg_;
  float hover_radius_ = 8.0f;
  // 动画状态
  SkColor bg_color_ = skia_colors::transparent;
  bool is_pressed_ = false;
};

SongItem::SongItem(const int index,
                   const std::string_view title,
                   const std::string_view artist,
                   const std::string_view duration,
                   Widget *parent) :
  Widget(parent), index_text_(std::to_string(index)), cover_bg_(), title_text_(title),
  artist_text_(artist), duration_text_(duration) {
  // 序号
  index_text_.setFontSize(12);
  index_text_.setColor(kMutedColor);
  index_text_.setAlignment(Alignment::Center);

  // 封面色块
  cover_bg_.setColor(kCoverColor);
  cover_bg_.radius = &cover_radius_;

  // 标题
  title_text_.setFontSize(14);
  title_text_.setColor(skia_colors::black);
  title_text_.setAlignment(Alignment::TopLeft);

  // 歌手
  artist_text_.setFontSize(12);
  artist_text_.setColor(kArtistColor);
  artist_text_.setAlignment(Alignment::TopLeft);

  // 时长
  duration_text_.setFontSize(12);
  duration_text_.setColor(kMutedColor);
  duration_text_.setAlignment(Alignment::CenterRight);

  // 悬浮背景
  hover_bg_.setColor(bg_color_);
  hover_bg_.radius = &hover_radius_;

  // 固定高度
  setMaxHeight(60);
  setMinHeight(60);
}

void SongItem::onMouseEnter(float, float) {
  is_pressed_ = false;
  yuri::info("enter: {}", index_text_.text());
  startAnimation<&SongItem::setHoverBackgroundColor>(bg_color_, kHoverBgColor, 150.0f);
}

void SongItem::onMouseLeave(float, float) {
  is_pressed_ = false;
  startAnimation<&SongItem::setHoverBackgroundColor>(bg_color_, skia_colors::transparent, 150.0f,
                                                     CubicBezier::EaseOut());
}

void SongItem::onMouseLeftPressed(float, float) {
  is_pressed_ = true;
  startAnimation<&SongItem::setHoverBackgroundColor>(bg_color_, kPressBgColor, 100.0f,
                                                     CubicBezier::EaseOut());
}

void SongItem::onMouseLeftReleased(float, float) {
  is_pressed_ = false;
  startAnimation<&SongItem::setHoverBackgroundColor>(bg_color_, kHoverBgColor, 100.0f);
}

void SongItem::setHoverBackgroundColor(const SkColor color) noexcept {
  bg_color_ = color;
  hover_bg_.setColor(color);
}

void SongItem::layoutChildren() {
  const auto rect = contentRect();
  const float pad_left = padding_.left;
  const float pad_top = padding_.top;

  // 悬浮背景
  hover_bg_.update(borderRect());

  // 序号区：20px 宽
  constexpr float idx_w = 20.0f;
  index_text_.update(SkRect::MakeXYWH(pad_left, pad_top, idx_w, rect.height()));

  // 封面：序号右边间隔 14px
  const float cover_x = pad_left + idx_w + 14.0f;
  const float cover_y = pad_top + (60.0f - kCoverSize) * 0.5f;
  cover_bg_.update(SkRect::MakeXYWH(cover_x, cover_y, kCoverSize, kCoverSize));

  // 文字区：封面右边间隔 14px
  const float text_x = cover_x + kCoverSize + 14.0f;
  const float text_w = rect.width() - text_x - 60.0f; // 右侧留出时长区

  // 标题：垂直居中偏上
  const float title_y = pad_top + 12.0f;
  title_text_.update(SkRect::MakeXYWH(text_x, title_y, text_w, 20.0f));

  // 歌手：标题下方
  const float artist_y = title_y + 22.0f;
  artist_text_.update(SkRect::MakeXYWH(text_x, artist_y, text_w, 16.0f));

  // 时长：右侧
  duration_text_.update(rect);
}

void SongItem::paint(SkCanvas *canvas) {
  hover_bg_.render(canvas);
  index_text_.render(canvas);
  cover_bg_.render(canvas);
  title_text_.render(canvas);
  artist_text_.render(canvas);
  duration_text_.render(canvas);
}

} // namespace components
