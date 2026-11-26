export module components:player_bar_track_info;

import std;
import models;
import qq_music_api;
import yuri.core;
import yuri.skia;
import yuri.ui;
import :player_bar_style;

using namespace skia;
using namespace ui::animation;
using namespace ui::layout;
using namespace ui::widgets;
using namespace ui::render;

namespace controls::player_bar {

constexpr float kTrackInfoSpacing = 14.0f;  // 封面与文字的间距
constexpr float kMinimumLabelWidth = 40.0f; // 保留能显示部分曲名的宽度

/** 专辑封面，下载时显示占位图。 */
class CoverArt : public Widget {
public:
  /**
   * 创建专辑封面。
   * @param parent 父控件
   */
  explicit CoverArt(Widget *parent);

  /** 根据布局尺寸更新封面占位图。 */
  void layout_children() override;

  /**
   * 绘制封面或占位图。
   * @param canvas 目标画布
   */
  void paint(SkCanvas *canvas) override;

  /**
   * 请求当前专辑的封面。
   * @param album_mid 专辑标识
   */
  void set_album(std::string_view album_mid);

  static constexpr float kSize = 52.0f; // 封面边长

private:
  std::string album_mid_;                       // 当前专辑
  std::shared_ptr<sk_sp<SkImage>> image_;       // 当前请求对应的图片槽位
  RenderSvg icon_ { "resources/svg/play.svg" }; // 封面占位图标
};

/** 歌曲信息中的粗体标题控件。 */
class SongTitle : public Widget {
public:
  /**
   * 创建标题。
   * @param parent 父控件
   */
  explicit SongTitle(Widget *parent);

  /**
   * 更新曲名。
   * @param title 歌曲标题
   */
  void set_text(std::string_view title);

  /** 依据布局宽度更新省略后的标题。 */
  void layout_children() override;

  /**
   * 绘制标题。
   * @param canvas 目标画布
   */
  void paint(SkCanvas *canvas) override;

private:
  RenderText text_; // 粗体文字节点
};

/** 曲名和歌手名组成的两行文字，换歌时一同淡入。 */
class SongLabels : public Widget {
public:
  /**
   * 创建曲名和歌手名两行文字。
   * @param parent 父控件
   */
  explicit SongLabels(Widget *parent);

  /** 停止文字淡入动画。 */
  ~SongLabels() override;

  /**
   * 更新曲名和歌手名。
   * @param song 当前歌曲
   */
  void set_song(const SongInfo &song);

  /**
   * 淡入并裁剪两行文字。
   * @param canvas 目标画布
   */
  void render(SkCanvas *canvas) override;

private:
  SongTitle *title_;  // 曲名
  Text *artist_;      // 歌手名
  float opacity_ = 1; // 文字淡入透明度
};

} // namespace controls::player_bar

export namespace controls::player_bar {

/** 播放栏左侧的专辑封面和歌曲信息。 */
class TrackInfo : public Widget {
public:
  /**
   * 创建歌曲信息区。
   * @param parent 父控件
   */
  explicit TrackInfo(Widget *parent);

  /**
   * 更新歌曲信息。
   * @param song 当前歌曲
   */
  void set_song(const SongInfo &song);

  /** 根据可用空间显示封面和文字。 */
  void layout_children() override;

private:
  CoverArt *cover_;    // 专辑封面
  SongLabels *labels_; // 曲名和歌手名
};

} // namespace controls::player_bar

using namespace controls::player_bar;

CoverArt::CoverArt(Widget *parent) : Widget(parent) {
  set_fixed_size(kSize, kSize);
  set_vertical_alignment(VerticalAlignment::Center);
  icon_.set_alignment(Alignment::Center);
}

void CoverArt::layout_children() {
  icon_.update(local_bounds());
}

void CoverArt::paint(SkCanvas *canvas) {
  constexpr float kRadius = 9.0f;
  const auto cover = local_bounds();
  draw_shadow(
    canvas,
    cover.makeOffset(0.0f, 4.0f).makeInset(2.0f, 0.0f),
    kRadius,
    6.0f,
    ColorFromARGB(34, 25, 36, 52)
  );

  if (image_ && *image_) {
    canvas->save();
    canvas->clipRRect(SkRRect::MakeRectXY(cover, kRadius, kRadius), true);
    canvas->drawImageRect(*image_, cover, SkSamplingOptions(SkFilterMode::kLinear), nullptr);
    canvas->restore();
  } else {
    canvas->drawRoundRect(cover, kRadius, kRadius, PaintDesc { .color = kAccentHot });
    icon_.draw(canvas);
  }

  const auto highlight = SkRect::MakeWH(width(), height() * 0.5f);
  canvas->drawRoundRect(
    highlight, kRadius, kRadius, PaintDesc { .color = ColorFromARGB(44, 255, 255, 255) }
  );
}

void CoverArt::set_album(const std::string_view album_mid) {
  if (album_mid_ == album_mid) return;
  album_mid_ = album_mid;
  image_.reset();
  if (album_mid.empty()) return;

  image_ = std::make_shared<sk_sp<SkImage>>();
  const std::weak_ptr pending_image = image_;
  thread_manager->addTask([pending_image, mid = album_mid_] {
    const auto data = curl::get(qqmusic_api::album::get_album_cover(300, mid));
    if (!data || pending_image.expired()) return;

    // 旧专辑和已销毁控件的请求失效，图片只在 UI 线程写入。
    ui::dispatcher.post([pending_image, image = decodeImage(*data)] {
      if (const auto current = pending_image.lock()) *current = image;
    });
  });
}

SongTitle::SongTitle(Widget *parent) : Widget(parent) {
  set_fixed_height(19.0f);
  text_.set_font(font::bold_font);
  text_.set_font_size(14.5f);
  text_.set_color(kTextPrimary);
  text_.set_alignment(Alignment::BottomLeft);
}

void SongTitle::set_text(const std::string_view title) {
  text_.set_text(title);
}

void SongTitle::layout_children() {
  text_.update(local_bounds());
}

void SongTitle::paint(SkCanvas *canvas) {
  text_.draw(canvas);
}

SongLabels::SongLabels(Widget *parent) :
  Widget(parent), title_(new SongTitle(this)), artist_(new Text("", this)) {
  auto *const rows = new VBoxLayout(VerticalAlignment::Center);
  rows->set_spacing(5.0f);
  set_layout(rows);
  set_preferred_width(0.0f);

  artist_->set_fixed_height(17.0f);
  artist_->set_alignment(Alignment::TopLeft);
  artist_->set_font_size(12.5f);
  artist_->set_color(kTextSecondary);
}

SongLabels::~SongLabels() {
  animation_manager->cancel(&opacity_);
}

void SongLabels::set_song(const SongInfo &song) {
  title_->set_text(song.title);
  artist_->set_text(song.artist);
  opacity_ = 0.0f;
  start_animation(opacity_, 1.0f, 250.0f, &opacity_, CubicBezier::EaseOut());
}

void SongLabels::render(SkCanvas *canvas) {
  canvas->save();
  const auto bounds = hit_test_bounds();
  canvas->clipRect(bounds);
  if (opacity_ < 1.0f) canvas->saveLayerAlphaf(&bounds, opacity_);
  Widget::render(canvas);
  if (opacity_ < 1.0f) canvas->restore();
  canvas->restore();
}

TrackInfo::TrackInfo(Widget *parent) :
  Widget(parent), cover_(new CoverArt(this)), labels_(new SongLabels(this)) {
  auto *const columns = new HBoxLayout;
  columns->set_spacing(kTrackInfoSpacing);
  set_layout(columns);
}

void TrackInfo::set_song(const SongInfo &song) {
  cover_->set_album(song.album_mid);
  labels_->set_song(song);
}

void TrackInfo::layout_children() {
  cover_->set_visible(width() >= CoverArt::kSize);
  labels_->set_visible(width() >= CoverArt::kSize + kTrackInfoSpacing + kMinimumLabelWidth);
  Widget::layout_children();
}
