//
// Created by love-yuri on 2026/11/24.
//

export module player:audio_player;

import models;
import qq_music_api;
import std;
import yuri.core;
import yuri.ui.runtime;
import :song_cache;

namespace player {

using namespace qqmusic_api::song;

export using AudioSource = std::variant<std::filesystem::path, std::string>; // 本地路径或网络 URL

/**
 * 播放状态
 */
export enum class PlaybackState {
  stopped, // 已停止
  loading, // 正在打开播放流
  playing, // 正在播放
  paused,  // 已暂停
  error,   // 播放失败
};

/** 音频播放接口。播放控制和信号处理在 UI 线程执行。 */
export class AudioPlayer {
public:
  Signal<PlaybackState> state_changed;                  // 播放状态变化事件
  Signal<std::shared_ptr<const SongInfo>> song_changed; // 当前正在播放的歌曲发生变化
  Signal<float> volume_changed;                         // 音量变化事件
  Signal<> finished;                                    // 自然播放完成事件

  /** 析构播放器，支持通过接口指针释放派生对象。 */
  virtual ~AudioPlayer() = default;

  /**
   * 播放并替换当前歌曲，自动选择播放源。
   * 优先使用本地缓存（file），否则使用网络 URL 并缓存。
   * @param song_info 歌曲信息
   */
  void play_song_auto(const SongInfo &song_info);

  /**
   * 播放并替换当前歌曲。
   * @param song_info 歌曲信息
   * @param source 已解析的本地路径或网络音频 URL
   */
  virtual void play_song(const SongInfo &song_info, AudioSource source) = 0;

  /** 开始播放，或从暂停位置继续播放。 */
  virtual void play() = 0;

  /** 暂停播放并保留当前位置。 */
  virtual void pause() = 0;

  /** 停止播放并释放当前音源。 */
  virtual void stop() = 0;

  /** 暂停当前播放，或开始及恢复播放。 */
  void toggle();

  /**
   * 跳转到指定的播放位置。
   * @param seconds 目标位置距音频起点的秒数。
   * @return 是否成功跳转
   */
  virtual bool seek(double seconds) = 0;

  /**
   * 设置播放音量。
   * @param volume 目标音量，范围为 0 到 1。
   */
  virtual void set_volume(float volume) = 0;

  /**
   * 查询当前播放位置。
   * @return 当前播放位置距音频起点的秒数。
   */
  virtual double position() const = 0;

  /**
   * 查询当前音频的总时长。
   * @return 当前音频的总时长，单位为秒。
   */
  virtual double duration() const = 0;

  /**
   * 查询当前播放状态。
   * @return 当前播放状态
   */
  [[nodiscard]] PlaybackState state() const;

  /** 查询当前选中的歌曲，包括已停止的歌曲。 */
  [[nodiscard]] std::shared_ptr<const SongInfo> current_song() const;

protected:
  /**
   * 更新并发送播放状态。
   * @param state 新的播放状态
   */
  void set_state(PlaybackState state);

  /**
   * 更新并发送当前歌曲信息。
   * @param song_info 新的歌曲信息
   */
  void set_current_song(const std::shared_ptr<const SongInfo> &song_info);

private:
  std::atomic<PlaybackState> state_ = PlaybackState::stopped; // 当前播放状态
  std::shared_ptr<const SongInfo> current_song_;              // 当前歌曲信息
};

/**
 * 按播放优先级列出歌曲可用的音源格式。
 * @param info 歌曲信息
 * @return 候选音源格式
 */
std::vector<SongFileFormat> preferred_formats(const SongInfo &info) {
  std::vector<SongFileFormat> formats;
  formats.reserve(5);

  if (info.has_flac) {
    formats.push_back(flac_format);
  }
  if (info.has_mp3_320) {
    formats.push_back(mp3_320_format);
  }
  if (info.has_mp3_128) {
    formats.push_back(mp3_128_format);
  }

  formats.push_back(m4a_format);
  if (info.has_ape) {
    formats.push_back(ape_format);
  }

  return formats;
}

/**
 * 优先查找本地缓存，缓存未命中时解析网络音源。
 * @param song 歌曲信息
 * @return 可用音源及其格式
 */
std::optional<std::pair<AudioSource, SongFileFormat>> resolve_source(const SongInfo &song) {
  const auto formats = preferred_formats(song);
  for (const auto &format : formats) {
    if (auto path = song_cache().find(song.mid, format); !path.empty()) {
      return std::pair { AudioSource(std::move(path)), format };
    }
  }

  for (const auto &format : formats) {
    if (auto url = get_song_download_url(song.mid, format); !url.empty()) {
      return std::pair { AudioSource(std::move(url)), format };
    }
  }
  return std::nullopt;
}

void AudioPlayer::play_song_auto(const SongInfo &song_info) {
  thread_manager->addTask([this, song = song_info] {
    try {
      const auto resolved = resolve_source(song);
      if (!resolved) {
        yuri::warn("未找到可用音源: {}", song.title);
        return;
      }

      const auto &[source, format] = *resolved;
      ui::dispatcher.post([this, song, source = source] mutable {
        play_song(song, std::move(source));
      });
      if (const auto *url = std::get_if<std::string>(&source)) {
        static_cast<void>(song_cache().store(song.mid, format, *url));
      }
    } catch (const std::exception &error) {
      yuri::warn("处理歌曲音源失败（{}）: {}", song.title, error.what());
    }
  });
}

void AudioPlayer::toggle() {
  if (state() == PlaybackState::playing || state() == PlaybackState::loading) {
    pause();
  } else {
    play();
  }
}

PlaybackState AudioPlayer::state() const {
  return state_.load();
}

std::shared_ptr<const SongInfo> AudioPlayer::current_song() const {
  return current_song_;
}

void AudioPlayer::set_state(const PlaybackState state) {
  if (state_.exchange(state) == state) {
    return;
  }
  state_changed.emit(state);
}

void AudioPlayer::set_current_song(const std::shared_ptr<const SongInfo> &song_info) {
  if (!current_song_ || *current_song_ != *song_info) {
    current_song_ = song_info;
    song_changed.emit(song_info);
  }
}

} // namespace player
