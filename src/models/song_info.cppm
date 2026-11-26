//
// Created by love-yuri on 2026/5/22.
//

export module models:song_info;

import std;
import qq_music_api;

using namespace qqmusic_api::song;

/**
 * 拼接歌手名称。
 * @param singers 歌手列表
 * @return 使用分隔符拼接后的歌手名称
 */
std::string format_singers(const std::vector<SingerType> &singers);

/**
 * 格式化歌曲时长。
 * @param seconds 歌曲时长，单位为秒
 * @return m:ss 格式的时长文本
 */
std::string format_duration(int seconds);

export {
  /**
   * 歌曲信息结构体
   */
  struct SongInfo {
    std::string title;      // 歌曲标题
    std::string artist;     // 歌手名
    std::string album_name; // 专辑名
    int duration;           // 时长 单位s
    std::string mid;        // 歌曲 mid
    std::string album_mid;  // 专辑 mid
    bool has_flac {};       // 是否存在 flac 音源
    bool has_ape {};        // 是否存在 ape 音源
    bool has_mp3_320 {};    // 是否存在 320k mp3 音源
    bool has_mp3_128 {};    // 是否存在 128k mp3 音源
    bool liked = false;     // 是否已喜欢

    /* 默认==实现 */
    bool operator==(const SongInfo &) const = default;

    /**
     * 获取持续市场的文本
     */
    std::string duration_text() const {
      char buffer[8];
      auto [end, ec] =
        std::format_to_n(buffer, sizeof(buffer) - 1, "{}:{:02}", duration / 60, duration % 60);
      *end = '\0';
      return { buffer, static_cast<std::size_t>(end - buffer) };
    }

    /**
     * 使用qq音乐api返回的信息直接构造
     * @param music api返回的music信息
     */
    static SongInfo from_api(const SonglistType& music) {
      return SongInfo {
        .title = std::string(music.title),
        .artist = format_singers(music.singer),
        .album_name = std::string(music.album.name),
        .duration = music.interval,
        .mid = std::string(music.mid),
        .album_mid = std::string(music.album.mid),
        .has_flac = music.file.size_flac > 0,
        .has_ape = music.file.size_ape > 0,
        .has_mp3_320 = music.file.size_320mp3 > 0,
        .has_mp3_128 = music.file.size_128mp3 > 0,
        .liked = true,
      };
    }
  };
}


std::string format_singers(const std::vector<SingerType> &singers) {
  if (singers.empty()) {
    return {};
  }
  if (singers.size() == 1) {
    return singers.front().name;
  }

  std::size_t length = 3 * (singers.size() - 1);
  for (const auto &singer : singers) {
    length += singer.name.size();
  }

  std::string result;
  result.reserve(length);
  result.append(singers.front().name);
  for (std::size_t index = 1; index < singers.size(); ++index) {
    result.append(" / ");
    result.append(singers[index].name);
  }
  return result;
}

std::string format_duration(const int seconds) {
  char buffer[8];
  auto [end, ec] =
    std::format_to_n(buffer, sizeof(buffer) - 1, "{}:{:02}", seconds / 60, seconds % 60);
  *end = '\0';
  return { buffer, static_cast<std::size_t>(end - buffer) };
}