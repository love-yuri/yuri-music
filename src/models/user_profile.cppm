//
// Created by Codex on 2026/6/24.
//
export module models:user_profile;

import std;
import thread_pool;
import qq_music_api;
import yuri_log;

namespace {

std::string trimCookiePart(std::string_view value) {
  while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
    value.remove_prefix(1);
  }
  while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
    value.remove_suffix(1);
  }
  return std::string(value);
}

std::string cookieValue(std::string_view cookie, std::string_view name) {
  std::size_t pos = 0;
  while (pos < cookie.size()) {
    const auto end = cookie.find(';', pos);
    const auto part = cookie.substr(pos, end == std::string_view::npos ? cookie.size() - pos : end - pos);
    const auto eq = part.find('=');
    if (eq != std::string_view::npos) {
      const auto key = trimCookiePart(part.substr(0, eq));
      if (key == name) {
        return trimCookiePart(part.substr(eq + 1));
      }
    }

    if (end == std::string_view::npos) {
      break;
    }
    pos = end + 1;
  }
  return {};
}

int hexValue(char ch) {
  if (ch >= '0' && ch <= '9') {
    return ch - '0';
  }
  if (ch >= 'a' && ch <= 'f') {
    return ch - 'a' + 10;
  }
  if (ch >= 'A' && ch <= 'F') {
    return ch - 'A' + 10;
  }
  return -1;
}

std::string percentDecode(std::string_view value) {
  std::string decoded;
  decoded.reserve(value.size());
  for (std::size_t i = 0; i < value.size(); ++i) {
    if (value[i] == '%' && i + 2 < value.size()) {
      const int hi = hexValue(value[i + 1]);
      const int lo = hexValue(value[i + 2]);
      if (hi >= 0 && lo >= 0) {
        decoded.push_back(static_cast<char>((hi << 4) | lo));
        i += 2;
        continue;
      }
    }
    decoded.push_back(value[i] == '+' ? ' ' : value[i]);
  }
  return decoded;
}

std::string firstUtf8Char(std::string_view text) {
  if (text.empty()) {
    return {};
  }
  const auto ch = static_cast<unsigned char>(text.front());
  std::size_t size = 1;
  if ((ch & 0b1110'0000) == 0b1100'0000) {
    size = 2;
  } else if ((ch & 0b1111'0000) == 0b1110'0000) {
    size = 3;
  } else if ((ch & 0b1111'1000) == 0b1111'0000) {
    size = 4;
  }
  return std::string(text.substr(0, std::min(size, text.size())));
}

std::string nicknameFromCookie(std::string_view cookie) {
  for (const std::string_view key : {"qqmusic_nick", "nickname", "nick", "wxnickname"}) {
    if (auto value = cookieValue(cookie, key); !value.empty()) {
      return percentDecode(value);
    }
  }
  return {};
}

std::string profileDisplayName() {
  if (auto nickname = nicknameFromCookie(qqmusic_api_config.cookie); !nickname.empty()) {
    return nickname;
  }
  return "QQ 音乐用户";
}

std::string profileAvatarLabel(const std::string &display_name) {
  if (display_name.empty()) {
    return "音";
  }
  if (display_name == "QQ 音乐用户") {
    return "音";
  }
  return firstUtf8Char(display_name);
}

} // namespace

export namespace models {

struct UserProfile {
  bool logged_in = false;        // 是否拥有可用登录态
  bool loading = false;          // 是否正在刷新远端资料
  std::string qq{};              // QQ 号
  std::string display_name{};    // 展示名称
  std::string status_text{};     // 登录/错误状态
  std::string avatar_label{};    // 头像文字
  int playlist_count = 0;        // 歌单数量
  int song_count = 0;            // 歌单歌曲合计
};

class UserProfileStore {
public:
  [[nodiscard]] UserProfile snapshot() const {
    std::lock_guard lock(mutex_);
    return profile_;
  }

  void refreshAsync(bool force = false) {
    const auto key = currentConfigKey();
    if (!qqmusic_api_config.has_login || qqmusic_api_config.qq.empty() || qqmusic_api_config.cookie.empty()) {
      std::lock_guard lock(mutex_);
      if (force || profile_.logged_in || profile_.loading || loaded_key_ != key) {
        profile_ = {};
        profile_.display_name = "未登录";
        profile_.status_text = "登录后显示 QQ 音乐个人信息";
        profile_.avatar_label = "未";
        loaded_key_ = key;
      }
      return;
    }

    const auto display_name = profileDisplayName();
    {
      std::lock_guard lock(mutex_);
      if (!force && loaded_key_ == key && !profile_.loading) {
        return;
      }
      profile_.loading = true;
      profile_.logged_in = true;
      profile_.qq = qqmusic_api_config.qq;
      profile_.display_name = display_name;
      profile_.status_text = std::format("QQ {}", qqmusic_api_config.qq);
      profile_.avatar_label = profileAvatarLabel(display_name);
      loaded_key_ = key;
    }

    bool expected = false;
    if (!refreshing_.compare_exchange_strong(expected, true)) {
      return;
    }

    thread_manager->addTask([this, key, display_name] {
      UserProfile next;
      next.logged_in = true;
      next.qq = qqmusic_api_config.qq;
      next.display_name = display_name;
      next.status_text = std::format("QQ {}", next.qq);
      next.avatar_label = profileAvatarLabel(display_name);

      try {
        const auto playlists = qqmusic_api::playlist::get_user_playlists(50);
        next.playlist_count = playlists.data.totoal > 0
                                ? playlists.data.totoal
                                : static_cast<int>(playlists.data.disslist.size());
        for (const auto &playlist : playlists.data.disslist) {
          next.song_count += std::max(0, playlist.song_cnt);
        }
      } catch (const std::exception &e) {
        next.status_text = "个人信息读取失败，请稍后刷新";
        yuri::warn("刷新 QQ 音乐个人信息失败: {}", e.what());
      }

      {
        std::lock_guard lock(mutex_);
        if (loaded_key_ == key) {
          next.loading = false;
          profile_ = std::move(next);
        }
      }
      refreshing_.store(false);
    });
  }

private:
  static std::string currentConfigKey() {
    return std::format("{}:{}", qqmusic_api_config.qq, std::hash<std::string>{}(qqmusic_api_config.cookie));
  }

  mutable std::mutex mutex_{};
  UserProfile profile_{};
  std::string loaded_key_{};
  std::atomic_bool refreshing_ = false;
};

UserProfileStore &userProfileStore() {
  static UserProfileStore store;
  return store;
}

} // namespace models
