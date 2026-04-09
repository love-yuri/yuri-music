//
// Created by yuri on 2026/2/7.
//
export module pages:home;

import std;
import ui;
import skia;
import components;

using namespace ui::layout;
using namespace ui::widgets;
using namespace ui::render;
using namespace skia;
using namespace components;

// 副标题颜色
constexpr SkColor kSubtitleColor = ColorFromARGB(255, 140, 140, 140);

// 图标颜色
constexpr SkColor kIconBlue = ColorFromARGB(255, 59, 130, 246);
constexpr SkColor kIconPurple = ColorFromARGB(255, 139, 92, 246);
constexpr SkColor kIconTeal = ColorFromARGB(255, 6, 182, 212);
constexpr SkColor kIconOrange = ColorFromARGB(255, 249, 115, 22);

// 布局尺寸
static constexpr float kHeaderHeight = 70.0f;
static constexpr float kCardRowHeight = 82.0f;
static constexpr float kSectionHeight = 24.0f;
static constexpr float kSongItemHeight = 64.0f;

export namespace pages {

// 首页
class HomePage : public Widget {
public:
  explicit HomePage(Widget *parent = nullptr);

  void layoutChildren() override;
  void paint(SkCanvas *canvas) override;

private:
  // 标题区（直接渲染在 padding 区域）
  RenderText greeting_text_{ "下午好" };
  RenderText subtitle_text_{ "发现适合现在聆听的音乐" };

  // 卡片行容器
  Widget *card_row1_;
  Widget *card_row2_;

  // 分段标题
  SectionHeader *section_recent_;

  // 最近播放
  SongItem *song_1_;
  SongItem *song_2_;
  SongItem *song_3_;
};

HomePage::HomePage(Widget *parent) : Widget(parent) {
  // 内边距：左32，上70（标题区），右32，下0
  setPadding(Insets(32, kHeaderHeight, 32, 0));
  // 使用垂直布局排列子控件
  setLayout<VBoxLayout<Widget>>();

  // 标题
  greeting_text_.setFontSize(28);
  greeting_text_.setColor(skia_colors::black);
  greeting_text_.setAlignment(Alignment::TopLeft);

  // 副标题
  subtitle_text_.setFontSize(13);
  subtitle_text_.setColor(kSubtitleColor);
  subtitle_text_.setAlignment(Alignment::TopLeft);

  // ── 卡片行1 ──
  card_row1_ = new Widget(this);
  card_row1_->setLayout<HBoxLayout<Widget>>();
  card_row1_->setMinHeight(kCardRowHeight);
  card_row1_->setMaxHeight(kCardRowHeight);

  new QuickCard("每日推荐", "为你精选的 30 首歌曲", kIconBlue, card_row1_);
  new QuickCard("热门榜单", "实时更新的流行音乐", kIconPurple, card_row1_);

  // ── 卡片行2 ──
  card_row2_ = new Widget(this);
  card_row2_->setLayout<HBoxLayout<Widget>>();
  card_row2_->setMinHeight(kCardRowHeight);
  card_row2_->setMaxHeight(kCardRowHeight);
  new QuickCard("私人电台", "根据你的口味生成", kIconTeal, card_row2_);
  new QuickCard("助眠音乐", "放松身心的轻音乐", kIconOrange, card_row2_);

  // ── 分段标题 ──
  section_recent_ = new SectionHeader("最近播放", "查看全部", this);
  section_recent_->setMinHeight(kSectionHeight);
  section_recent_->setMaxHeight(kSectionHeight);

  // ── 歌曲列表 ──
  song_1_ = new SongItem(1, "夜曲", "周杰伦 · 十一月的肖邦", "4:25", this);
  song_1_->setMinHeight(kSongItemHeight);
  song_1_->setMaxHeight(kSongItemHeight);

  song_2_ = new SongItem(2, "晴天", "周杰伦 · 叶惠美", "4:29", this);
  song_2_->setMinHeight(kSongItemHeight);
  song_2_->setMaxHeight(kSongItemHeight);

  song_3_ = new SongItem(3, "起风了", "买辣椒也用券", "5:21", this);
  song_3_->setMinHeight(kSongItemHeight);
  song_3_->setMaxHeight(kSongItemHeight);
}

void HomePage::layoutChildren() {
  // 更新标题渲染节点（绘制在 padding 区域）
  greeting_text_.update(SkRect::MakeXYWH(32, 16, width_ - 64, 36));
  subtitle_text_.update(SkRect::MakeXYWH(32, 52, width_ - 64, 18));

  // 让 VBoxLayout 排列子控件并递归更新
  Widget::layoutChildren();
}

void HomePage::paint(SkCanvas *canvas) {
  greeting_text_.render(canvas);
  subtitle_text_.render(canvas);
}

} // namespace pages
