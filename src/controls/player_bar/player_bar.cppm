export module components:player_bar;

import std;
import models;
import player;
import yuri.core;
import yuri.skia;
import yuri.ui;
import :player_bar_actions;
import :player_bar_playback_controls;
import :player_bar_style;
import :player_bar_track_info;
import :player_bar_volume_popup;

using namespace skia;
using namespace ui::animation;
using namespace ui::layout;
using namespace ui::widgets;
using namespace controls::player_bar;
using ui::render::PaintDesc;

export namespace components {

/** 组合左侧歌曲信息、中间播放控件和右侧附加操作。 */
class PlayerBar : public Widget {
public:
  /**
   * 创建播放栏。
   * @param parent 父控件
   */
  explicit PlayerBar(Widget *parent);

  /** 断开播放器订阅并停止入场动画。 */
  ~PlayerBar() override;

  Signal<> previous_clicked;          // 上一首
  Signal<> next_clicked;              // 下一首
  Signal<bool> playback_mode_changed; // 是否切换到随机播放

  /**
   * 点击浮层和音量按钮之外时关闭浮层。
   * @param x 鼠标在窗口中的横坐标
   * @param y 鼠标在窗口中的纵坐标
   */
  void dismiss_volume_popup_at(float x, float y) const;

  /**
   * 绘制播放栏的外层背景。
   * @param canvas 目标画布
   */
  void paint(SkCanvas *canvas) override;

  /**
   * 将入场动画应用到整个播放栏。
   * @param canvas 目标画布
   */
  void render(SkCanvas *canvas) override;

  /** 更新播放栏子控件和音量浮层的位置。 */
  void layout_children() override;

protected:
  /**
   * 将本地坐标转换为入场动画中的子控件坐标。
   * @param x 本地横坐标
   * @param y 本地纵坐标
   * @return 转换后的坐标
   */
  [[nodiscard]] SkPoint mouse_content_position(float x, float y) const override;

private:
  /**
   * 更新曲目信息并显示播放栏。
   * @param song 播放器报告的歌曲
   */
  void on_song_changed(const std::shared_ptr<const SongInfo> &song);

  /**
   * 同步播放按钮和进度条。
   * @param state 播放器报告的状态
   */
  void on_state_changed(player::PlaybackState state);

  /** 定时读取当前播放进度。 */
  void sync_playback_position();

  /** 切换音量浮层的显示状态。 */
  void toggle_volume_popup() const;

  /** 定位根窗口上的音量浮层。 */
  void position_volume_popup() const;

  /** 获取播放栏的位置，使用窗口坐标。 */
  [[nodiscard]] SkPoint window_position() const noexcept;

  /** 获取入场动画中的垂直偏移。 */
  [[nodiscard]] float slide_offset() const noexcept;

  TrackInfo *track_;                        // 左侧歌曲信息
  PlaybackControls *controls_;              // 中间播放控件
  VolumeSliderPopup *volume_popup_;         // 挂在根窗口上的音量浮层
  float reveal_ = 0.0f;                     // 滑入与淡入进度
  std::uint64_t last_position_sync_us_ = 0; // 上次采样播放进度的时间
};

PlayerBar::PlayerBar(Widget *parent) :
  Widget(parent), track_(new TrackInfo(this)), controls_(new PlaybackControls(this)),
  volume_popup_(new VolumeSliderPopup(root_widget())) {
  constexpr float kSidePadding = 34.0f;
  constexpr float kSidePreferredWidth = 48.0f;
  auto *const actions = new PlayerActions(this);
  // 两侧拥有相同的伸缩起点，中央播放键随窗口中线移动。
  track_->set_preferred_width(kSidePreferredWidth);
  actions->set_preferred_width(kSidePreferredWidth);
  track_->set_margin(Insets::from_left(kSidePadding));
  actions->set_margin(Insets::from_right(kSidePadding));
  set_layout(new HBoxLayout);
  set_fixed_height(86.0f);
  set_visible(false);

  controls_->previous_clicked.connect<&Signal<>::emit>(&previous_clicked);
  controls_->next_clicked.connect<&Signal<>::emit>(&next_clicked);
  controls_->playback_mode_changed.connect<&Signal<bool>::emit>(&playback_mode_changed);
  controls_->volume_clicked.connect<&PlayerBar::toggle_volume_popup>(this);
  auto &audio = player::bass24_player();
  audio.song_changed.connect<&PlayerBar::on_song_changed>(this);
  audio.state_changed.connect<&PlayerBar::on_state_changed>(this);
}

PlayerBar::~PlayerBar() {
  auto &audio = player::bass24_player();
  audio.song_changed.disconnect(this);
  audio.state_changed.disconnect(this);
  animation_manager->cancel(&reveal_);
}

void PlayerBar::dismiss_volume_popup_at(const float x, const float y) const {
  if (!volume_popup_->visible() || volume_popup_->contains(x, y)) {
    return;
  }
  const auto origin = window_position();
  const auto button =
    controls_->volume_button_bounds().makeOffset(origin.x(), origin.y() + slide_offset());
  if (!button.contains(x, y)) {
    volume_popup_->set_visible(false);
    controls_->set_volume_popup_open(false);
  }
}

void PlayerBar::paint(SkCanvas *canvas) {
  constexpr float kRadius = 15.0f;
  const auto panel = local_bounds().makeInset(14.0f, 6.0f);
  if (panel.isEmpty()) return;

  draw_shadow(
    canvas,
    panel.makeOffset(0.0f, 7.0f).makeInset(7.0f, 7.0f),
    kRadius,
    12.0f,
    ColorFromARGB(24, 25, 36, 52)
  );
  canvas->drawRect(
    SkRect::MakeWH(width(), 1.0f), PaintDesc { .color = ColorFromARGB(52, 42, 52, 68) }
  );
  canvas->drawRoundRect(
    panel, kRadius, kRadius, PaintDesc { .color = ColorFromARGB(186, 255, 255, 255) }
  );

  const auto glow =
    SkRect::MakeXYWH(panel.left() + 1.0f, panel.top() + 1.0f, panel.width() - 2.0f, 24.0f);
  canvas->drawRoundRect(
    glow, kRadius - 2.0f, kRadius - 2.0f, PaintDesc { .color = ColorFromARGB(50, 255, 255, 255) }
  );
  canvas->drawRoundRect(
    panel.makeInset(0.5f, 0.5f),
    kRadius,
    kRadius,
    PaintDesc {
      .color = ColorFromARGB(126, 255, 255, 255),
      .strokeWidth = 1.0f,
      .style = SkPaint::kStroke_Style,
    }
  );
}

void PlayerBar::render(SkCanvas *canvas) {
  sync_playback_position();
  canvas->save();
  canvas->translate(0.0f, slide_offset());
  if (reveal_ < 1.0f) {
    canvas->saveLayerAlphaf(nullptr, reveal_);
  }
  Widget::render(canvas);
  if (reveal_ < 1.0f) {
    canvas->restore();
  }
  canvas->restore();
}

void PlayerBar::layout_children() {
  Widget::layout_children();
  if (volume_popup_->visible()) {
    position_volume_popup();
  }
}

SkPoint PlayerBar::mouse_content_position(const float x, const float y) const {
  return SkPoint::Make(x, y - slide_offset());
}

void PlayerBar::on_song_changed(const std::shared_ptr<const SongInfo> &song) {
  if (!song) return;
  track_->set_song(*song);
  controls_->reset_progress(song->duration_text());
  if (!visible()) {
    set_visible(true);
    start_animation(reveal_, 1.0f, 350.0f, &reveal_, CubicBezier::EaseOut());
  }
}

void PlayerBar::on_state_changed(const player::PlaybackState state) {
  using player::PlaybackState;
  controls_->set_loading(state == PlaybackState::loading);
  controls_->set_playing(state == PlaybackState::playing);
  if (state == PlaybackState::playing) {
    last_position_sync_us_ = 0;
    controls_->set_duration(player::bass24_player().duration());
  } else if (state == PlaybackState::stopped) {
    controls_->set_position(0.0);
  }
}

void PlayerBar::sync_playback_position() {
  const auto &audio = player::bass24_player();
  if (audio.state() != player::PlaybackState::playing) return;

  constexpr std::uint64_t kPositionSyncIntervalUs = 100000; // 播放位置同步间隔
  const auto now = profiling::frame_clock.now;
  if (last_position_sync_us_ == 0 || now - last_position_sync_us_ >= kPositionSyncIntervalUs) {
    last_position_sync_us_ = now;
    controls_->set_position(audio.position());
  }
}

void PlayerBar::toggle_volume_popup() const {
  const bool open = !volume_popup_->visible();
  if (open) position_volume_popup();
  volume_popup_->set_visible(open);
  controls_->set_volume_popup_open(open);
}

void PlayerBar::position_volume_popup() const {
  constexpr float kPopupOverlap = 14.0f;                         // 浮层与播放栏的重叠距离
  constexpr float kWidth = VolumeSliderPopup::kPreferredWidth;   // 浮层宽度
  constexpr float kHeight = VolumeSliderPopup::kPreferredHeight; // 浮层高度
  const auto origin = window_position();
  const auto button = controls_->volume_button_bounds();
  const float x = std::round(origin.x() + button.left() + (button.width() - kWidth) * 0.5f);
  const float y = std::round(origin.y() - kHeight + kPopupOverlap);
  volume_popup_->set_geometry(x, y, kWidth, kHeight);
  volume_popup_->update_layout();
}

SkPoint PlayerBar::window_position() const noexcept {
  return SkPoint::Make(x() + parent_->x(), y() + parent_->y());
}

float PlayerBar::slide_offset() const noexcept {
  return std::round((1.0f - reveal_) * height());
}

} // namespace components
