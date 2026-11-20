//
// Created by yuri on 2026/2/7.
//

export module pages:favorites;

import std;
import components;
import yuri.core;
import models;
import playback;
import qq_music_api;
import yuri.skia;
import store;
import yuri.ui;

using namespace components;
using namespace skia;
using namespace ui::layout;
using namespace ui::render;
using namespace ui::widgets;

constexpr std::string_view kFavoritesContextId = "favorites"; // 我喜欢播放上下文标识
constexpr int kFavoritesPageSize = 30;                        // 每次加载的歌曲数量

/**
 * 格式化歌曲时长。
 * @param seconds 歌曲时长，单位为秒
 * @return m:ss 格式的时长文本
 */
std::string formatDuration(int seconds);

/**
 * 拼接歌手名称。
 * @param singers 歌手列表
 * @return 使用分隔符拼接后的歌手名称
 */
std::string formatSingers(const std::vector<SingerType> &singers);

/**
 * 从 QQ 音乐歌曲信息构建列表项数据。
 * @param music QQ 音乐歌单歌曲信息
 * @return 歌曲行展示和播放所需的数据
 */
SongInfo makeSongInfo(const SonglistType &music);

export namespace pages {

class FavoritesPage : public Border {
public:
  /**
   * 创建我喜欢页面。
   * @param parent 父级控件
   */
  explicit FavoritesPage(Widget *parent = nullptr);

  /**
   * 绘制我喜欢背景。
   * @param canvas Skia 绘制画布
   */
  void paint(SkCanvas *canvas) override;

  Signal<const SongInfo &> songContextMenuRequested{}; // 歌曲上下文菜单请求事件

private:
  /**
   * 根据滚动位置检查是否需要加载更多歌曲。
   * @param scroll_offset 当前滚动偏移
   */
  void checkLoadMore(float scroll_offset);

  /** 显示下一页歌曲，必要时等待后台加载。 */
  void loadMore();

  /** 在后台获取歌单的下一页。 */
  void fetchNextPage();

  /** 将待显示的一页歌曲添加到列表。 */
  void showLoadedSongs();

  /**
   * 将后台加载结果应用到页面。
   * @param tid 我喜欢歌单 ID
   * @param songs 本次加载的歌曲
   * @param song_count 歌单歌曲总数，未知时为 0
   */
  void applyLoadedSongs(std::uint64_t tid, std::vector<SongInfo> songs, int song_count);

  /**
   * 处理歌曲加载失败。
   * @param message 失败信息
   */
  void onLoadFailed(const std::string &message);

  /**
   * 处理歌曲行双击事件。
   * @param item 被双击的歌曲行
   */
  void onSongDoubleClicked(const SongItem *item);

  /**
   * 在随机播放等待歌单时继续后台加载。
   * @param context_id 请求加载的播放上下文标识
   */
  void onContextSongsRequested(std::string_view context_id);

  /**
   * 转发歌曲行上下文菜单请求。
   * @param item 请求菜单的歌曲行
   */
  void onSongContextMenuRequested(const SongItem *item);

  /**
   * 根据应用级当前歌曲更新页面选中状态。
   * @param info 当前播放歌曲
   */
  void onCurrentSongChanged(const SongInfo &info);

  SongItem *selected_item{};            // 当前页面选中的歌曲项
  ScrollArea *items_{};                 // 歌曲列表
  std::vector<SongItem *> song_items{}; // 按列表顺序保存歌曲项
  std::vector<SongInfo> playlist_songs; // 按歌单顺序缓存已获取的歌曲
  std::string current_song_mid;         // 当前歌曲标识，用于选中稍后显示的歌曲行
  std::uint64_t tid_{};                 // 当前歌单 ID
  int playlist_song_count{};            // 最近获取的歌单歌曲总数
  bool loading_ = false;                // 是否正在加载
  bool playlist_complete = false;       // 是否已获取完整歌单
  bool prefetch_all = false;            // 是否正在为播放预取整张歌单
  bool show_more = false;               // 是否有待显示的分页
};

FavoritesPage::FavoritesPage(Widget *parent) : Border(parent) {
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
  items_->scroll_changed.connect<&FavoritesPage::checkLoadMore>(this);
  store::user_profile_store.status_changed.connect<&FavoritesPage::loadMore>(this);
  playback::controller.currentSongChanged.connect<&FavoritesPage::onCurrentSongChanged>(this);
  playback::controller.contextSongsRequested.connect<&FavoritesPage::onContextSongsRequested>(this);
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

void FavoritesPage::checkLoadMore(const float scroll_offset) {
  const auto item_count = static_cast<int>(song_items.size());
  if (item_count == 0) {
    return;
  }

  constexpr float kSongItemHeight = 68.0f;
  const int bottom_index = static_cast<int>((scroll_offset + items_->height()) / kSongItemHeight);
  if (item_count - bottom_index <= 5) {
    loadMore();
  }
}

void FavoritesPage::loadMore() {
  if (!show_more) {
    show_more = true;
    // 滚动信号可能在布局期间触发，等布局结束后再添加歌曲行。
    ui::dispatcher.post([this] { showLoadedSongs(); });
  }
  fetchNextPage();
}

void FavoritesPage::fetchNextPage() {
  if (loading_ || playlist_complete) {
    return;
  }

  loading_ = true;
  const auto current_offset = static_cast<int>(playlist_songs.size());
  const std::uint64_t current_tid = tid_;
  const int known_song_count = playlist_song_count;
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
        ui::dispatcher.post([this] { applyLoadedSongs(0, {}, 0); });
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
        songs.emplace_back(makeSongInfo(music));
      }

      ui::dispatcher.post([this, resolved_tid, songs = std::move(songs), song_count] mutable {
        applyLoadedSongs(resolved_tid, std::move(songs), song_count);
      });
    } catch (const std::exception &e) {
      const std::string message = e.what();
      ui::dispatcher.post([this, message] { onLoadFailed(message); });
    }
  });
}

void FavoritesPage::applyLoadedSongs(
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
  playlist_song_count = song_count;
  playlist_songs.append_range(songs | std::views::as_rvalue);
  playlist_complete =
    songs.empty() || (song_count > 0 && std::cmp_greater_equal(playlist_songs.size(), song_count));
  showLoadedSongs();
  if (playlist_complete) {
    playback::controller.updateContext(kFavoritesContextId, playlist_songs);
  } else if (prefetch_all || show_more) {
    fetchNextPage();
  }
}

void FavoritesPage::showLoadedSongs() {
  const auto first = song_items.size();
  const auto end = std::min(first + kFavoritesPageSize, playlist_songs.size());
  if (!show_more || first == end) {
    return;
  }

  show_more = false;
  for (auto index = first; index < end; ++index) {
    const auto &song = playlist_songs[index];
    auto *const item = new SongItem(static_cast<int>(index), song, false, items_);
    item->doubleClicked.connect<&FavoritesPage::onSongDoubleClicked>(this);
    item->contextMenuRequested.connect<&FavoritesPage::onSongContextMenuRequested>(this);
    song_items.push_back(item);
    if (selected_item == nullptr && !current_song_mid.empty() && song.mid == current_song_mid) {
      selected_item = item;
      item->setSelected(true);
    }
  }

  if (!playlist_complete) {
    playback::controller.updateContext(kFavoritesContextId, playlist_songs, false);
  }
  mark_layout_dirty();
}

void FavoritesPage::onLoadFailed(const std::string &message) {
  loading_ = false;
  yuri::warn("加载我喜欢歌单失败: {}", message);
}

void FavoritesPage::onSongDoubleClicked(const SongItem *item) {
  if (item == nullptr) {
    return;
  }
  prefetch_all = true;
  playback::controller.playFromContext(
    std::string(kFavoritesContextId), playlist_songs, item->info().mid, playlist_complete
  );
  fetchNextPage();
}

void FavoritesPage::onContextSongsRequested(const std::string_view context_id) {
  if (context_id == kFavoritesContextId) {
    fetchNextPage();
  }
}

void FavoritesPage::onSongContextMenuRequested(const SongItem *item) {
  if (item != nullptr) {
    songContextMenuRequested.emit(item->info());
  }
}

void FavoritesPage::onCurrentSongChanged(const SongInfo &info) {
  current_song_mid = info.mid;
  const auto selected = std::ranges::find_if(song_items, [&info](const SongItem *item) {
    return item->info().mid == info.mid;
  });

  SongItem *const next_selected = selected == song_items.end() ? nullptr : *selected;
  if (selected_item != nullptr && selected_item != next_selected) {
    selected_item->setSelected(false);
  }
  selected_item = next_selected;
  if (selected_item != nullptr) {
    selected_item->setSelected(true);
  }
}

} // namespace pages

std::string formatDuration(const int seconds) {
  char buffer[8];
  auto [end, ec] =
    std::format_to_n(buffer, sizeof(buffer) - 1, "{}:{:02}", seconds / 60, seconds % 60);
  *end = '\0';
  return { buffer, static_cast<std::size_t>(end - buffer) };
}

std::string formatSingers(const std::vector<SingerType> &singers) {
  if (singers.empty()) {
    return {};
  }
  if (singers.size() == 1) {
    return singers.front().name;
  }

  std::size_t length = 3 * (singers.size() - 1);
  for (const auto &singer : singers) {
    length += singer.name.size();
  }

  std::string result;
  result.reserve(length);
  result.append(singers.front().name);
  for (std::size_t index = 1; index < singers.size(); ++index) {
    result.append(" / ");
    result.append(singers[index].name);
  }
  return result;
}

SongInfo makeSongInfo(const SonglistType &music) {
  return SongInfo{
    .title = std::string(music.title),
    .artist = formatSingers(music.singer),
    .album_name = std::string(music.album.name),
    .duration = formatDuration(music.interval),
    .mid = std::string(music.mid),
    .album_mid = std::string(music.album.mid),
    .has_flac = music.file.size_flac > 0,
    .has_ape = music.file.size_ape > 0,
    .has_mp3_320 = music.file.size_320mp3 > 0,
    .has_mp3_128 = music.file.size_128mp3 > 0,
    .liked = true,
  };
}
