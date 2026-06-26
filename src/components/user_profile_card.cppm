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

using namespace ui::layout;
using namespace ui::widgets;
using namespace ui::render;
using namespace skia;

namespace {

constexpr SkColor kProfileCardBg = ColorFromARGB(150, 255, 255, 255);
constexpr SkColor kProfileCardBorder = ColorFromARGB(116, 255, 255, 255);
constexpr SkColor kProfileName = ColorFromARGB(255, 22, 28, 38);
constexpr SkColor kProfileMuted = ColorFromARGB(174, 58, 70, 88);
constexpr SkColor kProfileAvatarBg = ColorFromARGB(255, 38, 189, 220);

std::string fullStats(const models::UserProfile &profile) {
  if (!profile.logged_in) {
    return "登录后会显示账号、歌单和歌曲统计";
  }
  if (profile.loading) {
    return "正在同步你的 QQ 音乐资料";
  }
  return std::format("{} 个歌单 / {} 首歌曲", profile.playlist_count, profile.song_count);
}

} // namespace

export namespace components {

class UserProfileCard : public Box {
public:
  explicit UserProfileCard(bool compact, Widget *parent = nullptr);

  void layoutChildren() override;
  void paint(SkCanvas *canvas) override;

private:
  void updateTextNodes(const models::UserProfile &profile);
  void drawAvatar(SkCanvas *canvas, const models::UserProfile &profile, const SkRect &rect);

  bool compact_ = true;
  RenderText name_text;
  RenderText status_text;
  RenderText stats_text;
  RenderText avatar_text;
};

UserProfileCard::UserProfileCard(const bool compact, Widget *parent)
    : Box(parent), compact_(compact) {
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

  stats_text.setFontSize(13.5f);
  stats_text.setColor(ColorFromARGB(214, 40, 52, 68));
  stats_text.setAlignment(Alignment::CenterLeft);

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
}

void UserProfileCard::layoutChildren() {
  render_bg.update(borderRect());
  render_border.update(borderRect());

  const auto profile = models::userProfileStore().snapshot();
  updateTextNodes(profile);
}

void UserProfileCard::paint(SkCanvas *canvas) {
  models::userProfileStore().refreshAsync();
  const auto profile = models::userProfileStore().snapshot();
  updateTextNodes(profile);

  if (!compact_) {
    render_bg.render(canvas);

    SkPaint shine;
    shine.setAntiAlias(true);
    shine.setColor(ColorFromARGB(42, 255, 255, 255));
    canvas->drawRoundRect(SkRect::MakeXYWH(1.0f, 1.0f, width_ - 2.0f, height_ * 0.45f), radius, radius, shine);
  }

  const float avatar_size = compact_ ? 34.0f : 72.0f;
  const float avatar_x = compact_ ? 2.0f : 28.0f;
  const float avatar_y = compact_ ? (height_ - avatar_size) * 0.5f : 30.0f;
  drawAvatar(canvas, profile, SkRect::MakeXYWH(avatar_x, avatar_y, avatar_size, avatar_size));

  name_text.render(canvas);
  if (!compact_) {
    status_text.render(canvas);
    stats_text.render(canvas);
    render_border.render(canvas);
  } else {
    SkPaint divider;
    divider.setAntiAlias(true);
    divider.setColor(ColorFromARGB(34, 24, 31, 42));
    canvas->drawRect(SkRect::MakeXYWH(0.0f, height_ - 1.0f, width_, 1.0f), divider);
  }
}

void UserProfileCard::updateTextNodes(const models::UserProfile &profile) {
  const std::string name = profile.logged_in ? profile.display_name : "未登录";
  const std::string status = profile.loading ? "正在读取个人信息" : profile.status_text;
  const std::string stats = fullStats(profile);

  name_text.setText(name);
  status_text.setText(status);
  stats_text.setText(stats);
  avatar_text.setText(profile.avatar_label.empty() ? "音" : profile.avatar_label);

  if (compact_) {
    const float text_x = 46.0f;
    const float text_w = std::max(0.0f, width_ - text_x - 2.0f);
    name_text.update(SkRect::MakeXYWH(text_x, 0.0f, text_w, height_));
  } else {
    const float text_x = 116.0f;
    const float text_w = std::max(0.0f, width_ - text_x - 30.0f);
    name_text.update(SkRect::MakeXYWH(text_x, 34.0f, text_w, 34.0f));
    status_text.update(SkRect::MakeXYWH(text_x, 72.0f, text_w, 22.0f));
    stats_text.update(SkRect::MakeXYWH(30.0f, height_ - 58.0f, width_ - 60.0f, 24.0f));
  }
}

void UserProfileCard::drawAvatar(SkCanvas *canvas, const models::UserProfile &profile, const SkRect &rect) {
  SkPaint avatar;
  avatar.setAntiAlias(true);
  avatar.setColor(profile.logged_in ? kProfileAvatarBg : ColorFromARGB(255, 128, 140, 156));
  canvas->drawCircle(rect.centerX(), rect.centerY(), rect.width() * 0.5f, avatar);

  SkPaint inner;
  inner.setAntiAlias(true);
  inner.setColor(ColorFromARGB(42, 255, 255, 255));
  canvas->drawCircle(rect.centerX() - rect.width() * 0.16f, rect.centerY() - rect.height() * 0.18f, rect.width() * 0.22f, inner);

  avatar_text.update(rect);
  avatar_text.render(canvas);
}

} // namespace components
