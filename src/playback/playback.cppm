//
// Created by Codex on 2026/8/24.
//
// ReSharper disable CppMemberFunctionMayBeStatic
module;
#include <filesystem>

export module playback;

import std;
import bass24;
import yuri.core;
import models;
import qq_music_api;
import yuri.ui.runtime;

namespace {

constexpr std::size_t kMaxPlaybackHistory = 100; // 最大播放历史数量

/**
 * 按客户端播放策略生成候选音源格式。
 * @param info 歌曲信息
 * @return 从高到低排序的候选音源格式
 */
std::vector<qqmusic_api::song::SongFileFormat> preferredFormats(const SongInfo &info);

/**
 * 清理缓存文件名中的非法字符。
 * @param name 原始文件名
 * @return 可用于文件系统的文件名
 */
std::string sanitizeFileName(std::string name);

/**
 * 生成歌曲缓存路径。
 * @param info 歌曲信息
 * @param format 音源格式
 * @param cache_dir 缓存目录
 * @return 对应音源格式的缓存文件路径
 */
std::filesystem::path cachePathFor(
  const SongInfo &info,
  const qqmusic_api::song::SongFileFormat &format,
  const std::filesystem::path &cache_dir
);

/**
 * 查找已经存在的歌曲缓存。
 * @param info 歌曲信息
 * @param formats 候选音源格式
 * @param cache_dir 缓存目录
 * @return 命中的缓存文件路径，未命中时返回空路径
 */
std::filesystem::path cachedSongPath(
  const SongInfo &info,
  const std::vector<qqmusic_api::song::SongFileFormat> &formats,
  const std::filesystem::path &cache_dir
);

/**
 * 将歌曲保存到本地缓存。
 * @param info 歌曲信息
 * @param url 歌曲下载地址
 * @param format 音源格式
 * @param cache_dir 缓存目录
 * @return 保存完成后的缓存路径，失败时返回空路径
 */
std::filesystem::path cacheSongFile(
  const SongInfo &info,
  std::string_view url,
  const qqmusic_api::song::SongFileFormat &format,
  const std::filesystem::path &cache_dir
);

std::vector<qqmusic_api::song::SongFileFormat> preferredFormats(const SongInfo &info) {
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

std::string sanitizeFileName(std::string name) {
  constexpr std::string_view kInvalidCharacters = R"(<>:"/\|?*)";
  for (char &ch : name) {
    if (
      static_cast<unsigned char>(ch) < 32 || kInvalidCharacters.find(ch) != std::string_view::npos
    ) {
      ch = '_';
    }
  }

  while (!name.empty() && (name.back() == '.' || name.back() == ' ')) {
    name.pop_back();
  }

  return name.empty() ? "unknown" : std::move(name);
}

std::filesystem::path cachePathFor(
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

std::filesystem::path cachedSongPath(
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

std::filesystem::path cacheSongFile(
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

  auto path = cachePathFor(info, format, cache_dir);
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

} // namespace

export namespace playback {

/** 应用级播放控制器。 */
class PlaybackController {
public:
  /** 创建播放控制器并接管底层播放器事件。 */
  PlaybackController();

  /**
   * 从指定播放上下文开始播放歌曲。
   * @param context_id 播放上下文标识
   * @param songs 播放上下文中的有序歌曲
   * @param mid 需要立即播放的歌曲 mid
   */
  void playFromContext(std::string context_id, std::vector<SongInfo> songs, std::string_view mid);

  /**
   * 更新已经激活的播放上下文。
   * @param context_id 播放上下文标识
   * @param songs 上下文中的最新歌曲列表
   */
  void updateContext(std::string_view context_id, std::vector<SongInfo> songs);

  /**
   * 将歌曲加入显式下一首队列。
   * @param song 待排队歌曲
   */
  void enqueueNext(const SongInfo &song);

  /** 播放上一首歌曲。 */
  void previous();

  /** 播放下一首歌曲。 */
  void next();

  /**
   * 设置随机播放模式。
   * @param enabled 是否启用随机播放
   */
  void setRandomPlayback(bool enabled) noexcept;

  /** 切换播放和暂停状态。 */
  void togglePause() const;

  /**
   * 跳转到指定播放进度。
   * @param ratio 目标播放进度比例
   * @return 跳转成功时返回 true
   */
  bool seekRatio(double ratio) const;

  /**
   * 设置播放音量。
   * @param volume 目标音量
   * @return 设置成功时返回 true
   */
  bool setVolume(float volume) const;

  /**
   * 获取当前播放位置。
   * @return 当前播放位置，单位为秒
   */
  [[nodiscard]] double positionSeconds() const;

  Signal<const SongInfo &> currentSongChanged{};        // 当前歌曲变化事件
  Signal<bass24::PlaybackState> playbackStateChanged{}; // 播放状态变化事件
  Signal<double> playbackDurationChanged{};             // 播放时长变化事件
  Signal<float> volumeChanged{};                        // 播放音量变化事件

private:
  /** 播放队列中的歌曲记录。 */
  struct PlaybackEntry {
    SongInfo song{};                            // 歌曲信息
    std::optional<std::size_t> context_index{}; // 所属上下文位置
  };

  /**
   * 播放指定队列记录。
   * @param entry 待播放记录
   */
  void playEntry(PlaybackEntry entry);

  /** 将当前歌曲加入受限长度的播放历史。 */
  void rememberCurrent();

  /**
   * 解析并启动歌曲播放。
   * @param song 待播放歌曲
   */
  void beginPlayback(const SongInfo &song);

  /**
   * 处理底层播放状态变化。
   * @param state 最新播放状态
   */
  void onBassStateChanged(bass24::PlaybackState state);

  /**
   * 处理底层播放时长变化。
   * @param duration_seconds 最新播放时长
   */
  void onBassDurationChanged(double duration_seconds);

  /**
   * 处理底层播放音量变化。
   * @param volume 最新播放音量
   */
  void onBassVolumeChanged(float volume);

  /** 处理底层自然播放完成事件。 */
  void onBassFinished();

  std::string context_id{};                             // 当前播放上下文标识
  std::vector<SongInfo> context_songs{};                // 当前播放上下文歌曲
  std::optional<std::size_t> context_index{};           // 当前上下文游标
  std::optional<PlaybackEntry> current_entry{};         // 当前播放记录
  std::vector<PlaybackEntry> history{};                 // 已播放历史
  std::deque<SongInfo> next_queue{};                    // 显式下一首队列
  std::mt19937 random_engine{ std::random_device{}() }; // 随机播放引擎
  std::atomic_uint64_t playback_generation{};           // 播放请求版本
  bool random_playback = false;                         // 是否随机播放
};

PlaybackController::PlaybackController() {
  bass24::bass24_player.stateChanged.connect<&PlaybackController::onBassStateChanged>(this);
  bass24::bass24_player.durationChanged.connect<&PlaybackController::onBassDurationChanged>(this);
  bass24::bass24_player.volumeChanged.connect<&PlaybackController::onBassVolumeChanged>(this);
  bass24::bass24_player.finished.connect<&PlaybackController::onBassFinished>(this);
}

void PlaybackController::playFromContext(
  std::string context_id,
  std::vector<SongInfo> songs,
  const std::string_view mid
) {
  const auto selected = std::ranges::find(songs, mid, &SongInfo::mid);
  if (selected == songs.end()) {
    yuri::warn("播放上下文中未找到歌曲: {}", mid);
    return;
  }

  const auto selected_index = static_cast<std::size_t>(selected - songs.begin());
  this->context_id = std::move(context_id);
  context_songs = std::move(songs);
  context_index = selected_index;
  history.clear();
  playEntry(PlaybackEntry{ context_songs[*context_index], context_index });
}

void PlaybackController::updateContext(
  const std::string_view context_id,
  std::vector<SongInfo> songs
) {
  if (this->context_id != context_id) {
    return;
  }

  std::string cursor_mid;
  if (context_index && *context_index < context_songs.size()) {
    cursor_mid = context_songs[*context_index].mid;
  }

  context_songs = std::move(songs);
  if (cursor_mid.empty()) {
    context_index.reset();
    return;
  }

  const auto cursor = std::ranges::find(context_songs, cursor_mid, &SongInfo::mid);
  if (cursor == context_songs.end()) {
    context_index.reset();
    return;
  }
  context_index = static_cast<std::size_t>(cursor - context_songs.begin());
}

void PlaybackController::enqueueNext(const SongInfo &song) {
  if (song.mid.empty()) {
    yuri::warn("歌曲播放信息缺失，无法加入下一首队列");
    return;
  }
  next_queue.push_back(song);
  yuri::info("已加入下一首队列: {}", song.title);
}

void PlaybackController::previous() {
  if (history.empty() && (!context_index || *context_index == 0 || random_playback)) {
    return;
  }

  if (!history.empty()) {
    auto previous_entry = std::move(history.back());
    history.pop_back();
    if (previous_entry.context_index) {
      context_index = previous_entry.context_index;
    }
    playEntry(std::move(previous_entry));
    return;
  }

  --*context_index;
  playEntry(PlaybackEntry{ context_songs[*context_index], context_index });
}

void PlaybackController::next() {
  if (!next_queue.empty()) {
    rememberCurrent();
    auto song = std::move(next_queue.front());
    next_queue.pop_front();
    playEntry(PlaybackEntry{ std::move(song), std::nullopt });
    return;
  }

  if (context_songs.empty()) {
    return;
  }

  std::size_t next_index = 0;
  if (random_playback) {
    if (context_songs.size() > 1 && context_index) {
      std::uniform_int_distribution<std::size_t> distribution(0, context_songs.size() - 2);
      next_index = distribution(random_engine);
      if (next_index >= *context_index) {
        ++next_index;
      }
    } else if (context_songs.size() > 1) {
      std::uniform_int_distribution<std::size_t> distribution(0, context_songs.size() - 1);
      next_index = distribution(random_engine);
    }
  } else if (context_index) {
    next_index = *context_index + 1;
    if (next_index >= context_songs.size()) {
      return;
    }
  }

  rememberCurrent();
  context_index = next_index;
  playEntry(PlaybackEntry{ context_songs[next_index], context_index });
}

void PlaybackController::setRandomPlayback(const bool enabled) noexcept {
  random_playback = enabled;
}

void PlaybackController::togglePause() const {
  void(bass24::bass24_player.togglePause());
}

bool PlaybackController::seekRatio(const double ratio) const {
  return bass24::bass24_player.seekRatio(std::clamp(ratio, 0.0, 1.0));
}

bool PlaybackController::setVolume(const float volume) const {
  return bass24::bass24_player.setVolume(volume);
}

double PlaybackController::positionSeconds() const {
  return bass24::bass24_player.positionSeconds();
}

void PlaybackController::playEntry(PlaybackEntry entry) {
  if (entry.song.mid.empty()) {
    yuri::warn("歌曲播放信息缺失，无法播放");
    return;
  }

  current_entry = std::move(entry);
  currentSongChanged.emit(current_entry->song);
  beginPlayback(current_entry->song);
}

void PlaybackController::rememberCurrent() {
  if (!current_entry) {
    return;
  }
  history.push_back(*current_entry);
  if (history.size() > kMaxPlaybackHistory) {
    history.erase(history.begin());
  }
}

void PlaybackController::beginPlayback(const SongInfo &song) {
  const auto generation = playback_generation.fetch_add(1) + 1;
  const auto formats = preferredFormats(song);
  constexpr auto kCacheDirectory = "musics";
  if (const auto path = cachedSongPath(song, formats, kCacheDirectory); !path.empty()) {
    yuri::info("使用本地音乐缓存播放: {}", path.string());
    void(bass24::bass24_player.play(path));
    return;
  }

  bass24::bass24_player.stop();
  bass24::bass24_player.beginLoading();
  thread_manager->addTask([this, song, formats, generation] {
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

      ui::dispatcher.post([this, song, url = std::move(url), source_format, generation]() mutable {
        if (playback_generation.load() != generation) {
          return;
        }
        if (url.empty()) {
          yuri::error("获取歌曲下载链接失败: {}", song.title);
          bass24::bass24_player.stop();
          return;
        }
        if (!bass24::bass24_player.playUrl(url)) {
          return;
        }

        yuri::info("流式播放: {}", song.title);
        thread_manager->addTask([song, url = std::move(url), source_format] {
          cacheSongFile(song, url, source_format, "musics");
        });
      });
    } catch (const std::exception &e) {
      const std::string message = e.what();
      ui::dispatcher.post([this, song, generation, message] {
        if (playback_generation.load() != generation) {
          return;
        }
        yuri::error("播放歌曲异常: {}", message);
        bass24::bass24_player.stop();
      });
    }
  });
}

void PlaybackController::onBassStateChanged(const bass24::PlaybackState state) {
  ui::dispatcher.post([this, state] {
    playbackStateChanged.emit(state);
  });
}

void PlaybackController::onBassDurationChanged(const double duration_seconds) {
  ui::dispatcher.post([this, duration_seconds] {
    playbackDurationChanged.emit(duration_seconds);
  });
}

void PlaybackController::onBassVolumeChanged(const float volume) {
  ui::dispatcher.post([this, volume] {
    volumeChanged.emit(volume);
  });
}

void PlaybackController::onBassFinished() {
  ui::dispatcher.post([this] { next(); });
}

PlaybackController controller;

} // namespace playback
