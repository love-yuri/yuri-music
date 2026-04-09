//
// Created by yuri on 2026/2/7.
//
export module pages:search;

import std;
import ui;
import skia;

using namespace ui::layout;
using namespace ui::widgets;
using namespace ui::render;
using namespace skia;

export namespace pages {

class SearchPage : public Widget {

public:
  explicit SearchPage(Widget* parent = nullptr);

  void layoutChildren() override;
  void paint(SkCanvas* canvas) override;

private:
  RenderBackground render_bg;     // background节点
  RenderText render_text{"搜索"}; // 字体节点
};

SearchPage::SearchPage(Widget *parent): Widget(parent) {
  render_text.setAlignment(Alignment::Center);
  render_text.setColor(skia_colors::black);
  render_text.setFontSize(24);
}

void SearchPage::layoutChildren() {
  render_bg.update(borderRect());
  render_text.update(contentRect());
}

void SearchPage::paint(SkCanvas *canvas) {
  render_bg.render(canvas);
  render_text.render(canvas);
}

} // namespace pages
