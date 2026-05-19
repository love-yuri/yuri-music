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

class BrowsePage : public Widget {

public:
  // 创建浏览页面
  explicit BrowsePage(Widget *parent = nullptr);

  // 更新标题布局
  void layoutChildren() override;
  // 绘制页面背景和标题
  void paint(SkCanvas *canvas) override;

private:
  RenderText render_text{ "浏览" }; // 字体节点
};

BrowsePage::BrowsePage(Widget *parent) : Widget(parent) {
  render_text.setAlignment(Alignment::Center);
  render_text.setColor(skia_colors::black);
  render_text.setFontSize(24);
}

void BrowsePage::layoutChildren() {
  render_text.update(contentRect());
}

void BrowsePage::paint(SkCanvas *canvas) {
  SkPaint panel;
  panel.setAntiAlias(true);
  panel.setColor(ColorFromARGB(104, 255, 255, 255));
  canvas->drawRect(borderRect(), panel);
  render_text.render(canvas);
}

} // namespace pages
