//
// Created by yuri on 2026/2/7.
//
export module pages:library;

import std;
import ui;
import skia;
import webview2;
import components;
import models;

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
   * 更新标题、个人资料卡和登录按钮布局。
   */
  void layoutChildren() override;

  /**
   * 绘制页面背景和标题。
   */
  void paint(SkCanvas *canvas) override;

private:
  RenderText render_text{"音乐库"};              // 字体节点
  RenderText subtitle_text{"QQ 音乐个人资料"}; // 副标题
  components::UserProfileCard *profile_card{};  // 个人资料卡
  Button *login_button{};                        // QQ 音乐登录按钮

  /**
   * 打开 QQ 音乐登录窗口。
   */
  void openQqMusicLogin();
  void updateLoginButtonText();
};

LibraryPage::LibraryPage(Widget *parent)
    : Widget(parent),
      profile_card(new components::UserProfileCard(false, this)),
      login_button(new Button("登录 QQ 音乐", this)) {
  render_text.setAlignment(Alignment::Center);
  render_text.setColor(skia_colors::black);
  render_text.setFontSize(24);

  subtitle_text.setAlignment(Alignment::Center);
  subtitle_text.setColor(ColorFromARGB(166, 58, 70, 88));
  subtitle_text.setFontSize(13);

  login_button->resize(180.0f, 44.0f);
  login_button->radius = 8.0f;
  login_button->background().setColor(ColorFromARGB(255, 31, 185, 97));
  login_button->border().setWidth(0.0f);
  login_button->text().setColor(skia_colors::white);
  login_button->clicked.connect<&LibraryPage::openQqMusicLogin>(this);
}

void LibraryPage::layoutChildren() {
  updateLoginButtonText();

  const float card_w = std::min(520.0f, std::max(280.0f, width_ - 72.0f));
  constexpr float card_h = 214.0f;
  const float button_w = std::min(180.0f, std::max(120.0f, width_ - 56.0f));
  constexpr float button_h = 44.0f;
  const float center_y = height_ * 0.5f - 14.0f;

  render_text.update(SkRect::MakeXYWH(0.0f, center_y - 170.0f, width_, 40.0f));
  subtitle_text.update(SkRect::MakeXYWH(0.0f, center_y - 132.0f, width_, 22.0f));
  profile_card->setGeometry((width_ - card_w) * 0.5f, center_y - 90.0f, card_w, card_h);
  login_button->setGeometry((width_ - button_w) * 0.5f, center_y + 146.0f, button_w, button_h);

  Widget::layoutChildren();
}

void LibraryPage::paint(SkCanvas *canvas) {
  SkPaint panel;
  panel.setAntiAlias(true);
  panel.setColor(ColorFromARGB(104, 255, 255, 255));
  canvas->drawRect(borderRect(), panel);
  render_text.render(canvas);
  subtitle_text.render(canvas);
}

void LibraryPage::openQqMusicLogin() {
  const auto profile = models::userProfileStore().snapshot();
  if (!profile.logged_in) {
    webview2::launchQqMusicLogin();
  }
  models::userProfileStore().refreshAsync(true);
}

void LibraryPage::updateLoginButtonText() {
  const auto profile = models::userProfileStore().snapshot();
  login_button->text().setText(profile.logged_in ? "刷新个人信息" : "登录 QQ 音乐");
}

} // namespace pages
