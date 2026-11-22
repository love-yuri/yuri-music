//
// Created by yuri on 2026/2/7.
//
export module pages:library;

import std;
import yuri.ui;
import yuri.skia;
import webview2;
import components;
import store;
import yuri.core;

using namespace ui::layout;
using namespace ui::widgets;
using namespace ui::render;
using namespace skia;

/** 在后台线程打开 QQ 音乐登录窗口，并在关闭后刷新资料。 */
void open_qq_music_login();

export namespace pages {

class LibraryPage : public Border {
public:
  /**
   * 创建音乐库页面。
   * @param parent 父控件。
   */
  explicit LibraryPage(Widget *parent = nullptr);

  /** 断开用户资料的状态信号。 */
  ~LibraryPage() override;

private:
  /** 更新登录按钮文本。 */
  void update_login_button_text() const;

  Button *login_button_ {}; // QQ 音乐登录按钮
};

LibraryPage::LibraryPage(Widget *const parent) : Border(parent) {
  set_background(ColorFromARGB(104, 255, 255, 255));
  set_padding(Insets(28.0f, 0.0f, 28.0f, 8.0f));
  auto *const content = new Widget(this);
  content->set_layout(new VBoxLayout(VerticalAlignment::Center));

  auto *const title = new Text("音乐库", content);
  title->set_alignment(Alignment::Center);
  title->set_color(skia_colors::black);
  title->set_font_size(24.0f);
  title->set_preferred_height(38.0f);

  auto *const subtitle = new Text("QQ 音乐个人资料", content);
  subtitle->set_alignment(Alignment::Center);
  subtitle->set_color(ColorFromARGB(166, 58, 70, 88));
  subtitle->set_font_size(13.0f);
  subtitle->set_preferred_height(22.0f);

  auto *const profile_card = new components::UserProfileCard(false, content);
  profile_card->set_horizontal_alignment(HorizontalAlignment::Center);
  profile_card->set_preferred_size(520.0f, 214.0f);
  profile_card->set_margin(Insets(8.0f, 20.0f, 8.0f, 22.0f));

  login_button_ = new Button("登录 QQ 音乐", content);
  login_button_->set_horizontal_alignment(HorizontalAlignment::Center);
  login_button_->set_preferred_size(180.0f, 44.0f);
  const ButtonStyle login_style {
    .background = 0xff1fb961,
    .foreground = skia_colors::white,
    .border_width = 0.0f,
    .radius = 8.0f,
  };
  ButtonStyles styles { login_style, login_style, login_style, login_style };
  styles.hovered.background = 0xff19a855;
  styles.pressed.background = 0xff168d49;
  styles.disabled.background = 0xffa6c9b4;
  login_button_->set_style(styles);
  // Signal 不持有回调对象，保留静态函数指针供按钮使用。
  static auto login_clicked = &open_qq_music_login;
  login_button_->clicked.connect(login_clicked);
  store::user_profile_store.status_changed.connect<&LibraryPage::update_login_button_text>(this);
  update_login_button_text();
}

LibraryPage::~LibraryPage() {
  store::user_profile_store.status_changed.disconnect(this);
}

void LibraryPage::update_login_button_text() const {
  const auto &profile = store::user_profile_store;
  login_button_->set_text(profile.loggedIn() ? "刷新个人信息" : "登录 QQ 音乐");
}

} // namespace pages

void open_qq_music_login() {
  thread_manager->addTask([] {
    webview2::launch_qq_music_login();
    store::user_profile_store.reload();
  });
}
