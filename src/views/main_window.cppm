//
// Created by yuri on 2026/2/1.
//

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
void drawBlurredOval(SkCanvas *canvas, const SkRect &rect, SkColor color, float sigma);

export class MainWindow : public Window {
public:
  /** 创建主窗口和页面结构。 */
  MainWindow();

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
   * @param x 鼠标相对窗口的横坐标。
   * @param y 鼠标相对窗口的纵坐标。
   */
  void on_mouse_left_released(float x, float y) override;

private:
  /** 创建侧栏和各个页面，并绑定导航按钮。 */
  void setupNavigation();

  /**
   * 注册页面并创建与其关联的侧栏按钮。
   *
   * @param id 页面标识。
   * @param label 按钮文字。
   * @param icon SVG 图标路径。
   * @param page 页面控件。
   */
  void addPage(std::string_view id, std::string_view label, std::string_view icon, Widget &page);

  /**
   * 更新侧栏按钮选中状态。
   * @param id 当前页面标识。
   */
  void onPageChanged(std::string_view id);

  /**
   * 处理歌曲选中并显示播放栏。
   *
   * @param info 选中的歌曲信息。
   */
  void onCurrentSongChanged(const SongInfo &info) const;

  /**
   * 打开指定歌曲的上下文菜单。
   * @param info 请求菜单的歌曲信息
 */
  void onSongContextMenuRequested(const SongInfo &info);

  /**
   * 处理歌曲上下文菜单操作。
   * @param index 被点击的菜单项索引
   */
  void onSongContextMenuItemClicked(std::size_t index);

  /**
   * 同步 Bass 播放状态到播放栏。
   * @param state 当前播放状态
   */
  void onPlaybackStateChanged(bass24::PlaybackState state);

  /**
   * 同步歌曲总时长到播放栏。
   * @param duration_seconds 歌曲总时长（秒）
   */
  void onPlaybackDurationChanged(double duration_seconds);

  /**
   * 跳转到指定播放进度。
   *
   * @param ratio 目标播放进度比例。
   */
  void onSeekRequested(double ratio) const;

  /**
   * 向 Bass 请求更新播放器音量。
   *
   * @param volume 目标音量。
   */
  void onVolumeChanged(float volume) const;

  /**
   * 同步 Bass 音量到播放器控件。
   * @param volume 当前音量
   */
  void onPlayerVolumeChanged(float volume) const;

  /**
   * 同步播放器栏切换后的随机播放状态。
   *
   * @param random 是否启用随机播放。
   */
  void onPlaybackModeChanged(bool random) const;

  /** 打开或关闭竖向音量浮层。 */
  void onVolumeButtonClicked();

  /** 播放上一首歌曲。 */
  void onPreviousClicked() const;

  /** 切换播放或暂停状态。 */
  void onPlayPauseClicked();

  /** 播放下一首歌曲。 */
  void onNextClicked() const;

  /** 定期同步底层播放器位置。 */
  void syncPlaybackPosition();

  Widget *main_area = nullptr;                                           // 主内容与播放栏的布局容器
  NavigationView *navigation_ = nullptr;                                 // 侧栏和页面容器
  components::PlayerBar *player_bar = nullptr;                           // 底部播放栏
  components::VolumeSliderPopup *volume_popup = nullptr;                 // 竖向音量浮层
  ContextMenu *song_context_menu = nullptr;                              // 歌曲上下文菜单
  std::optional<SongInfo> context_menu_song{};                           // 菜单对应歌曲
  std::uint64_t last_position_sync_us = 0;                               // 上次播放位置采样时间
  double playback_duration_seconds = 0.0;                                // 当前歌曲总时长
  bass24::PlaybackState playback_state = bass24::PlaybackState::stopped; // 当前播放状态

  std::unordered_map<std::string_view, components::MenuButton *> menu_buttons{}; // 导航按钮集合

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
  main_area = new Widget(this);
  main_area->set_layout(new VBoxLayout());
  navigation_ = new NavigationView(main_area);
  navigation_->set_min_left_width(174.0f);
  navigation_->set_max_left_width(286.0f);
  navigation_->set_handle_visible(false);

  player_bar = new components::PlayerBar(main_area);
  player_bar->set_visible(false);
  player_bar->previousClicked.connect<&MainWindow::onPreviousClicked>(this);
  player_bar->playPauseClicked.connect<&MainWindow::onPlayPauseClicked>(this);
  player_bar->nextClicked.connect<&MainWindow::onNextClicked>(this);
  player_bar->seekRequested.connect<&MainWindow::onSeekRequested>(this);
  player_bar->playbackModeChanged.connect<&MainWindow::onPlaybackModeChanged>(this);
  player_bar->volumeButtonClicked.connect<&MainWindow::onVolumeButtonClicked>(this);

  volume_popup = new components::VolumeSliderPopup(this);
  volume_popup->set_visible(false);
  volume_popup->volume_changed.connect<&MainWindow::onVolumeChanged>(this);
  playback::controller.currentSongChanged.connect<&MainWindow::onCurrentSongChanged>(this);
  playback::controller.playbackStateChanged.connect<&MainWindow::onPlaybackStateChanged>(this);
  playback::controller.playbackDurationChanged.connect<&MainWindow::onPlaybackDurationChanged>(this);
  playback::controller.volumeChanged.connect<&MainWindow::onPlayerVolumeChanged>(this);

  setupNavigation();
  song_context_menu = new ContextMenu({
    ContextMenuItem("下一首播放"),
    ContextMenuItem("评论") },
    this
  );

  song_context_menu->item_clicked.connect<&MainWindow::onSongContextMenuItemClicked>(this);
  navigation_->pages().show_page("home");

  // 注入组件
  injector.addSingleton(player_bar);
}

void MainWindow::layout_children() {
  main_area->set_geometry(local_bounds());
  Widget::layout_children();

  if (player_bar->showing() && volume_popup->visible()) {
    constexpr float kPopupWidth = components::VolumeSliderPopup::kPreferredWidth;
    constexpr float kPopupHeight = components::VolumeSliderPopup::kPreferredHeight;
    const float popup_x = std::round(player_bar->volumeButtonCenterX() - kPopupWidth * 0.5f);
    const float popup_y = std::round(player_bar->y() - kPopupHeight + kVolumePopupOverlap);
    volume_popup->set_geometry(popup_x, popup_y, kPopupWidth, kPopupHeight);
    volume_popup->update_layout();
  }
}

void MainWindow::on_mouse_left_released(const float x, const float y) {
  if (!volume_popup->visible()) {
    return;
  }

  const bool inside_popup = volume_popup->contains(x, y);
  const bool inside_volume_button =
    player_bar->volumeButtonContains(x - player_bar->x(), y - player_bar->y());
  if (inside_popup || inside_volume_button) {
    return;
  }

  volume_popup->set_visible(false);
  player_bar->setVolumePopupVisible(false);
  mark_layout_dirty();
}

void MainWindow::setupNavigation() {
  auto &sidebar = navigation_->sidebar();
  sidebar.set_background(kSidebarFill);
  sidebar.set_padding(Insets(22, 22, 22, 18));
  auto *const layout = new VBoxLayout(VerticalAlignment::Top);
  layout->set_spacing(8.0f);
  auto &navigation = navigation_->navigation();
  navigation.set_layout(layout);
  new components::UserProfileCard(true, &navigation);

  auto *const pages = &navigation_->pages();
  pages->page_changed.connect<&MainWindow::onPageChanged>(this);
  addPage("home", "首页", kHomeSvg, *new pages::HomePage(pages));
  addPage("browse", "浏览", kBrowseSvg, *new pages::BrowsePage(pages));
  addPage("search", "搜索", kSearchSvg, *new pages::SearchPage(pages));
  addPage("library", "音乐库", kLibrarySvg, *new pages::LibraryPage(pages));

  auto *const favorites_page = new pages::FavoritesPage(pages);
  favorites_page->songContextMenuRequested.connect<&MainWindow::onSongContextMenuRequested>(this);
  addPage("favorites", "我喜欢", kFavoritesSvg, *favorites_page);
  addPage("recent", "最近播放", kRecentSvg, *new pages::RecentPage(pages));
  addPage("settings", "设置", kSettingsSvg, *new pages::SettingsPage(pages));
}

void MainWindow::addPage(
  const std::string_view id,
  const std::string_view label,
  const std::string_view icon,
  Widget &page
) {
  auto *const button = new components::MenuButton(label, icon, &navigation_->navigation());
  navigation_->add_page(id, page, *button);
  menu_buttons.emplace(id, button);
}

void MainWindow::onCurrentSongChanged(const SongInfo &info) const {
  player_bar->updateSong(info);
  player_bar->setPlaybackDuration(0.0);
  player_bar->setPlaybackPosition(0.0);
  player_bar->show();
}

void MainWindow::onSongContextMenuRequested(const SongInfo &info) {
  context_menu_song = info;
  song_context_menu->popup_at_cursor();
}

void MainWindow::onSongContextMenuItemClicked(const std::size_t index) {
  if (!context_menu_song) {
    return;
  }

  switch (index) {
    case 0: {
      playback::controller.enqueueNext(*context_menu_song);
      break;
    }
    case 1: {
      break;
    }
    default: break;
  }
  context_menu_song.reset();
}

void MainWindow::onPlaybackStateChanged(const bass24::PlaybackState state) {
  playback_state = state;
  switch (state) {
    case bass24::PlaybackState::loading:
    case bass24::PlaybackState::buffering:
      player_bar->setLoading(true);
      player_bar->setPlaying(false);
      break;
    case bass24::PlaybackState::playing:
      last_position_sync_us = 0;
      player_bar->setLoading(false);
      player_bar->setPlaying(true);
      break;
    case bass24::PlaybackState::paused:
    case bass24::PlaybackState::stopped:
      player_bar->setLoading(false);
      player_bar->setPlaying(false);
      break;
  }
}

void MainWindow::onPlaybackDurationChanged(const double duration_seconds) {
  playback_duration_seconds = duration_seconds;
  player_bar->setPlaybackDuration(playback_duration_seconds);
  if (playback_duration_seconds <= 0.001) {
    player_bar->setPlaybackPosition(0.0);
  }
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void MainWindow::onSeekRequested(const double ratio) const {
  if (playback::controller.seekRatio(ratio)) {
    player_bar->setPlaybackPosition(playback_duration_seconds * ratio);
  }
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void MainWindow::onVolumeChanged(const float volume) const {
  void(playback::controller.setVolume(volume));
}

void MainWindow::onPlayerVolumeChanged(const float volume) const {
  player_bar->setVolume(volume);
  volume_popup->setVolume(volume);
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void MainWindow::onPlaybackModeChanged(const bool random) const {
  playback::controller.setRandomPlayback(random);
}

void MainWindow::onVolumeButtonClicked() {
  const bool visible = !volume_popup->visible();
  volume_popup->set_visible(visible);
  player_bar->setVolumePopupVisible(visible);
  mark_layout_dirty();
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void MainWindow::onPreviousClicked() const {
  playback::controller.previous();
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void MainWindow::onPlayPauseClicked() {
  playback::controller.togglePause();
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void MainWindow::onNextClicked() const {
  playback::controller.next();
}

void MainWindow::syncPlaybackPosition() {
  if (!player_bar->showing() || playback_state != bass24::PlaybackState::playing) {
    return;
  }

  const auto now = profiling::frame_clock.now;
  // 限制 BASS 位置采样频率，避免每帧触发底层查询
  if (last_position_sync_us != 0 && now - last_position_sync_us < kPlaybackPositionSyncIntervalUs) {
    return;
  }
  last_position_sync_us = now;
  player_bar->setPlaybackPosition(playback::controller.positionSeconds());
}

void MainWindow::onPageChanged(const std::string_view id) {
  for (const auto &[menu_id, button] : menu_buttons) {
    button->setActive(menu_id == id);
  }
}

void MainWindow::render(SkCanvas *const canvas) {
  syncPlaybackPosition();

  canvas->clear(kWindowBg);

  const float w = width();
  const float h = height();

  SkPaint base;
  base.setAntiAlias(true);
  base.setColor(kWindowBg);
  canvas->drawRect(SkRect::MakeWH(w, h), base);

  drawBlurredOval(canvas, SkRect::MakeXYWH(-120.0f, -92.0f, 430.0f, 260.0f), kGlowRose, 42.0f);
  drawBlurredOval(canvas, SkRect::MakeXYWH(w - 320.0f, 48.0f, 420.0f, 260.0f), kGlowCyan, 48.0f);
  drawBlurredOval(
    canvas, SkRect::MakeXYWH(w * 0.28f, h - 160.0f, 360.0f, 220.0f), kGlowAmber, 46.0f
  );

  SkPaint grain;
  grain.setAntiAlias(true);
  grain.setColor(ColorFromARGB(18, 255, 255, 255));
  for (int i = 0; i < 12; ++i) {
    const float y = static_cast<float>(i) * 58.0f + 18.0f;
    canvas->drawRect(SkRect::MakeXYWH(0, y, w, 1.0f), grain);
  }

  Widget::render(canvas);
}

void drawBlurredOval(
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
