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

using namespace skia;
using namespace glfw;
using namespace ui::widgets;
using namespace ui::layout;

export class MainWindow : public Window {
  using Window::Window;

public:
  MainWindow();
  void render(SkCanvas *canvas) final;

private:
  void setupSidebar();
  void setupPages() const;
  void onMenuClicked(const std::string &id);

  Splitter *splitter_ = nullptr;
  Box *sidebar_ = nullptr;
  PageView *page_view = nullptr;

  std::unordered_map<std::string, MenuButton *> menu_buttons;

  static constexpr auto home_svg = "resources/svg/home.svg";
};

MainWindow::MainWindow() : Window(1024, 700) {
  splitter_ = new Splitter(this);

  sidebar_ = new Box(splitter_);
  sidebar_->setPadding(16);
  sidebar_->setLayout<VBoxLayout<Widget>>();

  page_view = new PageView(splitter_);

  setLayout<VBoxLayout<Widget>>();
  setupSidebar();
  setupPages();
  page_view->showPage("home");
  menu_buttons["home"]->setActive(true);
}

void MainWindow::setupSidebar() {
  struct MenuItem {
    std::string id;
    std::string label;
  };

  const std::vector<MenuItem> items = {
    { "home", "首页" },      { "browse", "浏览" },      { "search", "搜索" },
    { "library", "音乐库" }, { "favorites", "我喜欢" }, { "recent", "最近播放" },
    { "settings", "设置" },
  };

  for (const auto &[id, label] : items) {
    auto *btn = new MenuButton(label, home_svg, sidebar_);
    btn->setId(id);
    btn->clicked.connect<&MainWindow::onMenuClicked>(this);
    btn->setMaxHeight(50);
    menu_buttons[id] = btn;
  }
}

void MainWindow::setupPages() const {
  page_view->addPage("home", new pages::HomePage(page_view));
  page_view->addPage("browse", new pages::BrowsePage(page_view));
  page_view->addPage("search", new pages::SearchPage(page_view));
  page_view->addPage("library", new pages::LibraryPage(page_view));
  page_view->addPage("favorites", new pages::FavoritesPage(page_view));
  page_view->addPage("recent", new pages::RecentPage(page_view));
  page_view->addPage("settings", new pages::SettingsPage(page_view));
}

void MainWindow::onMenuClicked(const std::string &id) {
  page_view->showPage(id);

  for (auto &[mid, btn] : menu_buttons) {
    btn->setActive(mid == id);
  }
}

void MainWindow::render(SkCanvas *canvas) {
  canvas->clear(skia_colors::white);
  Widget::render(canvas);
}
