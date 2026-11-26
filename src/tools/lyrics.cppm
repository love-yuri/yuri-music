module;
#ifdef __linux__
#include <glib.h>
#include <glaze/glaze.hpp>
#endif

export module lyrics;

#ifdef __linux__
import std;
import qq_music_api;

export namespace lyrics {

/** 一条带时间戳的 LRC 歌词。 */
struct Line {
  std::int64_t time_ms; // 歌词开始时间（毫秒）
  std::string text;     // 歌词文本，空行表示清屏
};

/**
 * 获取 QQ 音乐提供的 LRC 歌词。
 * @param mid 歌曲 mid
 * @return 解码后的歌词文本，下载或解析失败时为空
 */
std::string fetch_lrc(std::string_view mid);

/**
 * 解析 LRC 歌词，展开重复时间标签并按播放时间排序。
 * @param lrc UTF-8 歌词原文
 * @return 排好序的歌词行
 */
std::vector<Line> parse_lrc(std::string_view lrc);

/**
 * 查询当前播放时间对应的歌词行。
 * @param lines 已按时间排序的歌词
 * @param position_ms 播放时间（毫秒）
 * @return 当前歌词，尚未开始时为空
 */
std::string_view line_at(const std::vector<Line> &lines, std::int64_t position_ms);

} // namespace lyrics

/**
 * 解析完整的十进制整数。
 * @param text 数字文本
 * @return 解析结果，格式错误时为空
 */
std::optional<int> parse_integer(std::string_view text);

/**
 * 将 LRC 时间标签转换为毫秒。
 * @param tag LRC 时间标签内容，不含方括号
 * @return 时间戳，格式错误时为空
 */
std::optional<std::int64_t> parse_timestamp(std::string_view tag);

std::optional<int> parse_integer(std::string_view text) {
  if (!text.empty() && text.front() == '+') {
    text.remove_prefix(1);
  }
  if (text.empty()) {
    return std::nullopt;
  }
  int value = 0;
  const auto *const end = text.data() + text.size();
  const auto [ptr, error] = std::from_chars(text.data(), end, value);
  if (error != std::errc {} || ptr != end) {
    return std::nullopt;
  }
  return value;
}

std::optional<std::int64_t> parse_timestamp(const std::string_view tag) {
  const auto colon = tag.find(':');
  if (colon == std::string_view::npos) {
    return std::nullopt;
  }
  const auto seconds_tag = tag.substr(colon + 1);
  const auto dot = seconds_tag.find('.');
  const auto minutes = parse_integer(tag.substr(0, colon));
  const auto seconds = parse_integer(seconds_tag.substr(0, dot));
  if (!minutes || *minutes < 0 || !seconds || *seconds < 0 || *seconds >= 60) {
    return std::nullopt;
  }

  std::int64_t millis = (static_cast<std::int64_t>(*minutes) * 60 + *seconds) * 1000;
  if (dot != std::string_view::npos) {
    const auto fraction = seconds_tag.substr(dot + 1);
    if (
      fraction.empty() || fraction.size() > 3
      || fraction.find_first_not_of("0123456789") != std::string_view::npos
    ) {
      return std::nullopt;
    }
    int fraction_ms = *parse_integer(fraction);
    for (auto digits = fraction.size(); digits < 3; ++digits) {
      fraction_ms *= 10;
    }
    millis += fraction_ms;
  }
  return millis;
}

std::string lyrics::fetch_lrc(const std::string_view mid) {
  const auto response = curl::get(
    "https://c.y.qq.com/lyric/fcgi-bin/fcg_query_lyric_new.fcg",
    { { "Referer", "https://y.qq.com/" } },
    { { "songmid", mid }, { "format", "json" }, { "platform", "yqq.json" }, { "nobase64", "0" } }
  );
  if (!response) {
    return {};
  }

  struct LyricResponse {
    int retcode = -1;  // QQ 音乐返回码
    std::string lyric; // Base64 编码的歌词
  } data;
  if (
    glz::read<glz::opts { .error_on_unknown_keys = false }>(data, *response) || data.retcode != 0
    || data.lyric.empty()
  ) {
    return {};
  }

  gsize size = 0;
  const std::unique_ptr<guchar, decltype(&g_free)> decoded(
    g_base64_decode(data.lyric.c_str(), &size), g_free
  );
  return { reinterpret_cast<const char *>(decoded.get()), size };
}

std::vector<lyrics::Line> lyrics::parse_lrc(std::string_view lrc) {
  if (lrc.empty() || !g_utf8_validate_len(lrc.data(), lrc.size(), nullptr)) {
    return {};
  }
  if (lrc.starts_with("\xef\xbb\xbf")) {
    lrc.remove_prefix(3);
  }
  std::vector<Line> lines;
  int offset_ms = 0;
  for (const auto line_range : lrc | std::views::split('\n')) {
    std::string_view line(line_range.begin(), line_range.end());
    if (!line.empty() && line.back() == '\r') {
      line.remove_suffix(1);
    }
    if (line.starts_with("[offset:") && line.ends_with(']')) {
      offset_ms = parse_integer(line.substr(8, line.size() - 9)).value_or(offset_ms);
      continue;
    }

    const auto first = lines.size();
    while (line.starts_with('[')) {
      const auto end = line.find(']');
      if (end == std::string_view::npos) {
        break;
      }
      const auto time = parse_timestamp(line.substr(1, end - 1));
      if (!time) {
        break;
      }
      lines.push_back({ *time, {} });
      line.remove_prefix(end + 1);
    }
    const auto start = line.find_first_not_of(" \t");
    const auto end = line.find_last_not_of(" \t");
    const auto text =
      start == std::string_view::npos ? std::string_view {} : line.substr(start, end - start + 1);
    for (auto &entry : std::span(lines).subspan(first)) {
      entry.text = text;
    }
  }
  for (auto &[time_ms, text] : lines) {
    time_ms -= offset_ms;
  }
  std::ranges::stable_sort(lines, {}, &Line::time_ms);
  return lines;
}

std::string_view lyrics::line_at(const std::vector<Line> &lines, const std::int64_t position_ms) {
  const auto next = std::ranges::upper_bound(lines, position_ms, {}, &Line::time_ms);
  return next == lines.begin() ? std::string_view {} : std::prev(next)->text;
}

#endif
