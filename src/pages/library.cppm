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
  // 下载歌曲文件
  void downloadSong(const SongItem *item);
  // 格式化歌曲时长（秒 -> m:ss）
  static std::string formatDuration(int seconds);
  // 拼接歌手名
  static std::string formatSingers(const std::vector<SingerType>& singers);
  // 生成可写入文件系统的文件名
  static std::string sanitizeFileName(std::string name);

  struct SongDownloadInfo {
    std::string mid;
    std::string title;
    std::string artist;
    bool has_flac{};
    bool has_ape{};
    bool has_mp3_320{};
    bool has_mp3_128{};
  };
  // 按音质从高到低生成候选格式
  static std::vector<qqmusic_api::song::SongFileFormat> preferredFormats(const SongDownloadInfo &song);

  SongItem *selected_item{}; // 被选中的item
  ScrollArea *items_{};      // 歌曲列表
  std::uint64_t tid_{};      // 当前歌单 ID
  int offset_{0};            // 当前加载偏移
  bool loading_{false};      // 是否正在加载
  bool has_more{true};       // 是否还有更多数据
  std::unordered_map<const SongItem *, SongDownloadInfo> song_downloads{}; // 歌曲下载信息
  std::unordered_set<std::string> downloading_mids{};                      // 正在下载的歌曲 mid
  std::mutex download_mutex{};                                             // 下载状态锁

public:
  Signal<std::string, std::string, std::string> songSelected{}; // 歌曲选中信号
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
          SongInfo info{std::string(music.title), formatSingers(music.singer), formatDuration(music.interval)};
          const auto title = info.title;
          const auto artist = info.artist;
          auto *item = new SongItem(index++, std::move(info), false, items_);
          song_downloads.emplace(item, SongDownloadInfo{
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

std::string LibraryPage::sanitizeFileName(std::string name) {
  constexpr std::string_view invalid = R"(<>:"/\|?*)";
  for (char &ch : name) {
    if (static_cast<unsigned char>(ch) < 32 || invalid.find(ch) != std::string_view::npos) {
      ch = '_';
    }
  }

  while (!name.empty() && (name.back() == '.' || name.back() == ' ')) {
    name.pop_back();
  }

  if (name.empty()) {
    return "unknown";
  }
  return name;
}

std::vector<qqmusic_api::song::SongFileFormat> LibraryPage::preferredFormats(const SongDownloadInfo &song) {
  std::vector<qqmusic_api::song::SongFileFormat> formats;
  formats.reserve(5);

  if (song.has_flac) {
    formats.push_back(qqmusic_api::song::flac_format);
  }
  if (song.has_ape) {
    formats.push_back(qqmusic_api::song::ape_format);
  }
  if (song.has_mp3_320) {
    formats.push_back(qqmusic_api::song::mp3_320_format);
  }
  if (song.has_mp3_128) {
    formats.push_back(qqmusic_api::song::mp3_128_format);
  }
  formats.push_back(qqmusic_api::song::m4a_format);

  return formats;
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
  downloadSong(item);
}

void LibraryPage::downloadSong(const SongItem *item) {
  const auto info_it = song_downloads.find(item);
  if (info_it == song_downloads.end() || info_it->second.mid.empty()) {
    yuri::warn("歌曲下载信息缺失，无法下载");
    return;
  }

  const SongDownloadInfo song = info_it->second;
  {
    std::lock_guard lock(download_mutex);
    if (downloading_mids.contains(song.mid)) {
      return;
    }
    downloading_mids.insert(song.mid);
  }

  thread_manager->addTask([this, song] {
    const auto cleanup = [this, mid = song.mid] {
      std::lock_guard lock(download_mutex);
      downloading_mids.erase(mid);
    };

    try {
      const auto dir = std::filesystem::path("musics");
      std::error_code ec;
      std::filesystem::create_directories(dir, ec);
      if (ec) {
        yuri::error("创建音乐目录失败: {}", ec.message());
        cleanup();
        return;
      }

      auto file_stem = song.title;
      if (!song.artist.empty()) {
        file_stem += " - ";
        file_stem += song.artist;
      }

      const auto safe_file_stem = sanitizeFileName(std::move(file_stem));
      std::string url;
      qqmusic_api::song::SongFileFormat selected_format = qqmusic_api::song::m4a_format;
      std::filesystem::path selected_path;
      for (const auto &format : preferredFormats(song)) {
        const auto path = dir / std::format("{}.{}", safe_file_stem, format.e);
        if (std::filesystem::exists(path, ec) && !ec) {
          yuri::info("歌曲已存在，跳过下载: {}", path.string());
          cleanup();
          return;
        }

        url = qqmusic_api::song::get_song_download_url(song.mid, format);
        if (!url.empty()) {
          selected_format = format;
          selected_path = path;
          break;
        }
      }

      if (url.empty() || selected_path.empty()) {
        yuri::error("获取歌曲下载链接失败: {}", song.title);
        cleanup();
        return;
      }

      const curl::KeyValueList headers = {
        {"referer", "https://y.qq.com/"},
        {"cookie", qqmusic_api_config.cookie},
        {"user-agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/142.0.0.0 Safari/537.36"},
      };
      const auto data = curl::get(url, headers);
      if (!data) {
        yuri::error("下载歌曲失败: {}.{}", song.title, selected_format.e);
        cleanup();
        return;
      }

      std::ofstream output(selected_path, std::ios::binary);
      if (!output.is_open()) {
        yuri::error("打开音乐文件失败: {}", selected_path.string());
        cleanup();
        return;
      }

      output.write(data->data(), static_cast<std::streamsize>(data->size()));
      yuri::info("歌曲下载完成: {}", selected_path.string());
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
      has_more = false;
      loading_ = false;
      return;
    }

    int index = current_offset;
    for (auto &music : res.songlist) {
      SongInfo info{std::string(music.title), formatSingers(music.singer), formatDuration(music.interval)};
      const auto title = info.title;
      const auto artist = info.artist;
      auto *item = new SongItem(index++, std::move(info), false, items_);
      song_downloads.emplace(item, SongDownloadInfo{
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
