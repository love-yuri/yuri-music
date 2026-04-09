//
// Created by yuri on 2026/2/7.
//
export module pages:recent;

import std;
import ui;
import skia;

using namespace ui::layout;
using namespace ui::widgets;
using namespace ui::render;
using namespace skia;

export namespace pages {

class RecentPage : public Widget {

public:
  explicit RecentPage(Widget* parent = nullptr);

  void layoutChildren() override;
  void paint(SkCanvas* canvas) override;

private:
  RenderBackground render_bg;         // background节点
  RenderText render_text{"最近播放"}; // 字体节点
};

RecentPage::RecentPage(Widget *parent): Widget(parent) {
  render_text.setAlignment(Alignment::Center);
  render_text.setColor(skia_colors::black);
  render_text.setFontSize(24);
}

void RecentPage::layoutChildren() {
  render_bg.update(borderRect());
  render_text.update(contentRect());
}

void RecentPage::paint(SkCanvas *canvas) {
  render_bg.render(canvas);
  render_text.render(canvas);
}

} // namespace pages
