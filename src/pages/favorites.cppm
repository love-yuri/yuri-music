//
// Created by yuri on 2026/2/7.
//
export module pages:favorites;

import std;
import ui;
import skia;

using namespace ui::layout;
using namespace ui::widgets;
using namespace ui::render;
using namespace skia;

export namespace pages {

class FavoritesPage : public Widget {

public:
  explicit FavoritesPage(Widget* parent = nullptr);

  void layoutChildren() override;
  void paint(SkCanvas* canvas) override;

private:
  RenderBackground render_bg;       // background节点
  RenderText render_text{"我喜欢"}; // 字体节点
};

FavoritesPage::FavoritesPage(Widget *parent): Widget(parent) {
  render_text.setAlignment(Alignment::Center);
  render_text.setColor(skia_colors::black);
  render_text.setFontSize(24);
}

void FavoritesPage::layoutChildren() {
  render_bg.update(borderRect());
  render_text.update(contentRect());
}

void FavoritesPage::paint(SkCanvas *canvas) {
  render_bg.render(canvas);
  render_text.render(canvas);
}

} // namespace pages
