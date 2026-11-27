//
// Created by love-yuri on 2026/11/26.
//

export module models:playlist_info;

import std;
import qq_music_api;

export {
  /**
   * 歌单信息结构体
   */
  struct PlayListInfo {
    std::uint64_t tid; // 歌单 tid，请求歌单详情时必需
    int dir_id {};     // 目录 id，增删歌单歌曲时使用
    std::string name;  // 歌单名称
    std::string cover; // 歌单封面
    int song_count {}; // 歌曲总数

    /**
     * 创建歌单信息
     * @param tid 歌单 tid
     */
    explicit PlayListInfo(const std::uint64_t tid) : tid(tid) {
    }

    /**
     * 使用qq音乐api返回的信息直接构造
     * @param playlist api返回的歌单信息
     */
    static PlayListInfo from_api(const DisslistType &playlist);
  };
}

PlayListInfo PlayListInfo::from_api(const DisslistType &playlist) {
  PlayListInfo info { playlist.tid };
  info.dir_id = playlist.dirid;
  info.name = playlist.diss_name;
  info.cover = playlist.diss_cover;
  info.song_count = playlist.song_cnt;
  return info;
}
