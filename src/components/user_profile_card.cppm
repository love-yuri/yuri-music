module;
#if defined(_WIN32)
#include "include/private/SkFloatingPoint.h"
#endif
//
// Created by Codex on 2026/6/24.
//
export module components:user_profile_card;

import std;
import yuri.ui;
import yuri.skia;
import models;
import yuri.core;
import qq_music_api;
import store;

using namespace ui::layout;
using namespace ui::widgets;
using namespace ui::render;
using namespace store;
using namespace skia;

constexpr SkColor kProfileCardBg = ColorFromARGB(150, 255, 255, 255);     // 卡片背景色
constexpr SkColor kProfileCardBorder = ColorFromARGB(116, 255, 255, 255); // 卡片边线色
constexpr SkColor kProfileName = ColorFromARGB(255, 22, 28, 38);          // 用户名颜色
constexpr SkColor kProfileMuted = ColorFromARGB(174, 58, 70, 88);         // 登录状态文字颜色
constexpr SkColor kProfileAvatarBg = ColorFromARGB(255, 38, 189, 220);    // 已登录的头像底色

/**
 * 取出文本开头的 UTF-8 字符，空文本返回“音”。
 * @param text UTF-8 文本
 * @return 头像占位用的字符
 */
std::string firstUtf8Char(std::string_view text);

/** 根据登录状态和用户名生成头像占位文字。 */
std::string avatarLabel();

export namespace components {

class UserProfileCard : public Box {
public:
  /**
   * 创建用户资料卡。
   * @param compact 是否使用紧凑布局
   * @param parent 父控件
   */
  explicit UserProfileCard(bool compact, Widget *parent = nullptr);

  /** 解除登录状态变化信号的订阅。 */
  ~UserProfileCard() override;

  /** 更新卡片背景、边框和文字的绘制范围。 */
  void layout_children() override;

  /**
   * 绘制头像、用户名和登录状态。
   * @param canvas 目标画布
   */
  void paint(SkCanvas *canvas) override;

private:
  /** 刷新用户资料文字和头像。 */
  void updateTextNodes();

  /**
   * 异步加载头像图片。
   * @param url 头像地址
   */
  void loadAvatarImage(std::string_view url);

  /**
   * 将已加载的头像图片绘制到圆形区域。
   * @param canvas 目标画布
   * @param rect 头像区域
   * @return 是否绘制了头像图片
   */
  bool drawAvatarImage(SkCanvas *canvas, const SkRect &rect) const;

  /**
   * 绘制头像或占位文字。
   * @param canvas 目标画布
   * @param rect 头像区域
   */
  void drawAvatar(SkCanvas *canvas, const SkRect &rect);

  bool compact_ = true;          // 是否使用紧凑布局
  RenderText name_text;          // 用户名
  RenderText status_text;        // 登录状态文字
  RenderText avatar_text;        // 头像占位文字
  std::string avatar_url{};      // 当前头像地址
  sk_sp<SkImage> avatar_image{}; // 已加载的头像图片
};

UserProfileCard::UserProfileCard(const bool compact, Widget *parent) :
  Box(parent), compact_(compact) {
  radius = compact_ ? 0.0f : 18.0f;
  render_bg.set_color(kProfileCardBg);
  render_border.setColor(kProfileCardBorder);
  render_border.setWidth(1.0f);

  name_text.set_font_size(compact_ ? 14.0f : 26.0f);
  name_text.set_color(kProfileName);
  name_text.set_alignment(Alignment::CenterLeft);

  status_text.set_font_size(14.0f);
  status_text.set_color(kProfileMuted);
  status_text.set_alignment(Alignment::CenterLeft);

  avatar_text.set_font_size(compact_ ? 14.0f : 24.0f);
  avatar_text.set_color(skia_colors::white);
  avatar_text.set_alignment(Alignment::Center);

  if (compact_) {
    set_fixed_height(54.0f);
  } else {
    set_max_height(232.0f);
    set_min_height(206.0f);
  }

  user_profile_store.status_changed.connect<&UserProfileCard::updateTextNodes>(this);
  updateTextNodes();
}

UserProfileCard::~UserProfileCard() {
  user_profile_store.status_changed.disconnect(this);
}

void UserProfileCard::layout_children() {
  render_bg.update(local_bounds());
  render_border.update(local_bounds());

  if (compact_) {
    constexpr float kTextX = 46.0f;
    const float text_w = std::max(0.0f, width_ - kTextX - 2.0f);
    name_text.update(SkRect::MakeXYWH(kTextX, 0.0f, text_w, height_));
  } else {
    constexpr float kTextX = 116.0f;
    const float text_w = std::max(0.0f, width_ - kTextX - 30.0f);
    name_text.update(SkRect::MakeXYWH(kTextX, 48.0f, text_w, 34.0f));
    status_text.update(SkRect::MakeXYWH(kTextX, 88.0f, text_w, 22.0f));
  }
}

void UserProfileCard::paint(SkCanvas *canvas) {
  if (!compact_) {
    render_bg.draw(canvas);

    SkPaint shine;
    shine.setAntiAlias(true);
    shine.setColor(ColorFromARGB(42, 255, 255, 255));
    canvas->drawRoundRect(
      SkRect::MakeXYWH(1.0f, 1.0f, width_ - 2.0f, height_ * 0.45f), radius, radius, shine
    );
  }

  const float avatar_size = compact_ ? 34.0f : 72.0f;
  const float avatar_x = compact_ ? 2.0f : 28.0f;
  const float avatar_y = compact_ ? (height_ - avatar_size) * 0.5f : 30.0f;
  drawAvatar(canvas, SkRect::MakeXYWH(avatar_x, avatar_y, avatar_size, avatar_size));

  name_text.draw(canvas);
  if (!compact_) {
    status_text.draw(canvas);
    render_border.draw(canvas);
  } else {
    SkPaint divider;
    divider.setAntiAlias(true);
    divider.setColor(ColorFromARGB(34, 24, 31, 42));
    canvas->drawRect(SkRect::MakeXYWH(0.0f, height_ - 1.0f, width_, 1.0f), divider);
  }
}

void UserProfileCard::updateTextNodes() {
  const std::string_view name =
    user_profile_store.loggedIn() ? user_profile_store.displayName() : "未登录";

  name_text.set_text(name);
  status_text.set_text(user_profile_store.status());
  avatar_text.set_text(avatarLabel());
  loadAvatarImage(user_profile_store.avatarUrl());
}

void UserProfileCard::loadAvatarImage(const std::string_view url) {
  if (avatar_url == url) {
    return;
  }

  avatar_url = std::string(url);
  avatar_image = nullptr;
  if (avatar_url.empty()) {
    return;
  }

  const auto image_url = avatar_url;
  thread_manager->addTask([this, image_url] {
    try {
      const auto image_data = curl::get(image_url).value();
      avatar_image = decodeImage(image_data);
    } catch (...) {
      avatar_image = nullptr;
    }
  });
}

bool UserProfileCard::drawAvatarImage(SkCanvas *canvas, const SkRect &rect) const {
  if (!avatar_image) {
    return false;
  }

  canvas->save();
  canvas->clipRRect(SkRRect::MakeOval(rect), true);
  canvas->drawImageRect(
    avatar_image, rect, SkSamplingOptions(SkFilterMode::kLinear, SkMipmapMode::kLinear), nullptr
  );
  canvas->restore();
  return true;
}

void UserProfileCard::drawAvatar(SkCanvas *canvas, const SkRect &rect) {
  SkPaint avatar;
  avatar.setAntiAlias(true);
  avatar.setColor(
    user_profile_store.loggedIn() ? kProfileAvatarBg : ColorFromARGB(255, 128, 140, 156)
  );
  canvas->drawCircle(rect.centerX(), rect.centerY(), rect.width() * 0.5f, avatar);

  if (user_profile_store.loggedIn() && drawAvatarImage(canvas, rect)) {
    return;
  }

  SkPaint inner;
  inner.setAntiAlias(true);
  inner.setColor(ColorFromARGB(42, 255, 255, 255));
  canvas->drawCircle(
    rect.centerX() - rect.width() * 0.16f,
    rect.centerY() - rect.height() * 0.18f,
    rect.width() * 0.22f,
    inner
  );

  avatar_text.update(rect);
  avatar_text.draw(canvas);
}

} // namespace components

std::string firstUtf8Char(const std::string_view text) {
  if (text.empty()) {
    return "音";
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

std::string avatarLabel() {
  if (!user_profile_store.loggedIn()) {
    return "未";
  }
  return firstUtf8Char(user_profile_store.displayName());
}
