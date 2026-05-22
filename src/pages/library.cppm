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
  /**
   * 创建音乐库页面。
   * @param parent 父级控件
   */
  explicit LibraryPage(Widget *parent = nullptr);

  /**
   * 绘制音乐库背景。
   * @param canvas Skia 绘制画布
   */
  void paint(SkCanvas *canvas) override;

  /**
   * 播放当前选中歌曲的上一首。
   */
  void playPrevious();

  /**
   * 播放当前选中歌曲的下一首。
   */
  void playNext();

private:
  /**
   * 根据滚动位置检查是否需要加载更多歌曲。
   * @param scrollOffset 当前滚动偏移
   */
  void checkLoadMore(float scrollOffset);

  /**
   * 加载下一页歌曲。
   */
  void loadMore();

  /**
   * 处理歌曲行双击事件。
   * @param item 被双击的歌曲行
   */
  void onSongDoubleClicked(SongItem *item);

  /**
   * 播放指定歌曲，缓存未命中时使用网络流并后台缓存。
   * @param item 要播放的歌曲行
   */
  void playSong(const SongItem *item);

  /**
   * 格式化歌曲时长。
   * @param seconds 歌曲时长，单位为秒
   * @return m:ss 格式的时长文本
   */
  static std::string formatDuration(int seconds);

  /**
   * 拼接歌手名称。
   * @param singers 歌手列表
   * @return 使用分隔符拼接后的歌手名称
   */
  static std::string formatSingers(const std::vector<SingerType> &singers);

  /**
   * 从 QQ 音乐歌曲信息构建列表项数据。
   * @param music QQ 音乐歌单歌曲信息
   * @return 歌曲行展示和播放所需的数据
   */
  static SongInfo makeSongInfo(const SonglistType &music);

  /**
   * 按客户端播放策略生成候选音源格式。
   * @param info 歌曲行数据
   * @return 从高到低排序的候选音源格式
   */
  static std::vector<qqmusic_api::song::SongFileFormat> preferredFormats(const SongInfo &info);

  /**
   * 清理缓存文件名中的非法字符。
   * @param name 原始文件名
   * @return 可用于文件系统的文件名
   */
  static std::string sanitizeFileName(std::string name);

  /**
   * 生成歌曲缓存路径。
   * @param info 歌曲行数据
   * @param format 音源格式
   * @param cache_dir 缓存目录
   * @return 对应音源格式的缓存文件路径
   */
  static std::filesystem::path cachePathFor(const SongInfo &info,
                                            const qqmusic_api::song::SongFileFormat &format,
                                            const std::filesystem::path &cache_dir = "musics");

  /**
   * 查找已存在的歌曲缓存。
   * @param info 歌曲行数据
   * @param formats 候选音源格式
   * @param cache_dir 缓存目录
   * @return 命中的缓存文件路径；未命中时返回空路径
   */
  static std::filesystem::path cachedSongPath(
    const SongInfo &info,
    const std::vector<qqmusic_api::song::SongFileFormat> &formats,
    const std::filesystem::path &cache_dir = "musics");

  /**
   * 将已解析的歌曲 URL 保存到本地缓存。
   * @param info 歌曲行数据
   * @param url 歌曲下载 URL
   * @param format URL 对应的音源格式
   * @param cache_dir 缓存目录
   * @return 保存成功后的缓存文件路径；失败时返回空路径
   */
  static std::filesystem::path cacheSongFile(const SongInfo &info,
                                             std::string_view url,
                                             const qqmusic_api::song::SongFileFormat &format,
                                             const std::filesystem::path &cache_dir = "musics");

  /**
   * 判断当前选中歌曲是否仍然是指定 mid。
   * @param mid 歌曲 mid
   * @return 当前选中歌曲 mid 与参数一致时返回 true，否则返回 false
   */
  bool isSelectedMid(std::string_view mid);

  SongItem *selected_item{};                      // 被选中的item
  ScrollArea *items_{};                           // 歌曲列表
  std::vector<SongItem *> song_items{};           // 按列表顺序保存歌曲项
  std::uint64_t tid_{};                           // 当前歌单 ID
  int offset_{ 0 };                               // 当前加载偏移
  bool loading_{ false };                         // 是否正在加载
  bool has_more{ true };                          // 是否还有更多数据
  std::unordered_set<std::string> loading_mids{}; // 正在解析播放地址的歌曲 mid
  std::mutex loading_mutex{};                     // 播放地址解析状态锁
  std::string selected_mid{};                     // 当前选中的歌曲 mid
  std::mutex selected_mutex{};                    // 当前选中状态锁

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

  loading_ = true;
  thread_manager->addTask([this] {
    using namespace qqmusic_api::playlist;
    for (auto &value : get_user_playlists().data.disslist) {
      if (value.diss_name == "我喜欢") {
        tid_ = value.tid;
        loading_ = false;
        loadMore();
        break;
      }
    }

    if (tid_ == 0) {
      has_more = false;
      loading_ = false;
      yuri::warn("未找到“我喜欢”歌单");
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

SongInfo LibraryPage::makeSongInfo(const SonglistType &music) {
  return SongInfo{
    .title = std::string(music.title),
    .artist = formatSingers(music.singer),
    .duration = formatDuration(music.interval),
    .mid = std::string(music.mid),
    .album_mid = std::string(music.album.mid),
    .has_flac = music.file.size_flac > 0,
    .has_ape = music.file.size_ape > 0,
    .has_mp3_320 = music.file.size_320mp3 > 0,
    .has_mp3_128 = music.file.size_128mp3 > 0,
  };
}

std::vector<qqmusic_api::song::SongFileFormat> LibraryPage::preferredFormats(const SongInfo &info) {
  std::vector<qqmusic_api::song::SongFileFormat> formats;
  formats.reserve(5);

  if (info.has_flac) {
    formats.push_back(qqmusic_api::song::flac_format);
  }
  if (info.has_mp3_320) {
    formats.push_back(qqmusic_api::song::mp3_320_format);
  }
  if (info.has_mp3_128) {
    formats.push_back(qqmusic_api::song::mp3_128_format);
  }
  formats.push_back(qqmusic_api::song::m4a_format);
  if (info.has_ape) {
    formats.push_back(qqmusic_api::song::ape_format);
  }

  return formats;
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

  return name.empty() ? "unknown" : std::move(name);
}

std::filesystem::path LibraryPage::cachePathFor(
  const SongInfo &info,
  const qqmusic_api::song::SongFileFormat &format,
  const std::filesystem::path &cache_dir
) {
  auto stem = info.title;
  if (!info.artist.empty()) {
    stem += " - ";
    stem += info.artist;
  }

  return cache_dir / std::format("{}.{}", sanitizeFileName(std::move(stem)), format.e);
}

std::filesystem::path LibraryPage::cachedSongPath(
  const SongInfo &info,
  const std::vector<qqmusic_api::song::SongFileFormat> &formats,
  const std::filesystem::path &cache_dir
) {
  std::error_code ec;
  for (const auto &format : formats) {
    const auto path = cachePathFor(info, format, cache_dir);
    if (std::filesystem::exists(path, ec) && !ec) {
      return path;
    }
  }

  return {};
}

std::filesystem::path LibraryPage::cacheSongFile(
  const SongInfo &info,
  const std::string_view url,
  const qqmusic_api::song::SongFileFormat &format,
  const std::filesystem::path &cache_dir
) {
  if (url.empty()) {
    return {};
  }

  std::error_code ec;
  std::filesystem::create_directories(cache_dir, ec);
  if (ec) {
    yuri::error("创建音乐缓存目录失败: {}", ec.message());
    return {};
  }

  const auto path = cachePathFor(info, format, cache_dir);
  if (std::filesystem::exists(path, ec) && !ec) {
    return path;
  }

  auto temp_path = path;
  temp_path += ".part";
  std::filesystem::remove(temp_path, ec);

  if (!qqmusic_api::song::download_song_file(url, temp_path)) {
    std::filesystem::remove(temp_path, ec);
    yuri::error("下载音乐缓存失败: {}", info.title);
    return {};
  }

  std::filesystem::rename(temp_path, path, ec);
  if (ec) {
    std::filesystem::remove(temp_path, ec);
    yuri::error("保存音乐缓存失败: {}", path.string());
    return {};
  }

  yuri::info("音乐缓存完成: {}", path.string());
  return path;
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

  {
    std::lock_guard lock(selected_mutex);
    selected_mid = item->info().mid;
  }

  const auto &info = selected_item->info();
  songSelected.emit(info.title, info.artist, info.duration);
  playSong(item);
}

void LibraryPage::playSong(const SongItem *item) {
  const auto &info = item->info();
  if (info.mid.empty()) {
    yuri::warn("歌曲播放信息缺失，无法播放");
    return;
  }

  const auto formats = preferredFormats(info);
  if (const auto path = cachedSongPath(info, formats); !path.empty()) {
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
    std::lock_guard lock(loading_mutex);
    if (loading_mids.contains(info.mid)) {
      loadingStateChanged.emit(true);
      return;
    }
    loading_mids.insert(info.mid);
  }

  loadingStateChanged.emit(true);
  thread_manager->addTask([this, song = info, formats] {
    bool cleaned = false;
    const auto cleanup = [this, mid = song.mid, &cleaned] {
      if (cleaned) {
        return;
      }
      cleaned = true;
      {
        std::lock_guard lock(loading_mutex);
        loading_mids.erase(mid);
      }
      if (isSelectedMid(mid)) {
        loadingStateChanged.emit(false);
      }
    };

    try {
      std::string url;
      qqmusic_api::song::SongFileFormat source_format{};
      for (const auto &format : formats) {
        url = qqmusic_api::song::get_song_download_url(song.mid, format);
        if (!url.empty()) {
          source_format = format;
          break;
        }
      }
      if (url.empty()) {
        yuri::error("获取歌曲下载链接失败: {}", song.title);
        cleanup();
        return;
      }

      bool should_play = false;
      {
        std::lock_guard lock(selected_mutex);
        should_play = selected_mid == song.mid;
      }
      if (should_play) {
        if (bass24::player().playUrl(url)) {
          playbackStateChanged.emit(true);
          yuri::info("流式播放: {}", song.title);
          cleanup();
          cacheSongFile(song, url, source_format);
          return;
        }
      }
    } catch (const std::exception &e) {
      yuri::error("播放歌曲异常: {}", e.what());
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
  if (loading_ || !has_more || tid_ == 0) {
    return;
  }

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
      auto info = makeSongInfo(music);
      auto *item = new SongItem(index++, std::move(info), false, items_);
      song_items.push_back(item);
      item->doubleClicked.connect<&LibraryPage::onSongDoubleClicked>(this);
    }

    offset_ = current_offset + res.songlist.size();
    loading_ = false;
    markLayoutDirty();
  });
}

} // namespace pages
