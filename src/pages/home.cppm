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
constexpr SkColor kHomeHeaderFill = ColorFromARGB(70, 255, 255, 255);
constexpr SkColor kHomeHeaderStroke = ColorFromARGB(66, 255, 255, 255);

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
  // 更新首页内容布局
  void layoutChildren() override;

private:
  Text *greeting_text{};                         // 标题
  Text *subtitle_text{};                         // 子标题
  SectionHeader *recommend_header{};             // 推荐区标题
  std::vector<QuickCard *> recommendation_cards; // 推荐卡片
};

HomePage::HomePage(Widget *parent) :
  Widget(parent), greeting_text(new Text("下午好", this)),
  subtitle_text(new Text("发现适合现在聆听的音乐", this)),
  recommend_header(new SectionHeader("为你推荐", "查看全部", this)) {

  setPadding(Insets(34, 28, 36, 28));

  // 标题
  greeting_text->setFontSize(30);
  greeting_text->setColor(kHomeTitleColor);
  greeting_text->setAlignment(Alignment::CenterLeft);

  // 副标题
  subtitle_text->setFontSize(13);
  subtitle_text->setColor(kHomeSubtitleColor);
  subtitle_text->setAlignment(Alignment::CenterLeft);

  recommendation_cards.push_back(new QuickCard("每日推荐", "根据收藏与最近播放生成", kIconRose, this));
  recommendation_cards.push_back(new QuickCard("沉浸工作", "轻节奏、低干扰的专注歌单", kIconBlue, this));
  recommendation_cards.push_back(new QuickCard("夜间电台", "柔和人声与电子氛围", kIconTeal, this));
  recommendation_cards.push_back(new QuickCard("新歌速递", "本周值得先听的更新", kIconOrange, this));

  for (auto *card : recommendation_cards) {
    card->clicked.connect([] {
      yuri::info("hhhh");
    });
  }
}

void HomePage::layoutChildren() {
  const float w = contentWidth();
  const bool two_columns = w >= 560.0f;
  const float column_gap = two_columns ? 16.0f : 0.0f;
  const int columns = two_columns ? 2 : 1;
  const float card_w = two_columns ? (w - column_gap) * 0.5f : w;
  constexpr float card_h = 82.0f;
  constexpr float row_gap = 14.0f;

  greeting_text->setGeometry(0.0f, 0.0f, w, 42.0f);
  subtitle_text->setGeometry(0.0f, 42.0f, w, 24.0f);
  recommend_header->setGeometry(0.0f, 102.0f, w, 34.0f);

  const float grid_y = 148.0f;
  for (std::size_t i = 0; i < recommendation_cards.size(); ++i) {
    const int col = static_cast<int>(i % columns);
    const int row = static_cast<int>(i / columns);
    const float x = static_cast<float>(col) * (card_w + column_gap);
    const float y = grid_y + static_cast<float>(row) * (card_h + row_gap);
    recommendation_cards[i]->setGeometry(x, y, card_w, card_h);
  }

  Widget::layoutChildren();
}

void HomePage::paint(SkCanvas *canvas) {
  SkPaint panel;
  panel.setAntiAlias(true);
  panel.setColor(kHomePageFill);
  canvas->drawRect(borderRect(), panel);

  SkPaint header_fill;
  header_fill.setAntiAlias(true);
  header_fill.setColor(kHomeHeaderFill);
  const auto header_rect = SkRect::MakeXYWH(22.0f, 18.0f, std::max(0.0f, width_ - 44.0f), 92.0f);
  canvas->drawRoundRect(header_rect, 16.0f, 16.0f, header_fill);

  SkPaint header_stroke;
  header_stroke.setAntiAlias(true);
  header_stroke.setStyle(SkPaint::kStroke_Style);
  header_stroke.setStrokeWidth(1.0f);
  header_stroke.setColor(kHomeHeaderStroke);
  canvas->drawRoundRect(header_rect, 16.0f, 16.0f, header_stroke);

  SkPaint glow;
  glow.setAntiAlias(true);
  glow.setColor(ColorFromARGB(24, 255, 76, 119));
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
