//
// Created by yuri on 2026/2/7.
//
export module pages:settings;

import std;
import yuri.ui;
import yuri.skia;

using namespace ui::layout;
using namespace ui::widgets;
using namespace ui::render;
using namespace skia;

export namespace pages {

class SettingsPage : public Widget {

public:
  // 创建设置页面
  explicit SettingsPage(Widget *parent = nullptr);

  // 更新标题布局
  void layout_children() override;
  // 绘制页面背景和标题
  void paint(SkCanvas *canvas) override;

private:
  RenderText render_text{"设置"}; // 字体节点
};

SettingsPage::SettingsPage(Widget *parent) : Widget(parent) {
  render_text.set_alignment(Alignment::Center);
  render_text.set_color(skia_colors::black);
  render_text.set_font_size(24);
}

void SettingsPage::layout_children() {
  render_text.update(local_bounds());
}

void SettingsPage::paint(SkCanvas *canvas) {
  SkPaint panel;
  panel.setAntiAlias(true);
  panel.setColor(ColorFromARGB(104, 255, 255, 255));
  canvas->drawRect(local_bounds(), panel);
  render_text.draw(canvas);
}

} // namespace pages
