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
  PageView *page_view_ = nullptr;

  std::unordered_map<std::string, MenuButton *> menu_buttons_;

  static constexpr auto home_svg =
    R"(E:\love-yuri\pixel-journey\test\yuri-music\resources\svg\home.svg)";
};

MainWindow::MainWindow() : Window(1024, 700) {
  splitter_ = new Splitter(this);

  sidebar_ = new Box(splitter_);
  sidebar_->setPadding(16);
  sidebar_->setLayout<VBoxLayout<Widget>>();

  page_view_ = new PageView(splitter_);

  setLayout<VBoxLayout<Widget>>();
  setupSidebar();
  setupPages();
  page_view_->showPage("home");
  menu_buttons_["home"]->setActive(true);
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
    menu_buttons_[id] = btn;
  }
}

void MainWindow::setupPages() const {
  page_view_->addPage("home", new pages::HomePage(page_view_));
  page_view_->addPage("browse", new pages::BrowsePage(page_view_));
  page_view_->addPage("search", new pages::SearchPage(page_view_));
  page_view_->addPage("library", new pages::LibraryPage(page_view_));
  page_view_->addPage("favorites", new pages::FavoritesPage(page_view_));
  page_view_->addPage("recent", new pages::RecentPage(page_view_));
  page_view_->addPage("settings", new pages::SettingsPage(page_view_));
}

void MainWindow::onMenuClicked(const std::string &id) {
  page_view_->showPage(id);

  for (auto &[mid, btn] : menu_buttons_) {
    btn->setActive(mid == id);
  }
}

void MainWindow::render(SkCanvas *canvas) {
  canvas->clear(skia_colors::white);
  Widget::render(canvas);
}
