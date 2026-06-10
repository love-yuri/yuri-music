//
// Created by love-yuri on 2026/5/22.
//

export module models:song;

import std;

export {

  /**
   * 歌曲信息结构体
   */
  struct SongInfo {
    std::string title{};      // 歌曲标题
    std::string artist{};     // 歌手名
    std::string album_name{}; // 专辑名
    std::string duration{};   // 时长
    std::string mid{};        // 歌曲 mid
    std::string album_mid{};  // 专辑 mid
    bool has_flac{};          // 是否存在 flac 音源
    bool has_ape{};           // 是否存在 ape 音源
    bool has_mp3_320{};       // 是否存在 320k mp3 音源
    bool has_mp3_128{};       // 是否存在 128k mp3 音源
    bool liked = false;       // 是否已喜欢
  };
}
