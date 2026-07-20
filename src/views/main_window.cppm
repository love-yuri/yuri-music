//
// Created by yuri on 2026/2/1.
//

export module main_window;

import page_view;
import components;
import pages;
import core;
import glfw;
import skia;
import std;
import ui;
import bass24;
import models;

using namespace skia;
using namespace glfw;
using namespace ui::widgets;
using namespace ui::layout;
using namespace ui::render;

constexpr SkColor kWindowBg = ColorFromARGB(255, 238, 243, 249);
constexpr SkColor kSidebarFill = ColorFromARGB(104, 255, 255, 255);
constexpr SkColor kGlowRose = ColorFromARGB(58, 255, 82, 132);
constexpr SkColor kGlowCyan = ColorFromARGB(46, 38, 189, 220);
constexpr SkColor kGlowAmber = ColorFromARGB(34, 255, 184, 84);
constexpr std::uint64_t kPlaybackSyncIntervalUs = 100000;

// 绘制带边界的柔光椭圆
void drawBlurredOval(SkCanvas *canvas, const SkRect &rect, SkColor color, float sigma);

// 左侧整块背景面板
class SidebarSurface : public Box {
public:
  using Box::Box;

  // 绘制侧栏背景
  void paint(SkCanvas *canvas) override;
};

// 保留拖拽能力但不绘制分隔线的分栏
class MusicSplitter : public Splitter {
public:
  using Splitter::Splitter;

  // 绘制分栏拖拽区域
  void paint(SkCanvas *canvas) override;
};

void drawBlurredOval(SkCanvas *canvas, const SkRect &rect, SkColor color, float sigma) {
  auto bounds = rect.makeOutset(sigma * 3.0f, sigma * 3.0f);
  SkPaint layer;
  layer.setImageFilter(SkImageFilters::Blur(sigma, sigma, nullptr));

  SkPaint fill;
  fill.setAntiAlias(true);
  fill.setColor(color);

  canvas->saveLayer(&bounds, &layer);
  canvas->drawOval(rect, fill);
  canvas->restore();
}

void SidebarSurface::paint(SkCanvas *canvas) {
  SkPaint fill;
  fill.setAntiAlias(true);
  fill.setColor(kSidebarFill);
  canvas->drawRect(borderRect(), fill);

  SkPaint edge;
  edge.setAntiAlias(true);
  edge.setColor(ColorFromARGB(34, 255, 255, 255));
  canvas->drawRect(SkRect::MakeXYWH(width_ - 1.0f, 0.0f, 1.0f, height_), edge);
}

void MusicSplitter::paint(SkCanvas *) {
  // 保留拖拽命中区域，视觉上不绘制桌面分隔线
}

export class MainWindow : public Window {
  using Window::Window;

public:
  // 创建主窗口和页面结构
  MainWindow();
  // 渲染窗口背景和子控件
  void render(SkCanvas *canvas) final;
  // 布局侧栏、页面和底部播放栏
  void layoutChildren() override;

private:
  // 创建左侧菜单
  void setupSidebar();
  // 创建右侧页面
  void setupPages();
  // 处理菜单点击切页
  void onMenuClicked(const std::string &id);
  // 处理歌曲选中并显示播放栏
  void onSongSelected(const SongInfo &info) const;
  void onPlaybackStateChanged(bool playing) const;
  void onLoadingStateChanged(bool loading) const;
  void onSeekRequested(double ratio);
  void onVolumeChanged(float volume);
  void onPreviousClicked() const;
  void onPlayPauseClicked() const;
  void onNextClicked() const;
  void syncPlaybackState();

  Splitter *splitter_ = nullptr;                        // 主分栏
  Box *sidebar_ = nullptr;                              // 左侧菜单面板
  components::UserProfileCard *profile_card_ = nullptr; // 侧栏个人资料
  PageView *page_view = nullptr;                        // 页面容器
  components::PlayerBar *player_bar = nullptr;          // 底部播放栏
  pages::FavoritesPage *favorites_page = nullptr;       // 我喜欢页面
  std::uint64_t last_playback_sync_us = 0;              // 上次 BASS 状态采样时间

  std::unordered_map<std::string, MenuButton *> menu_buttons; // 菜单按钮集合

  // 播放器底栏高度
  static constexpr float kPlayerBarHeight = 86.0f;
  // 各菜单项对应的 SVG 图标路径
  static constexpr auto home_svg = "resources/svg/home.svg";
  static constexpr auto browse_svg = "resources/svg/browse.svg";
  static constexpr auto search_svg = "resources/svg/search.svg";
  static constexpr auto library_svg = "resources/svg/library.svg";
  static constexpr auto favorites_svg = "resources/svg/favorites.svg";
  static constexpr auto recent_svg = "resources/svg/recent.svg";
  static constexpr auto settings_svg = "resources/svg/settings.svg";
};

MainWindow::MainWindow() : Window(1024, 700) {
  splitter_ = new MusicSplitter(this);
  splitter_->setMinLeftWidth(174.f);
  splitter_->setMaxLeftWidth(286.f);

  sidebar_ = new SidebarSurface(splitter_);
  sidebar_->setPadding(Insets(22, 22, 22, 18));
  sidebar_->setLayout<VBoxLayout<Widget>>();
  sidebar_->layout()->setSpacing(8);

  page_view = new PageView(splitter_);

  player_bar = new components::PlayerBar(this);
  player_bar->setVisible(false);
  player_bar->previousClicked.connect<&MainWindow::onPreviousClicked>(this);
  player_bar->playPauseClicked.connect<&MainWindow::onPlayPauseClicked>(this);
  player_bar->nextClicked.connect<&MainWindow::onNextClicked>(this);
  player_bar->seekRequested.connect<&MainWindow::onSeekRequested>(this);
  player_bar->volumeChanged.connect<&MainWindow::onVolumeChanged>(this);
  bass24::bass24_player.finished.connect<&MainWindow::onNextClicked>(this);

  setupSidebar();
  setupPages();
  page_view->showPage("home");
  menu_buttons["home"]->setActive(true);
  markLayoutDirty();
}

void MainWindow::layoutChildren() {
  const float w = contentWidth();
  const float h = contentHeight();
  const float bar_h = player_bar->showing() ? kPlayerBarHeight : 0.0f;
  const float main_h = h - bar_h;

  splitter_->setGeometry(0, 0, w, main_h);
  splitter_->updateLayout();

  if (player_bar->showing()) {
    player_bar->setGeometry(0, main_h, w, bar_h);
    player_bar->updateLayout();
  }
}

void MainWindow::setupSidebar() {
  profile_card_ = new components::UserProfileCard(true, sidebar_);

  struct MenuItem {
    std::string id;
    std::string label;
    std::string_view icon;
  };

  const std::vector<MenuItem> items = {
    { "home", "首页", home_svg },
    { "browse", "浏览", browse_svg },
    { "search", "搜索", search_svg },
    { "library", "音乐库", library_svg },
    { "favorites", "我喜欢", favorites_svg },
    { "recent", "最近播放", recent_svg },
    { "settings", "设置", settings_svg },
  };

  for (const auto &[id, label, icon] : items) {
    auto *btn = new MenuButton(label, icon, sidebar_);
    btn->setId(id);
    btn->clicked.connect<&MainWindow::onMenuClicked>(this);
    btn->setMaxHeight(50);
    menu_buttons[id] = btn;
  }
}

void MainWindow::setupPages() {
  if (page_view == nullptr) {
    return;
  }

  page_view->addPage("home", new pages::HomePage(page_view));
  page_view->addPage("browse", new pages::BrowsePage(page_view));
  page_view->addPage("search", new pages::SearchPage(page_view));

  page_view->addPage("library", new pages::LibraryPage(page_view));

  favorites_page = new pages::FavoritesPage(page_view);
  favorites_page->songSelected.connect<&MainWindow::onSongSelected>(this);
  favorites_page->playbackStateChanged.connect<&MainWindow::onPlaybackStateChanged>(this);
  favorites_page->loadingStateChanged.connect<&MainWindow::onLoadingStateChanged>(this);
  page_view->addPage("favorites", favorites_page);
  page_view->addPage("recent", new pages::RecentPage(page_view));
  page_view->addPage("settings", new pages::SettingsPage(page_view));
}

void MainWindow::onSongSelected(const SongInfo &info) const {
  player_bar->updateSong(info);
  player_bar->show();
}

void MainWindow::onPlaybackStateChanged(const bool playing) const {
  player_bar->setPlaying(playing);
}

void MainWindow::onLoadingStateChanged(const bool loading) const {
  player_bar->setLoading(loading);
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void MainWindow::onSeekRequested(const double ratio) {
  void(bass24::bass24_player.seekRatio(ratio));
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void MainWindow::onVolumeChanged(const float volume) {
  bass24::bass24_player.setVolume(volume);
}

void MainWindow::onPreviousClicked() const {
  if (favorites_page != nullptr) {
    favorites_page->playPrevious();
  }
}

void MainWindow::onPlayPauseClicked() const {
  if (bass24::bass24_player.togglePause()) {
    player_bar->setPlaying(bass24::bass24_player.playing());
  }
}

void MainWindow::onNextClicked() const {
  if (favorites_page != nullptr) {
    favorites_page->playNext();
  }
}

void MainWindow::syncPlaybackState() {
  if (player_bar == nullptr || !player_bar->showing()) {
    return;
  }

  const auto now = profiling::frame_clock.now;
  if (last_playback_sync_us != 0 && now - last_playback_sync_us < kPlaybackSyncIntervalUs) {
    return;
  }
  last_playback_sync_us = now;

  const auto [has_stream, playing, position_seconds, duration_seconds, volume] =
    bass24::bass24_player.state();
  player_bar->setVolume(volume);
  player_bar->setPlaying(playing);
  if (has_stream) {
    player_bar->setPlaybackPosition(position_seconds, duration_seconds);
  }
}

void MainWindow::onMenuClicked(const std::string &id) {
  page_view->showPage(id);

  for (auto &[mid, btn] : menu_buttons) {
    btn->setActive(mid == id);
  }
}

void MainWindow::render(SkCanvas *canvas) {
  syncPlaybackState();

  canvas->clear(kWindowBg);

  const float w = contentWidth();
  const float h = contentHeight();

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
