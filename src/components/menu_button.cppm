//
// Created by yuri on 2026/2/7.
//
module;
#if defined(_WIN32)
// ReSharper disable once CppUnusedIncludeDirective
#include "include/private/base/SkFloatingPoint.h"
#endif
export module components:menu_button;

import std;
import ui;
import skia;
import signal;

using namespace ui::render;
using namespace ui::layout;
using namespace skia;
using namespace skia::skia_colors;

export namespace ui::widgets {

struct MenuItem {
  std::string text;
  std::function<void()> callback;
  bool is_separator = false;
};

class MenuButton : public Box {
  RenderText render_text;
  bool menu_open_ = false;
  bool hover_menu_ = false;
  int hovered_item_ = -1;
  float menu_width_ = 150.f;
  float item_height_ = 28.f;
  std::vector<MenuItem> items_;
  SkColor bg_color_ = light_gray;
  SkColor hover_color_ = accent_blue;
  SkColor text_color_ = black;

  void drawArrow(SkCanvas* canvas, float x, float y, float size);
  void drawMenuItem(SkCanvas* canvas, float x, float y, float w, float h, const MenuItem& item, bool hovered);
  void drawMenu(SkCanvas* canvas);

public:
  Signal<std::string&> itemClicked;

  explicit MenuButton(std::string_view text, Widget* parent = nullptr);

  void setMenuWidth(float width) noexcept { menu_width_ = width; }
  void setItemHeight(float height) noexcept { item_height_ = height; }
  void setBgColor(SkColor color) noexcept { bg_color_ = color; }
  void setHoverColor(SkColor color) noexcept { hover_color_ = color; }
  void setTextColor(SkColor color) noexcept { text_color_ = color; }

  void addItem(std::string_view text, std::function<void()> callback = nullptr);
  void addSeparator();

  void paint(SkCanvas* canvas) override;
  void layoutChildren() override;

  bool isMenuOpen() const noexcept { return menu_open_; }
  void openMenu() noexcept { menu_open_ = true; }
  void closeMenu() noexcept { menu_open_ = false; hovered_item_ = -1; }
  void toggleMenu() noexcept { menu_open_ ? closeMenu() : openMenu(); }

  void onMouseLeftPressed(float x, float y) override;
  void onMouseMove(float x, float y) override;
  void onMouseLeave(float x, float y) override;

  int getHoveredItem(float x, float y);
};

MenuButton::MenuButton(const std::string_view text, Widget* parent)
  : Box(parent), render_text(text, {}) {
  render_text.setTextAndAlignment(text, Alignment::Left | Alignment::VCenter);
  render_bg.setColor(white);
  render_border.setColor(light_gray);
  render_border.setWidth(1);
  radius = 4;

  const auto& textBound = render_text.textBound();
  resize(textBound.width() + 48, textBound.height() + 12);
  setPadding(Insets{6, 12, 6, 12});
}

void MenuButton::addItem(const std::string_view text, std::function<void()> callback) {
  items_.push_back({std::string(text), std::move(callback), false});
}

void MenuButton::addSeparator() {
  items_.push_back({"", nullptr, true});
}

void MenuButton::drawArrow(SkCanvas* canvas, const float x, const float y, const float size) {
  const auto path = SkPath::Polygon(
    {
      SkPoint{x, y},
      SkPoint{x + size, y},
      SkPoint{x + size / 2, y + size * 0.6f},
    },
    true
  );

  SkPaint paint;
  paint.setColor(text_color_);
  paint.setAntiAlias(true);
  canvas->drawPath(path, paint);
}

void MenuButton::drawMenuItem(SkCanvas* canvas, const float x, const float y,
                               const float w, const float h,
                               const MenuItem& item, const bool hovered) {
  if (item.is_separator) {
    SkPaint paint;
    paint.setColor(light_gray);
    paint.setStrokeWidth(1);
    canvas->drawLine(x + 8, y + h / 2, x + w - 8, y + h / 2, paint);
    return;
  }

  if (hovered) {
    SkPaint paint;
    paint.setColor(hover_color_);
    paint.setAntiAlias(true);
    canvas->drawRect(SkRect::MakeXYWH(x, y, w, h), paint);
  }

  SkFont font;
  font.setSize(13);
  SkPaint textPaint;
  textPaint.setColor(hovered ? white : text_color_);
  textPaint.setAntiAlias(true);

  SkRect bounds;
  font.measureText(item.text.data(), item.text.size(), SkTextEncoding::kUTF8, &bounds);

  const float textX = x + 12 - bounds.left();
  const float textY = y + h / 2 - bounds.centerY();
  auto blob = SkTextBlob::MakeFromText(item.text.data(), item.text.size(), font, SkTextEncoding::kUTF8);
  canvas->drawTextBlob(blob, textX, textY, textPaint);
}

void MenuButton::drawMenu(SkCanvas* canvas) {
  const float menuX = 0;
  const float menuY = height_;
  const float menuHeight = static_cast<float>(items_.size()) * item_height_;

  canvas->save();

  SkPaint shadowPaint;
  shadowPaint.setColor(ColorFromARGB(0x30, 0x00, 0x00, 0x00));
  shadowPaint.setAntiAlias(true);
  canvas->drawRect(SkRect::MakeXYWH(menuX + 2, menuY + 2, menu_width_, menuHeight), shadowPaint);

  SkPaint bgPaint;
  bgPaint.setColor(white);
  bgPaint.setAntiAlias(true);
  canvas->drawRect(SkRect::MakeXYWH(menuX, menuY, menu_width_, menuHeight), bgPaint);

  SkPaint borderPaint;
  borderPaint.setColor(light_gray);
  borderPaint.setStrokeWidth(1);
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setAntiAlias(true);
  canvas->drawRect(SkRect::MakeXYWH(menuX, menuY, menu_width_, menuHeight), borderPaint);

  for (size_t i = 0; i < items_.size(); ++i) {
    drawMenuItem(
      canvas,
      menuX,
      menuY + i * item_height_,
      menu_width_,
      item_height_,
      items_[i],
      static_cast<int>(i) == hovered_item_
    );
  }

  canvas->restore();
}

void MenuButton::paint(SkCanvas* canvas) {
  render_bg.render(canvas);
  render_border.render(canvas);
  render_text.render(canvas);

  constexpr float arrowSize = 8;
  const float arrowX = width_ - padding_.right - arrowSize - 4;
  const float arrowY = height_ / 2 - arrowSize / 2 + 2;
  drawArrow(canvas, arrowX, arrowY, arrowSize);

  if (menu_open_) {
    drawMenu(canvas);
  }
}

void MenuButton::layoutChildren() {
  render_text.update(contentRect());
  render_bg.update(borderRect());
  render_border.update(borderRect());
}

int MenuButton::getHoveredItem(const float x, const float y) {
  if (!menu_open_) return -1;

  const float menuY = height_;
  if (x < 0 || x > menu_width_ || y < menuY || y > menuY + items_.size() * item_height_) {
    return -1;
  }

  return static_cast<int>((y - menuY) / item_height_);
}

void MenuButton::onMouseLeftPressed(const float x, const float y) {
  if (y < height_) {
    toggleMenu();
  } else if (menu_open_) {
    const int itemIndex = getHoveredItem(x, y);
    if (itemIndex >= 0 && itemIndex < static_cast<int>(items_.size()) && !items_[itemIndex].is_separator) {
      if (items_[itemIndex].callback) {
        items_[itemIndex].callback();
      }
      itemClicked.emit(items_[itemIndex].text);
      closeMenu();
    }
  }
}

void MenuButton::onMouseMove(const float x, const float y) {
  hovered_item_ = getHoveredItem(x, y);
}

void MenuButton::onMouseLeave(const float, const float) {
  hovered_item_ = -1;
}

} // namespace ui::widgets
