//
// Created by yuri on 2026/2/7.
//
export module pages:recent;

import std;
import yuri.ui;
import yuri.skia;

using namespace ui::layout;
using namespace ui::widgets;
using namespace ui::render;
using namespace skia;

export namespace pages {

class RecentPage : public Widget {

public:
  // 创建最近播放页面
  explicit RecentPage(Widget *parent = nullptr);

  // 更新标题布局
  void layout_children() override;

  // 绘制页面背景和标题
  void paint(SkCanvas *canvas) override;

private:
  RenderText render_text{"最近播放"}; // 字体节点
};

RecentPage::RecentPage(Widget *parent) : Widget(parent) {
  render_text.set_alignment(Alignment::Center);
  render_text.set_color(skia_colors::black);
  render_text.set_font_size(24);
}

void RecentPage::layout_children() {
  render_text.update(local_bounds());
}

void RecentPage::paint(SkCanvas *canvas) {
  SkPaint panel;
  panel.setAntiAlias(true);
  panel.setColor(ColorFromARGB(104, 255, 255, 255));
  canvas->drawRect(local_bounds(), panel);
  render_text.draw(canvas);
}

} // namespace pages
