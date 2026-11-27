//
// Created by yuri on 2026/2/7.
//

export module pages:favorites;

import std;
import components;
import yuri.core;
import models;
import qq_music_api;
import yuri.skia;
import store;
import yuri.ui;
import player;

using namespace components;
using namespace skia;
using namespace ui::layout;
using namespace ui::render;
using namespace ui::widgets;
using namespace player;

constexpr std::string_view kFavoritesName = "我喜欢"; // “我喜欢”歌单的名称

export namespace pages {

class FavoritesPage : public Border {
public:
  /**
   * 创建我喜欢页面。
   * @param parent 父级控件
   */
  explicit FavoritesPage(Widget *parent = nullptr);

  /** 断开用户状态、播放和菜单的订阅。 */
  ~FavoritesPage() override;

  /**
   * 绘制我喜欢背景。
   * @param canvas Skia 绘制画布
   */
  void paint(SkCanvas *canvas) override;

private:
  /**
   * 根据滚动位置检查是否需要加载更多歌曲。
   * @param scroll_offset 当前滚动偏移
   */
  void on_scroll_changed(float scroll_offset);

  /** 请求加载下一页歌曲。 */
  void load_more();

  /** 在后台查找“我喜欢”歌单。 */
  void find_playlist();

  /**
   * 将新加载的歌曲添加到列表。
   * @param count 这次加载的歌曲数量
   */
  void show_loaded_songs(std::size_t count);

  /**
   * 处理歌曲行双击事件。
   * @param item 被双击的歌曲行
   */
  void on_song_double_clicked(const SongItem *item);

  /**
   * 打开歌曲行的上下文菜单。
   * @param item 请求菜单的歌曲行
   */
  void on_song_context_menu_requested(const SongItem *item);

  /**
   * 处理歌曲上下文菜单点击。
   * @param index 菜单项索引
   */
  void on_song_context_menu_item_clicked(std::size_t index);

  /**
   * 根据应用级当前歌曲更新页面选中状态。
   * @param info 当前播放歌曲
   */
  void on_current_song_changed(const std::shared_ptr<const SongInfo> &info);

  std::shared_ptr<PlaylistSource> playlist_source_; // 页面展示的“我喜欢”歌单
  SongItem *selected_item_ {};                      // 当前页面选中的歌曲项
  ScrollArea *items_ {};                            // 歌曲列表
  std::vector<SongItem *> song_items_;              // 按列表顺序保存歌曲项
  ContextMenu *song_context_menu_;                  // 挂在根窗口上的歌曲上下文菜单
  const SongItem *context_menu_item_ {};            // 菜单对应的歌曲行
  std::string current_song_mid_;                    // 当前歌曲标识，用于选中稍后显示的歌曲行
  bool finding_playlist_ = false;                   // 是否正在查找歌单
  std::shared_ptr<bool> lifetime_ = std::make_shared<bool>(); // 延迟任务使用的页面存活标记
};

FavoritesPage::FavoritesPage(Widget *parent) :
  Border(parent),
  song_context_menu_(
    new ContextMenu({ ContextMenuItem("下一首播放"), ContextMenuItem("评论") }, this)
  ) {
  set_padding(Insets(28, 24, 34, 12));
  auto *const content = new Widget(this);
  auto *const layout = new VBoxLayout();
  layout->set_spacing(8.0f);
  content->set_layout(layout);

  const auto title = new Text("我喜欢", content);
  title->set_font_size(30);
  title->set_color(ColorFromARGB(255, 20, 26, 36));
  title->set_alignment(Alignment::CenterLeft);
  title->set_fixed_height(42.0f);

  const auto subtitle = new Text("双击歌曲开始播放，列表会在滚动到底部时继续加载", content);
  subtitle->set_font_size(12.5f);
  subtitle->set_color(ColorFromARGB(170, 56, 68, 86));
  subtitle->set_alignment(Alignment::CenterLeft);
  subtitle->set_fixed_height(24.0f);

  items_ = new ScrollArea(content);
  items_->scroll_changed.connect<&FavoritesPage::on_scroll_changed>(this);
  store::user_profile_store.status_changed.connect<&FavoritesPage::load_more>(this);
  song_context_menu_->item_clicked.connect<&FavoritesPage::on_song_context_menu_item_clicked>(this);
  bass24_player().song_changed.connect<&FavoritesPage::on_current_song_changed>(this);
}

FavoritesPage::~FavoritesPage() {
  song_context_menu_->item_clicked.disconnect(this);
  store::user_profile_store.status_changed.disconnect(this);
  bass24_player().song_changed.disconnect(this);
}

void FavoritesPage::paint(SkCanvas *canvas) {
  SkPaint fill;
  fill.setAntiAlias(true);
  fill.setColor(ColorFromARGB(110, 255, 255, 255));
  canvas->drawRect(local_bounds(), fill);

  SkPaint top_light;
  top_light.setAntiAlias(true);
  top_light.setColor(ColorFromARGB(34, 255, 255, 255));
  canvas->drawRect(SkRect::MakeXYWH(0.0f, 0.0f, width_, 92.0f), top_light);
}

void FavoritesPage::on_scroll_changed(const float scroll_offset) {
  const auto item_count = static_cast<int>(song_items_.size());
  if (item_count == 0) {
    return;
  }

  constexpr float kSongItemHeight = 68.0f;
  const int bottom_index = static_cast<int>((scroll_offset + items_->height()) / kSongItemHeight);
  if (item_count - bottom_index <= 5) {
    load_more();
  }
}

void FavoritesPage::load_more() {
  if (playlist_source_) {
    playlist_source_->load_more();
  } else {
    find_playlist();
  }
}

void FavoritesPage::find_playlist() {
  if (finding_playlist_) {
    return;
  }

  finding_playlist_ = true;
  thread_manager->addTask([this, lifetime = std::weak_ptr(lifetime_)] {
    std::optional<PlayListInfo> info;
    try {
      for (const auto &value : qqmusic_api::playlist::get_user_playlists().data.disslist) {
        if (value.diss_name == kFavoritesName) {
          info = PlayListInfo::from_api(value);
          break;
        }
      }
    } catch (const std::exception &error) {
      yuri::warn("查找“我喜欢”歌单失败: {}", error.what());
    }

    ui::dispatcher.post([this, lifetime, info = std::move(info)] mutable {
      if (lifetime.expired()) {
        return;
      }

      finding_playlist_ = false;
      if (info) {
        playlist_source_ = std::make_shared<PlaylistSource>(std::move(*info));
        playlist_source_->connect_page_loaded([this, lifetime](const std::span<SongInfo> songs) {
          ui::dispatcher.post([this, lifetime, count = songs.size()] {
            if (!lifetime.expired()) {
              show_loaded_songs(count);
            }
          });
        });
        load_more();
      }
    });
  });
}

void FavoritesPage::show_loaded_songs(const std::size_t count) {
  const auto first = song_items_.size();
  const auto end = first + count;

  for (auto index = first; index < end; ++index) {
    const auto song = playlist_source_->song(index);
    auto *const item = new SongItem(static_cast<int>(index), song, false, items_);
    item->doubleClicked.connect<&FavoritesPage::on_song_double_clicked>(this);
    item->contextMenuRequested.connect<&FavoritesPage::on_song_context_menu_requested>(this);
    song_items_.push_back(item);
    if (selected_item_ == nullptr && !current_song_mid_.empty() && song.mid == current_song_mid_) {
      selected_item_ = item;
      item->setSelected(true);
    }
  }

  mark_layout_dirty();
}

void FavoritesPage::on_song_double_clicked(const SongItem *item) {
  auto &player = playlist_player();
  player.set_source(playlist_source_);
  player.play(item->info().mid);
}

void FavoritesPage::on_song_context_menu_requested(const SongItem *item) {
  if (item != nullptr) {
    context_menu_item_ = item;
    song_context_menu_->popup_at_cursor();
  }
}

void FavoritesPage::on_song_context_menu_item_clicked(const std::size_t index) {
  if (index == 0 && context_menu_item_) {
    playlist_player().enqueue_next(context_menu_item_->info());
  }
  context_menu_item_ = nullptr;
}

void FavoritesPage::on_current_song_changed(const std::shared_ptr<const SongInfo> &info) {
  if (info == nullptr) {
    return;
  }
  current_song_mid_ = info->mid;
  const auto selected = std::ranges::find_if(song_items_, [&info](const SongItem *item) {
    return item->info().mid == info->mid;
  });

  SongItem *const next_selected = selected == song_items_.end() ? nullptr : *selected;
  if (selected_item_ != nullptr && selected_item_ != next_selected) {
    selected_item_->setSelected(false);
  }
  selected_item_ = next_selected;
  if (selected_item_ != nullptr) {
    selected_item_->setSelected(true);
  }
}

} // namespace pages
