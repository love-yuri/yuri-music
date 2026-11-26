//
// Created by love-yuri on 2026/11/25.
//

export module player:bass24_player;

import :audio_player;
import :bass24;
import models;
import std;

namespace player {

/** 使用 BASS 24 实现的基础音频播放器。 */
export class Bass24Player final : public AudioPlayer {
public:
  /**
   * 打开音源并替换当前歌曲。
   * @param song_info 歌曲信息
   * @param source 已解析的本地路径或网络音频 URL
   */
  void play_song(const SongInfo &song_info, AudioSource source) override;

  /** 启动或恢复当前歌曲。 */
  void play() override;

  /** 暂停当前歌曲。 */
  void pause() override;

  /** 停止播放并释放音频流。 */
  void stop() override;

  /**
   * 跳转到指定位置。
   * @param seconds 目标位置距歌曲起点的秒数
   * @return 是否成功跳转
   */
  bool seek(double seconds) override;

  /**
   * 设置当前音量并保存到下一首歌曲。
   * @param volume 目标音量
   */
  void set_volume(float volume) override;

  /**
   * 查询当前播放进度。
   * @return 当前播放位置，单位为秒
   */
  [[nodiscard]] double position() const override;

  /**
   * 查询当前歌曲时长。
   * @return 当前歌曲时长，单位为秒，未知时为 0
   */
  [[nodiscard]] double duration() const override;

private:
  bass24::Bass24 bass_;               // BASS 音频流
  std::optional<AudioSource> source_; // 停止后可以重新打开的音源
  float volume_ { 1.0f };             // 跨歌曲保留的音量
};

void Bass24Player::play_song(const SongInfo &song_info, AudioSource source) {
  bass_.close();
  set_state(PlaybackState::loading);
  set_current_song(std::make_shared<const SongInfo>(song_info));

  source_ = std::move(source);
  const auto *const path = std::get_if<std::filesystem::path>(&*source_);
  const bool opened =
    path ? bass_.open_file(*path) : bass_.open_url(std::get<std::string>(*source_));
  if (
    !opened || !bass_.set_volume(volume_) || !bass_.watch_end([this] {
      stop();
      finished.emit();
    })
    || !bass_.play(false)
  ) {
    bass_.close();
    source_.reset();
    set_state(PlaybackState::error);
    return;
  }

  set_state(PlaybackState::playing);
}

void Bass24Player::play() {
  if (state() == PlaybackState::paused) {
    set_state(bass_.play(false) ? PlaybackState::playing : PlaybackState::error);
  } else if (state() == PlaybackState::stopped || state() == PlaybackState::error) {
    if (const auto song = current_song()) {
      if (source_) {
        play_song(*song, *source_);
      } else {
        play_song_auto(*song);
      }
    }
  }
}

void Bass24Player::pause() {
  if (state() == PlaybackState::playing && bass_.pause()) {
    set_state(PlaybackState::paused);
  }
}

void Bass24Player::stop() {
  bass_.close();
  set_state(PlaybackState::stopped);
}

bool Bass24Player::seek(const double seconds) {
  return bass_.set_position_sec(seconds);
}

void Bass24Player::set_volume(const float volume) {
  if (volume != volume_ && (!bass_.is_open() || bass_.set_volume(volume))) {
    volume_ = volume;
    volume_changed.emit(volume_);
  }
}

double Bass24Player::position() const {
  return bass_.position_sec();
}

double Bass24Player::duration() const {
  if (const auto seconds = bass_.duration_sec(); seconds > 0) {
    return seconds;
  }
  const auto song = current_song();
  return song ? song->duration : 0;
}

/**
 * 全局player
 */
export Bass24Player &bass24_player() {
  static Bass24Player instance;
  return instance;
}

} // namespace player
