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
import bass24;

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
  void playPrevious();
  void playNext();

private:
  // 检查是否需要加载更多
  void checkLoadMore(float scrollOffset);
  // 加载更多歌曲
  void loadMore();
  // 歌曲双击处理
  void onSongDoubleClicked(SongItem *item);
  // 下载歌曲文件
  void downloadSong(const SongItem *item);
  // 格式化歌曲时长（秒 -> m:ss）
  static std::string formatDuration(int seconds);
  // 拼接歌手名
  static std::string formatSingers(const std::vector<SingerType> &singers);
  // 当前选中的歌曲是否仍然是指定 mid
  bool isSelectedMid(std::string_view mid);

  SongItem *selected_item{};                                               // 被选中的item
  ScrollArea *items_{};                                                    // 歌曲列表
  std::vector<SongItem *> song_items{};                                    // 按列表顺序保存歌曲项
  std::uint64_t tid_{};                                                    // 当前歌单 ID
  int offset_{ 0 };                                                        // 当前加载偏移
  bool loading_{ false };                                                  // 是否正在加载
  bool has_more{ true };                                                   // 是否还有更多数据
  std::unordered_map<const SongItem *, qqmusic_api::song::SongDownloadInfo>
    song_downloads{};                                 // 歌曲下载信息
  std::unordered_set<std::string> downloading_mids{}; // 正在下载的歌曲 mid
  std::mutex download_mutex{};                        // 下载状态锁
  std::string selected_mid{};                         // 当前选中的歌曲 mid
  std::mutex selected_mutex{};                        // 当前选中状态锁

public:
  Signal<std::string, std::string, std::string> songSelected{}; // 歌曲选中信号
  Signal<bool> playbackStateChanged{};                          // 播放状态变化信号
  Signal<bool> loadingStateChanged{};                           // 当前歌曲加载状态变化信号
};

LibraryPage::LibraryPage(Widget *parent) : Widget(parent) {
  setLayout<VBoxLayout<Widget>>();
  layout()->setSpacing(8);
  setPadding(Insets(28, 24, 34, 24));

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
          SongInfo info{ std::string(music.title), formatSingers(music.singer),
                         formatDuration(music.interval) };
          const auto title = info.title;
          const auto artist = info.artist;
          auto *item = new SongItem(index++, std::move(info), false, items_);
          song_items.push_back(item);
          // clang-format off
          song_downloads.emplace(item,
            qqmusic_api::song::SongDownloadInfo{
               std::string(music.mid),
               title,
               artist,
               music.file.size_flac > 0,
               music.file.size_ape > 0,
               music.file.size_320mp3 > 0,
               music.file.size_128mp3 > 0,
            }
          );
          // clang-format on
          item->doubleClicked.connect<&LibraryPage::onSongDoubleClicked>(this);
        }
        offset_ += res.songlist.size();
        has_more = !res.songlist.empty();
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

void LibraryPage::playPrevious() {
  if (song_items.empty()) return;

  const auto it = std::ranges::find(song_items, selected_item);
  if (it == song_items.end() || it == song_items.begin()) {
    return;
  }

  onSongDoubleClicked(*std::prev(it));
}

void LibraryPage::playNext() {
  if (song_items.empty()) return;

  const auto it = std::ranges::find(song_items, selected_item);
  if (it == song_items.end()) {
    onSongDoubleClicked(song_items.front());
    return;
  }

  const auto next = std::next(it);
  if (next == song_items.end()) {
    if (has_more && !loading_) {
      loadMore();
    }
    return;
  }

  onSongDoubleClicked(*next);
}

// 格式化歌曲时长（秒 -> m:ss）
std::string LibraryPage::formatDuration(const int seconds) {
  char buf[8];
  auto [ptr, ec] = std::format_to_n(buf, sizeof(buf) - 1, "{}:{:02}", seconds / 60, seconds % 60);
  *ptr = '\0';
  return { buf, static_cast<std::size_t>(ptr - buf) };
}

// 拼接歌手名
std::string LibraryPage::formatSingers(const std::vector<SingerType> &singers) {
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

bool LibraryPage::isSelectedMid(const std::string_view mid) {
  std::lock_guard lock(selected_mutex);
  return selected_mid == mid;
}

// 歌曲双击处理：查找选中项并发射信号
void LibraryPage::onSongDoubleClicked(SongItem *item) {
  item->setSelected(true);
  if (selected_item != nullptr && selected_item != item) {
    selected_item->setSelected(false);
  }
  selected_item = item;

  if (const auto info_it = song_downloads.find(item); info_it != song_downloads.end()) {
    std::lock_guard lock(selected_mutex);
    selected_mid = info_it->second.mid;
  }

  const auto &[title, artist, duration] = selected_item->info();
  songSelected.emit(title, artist, duration);
  downloadSong(item);
}

void LibraryPage::downloadSong(const SongItem *item) {
  const auto info_it = song_downloads.find(item);
  if (info_it == song_downloads.end() || info_it->second.mid.empty()) {
    yuri::warn("歌曲下载信息缺失，无法下载");
    return;
  }

  const auto song = info_it->second;
  if (const auto path = qqmusic_api::song::cached_song_path(song); !path.empty()) {
    yuri::info("使用本地音乐缓存播放: {}", path.string());
    loadingStateChanged.emit(false);
    if (bass24::player().play(path)) {
      playbackStateChanged.emit(true);
    }
    return;
  }

  bass24::player().stop();
  playbackStateChanged.emit(false);

  {
    std::lock_guard lock(download_mutex);
    if (downloading_mids.contains(song.mid)) {
      loadingStateChanged.emit(true);
      return;
    }
    downloading_mids.insert(song.mid);
  }

  loadingStateChanged.emit(true);
  thread_manager->addTask([this, song] {
    const auto cleanup = [this, mid = song.mid] {
      {
        std::lock_guard lock(download_mutex);
        downloading_mids.erase(mid);
      }
      if (isSelectedMid(mid)) {
        loadingStateChanged.emit(false);
      }
    };

    try {
      const auto path = qqmusic_api::song::download_song_file(song);
      if (path.empty()) {
        cleanup();
        return;
      }

      bool should_play = false;
      {
        std::lock_guard lock(selected_mutex);
        should_play = selected_mid == song.mid;
      }
      if (should_play) {
        if (bass24::player().play(path)) {
          playbackStateChanged.emit(true);
          yuri::info("播放: {}", path.string());
        }
      }
    } catch (const std::exception &e) {
      yuri::error("下载歌曲异常: {}", e.what());
    }

    cleanup();
  });
}

// 检查是否需要加载更多歌曲
void LibraryPage::checkLoadMore(const float scrollOffset) {
  if (loading_ || !has_more) return;

  const auto n = static_cast<int>(items_->children().size());
  if (n == 0) return;

  constexpr float kSongItemHeight = 52.0f;
  const int bottom_index =
    static_cast<int>((scrollOffset + items_->contentHeight()) / kSongItemHeight);

  if (n - bottom_index <= 5) {
    loadMore();
  }
}

// 加载下一页歌曲
void LibraryPage::loadMore() {
  loading_ = true;
  const int current_offset = offset_;
  thread_manager->addTask([this, current_offset] {
    const auto res =
      qqmusic_api::playlist::get_user_playlists_detail(tid_, current_offset, 30).req_1.data;
    if (res.songlist.empty()) {
      has_more = false;
      loading_ = false;
      return;
    }

    int index = current_offset;
    for (auto &music : res.songlist) {
      SongInfo info{ std::string(music.title), formatSingers(music.singer),
                     formatDuration(music.interval) };
      const auto title = info.title;
      const auto artist = info.artist;
      auto *item = new SongItem(index++, std::move(info), false, items_);
      song_items.push_back(item);
      song_downloads.emplace(item, qqmusic_api::song::SongDownloadInfo{
                                     std::string(music.mid),
                                     title,
                                     artist,
                                     music.file.size_flac > 0,
                                     music.file.size_ape > 0,
                                     music.file.size_320mp3 > 0,
                                     music.file.size_128mp3 > 0,
                                   });
      item->doubleClicked.connect<&LibraryPage::onSongDoubleClicked>(this);
    }

    offset_ = current_offset + res.songlist.size();
    loading_ = false;
    markLayoutDirty();
  });
}

} // namespace pages
