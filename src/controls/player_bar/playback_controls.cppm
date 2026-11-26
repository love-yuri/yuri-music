export module components:player_bar_playback_controls;

import std;
import player;
import yuri.core;
import yuri.skia;
import yuri.ui;
import :player_bar_slider;
import :player_bar_style;
import :player_bar_transport_button;

using namespace skia;
using namespace ui::layout;
using namespace ui::widgets;
using ui::render::PaintDesc;

/**
 * 将整秒格式化为 m:ss。
 * @param seconds 总秒数
 * @return 时长文字
 */
std::string format_time(int seconds);

/**
 * 将歌曲信息中的 m:ss 转为秒数。
 * @param text 时长文字
 * @return 秒数，无法解析时返回零
 */
double parse_duration(std::string_view text);

namespace controls::player_bar {

constexpr float kTrayWidth = 256.0f; // 播放按钮行的标准宽度

/** 播放按钮行的胶囊背景。 */
class TransportTray : public Widget {
public:
  /**
   * 创建按钮行。
   * @param parent 父控件
   */
  explicit TransportTray(Widget *parent);

  /**
   * 绘制按钮后面的胶囊面板。
   * @param canvas 目标画布
   */
  void paint(SkCanvas *canvas) override;
};

/** 用水平布局组合时长标签与可拖动的进度条。 */
class ProgressRow : public Widget {
public:
  /**
   * 创建进度行。
   * @param parent 父控件
   */
  explicit ProgressRow(Widget *parent);

  /**
   * 开始显示新歌曲的时长和进度。
   * @param duration 歌曲信息中的时长文字
   */
  void reset(std::string_view duration);

  /**
   * 同步播放器报告的时长。
   * @param seconds 时长，单位为秒
   */
  void set_duration(double seconds);

  /**
   * 同步播放器报告的播放位置。
   * @param seconds 当前播放位置，单位为秒
   */
  void set_position(double seconds);

private:
  /**
   * 提交滑条操作并更新经过时间。
   * @param ratio 归一化播放位置
   */
  void seek(float ratio);

  Text *elapsed_;           // 已播放时间
  PlayerSlider *slider_;    // 播放进度滑条
  Text *total_;             // 总时长
  double duration_ = 0.0;   // 当前歌曲时长
  int elapsed_seconds_ = 0; // 最近显示的已播放秒数
};

/** 由传输按钮行与进度行组成的播放栏中央控件。 */
export class PlaybackControls : public Widget {
public:
  Signal<> previous_clicked;          // 上一首
  Signal<> next_clicked;              // 下一首
  Signal<bool> playback_mode_changed; // 是否切换到随机播放
  Signal<> volume_clicked;            // 音量按钮点击

  /**
   * 创建中央播放控件。
   * @param parent 父控件
   */
  explicit PlaybackControls(Widget *parent);

  /** 断开播放器的音量订阅。 */
  ~PlaybackControls() override;

  /**
   * 重置新歌曲的进度条。
   * @param duration 歌曲信息中的时长文字
   */
  void reset_progress(std::string_view duration);

  /**
   * 同步播放状态。
   * @param playing 是否正在播放
   */
  void set_playing(bool playing);

  /**
   * 同步加载状态。
   * @param loading 是否正在加载
   */
  void set_loading(bool loading);

  /**
   * 同步歌曲时长。
   * @param seconds 总时长，单位为秒
   */
  void set_duration(double seconds) const;

  /**
   * 同步播放位置。
   * @param seconds 播放位置，单位为秒
   */
  void set_position(double seconds) const;

  /**
   * 同步音量浮层状态。
   * @param open 浮层是否已打开
   */
  void set_volume_popup_open(bool open);

  /** 获取由布局分配的音量按钮区域。 */
  [[nodiscard]] SkRect volume_button_bounds() const noexcept;

private:
  /**
   * 同步音量按钮的静音状态。
   * @param volume 归一化音量
   */
  void set_volume(float volume);

  /** 切换顺序和随机播放。 */
  void toggle_playback_mode();

  /** 更新播放按钮的图标与可用状态。 */
  void update_play_button() const;

  /** 更新音量按钮的图标与选中状态。 */
  void update_volume_button() const;

  TransportTray *tray_;               // 按钮行
  TransportButton *mode_;             // 播放方式按钮
  TransportButton *play_ = nullptr;   // 播放主按钮
  TransportButton *volume_ = nullptr; // 音量按钮
  ProgressRow *progress_ = nullptr;   // 进度行
  bool random_ = false;               // 是否随机播放
  bool playing_ = false;              // 是否正在播放
  bool loading_ = false;              // 是否正在加载
  bool muted_ = false;                // 是否静音
  bool volume_popup_open_ = false;    // 音量浮层是否打开
};

} // namespace controls::player_bar

using namespace controls::player_bar;

TransportTray::TransportTray(Widget *parent) : Widget(parent) {
  auto *const buttons = new HBoxLayout(HorizontalAlignment::Center);
  buttons->set_spacing(6.0f);
  set_layout(buttons);
  set_fixed_height(48.0f);
  set_horizontal_alignment(HorizontalAlignment::Center);
  set_preferred_width(kTrayWidth);
}

void TransportTray::paint(SkCanvas *canvas) {
  constexpr float kRadius = 21.0f;
  const auto tray = local_bounds().makeInset(0.0f, 3.0f);
  if (tray.isEmpty()) return;

  draw_shadow(
    canvas,
    tray.makeOffset(0.0f, 6.0f).makeInset(6.0f, 6.0f),
    kRadius - 4.0f,
    10.0f,
    ColorFromARGB(24, 24, 31, 42)
  );
  canvas->drawRoundRect(
    tray, kRadius, kRadius, PaintDesc { .color = ColorFromARGB(148, 255, 255, 255) }
  );
  canvas->drawRoundRect(
    tray.makeInset(0.5f, 0.5f),
    kRadius,
    kRadius,
    PaintDesc {
      .color = ColorFromARGB(142, 255, 255, 255),
      .strokeWidth = 1.0f,
      .style = SkPaint::kStroke_Style,
    }
  );
}

ProgressRow::ProgressRow(Widget *parent) :
  Widget(parent), elapsed_(new Text("0:00", this)),
  slider_(new PlayerSlider(PlayerSlider::Mode::Seek, this)), total_(new Text("", this)) {
  set_layout(new HBoxLayout);
  set_fixed_height(16.0f);

  for (auto *const label : { elapsed_, total_ }) {
    label->set_preferred_width(42.0f);
    label->set_horizontal_alignment(HorizontalAlignment::Left);
    label->set_font_size(11.5f);
    label->set_color(kTextMuted);
  }
  elapsed_->set_alignment(Alignment::CenterRight);
  total_->set_alignment(Alignment::CenterLeft);
  slider_->set_margin(Insets(6.0f, 0.0f));
  slider_->set_preferred_width(180.0f);
  slider_->value_changed.connect<&ProgressRow::seek>(this);
}

void ProgressRow::reset(const std::string_view duration) {
  duration_ = parse_duration(duration);
  total_->set_text(duration);
  set_position(0.0);
}

void ProgressRow::set_duration(const double seconds) {
  duration_ = seconds;
  total_->set_text(format_time(static_cast<int>(std::lround(seconds))));
}

void ProgressRow::set_position(const double seconds) {
  slider_->set_value(duration_ > 0.001 ? static_cast<float>(seconds / duration_) : 0.0f);
  const auto whole_seconds = static_cast<int>(seconds);
  if (whole_seconds == elapsed_seconds_) return;

  elapsed_seconds_ = whole_seconds;
  elapsed_->set_text(format_time(whole_seconds));
}

void ProgressRow::seek(const float ratio) {
  if (duration_ > 0.001) set_position(duration_ * ratio);
  auto &audio = player::bass24_player();
  if (audio.seek(audio.duration() * ratio)) {
    set_position(audio.position());
  }
}

PlaybackControls::PlaybackControls(Widget *parent) :
  Widget(parent), tray_(new TransportTray(this)),
  mode_(new TransportButton(TransportIcon::Repeat, false, tray_)) {
  auto *const rows = new VBoxLayout(VerticalAlignment::Center);
  rows->set_spacing(2.0f);
  set_layout(rows);
  set_margin(Insets::from_top(6.0f));
  set_preferred_width(kTrayWidth);
  set_max_width(664.0f);

  auto *const previous = new TransportButton(TransportIcon::Previous, false, tray_);
  play_ = new TransportButton(TransportIcon::Play, true, tray_);
  auto *const next = new TransportButton(TransportIcon::Next, false, tray_);
  volume_ = new TransportButton(TransportIcon::Volume, false, tray_);
  progress_ = new ProgressRow(this);

  mode_->clicked.connect<&PlaybackControls::toggle_playback_mode>(this);
  previous->clicked.connect<&Signal<>::emit>(&previous_clicked);
  auto &audio = player::bass24_player();
  play_->clicked.connect<&player::AudioPlayer::toggle>(&audio);
  audio.volume_changed.connect<&PlaybackControls::set_volume>(this);
  next->clicked.connect<&Signal<>::emit>(&next_clicked);
  volume_->clicked.connect<&Signal<>::emit>(&volume_clicked);
}

PlaybackControls::~PlaybackControls() {
  player::bass24_player().volume_changed.disconnect(this);
}

void PlaybackControls::reset_progress(const std::string_view duration) {
  progress_->reset(duration);
}

void PlaybackControls::set_playing(const bool playing) {
  playing_ = playing;
  update_play_button();
}

void PlaybackControls::set_loading(const bool loading) {
  loading_ = loading;
  update_play_button();
}

void PlaybackControls::set_duration(const double seconds) const {
  progress_->set_duration(seconds);
}

void PlaybackControls::set_position(const double seconds) const {
  progress_->set_position(seconds);
}

void PlaybackControls::set_volume(const float volume) {
  muted_ = volume <= 0.001f;
  update_volume_button();
}

void PlaybackControls::set_volume_popup_open(const bool open) {
  volume_popup_open_ = open;
  update_volume_button();
}

SkRect PlaybackControls::volume_button_bounds() const noexcept {
  return volume_->hit_test_bounds().makeOffset(x() + tray_->x(), y() + tray_->y());
}

void PlaybackControls::toggle_playback_mode() {
  random_ = !random_;
  mode_->set_icon(random_ ? TransportIcon::Shuffle : TransportIcon::Repeat, random_);
  playback_mode_changed.emit(random_);
}

void PlaybackControls::update_play_button() const {
  const auto playback_icon = playing_ ? TransportIcon::Pause : TransportIcon::Play;
  play_->set_icon(loading_ ? TransportIcon::Loading : playback_icon);
}

void PlaybackControls::update_volume_button() const {
  volume_->set_icon(muted_ ? TransportIcon::Muted : TransportIcon::Volume, volume_popup_open_);
}

std::string format_time(const int seconds) {
  return std::format("{}:{:02}", seconds / 60, seconds % 60);
}

double parse_duration(const std::string_view text) {
  const auto colon = text.find(':');
  if (colon == std::string_view::npos) return 0.0;

  int minutes = 0;
  int seconds = 0;
  const auto *const split = text.data() + colon;
  const auto *const end = text.data() + text.size();
  const auto minutes_result = std::from_chars(text.data(), split, minutes);
  const auto seconds_result = std::from_chars(split + 1, end, seconds);
  if (
    minutes_result.ec != std::errc {} || minutes_result.ptr != split
    || seconds_result.ec != std::errc {} || seconds_result.ptr != end
  ) {
    return 0.0;
  }
  return std::max(0.0, minutes * 60.0 + seconds);
}
