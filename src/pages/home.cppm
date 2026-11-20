//
// Created by yuri on 2026/2/7.
//
export module pages:home;

import std;
import yuri.ui;
import yuri.skia;
import components;

using namespace ui::layout;
using namespace ui::widgets;
using namespace skia;
using namespace components;

constexpr SkColor kHomeTitleColor = ColorFromARGB(255, 20, 26, 36);     // 首页标题色
constexpr SkColor kHomeSubtitleColor = ColorFromARGB(178, 55, 68, 87);  // 首页副标题色
constexpr SkColor kHomePageFill = ColorFromARGB(104, 255, 255, 255);    // 页面底色
constexpr SkColor kHomeHeaderFill = ColorFromARGB(70, 255, 255, 255);   // 页眉底色
constexpr SkColor kHomeHeaderStroke = ColorFromARGB(66, 255, 255, 255); // 页眉边线色
constexpr Insets kHomePadding(34.0f, 28.0f, 36.0f, 28.0f);              // 首页内容内边距
constexpr float kWideGrid = 560.0f;                                     // 改用两列的内容宽度
constexpr float kGridTop = 148.0f;   // 推荐列表相对内容顶部的距离
constexpr float kCardHeight = 82.0f; // 推荐卡片高度
constexpr float kRowGap = 14.0f;     // 卡片行间距

constexpr SkColor kIconBlue = ColorFromARGB(255, 42, 140, 233);   // 蓝色图标
constexpr SkColor kIconRose = ColorFromARGB(255, 255, 76, 119);   // 玫红图标
constexpr SkColor kIconTeal = ColorFromARGB(255, 16, 178, 180);   // 青色图标
constexpr SkColor kIconOrange = ColorFromARGB(255, 245, 155, 54); // 橙色图标

/**
 * 根据外部宽度选择推荐卡片的列数。
 * @param width 滚动视口宽度
 * @return 推荐卡片列数
 */
std::size_t columnCount(float width);

namespace pages {

/** 包含标题和推荐卡片的可滚动内容。 */
class HomeContent : public Widget {
public:
  /**
   * 创建首页内容。
   * @param parent 首页滚动视口
   */
  explicit HomeContent(Widget *parent);

  /**
   * 按视口宽度计算显示完整内容所需的高度。
   * @param width 滚动视口宽度
   * @return 显示完整内容所需的高度
   */
  [[nodiscard]] float requiredHeight(float width) const;

  /** 更新标题与推荐卡片布局。 */
  void layout_children() override;

  /**
   * 绘制首页背景。
   * @param canvas 目标画布
   */
  void paint(SkCanvas *canvas) override;

private:
  Text *greeting_text{};                         // 标题
  Text *subtitle_text{};                         // 子标题
  SectionHeader *recommend_header{};             // 推荐区标题
  std::vector<QuickCard *> recommendation_cards; // 推荐卡片
};

/** 窄窗口或播放栏占用空间时，首页仍可滚动浏览全部推荐。 */
export class HomePage : public ScrollArea {
public:
  /**
   * 创建首页和滚动内容。
   * @param parent 父控件
   */
  explicit HomePage(Widget *parent = nullptr);

  /** 根据视口与卡片网格的高度更新滚动范围。 */
  void layout_children() override;

private:
  HomeContent *content_; // 首页内容容器
};

HomeContent::HomeContent(Widget *parent) :
  Widget(parent), greeting_text(new Text("下午好", this)),
  subtitle_text(new Text("发现适合现在聆听的音乐", this)),
  recommend_header(new SectionHeader("为你推荐", "查看全部", this)) {
  // 标题
  greeting_text->set_font_size(30);
  greeting_text->set_color(kHomeTitleColor);
  greeting_text->set_alignment(Alignment::CenterLeft);

  // 副标题
  subtitle_text->set_font_size(13);
  subtitle_text->set_color(kHomeSubtitleColor);
  subtitle_text->set_alignment(Alignment::CenterLeft);

  recommendation_cards.push_back(new QuickCard("每日推荐", "根据收藏与最近播放生成", kIconRose, this));
  recommendation_cards.push_back(new QuickCard("沉浸工作", "轻节奏、低干扰的专注歌单", kIconBlue, this));
  recommendation_cards.push_back(new QuickCard("夜间电台", "柔和人声与电子氛围", kIconTeal, this));
  recommendation_cards.push_back(new QuickCard("新歌速递", "本周值得先听的更新", kIconOrange, this));
}

float HomeContent::requiredHeight(const float width) const {
  const auto columns = columnCount(width);
  const auto rows = (recommendation_cards.size() + columns - 1) / columns;
  return kHomePadding.top + kGridTop + static_cast<float>(rows) * (kCardHeight + kRowGap) - kRowGap
         + kHomePadding.bottom;
}

void HomeContent::layout_children() {
  const float w = std::max(0.0f, width() - kHomePadding.left - kHomePadding.right);
  const auto columns = columnCount(width());
  const float column_gap = columns == 2 ? 16.0f : 0.0f;
  const float card_w = (w - column_gap) / static_cast<float>(columns);

  greeting_text->set_geometry(kHomePadding.left, kHomePadding.top, w, 42.0f);
  subtitle_text->set_geometry(kHomePadding.left, kHomePadding.top + 42.0f, w, 24.0f);
  recommend_header->set_geometry(kHomePadding.left, kHomePadding.top + 102.0f, w, 34.0f);

  for (std::size_t i = 0; i < recommendation_cards.size(); ++i) {
    const float x = kHomePadding.left + static_cast<float>(i % columns) * (card_w + column_gap);
    const float y =
      kHomePadding.top + kGridTop + static_cast<float>(i / columns) * (kCardHeight + kRowGap);
    recommendation_cards[i]->set_geometry(x, y, card_w, kCardHeight);
  }

  Widget::layout_children();
}

void HomeContent::paint(SkCanvas *canvas) {
  SkPaint panel;
  panel.setAntiAlias(true);
  panel.setColor(kHomePageFill);
  canvas->drawRect(local_bounds(), panel);

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
  constexpr float kGlowSigma = 24.0f;
  const auto glow_rect = SkRect::MakeXYWH(width_ - 220.0f, -58.0f, 230.0f, 150.0f);
  const auto glow_bounds = glow_rect.makeOutset(kGlowSigma * 3.0f, kGlowSigma * 3.0f);
  SkPaint glow_layer;
  glow_layer.setImageFilter(SkImageFilters::Blur(kGlowSigma, kGlowSigma, nullptr));
  canvas->saveLayer(&glow_bounds, &glow_layer);
  canvas->drawOval(glow_rect, glow);
  canvas->restore();
}

HomePage::HomePage(Widget *parent) : ScrollArea(parent), content_(new HomeContent(this)) {
}

void HomePage::layout_children() {
  content_->set_preferred_height(std::max(height(), content_->requiredHeight(width())));
  ScrollArea::layout_children();
}

} // namespace pages

std::size_t columnCount(const float width) {
  return width - kHomePadding.left - kHomePadding.right >= kWideGrid ? 2 : 1;
}
