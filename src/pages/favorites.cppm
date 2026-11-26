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

constexpr int kFavoritesPageSize = 30; // 每次加载的歌曲数量

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

  /** 播放我喜欢歌单中的上一首。 */
  void play_previous();

  /** 播放我喜欢歌单中的下一首。 */
  void play_next();

  /**
   * 设置歌单的随机播放模式。
   * @param random 是否启用随机播放
   */
  void set_random(bool random);

private:
  /**
   * 根据滚动位置检查是否需要加载更多歌曲。
   * @param scroll_offset 当前滚动偏移
   */
  void check_load_more(float scroll_offset);

  /** 显示下一页歌曲，必要时等待后台加载。 */
  void load_more();

  /** 在后台获取歌单的下一页。 */
  void fetch_next_page();

  /** 将待显示的一页歌曲添加到列表。 */
  void show_loaded_songs();

  /**
   * 将后台加载结果应用到页面。
   * @param tid 我喜欢歌单 ID
   * @param songs 本次加载的歌曲
   * @param song_count 歌单歌曲总数，未知时为 0
   */
  void apply_loaded_songs(std::uint64_t tid, std::vector<SongInfo> songs, int song_count);

  /**
   * 处理歌曲加载失败。
   * @param message 失败信息
   */
  void on_load_failed(const std::string &message);

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

  Playlist playlist_;                    // 我喜欢歌单和播放顺序
  SongItem *selected_item_ {};           // 当前页面选中的歌曲项
  ScrollArea *items_ {};                 // 歌曲列表
  std::vector<SongItem *> song_items_;   // 按列表顺序保存歌曲项
  ContextMenu *song_context_menu_;       // 挂在根窗口上的歌曲上下文菜单
  const SongItem *context_menu_item_ {}; // 菜单对应的歌曲行
  std::string current_song_mid_;         // 当前歌曲标识，用于选中稍后显示的歌曲行
  std::uint64_t tid_ {};                 // 当前歌单 ID
  int playlist_song_count_ {};           // 最近获取的歌单歌曲总数
  bool loading_ = false;                 // 是否正在加载
  bool playlist_complete_ = false;       // 是否已获取完整歌单
  bool prefetch_all_ = false;            // 是否正在为播放预取整张歌单
  bool show_more_ = false;               // 是否有待显示的分页
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
  items_->scroll_changed.connect<&FavoritesPage::check_load_more>(this);
  store::user_profile_store.status_changed.connect<&FavoritesPage::load_more>(this);
  song_context_menu_->item_clicked.connect<&FavoritesPage::on_song_context_menu_item_clicked>(this);
  auto &audio = bass24_player();
  audio.song_changed.connect<&FavoritesPage::on_current_song_changed>(this);
  audio.finished.connect<&FavoritesPage::play_next>(this);
}

FavoritesPage::~FavoritesPage() {
  song_context_menu_->item_clicked.disconnect(this);
  store::user_profile_store.status_changed.disconnect(this);
  auto &audio = bass24_player();
  audio.song_changed.disconnect(this);
  audio.finished.disconnect(this);
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

void FavoritesPage::play_previous() {
  if (const auto *song = playlist_.previous()) {
    bass24_player().play_song_auto(*song);
  }
}

void FavoritesPage::play_next() {
  if (const auto *song = playlist_.next()) {
    bass24_player().play_song_auto(*song);
  }
}

void FavoritesPage::set_random(const bool random) {
  playlist_.set_random(random);
}

void FavoritesPage::check_load_more(const float scroll_offset) {
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
  if (!show_more_) {
    show_more_ = true;
    // 滚动信号可能在布局期间触发，等布局结束后再添加歌曲行。
    ui::dispatcher.post([this] {
      show_loaded_songs();
    });
  }
  fetch_next_page();
}

void FavoritesPage::fetch_next_page() {
  if (loading_ || playlist_complete_) {
    return;
  }

  loading_ = true;
  const auto current_offset = static_cast<int>(playlist_.songs().size());
  const std::uint64_t current_tid = tid_;
  const int known_song_count = playlist_song_count_;
  thread_manager->addTask([this, current_offset, current_tid, known_song_count] {
    using namespace qqmusic_api::playlist;
    try {
      std::uint64_t resolved_tid = current_tid;
      int song_count = known_song_count;
      if (resolved_tid == 0) {
        for (const auto &value : get_user_playlists().data.disslist) {
          if (value.diss_name == "我喜欢") {
            resolved_tid = value.tid;
            song_count = value.song_cnt;
            break;
          }
        }
      }

      if (resolved_tid == 0) {
        ui::dispatcher.post([this] {
          apply_loaded_songs(0, {}, 0);
        });
        return;
      }

      const auto result =
        get_user_playlists_detail(resolved_tid, current_offset, kFavoritesPageSize);
      if (result.code != 0 || result.req_1.code != 0) {
        throw std::runtime_error("获取歌单详情失败");
      }

      const auto &data = result.req_1.data;
      if (data.dirinfo.songnum > 0) {
        song_count = data.dirinfo.songnum;
      }
      if (data.songlist.empty() && current_offset < song_count) {
        throw std::runtime_error("歌单歌曲尚未加载完整");
      }

      std::vector<SongInfo> songs;
      songs.reserve(data.songlist.size());
      for (const auto &music : data.songlist) {
        songs.emplace_back(SongInfo::from_api(music));
      }

      ui::dispatcher.post([this, resolved_tid, songs = std::move(songs), song_count] mutable {
        apply_loaded_songs(resolved_tid, std::move(songs), song_count);
      });
    } catch (const std::exception &e) {
      const std::string message = e.what();
      ui::dispatcher.post([this, message] {
        on_load_failed(message);
      });
    }
  });
}

void FavoritesPage::apply_loaded_songs(
  const std::uint64_t tid,
  std::vector<SongInfo> songs,
  const int song_count
) {
  loading_ = false;
  if (tid == 0) {
    yuri::warn("未找到“我喜欢”歌单");
    return;
  }

  tid_ = tid;
  playlist_song_count_ = song_count;
  const bool empty_page = songs.empty();
  playlist_.append_songs(std::move(songs));
  playlist_complete_ =
    empty_page || (song_count > 0 && std::cmp_greater_equal(playlist_.songs().size(), song_count));
  show_loaded_songs();
  if (prefetch_all_ || show_more_) {
    fetch_next_page();
  }
}

void FavoritesPage::show_loaded_songs() {
  const auto songs = playlist_.songs();
  const auto first = song_items_.size();
  const auto end = std::min(first + kFavoritesPageSize, songs.size());
  if (!show_more_ || first == end) {
    return;
  }

  show_more_ = false;
  for (auto index = first; index < end; ++index) {
    const auto &song = songs[index];
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

void FavoritesPage::on_load_failed(const std::string &message) {
  loading_ = false;
  yuri::warn("加载我喜欢歌单失败: {}", message);
}

void FavoritesPage::on_song_double_clicked(const SongItem *item) {
  if (item == nullptr) {
    return;
  }
  if (const auto *song = playlist_.select_song(item->info().mid)) {
    bass24_player().play_song_auto(*song);
    prefetch_all_ = true;
    fetch_next_page();
  }
}

void FavoritesPage::on_song_context_menu_requested(const SongItem *item) {
  if (item != nullptr) {
    context_menu_item_ = item;
    song_context_menu_->popup_at_cursor();
  }
}

void FavoritesPage::on_song_context_menu_item_clicked(const std::size_t index) {
  if (index == 0 && context_menu_item_) {
    playlist_.enqueue_next(context_menu_item_->info());
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
