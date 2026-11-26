export module player:song_cache;

import qq_music_api;
import sqlite;
import std;

using namespace qqmusic_api::song;
namespace fs = std::filesystem;

namespace player {

/** 以歌曲 mid 和音源格式代码索引本地音频文件，数据库错误通过异常报告。 */
export class SongCache {
public:
  /**
   * 创建缓存中心，写入缓存时按需创建目录。
   * @param database 保存缓存索引的数据库，需在缓存中心的生命周期内保持打开
   * @param directory 保存音频文件的目录
   */
  explicit SongCache(sqlite::Database &database, const fs::path &directory);

  /**
   * 查找指定音源格式的缓存。
   * @param mid 歌曲mid
   * @param format 歌曲格式化信息
   * @return 命中的文件路径，未命中时为空路径
   */
  [[nodiscard]] fs::path find(const std::string &mid, const SongFileFormat &format) const;

  /**
   * 下载并登记歌曲缓存
   * @param mid 歌曲 mid
   * @param format 音源格式
   * @param url 已解析的歌曲下载 URL
   * @return 缓存文件路径，下载或文件操作失败时为空路径
   */
  [[nodiscard]] fs::path
  store(std::string_view mid, const SongFileFormat &format, std::string_view url) const;

private:
  sqlite::Database &database_; // 歌曲缓存索引
  fs::path directory_;         // 音频缓存目录的绝对路径
};

/**
 * 将缓存键编码成小写十六进制，在大小写不敏感的文件系统上保留原始键的差异。
 * @param key 歌曲 mid 或音源格式代码
 * @return 可用作文件名的编码
 */
std::string encode_key(const std::string_view key) {
  constexpr std::string_view kHex = "0123456789abcdef"; // 小写十六进制字符
  std::string encoded;
  encoded.reserve(key.size() * 2);
  for (const unsigned char byte : key) {
    encoded.push_back(kHex[byte >> 4]);
    encoded.push_back(kHex[byte & 0x0f]);
  }
  return encoded;
}

SongCache::SongCache(sqlite::Database &database, const fs::path &directory) :
  database_(database), directory_(fs::absolute(directory)) {
  // 先创建缓存目录
  fs::create_directory(directory_);
}

fs::path SongCache::find(const std::string &mid, const SongFileFormat &format) const {
  const auto file_name = database_.query_text(
    "SELECT file_name FROM song_cache WHERE mid = ? AND format = ?", { mid, format.e }
  );

  if (!file_name || file_name->empty()) {
    return {};
  }

  const auto path = directory_ / *file_name;
  return fs::is_regular_file(path) ? path : fs::path {};
}

fs::path SongCache::store(
  const std::string_view mid,
  const SongFileFormat &format,
  const std::string_view url
) const {
  const auto file_name = std::format("{}-{}.{}", encode_key(mid), encode_key(format.s), format.e);
  const auto path = directory_ / file_name;
  if (!curl::download_to_file(url, path)) {
    return {};
  }

  database_.execute(
    "INSERT INTO song_cache (mid, format, file_name) VALUES (?, ?, ?) "
    "ON CONFLICT (mid, format) DO UPDATE SET file_name = excluded.file_name",
    { mid, format.e, std::string_view(file_name) }
  );
  return path;
}

/**
 * 全局缓存
 */
export SongCache &song_cache() {
  static SongCache instance { sqlite::database(), "cache" };
  return instance;
}

} // namespace player
