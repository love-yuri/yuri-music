//
// Created by yuri on 2026/2/1.
//

export module main_window;

import components;
import pages;
import core;
import yuri_log;
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
  void setupPages();
  void showPage(const std::string &id);
  void onMenuClicked(const std::string &id);

  Splitter *splitter_ = nullptr;
  Box *sidebar_ = nullptr;
  Box *content_ = nullptr;

  std::unordered_map<std::string, Widget *> pages_;
  std::unordered_map<std::string, MenuButton *> menu_buttons_;
  std::string current_page_ = "home";

  static constexpr auto home_svg =
    R"(E:\love-yuri\pixel-journey\test\yuri-music\resources\svg\home.svg)";
};

MainWindow::MainWindow() : Window(1024, 700) {
  splitter_ = new Splitter(this);

  sidebar_ = new Box(splitter_);
  sidebar_->setPadding(16);
  sidebar_->setLayout<VBoxLayout<Widget>>();

  content_ = new Box(splitter_);
  content_->setLayout<VBoxLayout<Widget>>();

  setLayout<VBoxLayout<Widget>>();
  setupSidebar();
  setupPages();
  showPage("home");
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

void MainWindow::setupPages() {
  pages_["home"] = new pages::HomePage(content_);
  pages_["browse"] = new pages::BrowsePage(content_);
  pages_["search"] = new pages::SearchPage(content_);
  pages_["library"] = new pages::LibraryPage(content_);
  pages_["favorites"] = new pages::FavoritesPage(content_);
  pages_["recent"] = new pages::RecentPage(content_);
  pages_["settings"] = new pages::SettingsPage(content_);

  for (const auto &page : pages_ | std::views::values) {
    page->setVisible(false);
  }
}

void MainWindow::showPage(const std::string &id) {
  if (!pages_.contains(id)) return;

  for (auto &[pid, page] : pages_) {
    page->setVisible(pid == id);
  }

  for (auto &[mid, btn] : menu_buttons_) {
    btn->setActive(mid == id);
  }

  current_page_ = id;
}

void MainWindow::onMenuClicked(const std::string &id) {
  showPage(id);
}

void MainWindow::render(SkCanvas *canvas) {
  canvas->clear(skia_colors::white);
  Widget::render(canvas);
}
