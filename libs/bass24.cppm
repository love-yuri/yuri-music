//
// Created by yuri on 2026/5/20.
//
module;
#include <bass.h>

export module bass24;

import std;
import core;

export namespace bass24 {

struct PlaybackState {
  bool has_stream{};
  bool playing{};
  double position_seconds{};
  double duration_seconds{};
  float volume{1.0f};
};

class BassPlayer {
public:
  BassPlayer() = default;
  ~BassPlayer() { shutdown(); }

  BassPlayer(const BassPlayer &) = delete;
  BassPlayer &operator=(const BassPlayer &) = delete;

  bool play(const std::filesystem::path &path) {
    std::lock_guard lock(mutex_);

    if (!std::filesystem::exists(path)) {
      yuri::error("音乐文件不存在: {}", path.string());
      return false;
    }

    if (!ensureInitialized()) {
      return false;
    }

    releaseStream();

#ifdef _WIN32
    const auto native_path = path.wstring();
    stream_ = BASS_StreamCreateFile(FALSE, native_path.c_str(), 0, 0, BASS_ASYNCFILE | BASS_UNICODE);
#else
    const auto native_path = path.string();
    stream_ = BASS_StreamCreateFile(FALSE, native_path.c_str(), 0, 0, BASS_ASYNCFILE);
#endif
    if (stream_ == 0) {
      yuri::error("BASS 创建播放流失败: {}, error={}", path.string(), BASS_ErrorGetCode());
      return false;
    }

    BASS_ChannelSetAttribute(stream_, BASS_ATTRIB_VOL, volume_);
    duration_seconds_ = 0.0;
    const auto len = BASS_ChannelGetLength(stream_, BASS_POS_BYTE);
    if (len != static_cast<QWORD>(-1)) {
      duration_seconds_ = BASS_ChannelBytes2Seconds(stream_, len);
    }

    if (!BASS_ChannelPlay(stream_, TRUE)) {
      yuri::error("BASS 播放失败: {}, error={}", path.string(), BASS_ErrorGetCode());
      releaseStream();
      return false;
    }

    current_path_ = path;
    yuri::info("开始播放: {}", path.string());
    return true;
  }

  void stop() {
    std::lock_guard lock(mutex_);
    releaseStream();
    current_path_.clear();
  }

  bool pause() {
    std::lock_guard lock(mutex_);
    if (stream_ == 0) {
      return false;
    }
    return BASS_ChannelPause(stream_) != FALSE;
  }

  bool resume() {
    std::lock_guard lock(mutex_);
    if (stream_ == 0) {
      return false;
    }
    return BASS_ChannelPlay(stream_, FALSE) != FALSE;
  }

  bool togglePause() {
    std::lock_guard lock(mutex_);
    if (stream_ == 0) {
      return false;
    }

    const auto state = BASS_ChannelIsActive(stream_);
    if (state == BASS_ACTIVE_PLAYING || state == BASS_ACTIVE_STALLED) {
      return BASS_ChannelPause(stream_) != FALSE;
    }
    return BASS_ChannelPlay(stream_, FALSE) != FALSE;
  }

  bool seekSeconds(const double seconds) {
    std::lock_guard lock(mutex_);
    if (stream_ == 0) {
      return false;
    }

    const auto target = BASS_ChannelSeconds2Bytes(stream_, seconds < 0.0 ? 0.0 : seconds);
    if (target == static_cast<QWORD>(-1)) {
      return false;
    }
    return BASS_ChannelSetPosition(stream_, target, BASS_POS_BYTE) != FALSE;
  }

  bool seekRatio(const double ratio) {
    std::lock_guard lock(mutex_);
    if (stream_ == 0) {
      return false;
    }

    const auto len = BASS_ChannelGetLength(stream_, BASS_POS_BYTE);
    if (len == static_cast<QWORD>(-1) || len == 0) {
      return false;
    }

    const auto clamped = std::clamp(ratio, 0.0, 1.0);
    const auto target = static_cast<QWORD>(static_cast<double>(len) * clamped);
    return BASS_ChannelSetPosition(stream_, target, BASS_POS_BYTE) != FALSE;
  }

  [[nodiscard]] bool playing() const {
    std::lock_guard lock(mutex_);
    return stream_ != 0 && BASS_ChannelIsActive(stream_) == BASS_ACTIVE_PLAYING;
  }

  [[nodiscard]] bool hasStream() const {
    std::lock_guard lock(mutex_);
    return stream_ != 0;
  }

  [[nodiscard]] PlaybackState state() const {
    std::lock_guard lock(mutex_);

    PlaybackState result{};
    result.has_stream = stream_ != 0;
    result.volume = volume_;
    if (stream_ == 0) {
      return result;
    }

    result.playing = BASS_ChannelIsActive(stream_) == BASS_ACTIVE_PLAYING;

    const auto pos = BASS_ChannelGetPosition(stream_, BASS_POS_BYTE);
    if (pos != static_cast<QWORD>(-1)) {
      result.position_seconds = BASS_ChannelBytes2Seconds(stream_, pos);
    }

    result.duration_seconds = duration_seconds_;

    return result;
  }

  bool setVolume(const float value) {
    std::lock_guard lock(mutex_);
    volume_ = std::clamp(value, 0.0f, 1.0f);
    if (stream_ == 0) {
      return true;
    }
    return BASS_ChannelSetAttribute(stream_, BASS_ATTRIB_VOL, volume_) != FALSE;
  }

  void shutdown() {
    std::lock_guard lock(mutex_);
    releaseStream();
    if (initialized_) {
      BASS_Free();
      initialized_ = false;
    }
  }

private:
  bool ensureInitialized() {
    if (initialized_) {
      return true;
    }

    if (!BASS_Init(-1, 44100, 0, nullptr, nullptr)) {
      yuri::error("BASS 初始化失败: error={}", BASS_ErrorGetCode());
      return false;
    }

    initialized_ = true;
    return true;
  }

  void releaseStream() {
    if (stream_ != 0) {
      BASS_ChannelStop(stream_);
      BASS_StreamFree(stream_);
      stream_ = 0;
    }
    duration_seconds_ = 0.0;
  }

  mutable std::mutex mutex_{};
  HSTREAM stream_{};
  bool initialized_{false};
  float volume_{1.0f};
  double duration_seconds_{0.0};
  std::filesystem::path current_path_{};
};

BassPlayer &player() {
  static BassPlayer instance;
  return instance;
}

} // namespace bass24
