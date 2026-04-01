//
// Created by yuri on 2026/2/7.
//
export module pages:browse;

import std;
import ui;
import skia;

using namespace ui::layout;
using namespace ui::widgets;
using namespace ui::render;
using namespace skia;

export namespace pages {

class BrowsePage : public Box {
  RenderText render_text{"浏览"};
public:
  explicit BrowsePage(Widget* parent = nullptr) : Box(parent) {
    render_text.setAlignment(Alignment::Center);
    render_text.setColor(skia_colors::black);
    render_text.setFontSize(24);
  }

  void layoutChildren() override {
    render_bg.update(borderRect());
    render_border.update(borderRect());
    render_text.update(contentRect());
  }

  void paint(SkCanvas* canvas) override {
    Box::paint(canvas);
    render_text.render(canvas);
  }
};

}
