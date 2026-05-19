//
// Created by yuri on 2026/2/7.
//
export module pages:home;

import std;
import ui;
import core;
import skia;
import components;

using namespace ui::layout;
using namespace ui::widgets;
using namespace ui::render;
using namespace skia;
using namespace components;

constexpr SkColor kHomeTitleColor = ColorFromARGB(255, 20, 26, 36);
constexpr SkColor kHomeSubtitleColor = ColorFromARGB(178, 55, 68, 87);
constexpr SkColor kHomePageFill = ColorFromARGB(104, 255, 255, 255);

// 图标颜色
constexpr SkColor kIconBlue = ColorFromARGB(255, 42, 140, 233);
constexpr SkColor kIconRose = ColorFromARGB(255, 255, 76, 119);
constexpr SkColor kIconTeal = ColorFromARGB(255, 16, 178, 180);
constexpr SkColor kIconOrange = ColorFromARGB(255, 245, 155, 54);

export namespace pages {

// 首页
class HomePage : public Widget {
public:
  // 创建首页内容
  explicit HomePage(Widget *parent = nullptr);
  // 绘制首页背景
  void paint(SkCanvas *canvas) override;

private:
  Text *greeting_text{}; // 标题
  Text *subtitle_text{}; // 子标题
};

HomePage::HomePage(Widget *parent) :
  Widget(parent), greeting_text(new Text("下午好", this)),
  subtitle_text(new Text("发现适合现在聆听的音乐", this)) {

  setPadding(Insets(28, 24, 28, 24));

  // 使用垂直布局排列子控件
  setLayout<VBoxLayout<Widget>>();
  layout()->setSpacing(10);

  // 标题
  greeting_text->setFontSize(30);
  greeting_text->setColor(kHomeTitleColor);
  greeting_text->setAlignment(Alignment::CenterLeft);
  greeting_text->setMaxHeight(42.f);

  // 副标题
  subtitle_text->setFontSize(13);
  subtitle_text->setColor(kHomeSubtitleColor);
  subtitle_text->setAlignment(Alignment::CenterLeft);
  subtitle_text->setMaxHeight(24.f);

  auto header = new SectionHeader("为你推荐", "查看全部", this);
  header->setMaxHeight(34.0f);

  auto card = new QuickCard("每日推荐", "根据收藏与最近播放生成", kIconRose, this);
  card->clicked.connect([] {
    yuri::info("hhhh");
  });

  card = new QuickCard("沉浸工作", "轻节奏、低干扰的专注歌单", kIconBlue, this);
  card->clicked.connect([] {
    yuri::info("hhhh");
  });

  card = new QuickCard("夜间电台", "柔和人声与电子氛围", kIconTeal, this);
  card->clicked.connect([] {
    yuri::info("hhhh");
  });

  card = new QuickCard("新歌速递", "本周值得先听的更新", kIconOrange, this);
  card->clicked.connect([] {
    yuri::info("hhhh");
  });
}

void HomePage::paint(SkCanvas *canvas) {
  SkPaint panel;
  panel.setAntiAlias(true);
  panel.setColor(kHomePageFill);
  canvas->drawRect(borderRect(), panel);

  SkPaint glow;
  glow.setAntiAlias(true);
  glow.setColor(ColorFromARGB(28, 255, 76, 119));
  constexpr float glow_sigma = 24.0f;
  auto glow_rect = SkRect::MakeXYWH(width_ - 220.0f, -58.0f, 230.0f, 150.0f);
  auto glow_bounds = glow_rect.makeOutset(glow_sigma * 3.0f, glow_sigma * 3.0f);
  SkPaint glow_layer;
  glow_layer.setImageFilter(SkImageFilters::Blur(glow_sigma, glow_sigma, nullptr));
  canvas->saveLayer(&glow_bounds, &glow_layer);
  canvas->drawOval(glow_rect, glow);
  canvas->restore();
}

} // namespace pages
