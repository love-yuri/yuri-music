//
// Created by yuri on 2026/5/20.
//
module;
#include <bass.h>

export module bass24;

import std;
import yuri.core;

namespace bass24 {

export enum class PlaybackState {
  stopped,   // 已停止
  loading,   // 正在加载或解析播放流
  buffering, // 播放流正在等待数据
  playing,   // 正在播放
  paused,    // 已暂停
};

class BassPlayer {
public:
  Signal<PlaybackState> stateChanged; // 播放状态变化事件
  Signal<double> durationChanged;     // 当前歌曲总时长变化事件
  Signal<float> volumeChanged;        // 音量变化事件
  Signal<> finished;                  // 自然播放完成事件

  /**
   * 创建播放器实例。
   */
  BassPlayer() = default;

  /**
   * 析构播放器实例，并释放当前流、插件和 BASS 设备资源。
   */
  ~BassPlayer();

  /**
   * 禁止复制播放器实例。
   * @param other 被复制的播放器实例
   */
  BassPlayer(const BassPlayer &other) = delete;

  /**
   * 禁止复制赋值播放器实例。
   * @param other 被复制赋值的播放器实例
   * @return 当前播放器实例引用
   */
  BassPlayer &operator=(const BassPlayer &other) = delete;

  /**
   * 播放网络音频流。
   * @param url 可由 BASS 直接打开的音频 URL
   * @return 播放流创建并开始播放时返回 true，否则返回 false
   */
  bool playUrl(std::string_view url);

  /**
   * 播放本地音频文件。
   * @param path 本地音频文件路径
   * @return 文件流创建并开始播放时返回 true，否则返回 false
   */
  bool play(const std::filesystem::path &path);

  /** 通知播放器即将加载尚未解析完成的播放源。 */
  void beginLoading();

  /**
   * 停止当前播放并释放当前流。
   */
  void stop();

  /**
   * 暂停当前播放流。
   * @return 当前存在播放流且暂停成功时返回 true，否则返回 false
   */
  bool pause();

  /**
   * 恢复当前播放流。
   * @return 当前存在播放流且恢复成功时返回 true，否则返回 false
   */
  bool resume();

  /**
   * 在暂停和播放状态之间切换。
   * @return 当前存在播放流且状态切换成功时返回 true，否则返回 false
   */
  bool togglePause();

  /**
   * 跳转到指定播放时间。
   * @param seconds 目标播放位置，单位为秒；小于 0 时按 0 处理
   * @return 当前存在播放流且跳转成功时返回 true，否则返回 false
   */
  bool seekSeconds(double seconds) const;

  /**
   * 按播放进度比例跳转。
   * @param ratio 目标播放进度比例，范围会被限制到 0..1
   * @return 当前存在播放流且跳转成功时返回 true，否则返回 false
   */
  bool seekRatio(double ratio) const;

  /**
   * 获取当前播放位置。
   * @return 当前播放位置（秒），没有播放流或查询失败时返回 0
   */
  [[nodiscard]] double positionSeconds() const;

  /**
   * 设置播放器音量。
   * @param value 目标音量，范围会被限制到 0..1
   * @return 音量状态保存成功，且当前流音量设置成功时返回 true
   */
  bool setVolume(float value);

  /**
   * 释放当前播放流、已加载插件和 BASS 设备资源。
   */
  void shutdown();

private:
  /**
   * 确保 BASS 设备和必要插件已初始化。
   * @return 初始化完成或已经初始化时返回 true，否则返回 false
   */
  bool ensureInitialized();

  /**
   * 加载 BASSFLAC 插件。
   * @return 插件加载成功时返回 true，否则返回 false
   */
  bool loadFlacPlugin();

  /**
   * 为当前播放流注册播放完成和缓冲状态同步事件。
   * @return 全部注册成功时返回 true，否则返回 false
   */
  bool installPlaybackSyncs();

  /**
   * 接收 BASS 播放完成同步事件。
   * @param handle BASS 同步事件句柄
   * @param channel 已完成播放的流句柄
   * @param data BASS 同步事件附加数据
   * @param user 播放器实例指针
   */
  static void CALLBACK onPlaybackEnded(HSYNC handle, DWORD channel, DWORD data, void *user);

  /**
   * 接收 BASS 播放流缓冲状态同步事件。
   * @param handle BASS 同步事件句柄
   * @param channel 状态变化的播放流句柄
   * @param data 0 表示进入缓冲，非 0 表示恢复播放
   * @param user 播放器实例指针
   */
  static void CALLBACK onPlaybackStalled(HSYNC handle, DWORD channel, DWORD data, void *user);

  /**
   * 更新并发送播放状态。
   * @param state 新的播放状态
   */
  void setPlaybackState(PlaybackState state);

  /**
   * 从当前播放流刷新总时长缓存。
   */
  void updateDurationSeconds();

  /**
   * 停止并释放当前播放流。
   */
  void releaseStream();

  mutable std::mutex mutex_{}; // 保护 BASS 句柄和播放器状态
  HSTREAM stream_{};           // 当前播放流句柄，0 表示未打开
  HSYNC end_sync{};            // 当前播放完成同步事件句柄
  HSYNC stall_sync{};          // 当前播放缓冲状态同步事件句柄
  HPLUGIN flac_plugin{};       // BASSFLAC 插件句柄
  bool initialized_{ false };  // BASS 设备和插件是否已初始化
  std::atomic<PlaybackState> playback_state{ PlaybackState::stopped }; // 当前播放状态
  float volume_{ 1.0f };                                               // 当前音量，范围 0..1
  double duration_seconds{ 0.0 };                                      // 当前流总时长，未知时为 0
  std::filesystem::path current_path{}; // 当前本地播放文件，网络流时为空
};

BassPlayer::~BassPlayer() {
  shutdown();
}

bool BassPlayer::playUrl(const std::string_view url) {
  if (url.empty()) {
    yuri::error("音乐流 URL 为空");
    return false;
  }

  setPlaybackState(PlaybackState::loading);
  durationChanged.emit(0.0);

  std::lock_guard lock(mutex_);
  if (!ensureInitialized()) {
    setPlaybackState(PlaybackState::stopped);
    return false;
  }

  releaseStream();

  const std::string url_string(url);
  stream_ = BASS_StreamCreateURL(
    url_string.c_str(), 0, BASS_STREAM_BLOCK | BASS_STREAM_STATUS, nullptr, nullptr
  );
  if (stream_ == 0) {
    yuri::error("BASS 创建网络播放流失败: error={}", BASS_ErrorGetCode());
    setPlaybackState(PlaybackState::stopped);
    return false;
  }

  BASS_ChannelSetAttribute(stream_, BASS_ATTRIB_VOL, volume_);
  updateDurationSeconds();
  if (!installPlaybackSyncs()) {
    releaseStream();
    setPlaybackState(PlaybackState::stopped);
    return false;
  }

  if (!BASS_ChannelPlay(stream_, TRUE)) {
    yuri::error("BASS 网络流播放失败: error={}", BASS_ErrorGetCode());
    releaseStream();
    setPlaybackState(PlaybackState::stopped);
    return false;
  }

  current_path.clear();
  durationChanged.emit(duration_seconds);
  setPlaybackState(PlaybackState::playing);
  yuri::info("开始流式播放");
  return true;
}

bool BassPlayer::play(const std::filesystem::path &path) {
  if (!std::filesystem::exists(path)) {
    yuri::error("音乐文件不存在: {}", path.string());
    return false;
  }

  setPlaybackState(PlaybackState::loading);
  durationChanged.emit(0.0);

  std::lock_guard lock(mutex_);
  if (!ensureInitialized()) {
    setPlaybackState(PlaybackState::stopped);
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
    setPlaybackState(PlaybackState::stopped);
    return false;
  }

  BASS_ChannelSetAttribute(stream_, BASS_ATTRIB_VOL, volume_);
  updateDurationSeconds();
  if (!installPlaybackSyncs()) {
    releaseStream();
    setPlaybackState(PlaybackState::stopped);
    return false;
  }

  if (!BASS_ChannelPlay(stream_, TRUE)) {
    yuri::error("BASS 播放失败: {}, error={}", path.string(), BASS_ErrorGetCode());
    releaseStream();
    setPlaybackState(PlaybackState::stopped);
    return false;
  }

  current_path = path;
  durationChanged.emit(duration_seconds);
  setPlaybackState(PlaybackState::playing);
  yuri::info("开始播放: {}", path.string());
  return true;
}

void BassPlayer::beginLoading() {
  durationChanged.emit(0.0);
  setPlaybackState(PlaybackState::loading);
}

void BassPlayer::stop() {
  std::lock_guard lock(mutex_);
  releaseStream();
  current_path.clear();
  durationChanged.emit(0.0);
  setPlaybackState(PlaybackState::stopped);
}

bool BassPlayer::pause() {
  std::lock_guard lock(mutex_);
  if (stream_ == 0) {
    return false;
  }
  if (!BASS_ChannelPause(stream_)) {
    return false;
  }
  setPlaybackState(PlaybackState::paused);
  return true;
}

bool BassPlayer::resume() {
  std::lock_guard lock(mutex_);
  if (stream_ == 0) {
    return false;
  }
  if (!BASS_ChannelPlay(stream_, FALSE)) {
    return false;
  }
  setPlaybackState(PlaybackState::playing);
  return true;
}

bool BassPlayer::togglePause() {
  std::lock_guard lock(mutex_);
  if (stream_ == 0) {
    return false;
  }

  const auto active_state = BASS_ChannelIsActive(stream_);
  if (active_state == BASS_ACTIVE_PLAYING || active_state == BASS_ACTIVE_STALLED) {
    if (!BASS_ChannelPause(stream_)) {
      return false;
    }
    setPlaybackState(PlaybackState::paused);
    return true;
  }
  if (!BASS_ChannelPlay(stream_, FALSE)) {
    return false;
  }
  setPlaybackState(PlaybackState::playing);
  return true;
}

bool BassPlayer::seekSeconds(const double seconds) const {
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

bool BassPlayer::seekRatio(const double ratio) const {
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

double BassPlayer::positionSeconds() const {
  std::lock_guard lock(mutex_);
  if (stream_ == 0) {
    return 0.0;
  }

  const auto pos = BASS_ChannelGetPosition(stream_, BASS_POS_BYTE);
  if (pos == static_cast<QWORD>(-1)) {
    return 0.0;
  }
  return BASS_ChannelBytes2Seconds(stream_, pos);
}

bool BassPlayer::setVolume(const float value) {
  std::lock_guard lock(mutex_);
  const float next_volume = std::clamp(value, 0.0f, 1.0f);
  if (std::abs(volume_ - next_volume) <= 0.001f) {
    return true;
  }
  volume_ = next_volume;
  const bool applied = stream_ == 0 || BASS_ChannelSetAttribute(stream_, BASS_ATTRIB_VOL, volume_);
  volumeChanged.emit(volume_);
  return applied;
}

void BassPlayer::shutdown() {
  std::lock_guard lock(mutex_);
  releaseStream();
  if (initialized_) {
    if (flac_plugin != 0) {
      BASS_PluginFree(flac_plugin);
      flac_plugin = 0;
    }
    BASS_Free();
    initialized_ = false;
  }
}

bool BassPlayer::ensureInitialized() {
  if (initialized_) {
    return true;
  }

  BASS_SetConfigPtr(
    BASS_CONFIG_NET_AGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36"
  );
  BASS_SetConfig(BASS_CONFIG_NET_TIMEOUT, 8000);
  BASS_SetConfig(BASS_CONFIG_NET_READTIMEOUT, 8000);
  BASS_SetConfig(BASS_CONFIG_NET_BUFFER, 5000);
  BASS_SetConfig(BASS_CONFIG_NET_PREBUF, 20);

  if (!BASS_Init(-1, 44100, 0, nullptr, nullptr)) {
    yuri::error("BASS 初始化失败: error={}", BASS_ErrorGetCode());
    return false;
  }

  if (!loadFlacPlugin()) {
    BASS_Free();
    return false;
  }

  initialized_ = true;
  return true;
}

bool BassPlayer::loadFlacPlugin() {
#ifdef _WIN32
  flac_plugin = BASS_PluginLoad(L"bassflac.dll", BASS_UNICODE);
#else
  flac_plugin = BASS_PluginLoad("libbassflac.so", 0);
#endif
  if (flac_plugin == 0) {
    yuri::error("BASSFLAC 插件加载失败: error={}", BASS_ErrorGetCode());
    return false;
  }
  return true;
}

bool BassPlayer::installPlaybackSyncs() {
  constexpr DWORD kEndSyncType = BASS_SYNC_END | BASS_SYNC_ONETIME; // 单次播放完成事件
  end_sync = BASS_ChannelSetSync(stream_, kEndSyncType, 0, &BassPlayer::onPlaybackEnded, this);
  if (end_sync == 0) {
    yuri::error("BASS 注册播放完成事件失败: error={}", BASS_ErrorGetCode());
    return false;
  }

  stall_sync =
    BASS_ChannelSetSync(stream_, BASS_SYNC_STALL, 0, &BassPlayer::onPlaybackStalled, this);
  if (stall_sync == 0) {
    yuri::error("BASS 注册播放缓冲事件失败: error={}", BASS_ErrorGetCode());
    BASS_ChannelRemoveSync(stream_, end_sync);
    end_sync = 0;
    return false;
  }
  return true;
}

void CALLBACK BassPlayer::onPlaybackEnded(const HSYNC, const DWORD, const DWORD, void *const user) {
  if (user == nullptr) {
    return;
  }

  auto *const player = static_cast<BassPlayer *>(user);
  player->setPlaybackState(PlaybackState::stopped);
  player->finished.emit();
}

void CALLBACK
BassPlayer::onPlaybackStalled(const HSYNC, const DWORD, const DWORD data, void *const user) {
  if (user == nullptr) {
    return;
  }

  auto *const player = static_cast<BassPlayer *>(user);
  const auto current_state = player->playback_state.load();
  if (current_state == PlaybackState::paused || current_state == PlaybackState::stopped) {
    return;
  }
  player->setPlaybackState(data == 0 ? PlaybackState::buffering : PlaybackState::playing);
}

void BassPlayer::setPlaybackState(const PlaybackState state) {
  if (playback_state.exchange(state) == state) {
    return;
  }
  stateChanged.emit(state);
}

void BassPlayer::updateDurationSeconds() {
  duration_seconds = 0.0;
  const auto len = BASS_ChannelGetLength(stream_, BASS_POS_BYTE);
  if (len != static_cast<QWORD>(-1)) {
    duration_seconds = BASS_ChannelBytes2Seconds(stream_, len);
  }
}

void BassPlayer::releaseStream() {
  if (stream_ != 0) {
    if (end_sync != 0) {
      BASS_ChannelRemoveSync(stream_, end_sync);
      end_sync = 0;
    }
    if (stall_sync != 0) {
      BASS_ChannelRemoveSync(stream_, stall_sync);
      stall_sync = 0;
    }
    BASS_ChannelStop(stream_);
    BASS_StreamFree(stream_);
    stream_ = 0;
  }
  duration_seconds = 0.0;
}

// 导出player
export BassPlayer bass24_player;

} // namespace bass24
