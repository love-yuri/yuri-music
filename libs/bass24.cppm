//
// Created by love-yuri on 2026/11/25.
//

module;
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "bass.h"

module player:bass24;

import std;
import yuri.ui.runtime;

namespace bass24 {

/** BASS 通道的活动状态。 */
enum class ChannelActivity : DWORD {
  stopped = BASS_ACTIVE_STOPPED,             // 已停止
  playing = BASS_ACTIVE_PLAYING,             // 正在播放
  stalled = BASS_ACTIVE_STALLED,             // 等待数据
  paused = BASS_ACTIVE_PAUSED,               // 已暂停
  paused_device = BASS_ACTIVE_PAUSED_DEVICE, // 设备已暂停
};

/** BASS 设备和音频流的调用封装。 */
class Bass24 {
public:
  /** 创建尚未打开音频流的封装实例。 */
  Bass24() = default;

  /** 释放音频流。 */
  ~Bass24();

  /**
   * 禁止复制音频流句柄。
   * @param other 原实例
   */
  Bass24(const Bass24 &other) = delete;

  /**
   * 禁止复制赋值音频流句柄。
   * @param other 原实例
   * @return 当前实例的引用
   */
  Bass24 &operator=(const Bass24 &other) = delete;

  /**
   * 打开本地音频文件流。
   * @param path 音频文件路径
   * @return 是否成功打开
   */
  [[nodiscard]] bool open_file(const std::filesystem::path &path);

  /**
   * 打开网络音频流。
   * @param url 音频流 URL
   * @return 是否成功打开
   */
  [[nodiscard]] bool open_url(const std::string &url);

  /** 释放当前音频流。 */
  void close();

  /**
   * 在 UI 线程报告当前音频流自然结束。
   * @param callback 自然结束时调用的函数
   * @return 是否注册成功
   */
  [[nodiscard]] bool watch_end(std::function<void()> callback);

  /**
   * 查询是否持有音频流。
   * @return 是否已打开音频流
   */
  [[nodiscard]] bool is_open() const;

  /**
   * 调用 BASS 启动当前通道。
   * @param restart 是否从头播放
   * @return 是否成功启动播放
   */
  [[nodiscard]] bool play(bool restart) const;

  /**
   * 暂停当前音频流。
   * @return 是否成功暂停播放
   */
  [[nodiscard]] bool pause() const;

  /**
   * 查询 BASS 通道的活动状态。
   * @return 通道的活动状态，没有打开流时为已停止
   */
  [[nodiscard]] ChannelActivity activity() const;

  /**
   * 按秒设置通道位置。
   * @param seconds 目标位置，达到流末尾时定位到最后一个有效位置
   * @return 是否成功设置位置
   */
  [[nodiscard]] bool set_position_sec(double seconds) const;

  /**
   * 设置当前音频流的通道音量。
   * @param volume BASS 通道音量
   * @return 是否成功设置音量
   */
  [[nodiscard]] bool set_volume(double volume) const;

  /**
   * 查询当前播放位置。
   * @return 当前播放位置，单位为秒，查询失败时为 0
   */
  [[nodiscard]] double position_sec() const;

  /**
   * 查询当前流的总时长。
   * @return 总时长，单位为秒，查询失败时为 0
   */
  [[nodiscard]] double duration_sec() const;

  /**
   * 查询当前播放进度。
   * @return 百分比，范围为 0 到 100，计算失败时为 0
   */
  [[nodiscard]] double progress_percent() const;

private:
  class OutputDevice;

  /**
   * 将 BASS 播完通知投递到 UI 线程。
   * @param channel 播完的音频流句柄
   * @param user 当前 BASS 封装实例
   */
  static void CALLBACK on_end(HSYNC /*unused*/, DWORD channel, DWORD /*unused*/, void *user);

  /**
   * 查询通道的字节位置。
   * @return 字节位置，查询失败时为 0
   */
  [[nodiscard]] QWORD position_bytes() const;

  /**
   * 查询通道的字节长度。
   * @return 字节长度，查询失败时为 0
   */
  [[nodiscard]] QWORD length_bytes() const;

  /**
   * 根据通道格式将字节数转换为秒。
   * @param bytes 待转换的字节数
   * @return 对应的秒数，转换失败时为 0
   */
  [[nodiscard]] double bytes_to_seconds(QWORD bytes) const;

  /**
   * 获取并选中共享的 BASS 输出设备。
   * @return 是否可以在当前线程打开音频流
   */
  [[nodiscard]] bool select_output_device();

  std::shared_ptr<OutputDevice> device_;                       // 共享的 BASS 输出设备
  std::atomic<HSTREAM> stream_;                                // 当前音频流句柄
  std::atomic<std::shared_ptr<std::function<void()>>> on_end_; // 当前音频流的播完回调
};

/** 管理默认 BASS 输出设备和 FLAC 插件。 */
class Bass24::OutputDevice {
public:
  /** 释放已加载的插件和本实例初始化的设备。 */
  ~OutputDevice();

  /**
   * 初始化设备，并将其设为调用线程的当前设备。
   * @return 是否成功选中设备
   */
  [[nodiscard]] bool select();

private:
  std::mutex mutex_;               // 保护共享设备初始化
  std::optional<DWORD> device_id_; // BASS 输出设备编号
  HPLUGIN flac_plugin_ {};         // FLAC 插件句柄
  bool owns_device_ {};            // 是否由本实例负责释放设备
};

} // namespace bass24

constexpr QWORD kInvalidBytePosition = static_cast<QWORD>(-1); // BASS 字节查询失败值

/**
 * 读取 BASS 字节查询结果。
 * @param bytes BASS 返回的位置或长度
 * @return 字节数，查询失败时为 0
 */
QWORD bytes_or_zero(const QWORD bytes) {
  return bytes == kInvalidBytePosition ? 0 : bytes;
}

bass24::Bass24::~Bass24() {
  close();
}

bool bass24::Bass24::open_file(const std::filesystem::path &path) {
  close();
  if (path.empty() || !select_output_device()) {
    return false;
  }

#ifdef _WIN32
  stream_ = BASS_StreamCreateFile(BASS_FILE_NAME, path.c_str(), 0, 0, BASS_UNICODE);
#else
  stream_ = BASS_StreamCreateFile(BASS_FILE_NAME, path.c_str(), 0, 0, 0);
#endif
  return is_open();
}

bool bass24::Bass24::open_url(const std::string &url) {
  close();
  if (url.empty() || !select_output_device()) {
    return false;
  }
  stream_ = BASS_StreamCreateURL(url.c_str(), 0, 0, nullptr, nullptr);
  return is_open();
}

void bass24::Bass24::close() {
  on_end_.store(nullptr);
  if (const auto stream = stream_.exchange(0)) {
    BASS_StreamFree(stream);
  }
}

bool bass24::Bass24::watch_end(std::function<void()> callback) {
  on_end_.store(std::make_shared<std::function<void()>>(std::move(callback)));
  return BASS_ChannelSetSync(stream_, BASS_SYNC_END | BASS_SYNC_ONETIME, 0, on_end, this) != 0;
}

void CALLBACK bass24::Bass24::on_end(HSYNC /*unused*/, const DWORD channel, DWORD /*unused*/, void *const user) {
  const auto &bass = *static_cast<const Bass24 *>(user);
  const std::weak_ptr callback = bass.on_end_.load();
  if (bass.stream_.load() == channel) {
    ui::dispatcher.post([callback] {
      if (const auto notify = callback.lock()) {
        (*notify)();
      }
    });
  }
}

bool bass24::Bass24::is_open() const {
  return stream_ != 0;
}

bool bass24::Bass24::play(const bool restart) const {
  return is_open() && BASS_ChannelPlay(stream_, restart);
}

bool bass24::Bass24::pause() const {
  return is_open() && BASS_ChannelPause(stream_);
}

bass24::ChannelActivity bass24::Bass24::activity() const {
  return is_open() ? static_cast<ChannelActivity>(BASS_ChannelIsActive(stream_)) :
                     ChannelActivity::stopped;
}

bool bass24::Bass24::set_position_sec(const double seconds) const {
  if (!is_open()) {
    return false;
  }
  const QWORD target = BASS_ChannelSeconds2Bytes(stream_, std::max(0.0, seconds));
  if (target == kInvalidBytePosition) {
    return false;
  }
  // BASS 的字节定位要求偏移小于已知的流长度。
  const QWORD length = length_bytes();
  const QWORD position = length == 0 ? target : std::min(target, length - 1);
  return BASS_ChannelSetPosition(stream_, position, BASS_POS_BYTE);
}

bool bass24::Bass24::set_volume(const double volume) const {
  return is_open() && BASS_ChannelSetAttribute(stream_, BASS_ATTRIB_VOL, volume);
}

double bass24::Bass24::position_sec() const {
  const auto stream = stream_.load();
  if (!stream) {
    return 0;
  }
  const auto bytes = BASS_ChannelGetPosition(stream, BASS_POS_BYTE);
  return bytes == kInvalidBytePosition ? 0 :
                                         std::max(0.0, BASS_ChannelBytes2Seconds(stream, bytes));
}

double bass24::Bass24::duration_sec() const {
  return bytes_to_seconds(length_bytes());
}

double bass24::Bass24::progress_percent() const {
  const QWORD length = length_bytes();
  if (length == 0) {
    return 0.0f;
  }
  const QWORD position = std::min(position_bytes(), length);
  return 100.0 * static_cast<double>(position) / static_cast<double>(length);
}

QWORD bass24::Bass24::position_bytes() const {
  return is_open() ? bytes_or_zero(BASS_ChannelGetPosition(stream_, BASS_POS_BYTE)) : 0;
}

QWORD bass24::Bass24::length_bytes() const {
  return is_open() ? bytes_or_zero(BASS_ChannelGetLength(stream_, BASS_POS_BYTE)) : 0;
}

double bass24::Bass24::bytes_to_seconds(const QWORD bytes) const {
  return bytes == 0 ? 0.0f :
                      static_cast<double>(std::max(0.0, BASS_ChannelBytes2Seconds(stream_, bytes)));
}

bool bass24::Bass24::select_output_device() {
  static const auto device = std::make_shared<OutputDevice>(); // 进程内共享的默认输出设备
  device_ = device;
  return device_->select();
}

bass24::Bass24::OutputDevice::~OutputDevice() {
  if (flac_plugin_ != 0) {
    BASS_PluginFree(flac_plugin_);
  }
  if (owns_device_ && device_id_ && BASS_SetDevice(*device_id_)) {
    BASS_Free();
  }
}

bool bass24::Bass24::OutputDevice::select() {
  const std::lock_guard lock(mutex_);
  if (device_id_ && BASS_SetDevice(*device_id_)) {
    return true;
  }
  device_id_.reset();

  owns_device_ = BASS_Init(-1, 44100, 0, nullptr, nullptr);
  if (!owns_device_ && BASS_ErrorGetCode() != BASS_ERROR_ALREADY) {
    return false;
  }
  const DWORD device_id = BASS_GetDevice();
  if (device_id == static_cast<DWORD>(-1)) {
    if (owns_device_) {
      BASS_Free();
      owns_device_ = false;
    }
    return false;
  }
  device_id_ = device_id;

  if (flac_plugin_ == 0) {
#ifdef _WIN32
    flac_plugin_ = BASS_PluginLoad("bassflac.dll", 0);
#else
    flac_plugin_ = BASS_PluginLoad("libbassflac.so", 0);
#endif
  }
  return true;
}
