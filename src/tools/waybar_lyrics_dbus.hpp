#pragma once

namespace waybar_lyrics {

inline constexpr char kBusName[] = "io.github.loveyuri.YuriMusic";          // D-Bus 服务名
inline constexpr char kPath[] = "/io/github/loveyuri/YuriMusic/Lyrics";     // 歌词对象路径
inline constexpr char kInterface[] = "io.github.loveyuri.YuriMusic.Lyrics"; // 歌词接口
inline constexpr char kGetLine[] = "GetLine";                               // 获取当前歌词的方法名

} // namespace waybar_lyrics
