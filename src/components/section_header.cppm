//
// Created by yuri on 2026/4/8.
//
export module components:section_header;

import std;
import yuri.ui;
import yuri.skia;

using namespace ui::render;
using namespace ui::layout;
using namespace ui::widgets;
using namespace skia;

constexpr SkColor kTextColor = ColorFromARGB(255, 28, 36, 50);
constexpr SkColor kAccentColor = ColorFromARGB(255, 210, 52, 92);

export namespace components {

// 分段标题组件：左侧标题 + 右侧链接
class SectionHeader : public Widget {
public:
  explicit SectionHeader(std::string_view title,
                         std::string_view link = "",
                         Widget *parent = nullptr);

  void layout_children() override;
  void paint(SkCanvas *canvas) override;

private:
  RenderText title_text_;
  RenderText link_text_;
};

SectionHeader::SectionHeader(const std::string_view title,
                             const std::string_view link,
                             Widget *parent) :
  Widget(parent), title_text_(title), link_text_(link) {
  title_text_.set_alignment(Alignment::CenterLeft);
  title_text_.set_font_size(15.5f);
  title_text_.set_color(kTextColor);

  link_text_.set_alignment(Alignment::CenterRight);
  link_text_.set_font_size(12);
  link_text_.set_color(kAccentColor);
}

void SectionHeader::layout_children() {
  title_text_.update(local_bounds());
  link_text_.update(local_bounds());
}

void SectionHeader::paint(SkCanvas *canvas) {
  title_text_.draw(canvas);
  link_text_.draw(canvas);
}

} // namespace components
