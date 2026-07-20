//
// Created by Codex on 2026/6/24.
//
export module store:user_profile_store;

import std;
import thread_pool;
import qq_music_api;
import core;
import yuri_log;

std::string displayNameFrom(const qqmusic_api::user::UserInfoResult &user_info) {
  if (!user_info.data.creator.nick.empty()) {
    return user_info.data.creator.nick;
  }
  return "QQ 音乐用户";
}

namespace store {

class UserProfileStore {
public:
  Signal<> status_changed; // 状态改变事件

  /**
   * 重新加载信息
   */
  void reload() noexcept;

  /**
   * @return 展示名称
   */
  std::string_view displayName() const noexcept {
    return display_name;
  }

  /**
   * @return 是否拥有可用登录状态
   */
  bool loggedIn() const noexcept {
    return logged_in;
  }

  /**
   * @return 状态信息
   */
  std::string_view status() const noexcept {
    return status_text;
  }

  /**
   * @return 头像url
   */
  std::string_view avatarUrl() const noexcept {
    return avatar_url;
  }

private:
  bool logged_in = false;                             // 是否拥有可用登录态
  std::atomic_bool loading_ = false;                  // 是否正在验证登录态
  std::string display_name = "未登录";                // 展示名称
  std::string status_text = "登录后显示 QQ 音乐昵称"; // 登录/错误状态
  std::string avatar_url{};                           // 头像链接
};

void UserProfileStore::reload() noexcept {
  thread_manager->addTask([this] {
    try {
      qqmusic_api_config.loginFromFile();
      // ReSharper disable once CppTooWideScopeInitStatement
      const auto user_info = qqmusic_api::user::get_user_info();
      if (user_info.code == 0 && user_info.subcode == 0) {
        qqmusic_api_config.has_login = true;
        logged_in = true;
        display_name = displayNameFrom(user_info);
        avatar_url = user_info.data.creator.headpic;
        status_text = "";
      } else {
        qqmusic_api_config.has_login = false;
        status_text = "登录状态已失效，请重新登录";
      }
    } catch (const std::exception &e) {
      qqmusic_api_config.has_login = false;
      status_text = "登录状态验证失败，请稍后重试";
      yuri::warn("验证 QQ 音乐登录状态失败: {}", e.what());
    }
    status_changed.emit();
  });
}

export UserProfileStore user_profile_store;

} // namespace store
