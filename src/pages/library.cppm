//
// Created by yuri on 2026/2/7.
//
export module pages:library;

import std;
import ui;
import skia;
import webview2;

using namespace ui::layout;
using namespace ui::widgets;
using namespace ui::render;
using namespace skia;

export namespace pages {

class LibraryPage : public Widget {

public:
  /**
   * 创建音乐库页面。
   */
  explicit LibraryPage(Widget *parent = nullptr);

  /**
   * 更新标题和登录按钮布局。
   */
  void layoutChildren() override;

  /**
   * 绘制页面背景和标题。
   */
  void paint(SkCanvas *canvas) override;

private:
  RenderText render_text{"音乐库"}; // 字体节点
  Button *login_button{};           // QQ 音乐登录按钮

  /**
   * 打开 QQ 音乐登录窗口。
   */
  void openQqMusicLogin();
};

LibraryPage::LibraryPage(Widget *parent) : Widget(parent), login_button(new Button("QQ音乐登录", this)) {
  render_text.setAlignment(Alignment::Center);
  render_text.setColor(skia_colors::black);
  render_text.setFontSize(24);

  login_button->resize(180.0f, 44.0f);
  login_button->radius = 8.0f;
  login_button->background().setColor(ColorFromARGB(255, 31, 185, 97));
  login_button->border().setWidth(0.0f);
  login_button->text().setColor(skia_colors::white);
  login_button->clicked.connect<&LibraryPage::openQqMusicLogin>(this);
}

void LibraryPage::layoutChildren() {
  const float button_w = std::min(180.0f, std::max(120.0f, width_ - 56.0f));
  constexpr float button_h = 44.0f;
  const float center_y = height_ * 0.5f;

  render_text.update(SkRect::MakeXYWH(0.0f, center_y - 64.0f, width_, 40.0f));
  login_button->setGeometry((width_ - button_w) * 0.5f, center_y + 4.0f, button_w, button_h);

  Widget::layoutChildren();
}

void LibraryPage::paint(SkCanvas *canvas) {
  SkPaint panel;
  panel.setAntiAlias(true);
  panel.setColor(ColorFromARGB(104, 255, 255, 255));
  canvas->drawRect(borderRect(), panel);
  render_text.render(canvas);
}

void LibraryPage::openQqMusicLogin() {
  webview2::launchQqMusicLogin();
}

} // namespace pages
