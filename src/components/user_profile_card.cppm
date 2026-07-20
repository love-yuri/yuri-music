module;
#if defined(_WIN32)
#include "include/private/SkFloatingPoint.h"
#endif
//
// Created by Codex on 2026/6/24.
//
export module components:user_profile_card;

import std;
import ui;
import skia;
import models;
import qq_music_api;
import thread_pool;
import store;

using namespace ui::layout;
using namespace ui::widgets;
using namespace ui::render;
using namespace store;
using namespace skia;

constexpr SkColor kProfileCardBg = ColorFromARGB(150, 255, 255, 255);
constexpr SkColor kProfileCardBorder = ColorFromARGB(116, 255, 255, 255);
constexpr SkColor kProfileName = ColorFromARGB(255, 22, 28, 38);
constexpr SkColor kProfileMuted = ColorFromARGB(174, 58, 70, 88);
constexpr SkColor kProfileAvatarBg = ColorFromARGB(255, 38, 189, 220);

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

export namespace components {

class UserProfileCard : public Box {
public:
  explicit UserProfileCard(bool compact, Widget *parent = nullptr);
  ~UserProfileCard() override;

  void layoutChildren() override;
  void paint(SkCanvas *canvas) override;

private:
  void updateTextNodes();
  void loadAvatarImage(std::string_view url);
  bool drawAvatarImage(SkCanvas *canvas, const SkRect &rect) const;
  void drawAvatar(SkCanvas *canvas, const SkRect &rect);

  bool compact_ = true;
  RenderText name_text;
  RenderText status_text;
  RenderText avatar_text;
  std::string avatar_url_{};
  sk_sp<SkImage> avatar_image{};
};

UserProfileCard::UserProfileCard(const bool compact, Widget *parent) :
  Box(parent), compact_(compact) {
  radius = compact_ ? 0.0f : 18.0f;
  render_bg.setColor(kProfileCardBg);
  render_border.setColor(kProfileCardBorder);
  render_border.setWidth(1.0f);

  name_text.setFontSize(compact_ ? 14.0f : 26.0f);
  name_text.setColor(kProfileName);
  name_text.setAlignment(Alignment::CenterLeft);

  status_text.setFontSize(14.0f);
  status_text.setColor(kProfileMuted);
  status_text.setAlignment(Alignment::CenterLeft);

  avatar_text.setFontSize(compact_ ? 14.0f : 24.0f);
  avatar_text.setColor(skia_colors::white);
  avatar_text.setAlignment(Alignment::Center);

  if (compact_) {
    setMinHeight(54.0f);
    setMaxHeight(54.0f);
  } else {
    setMinHeight(206.0f);
    setMaxHeight(232.0f);
  }

  user_profile_store.status_changed.connect<&UserProfileCard::updateTextNodes>(this);
  updateTextNodes();
}

UserProfileCard::~UserProfileCard() {
  user_profile_store.status_changed.disconnect(this);
}

void UserProfileCard::layoutChildren() {
  render_bg.update(borderRect());
  render_border.update(borderRect());
}

void UserProfileCard::paint(SkCanvas *canvas) {
  if (!compact_) {
    render_bg.render(canvas);

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

  name_text.render(canvas);
  if (!compact_) {
    status_text.render(canvas);
    render_border.render(canvas);
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

  name_text.setText(name);
  status_text.setText(user_profile_store.status());
  avatar_text.setText(avatarLabel());
  loadAvatarImage(user_profile_store.avatarUrl());

  if (compact_) {
    constexpr float text_x = 46.0f;
    const float text_w = std::max(0.0f, width_ - text_x - 2.0f);
    name_text.update(SkRect::MakeXYWH(text_x, 0.0f, text_w, height_));
  } else {
    constexpr float text_x = 116.0f;
    const float text_w = std::max(0.0f, width_ - text_x - 30.0f);
    name_text.update(SkRect::MakeXYWH(text_x, 48.0f, text_w, 34.0f));
    status_text.update(SkRect::MakeXYWH(text_x, 88.0f, text_w, 22.0f));
  }
}

void UserProfileCard::loadAvatarImage(const std::string_view url) {
  if (avatar_url_ == url) {
    return;
  }

  avatar_url_ = std::string(url);
  avatar_image = nullptr;
  if (avatar_url_.empty()) {
    return;
  }

  const auto image_url = avatar_url_;
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
  avatar_text.render(canvas);
}

} // namespace components
