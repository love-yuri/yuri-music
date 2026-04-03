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
 * 管理多个页面, 同时仅展示一个, 支持 fade + translateY 切换动画
 */
export class PageView : public Widget {
public:
  using Widget::Widget;

  /**
   * 添加页面
   * @param id 页面唯一标识
   * @param page 页面控件
   */
  void addPage(std::string_view id, Widget *page);

  /**
   * 切换到指定页面, 带有 fade + translateY 动画
   * @param id 目标页面标识
   */
  void showPage(std::string_view id);

  /**
   * 获取当前页面标识
   */
  [[nodiscard]] const std::string &currentPage() const;

  /**
   * 布局所有可见的子页面
   */
  void layoutChildren() override;

  /**
   * 渲染页面内容, 动画期间对新旧页面应用透明度和位移
   * @param canvas Skia 画布
   */
  void render(SkCanvas *canvas) override;

private:
  using PageEntry = std::pair<std::string, Widget *>;

  /**
   * 动画进度回调
   * @param progress 动画进度 [0, 1]
   */
  void setAnimProgress(float progress);

  /**
   * 查找当前页面控件
   * @return 当前页面控件指针, 未设置时返回 nullptr
   */
  [[nodiscard]] Widget *currentPageWidget() const;

  std::vector<PageEntry> pages_;
  std::string current_page_;
  float anim_progress_ = 1.f;

  static constexpr float kFadeOffset = 8.f;
  static constexpr float kAnimDuration = 300.f;
};

void PageView::addPage(const std::string_view id, Widget *page) {
  page->setVisible(false);
  pages_.emplace_back(id, page);
}

void PageView::showPage(const std::string_view id) {
  const auto target = std::ranges::find(pages_, id, &PageEntry::first);
  if (target == pages_.end() || target->first == current_page_) return;

  // 立即隐藏旧页面, 避免新旧页面同时可见产生重合感
  const auto cur = std::ranges::find(pages_, current_page_, &PageEntry::first);
  if (cur != pages_.end()) {
    cur->second->setVisible(false);
  }

  current_page_ = id;
  target->second->setVisible(true);

  // 对新页面做初始布局
  const auto w = contentWidth();
  const auto h = contentHeight();
  target->second->setGeometry(0, 0, w, h);
  target->second->updateLayout();

  // 启动 fade + slide 动画
  anim_progress_ = 0.f;
  startAnimation(0.f, 1.f, kAnimDuration, &anim_progress_, CubicBezier::Ease());
}

[[nodiscard]] const std::string &PageView::currentPage() const {
  return current_page_;
}

void PageView::layoutChildren() {
  const auto w = contentWidth();
  const auto h = contentHeight();
  for (const auto &page : pages_ | std::views::values) {
    if (page->visible()) {
      page->setGeometry(0, 0, w, h);
      page->updateLayout();
    }
  }
}

void PageView::render(SkCanvas *canvas) {
  canvas->save();
  canvas->translate(x_, y_);
  paint(canvas);
  canvas->translate(padding_.left, padding_.top);
  // 裁剪到内容区域, 防止动画期间页面溢出
  canvas->clipRect(SkRect::MakeWH(contentWidth(), contentHeight()));

  const bool animating = anim_progress_ < 1.f;
  const Widget *cur_widget = currentPageWidget();

  for (auto *child : children_) {
    if (!child->visible()) continue;

    canvas->save();

    if (animating && child == cur_widget) {
      // 新页面: opacity 0→1, translateY +8px→0
      canvas->saveLayerAlphaf(nullptr, anim_progress_ * 255.f);
      canvas->translate(0, (1.f - anim_progress_) * kFadeOffset);
    }

    child->render(canvas);

    if (animating && child == cur_widget) {
      canvas->restore();
    }

    canvas->restore();
  }

  canvas->restore();
}

void PageView::setAnimProgress(const float progress) {
  anim_progress_ = progress;
}

[[nodiscard]] Widget *PageView::currentPageWidget() const {
  const auto it = std::ranges::find(pages_, current_page_, &PageEntry::first);
  return it != pages_.end() ? it->second : nullptr;
}
