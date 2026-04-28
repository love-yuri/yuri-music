//
// Created by yuri on 2026/2/7.
//
export module pages:home;

import std;
import ui;
import core;
import skia;
import components;
import thread_pool;
import qq_music_api;

using namespace ui::layout;
using namespace ui::widgets;
using namespace ui::render;
using namespace skia;
using namespace components;

// 副标题颜色
constexpr SkColor kSubtitleColor = ColorFromARGB(255, 140, 140, 140);

// 图标颜色
constexpr SkColor kIconBlue = ColorFromARGB(255, 59, 130, 246);
constexpr SkColor kIconPurple = ColorFromARGB(255, 139, 92, 246);
constexpr SkColor kIconTeal = ColorFromARGB(255, 6, 182, 212);
constexpr SkColor kIconOrange = ColorFromARGB(255, 249, 115, 22);

// 布局尺寸
static constexpr float kHeaderHeight = 70.0f;
static constexpr float kCardRowHeight = 82.0f;
static constexpr float kSectionHeight = 24.0f;
static constexpr float kSongItemHeight = 64.0f;

export namespace pages {

// 首页
class HomePage : public Widget {
public:
  explicit HomePage(Widget *parent = nullptr);

private:
  Text* greeting_text{}; // 标题
  Text* subtitle_text{}; // 子标题
};

HomePage::HomePage(Widget *parent) :
  Widget(parent), greeting_text(new Text("下午好", this)),
  subtitle_text(new Text("发现适合现在聆听的音乐", this)) {

  setPadding(Insets(16));

  // 使用垂直布局排列子控件
  setLayout<VBoxLayout<Widget>>();
  layout()->setSpacing(6);

  // 标题
  greeting_text->setFontSize(28);
  greeting_text->setColor(skia_colors::light_pink);
  greeting_text->setAlignment(Alignment::CenterLeft);
  greeting_text->setMaxHeight(40.f);

  // 副标题
  subtitle_text->setFontSize(13);
  subtitle_text->setColor(kSubtitleColor);
  subtitle_text->setAlignment(Alignment::CenterLeft);
  subtitle_text->setMaxHeight(40.f);

  auto card = new QuickCard("每日推荐", "为你精选的 30 首歌曲", kIconBlue, this);
  card->clicked.connect([] {
    yuri::info("hhhh");
  });

  card = new QuickCard("每日推荐", "为你精选的 30 首歌曲", kIconBlue, this);
  card->clicked.connect([] {
    yuri::info("hhhh");
  });

  card = new QuickCard("每日推荐", "为你精选的 30 首歌曲", kIconBlue, this);
  card->clicked.connect([] {
    yuri::info("hhhh");
  });

  const auto items = new Widget(this);
  items->setLayout<VBoxLayout<Widget>>();

  thread_manager->addTask([items, this] {
    auto list = qqmusic_api::playlist::get_user_playlists();
    for (auto &value : list.data.disslist) {
      yuri::info("---------- name: {} id: {} ------------", value.diss_name, value.tid);
      auto res = qqmusic_api::playlist::get_user_playlists_detail(value.tid, 0, 5).req_1.data;
      for (auto &music : res.songlist) {
        yuri::info("name: {}", music.name);
      }

      if (value.diss_name == "我喜欢") {
        int index = 0;
        for (auto &music : res.songlist) {
          new SongItem(index++, music.title, music.label, "5:21", false, items);
        }

        markLayoutDirty();
      }
    }
  });
}

} // namespace pages
