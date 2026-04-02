//
// Created by love-yuri on 2026/4/2.
//
export module page_view;

import skia;
import std;
import core;
import ui;

using namespace skia;
using namespace ui::widgets;
using namespace ui::animation;

/**
 * 页面容器
 * 管理多个页面, 同时仅展示一个, 支持滑动切换动画
 */
export class PageView : public Widget {
public:
  using Widget::Widget;

  void addPage(const std::string &id, Widget *page) {
    page->setVisible(false);
    pages_[id] = page;
  }

  void showPage(const std::string &id) {
    if (!pages_.contains(id)) return;
    if (id == current_page_) return;

    for (auto &[pid, page] : pages_) {
      page->setVisible(pid == id);
    }
    current_page_ = id;

    const auto w = contentWidth();
    const auto h = contentHeight();
    for (const auto &page : pages_ | std::views::values) {
      if (page->visible()) {
        page->setGeometry(0, 0, w, h);
        page->updateLayout();
      }
    }

    anim_progress_ = 1.f;
    animation_manager->start(1.f, 0.f, 250.f, CubicBezier::EaseOut(), &anim_progress_);
  }

  [[nodiscard]] const std::string &currentPage() const { return current_page_; }

  void layoutChildren() override {
    const auto w = contentWidth();
    const auto h = contentHeight();
    for (const auto &page : pages_ | std::views::values) {
      if (page->visible()) {
        page->setGeometry(0, 0, w, h);
        page->updateLayout();
      }
    }
  }

  void render(SkCanvas *canvas) override {
    canvas->save();
    canvas->translate(x_, y_);
    paint(canvas);
    canvas->translate(padding_.left, padding_.top);

    const auto cur = pages_.find(current_page_);

    for (auto *child : children_) {
      if (!child->visible()) continue;

      canvas->save();
      if (cur != pages_.end() && child == cur->second) {
        canvas->translate(anim_progress_ * contentWidth(), 0);
      }
      child->render(canvas);
      canvas->restore();
    }

    canvas->restore();
  }

private:
  std::unordered_map<std::string, Widget *> pages_;
  std::string current_page_;
  float anim_progress_ = 0.f;
};
