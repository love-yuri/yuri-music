//
// Created by yuri on 2026/2/1.
//

module;
#ifdef __linux__
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#endif

export module main_window;

import components;
import pages;
import yuri.core;
import yuri.glfw;
import yuri.skia;
import std;
import yuri.ui;
import bass24;
import models;
import playback;
#ifdef __linux__
import linux_tray;
import webview2;
#endif

using namespace skia;
using namespace glfw;
using namespace ui::widgets;
using namespace ui::layout;
using namespace ui::render;

constexpr SkColor kWindowBg = ColorFromARGB(255, 238, 243, 249);    // 窗口背景色
constexpr SkColor kSidebarFill = ColorFromARGB(104, 255, 255, 255); // 侧栏填充色
constexpr SkColor kGlowRose = ColorFromARGB(58, 255, 82, 132);      // 玫红柔光色
constexpr SkColor kGlowCyan = ColorFromARGB(46, 38, 189, 220);      // 青色柔光色
constexpr SkColor kGlowAmber = ColorFromARGB(34, 255, 184, 84);     // 琥珀柔光色
constexpr std::uint64_t kPlaybackPositionSyncIntervalUs = 100000;   // 播放位置同步间隔

/**
 * 绘制带边界的柔光椭圆。
 *
 * @param canvas 目标画布。
 * @param rect 椭圆边界。
 * @param color 填充颜色。
 * @param sigma 模糊半径。
 */
void draw_blurred_oval(SkCanvas *canvas, const SkRect &rect, SkColor color, float sigma);

/** 请求播放上一首歌曲。 */
void play_previous();

/** 请求切换播放或暂停状态。 */
void toggle_play_pause();

/** 请求播放下一首歌曲。 */
void play_next();

/**
 * 请求更新底层播放器音量。
 * @param volume 目标音量。
 */
void request_volume(float volume);

/**
 * 请求更新随机播放模式。
 * @param random 是否启用随机播放。
 */
void request_playback_mode(bool random);

export class MainWindow : public Window {
public:
  /** 创建主窗口和页面结构。 */
  MainWindow();

#ifdef __linux__
  /** 显示主窗口，并处理关窗转后台、托盘恢复与退出。 */
  void show();
#endif

  /**
   * 渲染窗口背景和子控件。
   *
   * @param canvas 目标画布。
   */
  void render(SkCanvas *canvas) final;

  /** 更新主内容区域与独立浮层的位置。 */
  void layout_children() override;

protected:
  /**
   * 点击音量浮层外部时关闭浮层。
   *
   * @param mouse_x 鼠标相对窗口的横坐标。
   * @param mouse_y 鼠标相对窗口的纵坐标。
   */
  void on_mouse_left_released(float mouse_x, float mouse_y) override;

private:
#ifdef __linux__
  /** 在主线程响应托盘双击，恢复隐藏的窗口或请求焦点。 */
  void activate_from_tray();

  /** 在主线程请求结束窗口循环和后台循环。 */
  void quit_from_tray();
#endif

  /** 创建侧栏和各个页面，并绑定导航按钮。 */
  void setup_navigation();

  /**
   * 注册页面并创建与其关联的侧栏按钮。
   *
   * @param id 页面标识。
   * @param label 按钮文字。
   * @param icon SVG 图标路径。
   * @param page 页面控件。
   */
  void add_page(std::string_view id, std::string_view label, std::string_view icon, Widget &page);

  /**
   * 更新侧栏按钮选中状态。
   * @param id 当前页面标识。
   */
  void on_page_changed(std::string_view id) const;

  /**
   * 处理歌曲选中并显示播放栏。
   *
   * @param info 选中的歌曲信息。
   */
  void on_current_song_changed(const SongInfo &info) const;

  /**
   * 打开指定歌曲的上下文菜单。
   * @param info 请求菜单的歌曲信息。
   */
  void on_song_context_menu_requested(const SongInfo &info);

  /**
   * 处理歌曲上下文菜单操作。
   * @param index 被点击的菜单项索引。
   */
  void on_song_context_menu_item_clicked(std::size_t index);

  /**
   * 同步 Bass 播放状态到播放栏。
   * @param state 当前播放状态。
   */
  void on_playback_state_changed(bass24::PlaybackState state);

  /**
   * 同步歌曲总时长到播放栏。
   * @param duration_seconds 歌曲总时长（秒）。
   */
  void on_playback_duration_changed(double duration_seconds);

  /**
   * 跳转到指定播放进度。
   *
   * @param ratio 目标播放进度比例。
   */
  void on_seek_requested(double ratio) const;

  /**
   * 同步 Bass 音量到播放器控件。
   * @param volume 当前音量。
   */
  void on_player_volume_changed(float volume) const;

  /** 打开或关闭竖向音量浮层。 */
  void on_volume_button_clicked();

  /** 定期同步底层播放器位置。 */
  void sync_playback_position();

  Widget *main_area_ = nullptr;                           // 主内容与播放栏的布局容器
  NavigationView *navigation_ = nullptr;                  // 侧栏和页面容器
  components::PlayerBar *player_bar_ = nullptr;           // 底部播放栏
  components::VolumeSliderPopup *volume_popup_ = nullptr; // 竖向音量浮层
  ContextMenu *song_context_menu_ = nullptr;              // 歌曲上下文菜单
  std::optional<SongInfo> context_menu_song_;             // 菜单对应歌曲
  std::uint64_t last_position_sync_us_ = 0;               // 上次播放位置采样时间
  double playback_duration_seconds_ = 0.0;                // 当前歌曲总时长
  bass24::PlaybackState playback_state_ = bass24::PlaybackState::stopped; // 当前播放状态
#ifdef __linux__
  bool hidden_to_tray_ = false; // 主窗口是否隐藏到托盘
  bool quit_requested_ = false; // 是否从托盘请求退出
#endif

  std::unordered_map<std::string_view, components::MenuButton *> menu_buttons_ ; // 导航按钮集合

  static constexpr float kVolumePopupOverlap = 14.0f;                  // 音量浮层重叠距离
  static constexpr auto kHomeSvg = "resources/svg/home.svg";           // 首页图标路径
  static constexpr auto kBrowseSvg = "resources/svg/browse.svg";       // 浏览图标路径
  static constexpr auto kSearchSvg = "resources/svg/search.svg";       // 搜索图标路径
  static constexpr auto kLibrarySvg = "resources/svg/library.svg";     // 音乐库图标路径
  static constexpr auto kFavoritesSvg = "resources/svg/favorites.svg"; // 我喜欢图标路径
  static constexpr auto kRecentSvg = "resources/svg/recent.svg";       // 最近播放图标路径
  static constexpr auto kSettingsSvg = "resources/svg/settings.svg";   // 设置图标路径
};

MainWindow::MainWindow() : Window(1024, 700) {
  main_area_ = new Widget(this);
  main_area_->set_layout(new VBoxLayout());
  navigation_ = new NavigationView(main_area_);
  navigation_->set_min_left_width(174.0f);
  navigation_->set_max_left_width(286.0f);
  navigation_->set_handle_visible(false);

  // Signal 不持有回调对象，静态函数指针在连接的整个生命周期内有效。
  static auto previous_clicked = &play_previous;
  static auto play_pause_clicked = &toggle_play_pause;
  static auto next_clicked = &play_next;
  static auto playback_mode_changed = &request_playback_mode;
  static auto volume_changed = &request_volume;

  player_bar_ = new components::PlayerBar(main_area_);
  player_bar_->set_visible(false);
  player_bar_->previousClicked.connect(previous_clicked);
  player_bar_->playPauseClicked.connect(play_pause_clicked);
  player_bar_->nextClicked.connect(next_clicked);
  player_bar_->seekRequested.connect<&MainWindow::on_seek_requested>(this);
  player_bar_->playbackModeChanged.connect(playback_mode_changed);
  player_bar_->volumeButtonClicked.connect<&MainWindow::on_volume_button_clicked>(this);

  volume_popup_ = new components::VolumeSliderPopup(this);
  volume_popup_->set_visible(false);
  volume_popup_->volume_changed.connect(volume_changed);
  playback::controller.currentSongChanged.connect<&MainWindow::on_current_song_changed>(this);
  playback::controller.playbackStateChanged.connect<&MainWindow::on_playback_state_changed>(this);
  playback::controller.playbackDurationChanged.connect<&MainWindow::on_playback_duration_changed>(
    this
  );
  playback::controller.volumeChanged.connect<&MainWindow::on_player_volume_changed>(this);

  setup_navigation();
  song_context_menu_ =
    new ContextMenu({ ContextMenuItem("下一首播放"), ContextMenuItem("评论") }, this);

  song_context_menu_->item_clicked.connect<&MainWindow::on_song_context_menu_item_clicked>(this);
  navigation_->pages().show_page("home");

  // 注入组件
  injector.addSingleton(player_bar_);
}

#ifdef __linux__
void MainWindow::show() {
  const LinuxTray tray(
    [this] { ui::dispatcher.post([this] { activate_from_tray(); }); },
    [this] { ui::dispatcher.post([this] { quit_from_tray(); }); }
  );

  while (!quit_requested_) {
    glfwSetWindowShouldClose(window_, GLFW_FALSE);
    Window::show();
    if (quit_requested_ || !tray.registered()) {
      break;
    }

    hidden_to_tray_ = true;
    glfwHideWindow(window_);
    // 隐藏时继续处理播放任务，双击托盘后才重新进入渲染循环。
    while (hidden_to_tray_ && !quit_requested_) {
      ui::dispatcher.processPending();
      if (hidden_to_tray_ && !quit_requested_) {
        glfwWaitEvents();
      }
    }
  }
  webview2::request_login_shutdown();
}

void MainWindow::activate_from_tray() {
  if (hidden_to_tray_) {
    hidden_to_tray_ = false;
  } else if (!quit_requested_) {
    glfwFocusWindow(window_);
  }
}

void MainWindow::quit_from_tray() {
  quit_requested_ = true;
  glfwSetWindowShouldClose(window_, GLFW_TRUE);
  // 渲染循环处理任务后可能再等待一次事件，唤醒它以便检查退出标记。
  glfwPostEmptyEvent();
}
#endif

void MainWindow::layout_children() {
  main_area_->set_geometry(local_bounds());
  Widget::layout_children();

  if (player_bar_->showing() && volume_popup_->visible()) {
    constexpr float kPopupWidth = components::VolumeSliderPopup::kPreferredWidth;
    constexpr float kPopupHeight = components::VolumeSliderPopup::kPreferredHeight;
    const float popup_x = std::round(player_bar_->volumeButtonCenterX() - kPopupWidth * 0.5f);
    const float popup_y = std::round(player_bar_->y() - kPopupHeight + kVolumePopupOverlap);
    volume_popup_->set_geometry(popup_x, popup_y, kPopupWidth, kPopupHeight);
    volume_popup_->update_layout();
  }
}

void MainWindow::on_mouse_left_released(const float mouse_x, const float mouse_y) {
  if (!volume_popup_->visible()) {
    return;
  }

  const bool inside_popup = volume_popup_->contains(mouse_x, mouse_y);
  const bool inside_volume_button =
    player_bar_->volumeButtonContains(mouse_x - player_bar_->x(), mouse_y - player_bar_->y());
  if (inside_popup || inside_volume_button) {
    return;
  }

  volume_popup_->set_visible(false);
  player_bar_->setVolumePopupVisible(false);
  mark_layout_dirty();
}

void MainWindow::setup_navigation() {
  auto &sidebar = navigation_->sidebar();
  sidebar.set_background(kSidebarFill);
  sidebar.set_padding(Insets(22, 22, 22, 18));
  auto *const layout = new VBoxLayout(VerticalAlignment::Top);
  layout->set_spacing(8.0f);
  auto &navigation = navigation_->navigation();
  navigation.set_layout(layout);
  new components::UserProfileCard(true, &navigation);

  auto *const pages = &navigation_->pages();
  pages->page_changed.connect<&MainWindow::on_page_changed>(this);
  add_page("home", "首页", kHomeSvg, *new pages::HomePage(pages));
  add_page("browse", "浏览", kBrowseSvg, *new pages::BrowsePage(pages));
  add_page("search", "搜索", kSearchSvg, *new pages::SearchPage(pages));
  add_page("library", "音乐库", kLibrarySvg, *new pages::LibraryPage(pages));

  auto *const favorites_page = new pages::FavoritesPage(pages);
  favorites_page->songContextMenuRequested.connect<&MainWindow::on_song_context_menu_requested>(
    this
  );
  add_page("favorites", "我喜欢", kFavoritesSvg, *favorites_page);
  add_page("recent", "最近播放", kRecentSvg, *new pages::RecentPage(pages));
  add_page("settings", "设置", kSettingsSvg, *new pages::SettingsPage(pages));
}

void MainWindow::add_page(
  const std::string_view id,
  const std::string_view label,
  const std::string_view icon,
  Widget &page
) {
  auto *const button = new components::MenuButton(label, icon, &navigation_->navigation());
  navigation_->add_page(id, page, *button);
  menu_buttons_.emplace(id, button);
}

void MainWindow::on_current_song_changed(const SongInfo &info) const {
  player_bar_->updateSong(info);
  player_bar_->setPlaybackDuration(0.0);
  player_bar_->setPlaybackPosition(0.0);
  player_bar_->show();
}

void MainWindow::on_song_context_menu_requested(const SongInfo &info) {
  context_menu_song_ = info;
  song_context_menu_->popup_at_cursor();
}

void MainWindow::on_song_context_menu_item_clicked(const std::size_t index) {
  if (!context_menu_song_) {
    return;
  }

  switch (index) {
    case 0: {
      playback::controller.enqueueNext(*context_menu_song_);
      break;
    }
    case 1: {
      break;
    }
    default: break;
  }
  context_menu_song_.reset();
}

void MainWindow::on_playback_state_changed(const bass24::PlaybackState state) {
  playback_state_ = state;
  switch (state) {
    case bass24::PlaybackState::loading:
    case bass24::PlaybackState::buffering:
      player_bar_->setLoading(true);
      player_bar_->setPlaying(false);
      break;
    case bass24::PlaybackState::playing:
      last_position_sync_us_ = 0;
      player_bar_->setLoading(false);
      player_bar_->setPlaying(true);
      break;
    case bass24::PlaybackState::paused:
    case bass24::PlaybackState::stopped:
      player_bar_->setLoading(false);
      player_bar_->setPlaying(false);
      break;
  }
}

void MainWindow::on_playback_duration_changed(const double duration_seconds) {
  playback_duration_seconds_ = duration_seconds;
  player_bar_->setPlaybackDuration(playback_duration_seconds_);
  if (playback_duration_seconds_ <= 0.001) {
    player_bar_->setPlaybackPosition(0.0);
  }
}

void MainWindow::on_seek_requested(const double ratio) const {
  if (playback::controller.seekRatio(ratio)) {
    player_bar_->setPlaybackPosition(playback_duration_seconds_ * ratio);
  }
}

void MainWindow::on_player_volume_changed(const float volume) const {
  player_bar_->setVolume(volume);
  volume_popup_->setVolume(volume);
}

void MainWindow::on_volume_button_clicked() {
  const bool visible = !volume_popup_->visible();
  volume_popup_->set_visible(visible);
  player_bar_->setVolumePopupVisible(visible);
  mark_layout_dirty();
}

void MainWindow::sync_playback_position() {
  if (!player_bar_->showing() || playback_state_ != bass24::PlaybackState::playing) {
    return;
  }

  const auto now = profiling::frame_clock.now;
  // 限制 BASS 位置采样频率，避免每帧触发底层查询
  if (
    last_position_sync_us_ != 0 && now - last_position_sync_us_ < kPlaybackPositionSyncIntervalUs
  ) {
    return;
  }
  last_position_sync_us_ = now;
  player_bar_->setPlaybackPosition(playback::controller.positionSeconds());
}

void MainWindow::on_page_changed(const std::string_view id) const {
  for (const auto &[menu_id, button] : menu_buttons_) {
    button->setActive(menu_id == id);
  }
}

void MainWindow::render(SkCanvas *const canvas) {
  sync_playback_position();

  canvas->clear(kWindowBg);

  const float window_width = width();
  const float window_height = height();

  SkPaint base;
  base.setAntiAlias(true);
  base.setColor(kWindowBg);
  canvas->drawRect(SkRect::MakeWH(window_width, window_height), base);

  const auto rose_bounds = SkRect::MakeXYWH(-120.0f, -92.0f, 430.0f, 260.0f);
  const auto cyan_bounds = SkRect::MakeXYWH(window_width - 320.0f, 48.0f, 420.0f, 260.0f);
  const auto amber_bounds =
    SkRect::MakeXYWH(window_width * 0.28f, window_height - 160.0f, 360.0f, 220.0f);
  draw_blurred_oval(canvas, rose_bounds, kGlowRose, 42.0f);
  draw_blurred_oval(canvas, cyan_bounds, kGlowCyan, 48.0f);
  draw_blurred_oval(canvas, amber_bounds, kGlowAmber, 46.0f);

  SkPaint grain;
  grain.setAntiAlias(true);
  grain.setColor(ColorFromARGB(18, 255, 255, 255));
  for (int row = 0; row < 12; ++row) {
    const float line_y = static_cast<float>(row) * 58.0f + 18.0f;
    canvas->drawRect(SkRect::MakeXYWH(0, line_y, window_width, 1.0f), grain);
  }

  Widget::render(canvas);
}

void draw_blurred_oval(
  SkCanvas *const canvas,
  const SkRect &rect,
  const SkColor color,
  const float sigma
) {
  const auto bounds = rect.makeOutset(sigma * 3.0f, sigma * 3.0f);
  SkPaint layer;
  layer.setImageFilter(SkImageFilters::Blur(sigma, sigma, nullptr));

  SkPaint fill;
  fill.setAntiAlias(true);
  fill.setColor(color);

  // 使用独立图层限制模糊范围，避免柔光影响其他绘制内容
  canvas->saveLayer(&bounds, &layer);
  canvas->drawOval(rect, fill);
  canvas->restore();
}

void play_previous() {
  playback::controller.previous();
}

void toggle_play_pause() {
  playback::controller.togglePause();
}

void play_next() {
  playback::controller.next();
}

void request_volume(const float volume) {
  void(playback::controller.setVolume(volume));
}

void request_playback_mode(const bool random) {
  playback::controller.setRandomPlayback(random);
}
