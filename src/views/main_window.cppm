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
import player;

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

/**
 * 绘制带边界的柔光椭圆。
 *
 * @param canvas 目标画布。
 * @param rect 椭圆边界。
 * @param color 填充颜色。
 * @param sigma 模糊半径。
 */
void draw_blurred_oval(SkCanvas *canvas, const SkRect &rect, SkColor color, float sigma);

export class MainWindow : public Window {
public:
  /** 创建主窗口和页面结构。 */
  MainWindow();

  /** 窗口销毁时停止播放。 */
  ~MainWindow() override;

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

  /** 更新主内容区域的位置。 */
  void layout_children() override;

protected:
  /**
   * 将窗口级点击交给播放栏处理浮层关闭。
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

  Widget *main_area_ = nullptr;                 // 主内容与播放栏的布局容器
  NavigationView *navigation_ = nullptr;        // 侧栏和页面容器
  components::PlayerBar *player_bar_ = nullptr; // 底部播放栏
#ifdef __linux__
  bool hidden_to_tray_ = false; // 主窗口是否隐藏到托盘
  bool quit_requested_ = false; // 是否从托盘请求退出
#endif

  std::unordered_map<std::string_view, components::MenuButton *> menu_buttons_; // 导航按钮集合

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

  player_bar_ = new components::PlayerBar(main_area_);

  setup_navigation();
  navigation_->pages().show_page("home");

  // 注入组件
  injector.addSingleton(player_bar_);
}

MainWindow::~MainWindow() {
  player::bass24_player().stop();
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
}

void MainWindow::on_mouse_left_released(const float mouse_x, const float mouse_y) {
  player_bar_->dismiss_volume_popup_at(mouse_x, mouse_y);
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
  player_bar_->previous_clicked.connect<&pages::FavoritesPage::play_previous>(favorites_page);
  player_bar_->next_clicked.connect<&pages::FavoritesPage::play_next>(favorites_page);
  player_bar_->playback_mode_changed.connect<&pages::FavoritesPage::set_random>(favorites_page);
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

void MainWindow::on_page_changed(const std::string_view id) const {
  for (const auto &[menu_id, button] : menu_buttons_) {
    button->setActive(menu_id == id);
  }
}

void MainWindow::render(SkCanvas *const canvas) {
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
