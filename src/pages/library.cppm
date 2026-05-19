//
// Created by yuri on 2026/2/7.
//
export module pages:library;

import std;
import ui;
import skia;
import components;
import thread_pool;
import qq_music_api;
import core;

using namespace ui::layout;
using namespace ui::widgets;
using namespace ui::render;
using namespace skia;
using namespace components;

export namespace pages {

class LibraryPage : public Widget {

public:
  // 创建音乐库页面
  explicit LibraryPage(Widget *parent = nullptr);
  // 绘制音乐库背景
  void paint(SkCanvas *canvas) override;

private:
  // 检查是否需要加载更多
  void checkLoadMore(float scrollOffset);
  // 加载更多歌曲
  void loadMore();
  // 歌曲双击处理
  void onSongDoubleClicked(SongItem *item);
  // 格式化歌曲时长（秒 -> m:ss）
  static std::string formatDuration(int seconds);
  // 拼接歌手名
  static std::string formatSingers(const std::vector<SingerType>& singers);

  SongItem *selected_item{}; // 被选中的item
  ScrollArea *items_{};      // 歌曲列表
  std::uint64_t tid_{};      // 当前歌单 ID
  int offset_{0};            // 当前加载偏移
  bool loading_{false};      // 是否正在加载
  bool has_more_{true};      // 是否还有更多数据

public:
  Signal<std::string, std::string, std::string> songSelected{}; // 歌曲选中信号
};

LibraryPage::LibraryPage(Widget *parent) : Widget(parent) {
  setLayout<VBoxLayout<Widget>>();
  layout()->setSpacing(8);
  setPadding(Insets(28, 24, 28, 24));

  const auto title = new Text("音乐库", this);
  title->setFontSize(30);
  title->setColor(ColorFromARGB(255, 20, 26, 36));
  title->setAlignment(Alignment::CenterLeft);
  title->setMaxHeight(42.f);

  const auto subtitle = new Text("双击歌曲开始播放，列表会在滚动到底部时继续加载", this);
  subtitle->setFontSize(12.5f);
  subtitle->setColor(ColorFromARGB(170, 56, 68, 86));
  subtitle->setAlignment(Alignment::CenterLeft);
  subtitle->setMaxHeight(24.0f);

  items_ = new ScrollArea(this);

  // 滚动到底部附近时加载更多
  items_->scrollChanged.connect<&LibraryPage::checkLoadMore>(this);

  // 初始加载歌单
  thread_manager->addTask([this] {
    const auto list = qqmusic_api::playlist::get_user_playlists();
    for (auto &value : list.data.disslist) {
      if (value.diss_name == "我喜欢") {
        tid_ = value.tid;
        const auto res = qqmusic_api::playlist::get_user_playlists_detail(tid_, 0, 30).req_1.data;
        int index = 0;
        for (auto &music : res.songlist) {
          SongInfo info{std::string(music.title), formatSingers(music.singer), formatDuration(music.interval)};
          auto *item = new SongItem(index++, std::move(info), false, items_);
          item->doubleClicked.connect<&LibraryPage::onSongDoubleClicked>(this);
        }
        offset_ += res.songlist.size();
        has_more_ = !res.songlist.empty();
        markLayoutDirty();
        break;
      }
    }
  });
}

void LibraryPage::paint(SkCanvas *canvas) {
  SkPaint fill;
  fill.setAntiAlias(true);
  fill.setColor(ColorFromARGB(110, 255, 255, 255));
  canvas->drawRect(borderRect(), fill);

  SkPaint topLight;
  topLight.setAntiAlias(true);
  topLight.setColor(ColorFromARGB(34, 255, 255, 255));
  canvas->drawRect(SkRect::MakeXYWH(0.0f, 0.0f, width_, 92.0f), topLight);
}

// 格式化歌曲时长（秒 -> m:ss）
std::string LibraryPage::formatDuration(const int seconds) {
  char buf[8];
  auto [ptr, ec] = std::format_to_n(buf, sizeof(buf) - 1, "{}:{:02}", seconds / 60, seconds % 60);
  *ptr = '\0';
  return {buf, static_cast<std::size_t>(ptr - buf)};
}

// 拼接歌手名
std::string LibraryPage::formatSingers(const std::vector<SingerType>& singers) {
  if (singers.empty()) return {};
  if (singers.size() == 1) return singers[0].name;

  std::size_t len = 3 * (singers.size() - 1); // " / "
  for (auto &s : singers) len += s.name.size();

  std::string result;
  result.reserve(len);
  result.append(singers[0].name);
  for (std::size_t i = 1; i < singers.size(); ++i) {
    result.append(" / ");
    result.append(singers[i].name);
  }
  return result;
}

// 歌曲双击处理：查找选中项并发射信号
void LibraryPage::onSongDoubleClicked(SongItem *item) {
  item->setSelected(true);
  if (selected_item != nullptr) {
    selected_item->setSelected(false);
  }
  selected_item = item;
  const auto &[title, artist, duration] = selected_item->info();
  songSelected.emit(title, artist, duration);
}

// 检查是否需要加载更多歌曲
void LibraryPage::checkLoadMore(const float scrollOffset) {
  if (loading_ || !has_more_) return;

  const auto n = static_cast<int>(items_->children().size());
  if (n == 0) return;

  constexpr float kSongItemHeight = 52.0f;
  const int bottom_index = static_cast<int>((scrollOffset + items_->contentHeight()) / kSongItemHeight);

  if (n - bottom_index <= 5) {
    loadMore();
  }
}

// 加载下一页歌曲
void LibraryPage::loadMore() {
  loading_ = true;
  const int current_offset = offset_;
  thread_manager->addTask([this, current_offset] {
    const auto res = qqmusic_api::playlist::get_user_playlists_detail(tid_, current_offset, 30).req_1.data;
    if (res.songlist.empty()) {
      has_more_ = false;
      loading_ = false;
      return;
    }

    int index = current_offset;
    for (auto &music : res.songlist) {
      SongInfo info{std::string(music.title), formatSingers(music.singer), formatDuration(music.interval)};
      auto *item = new SongItem(index++, std::move(info), false, items_);
      item->doubleClicked.connect<&LibraryPage::onSongDoubleClicked>(this);
    }

    offset_ = current_offset + res.songlist.size();
    loading_ = false;
    markLayoutDirty();
  });
}

} // namespace pages
