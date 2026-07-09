//
// Created by Codex on 2026/6/24.
//
export module models:user_profile;

import std;
import thread_pool;
import qq_music_api;
import yuri_log;

namespace {

bool isUserInfoSuccess(const qqmusic_api::user::UserInfoResult &user_info) {
  return user_info.code == 0 && user_info.subcode == 0;
}

std::string displayNameFrom(const qqmusic_api::user::UserInfoResult &user_info) {
  if (!user_info.data.creator.nick.empty()) {
    return user_info.data.creator.nick;
  }
  return "QQ 音乐用户";
}

} // namespace

export namespace models {

struct UserProfile {
  bool logged_in = false;                            // 是否拥有可用登录态
  bool loading = false;                              // 是否正在验证登录态
  std::string display_name = "未登录";               // 展示名称
  std::string status_text = "登录后显示 QQ 音乐昵称"; // 登录/错误状态
  std::string avatar_url{};                          // 头像链接
};

class UserProfileStore {
public:
  [[nodiscard]] UserProfile snapshot() const {
    std::lock_guard lock(mutex_);
    return profile_;
  }

  void refreshAsync(const bool force = false) {
    const auto key = currentConfigKey();
    if (!qqmusic_api_config.has_login || qqmusic_api_config.cookie.empty()) {
      setLoggedOut(key, "登录后显示 QQ 音乐昵称");
      return;
    }

    {
      std::lock_guard lock(mutex_);
      if (!force && loaded_key_ == key && !profile_.loading) {
        return;
      }
    }

    bool expected = false;
    if (!refreshing_.compare_exchange_strong(expected, true)) {
      return;
    }

    {
      std::lock_guard lock(mutex_);
      profile_.loading = true;
      profile_.status_text = "正在验证 QQ 音乐登录状态";
      loaded_key_ = key;
    }

    thread_manager->addTask([this, key] {
      UserProfile next;
      try {
        const auto user_info = qqmusic_api::user::get_user_info();
        if (isUserInfoSuccess(user_info)) {
          qqmusic_api_config.has_login = true;
          next.logged_in = true;
          next.display_name = displayNameFrom(user_info);
          next.avatar_url = user_info.data.creator.headpic;
          next.status_text = "";
        } else {
          qqmusic_api_config.has_login = false;
          next.status_text = "登录状态已失效，请重新登录";
        }
      } catch (const std::exception &e) {
        qqmusic_api_config.has_login = false;
        next.status_text = "登录状态验证失败，请稍后重试";
        yuri::warn("验证 QQ 音乐登录状态失败: {}", e.what());
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
    return std::format("{}", std::hash<std::string>{}(qqmusic_api_config.cookie));
  }

  void setLoggedOut(const std::string &key, const std::string &status_text) {
    std::lock_guard lock(mutex_);
    profile_ = {};
    profile_.status_text = status_text;
    loaded_key_ = key;
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