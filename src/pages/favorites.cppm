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

export namespace pages {

class FavoritesPage : public Widget {
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

  /** 加载下一页歌曲。 */
  void loadMore();

  /** 处理用户资料状态变化并尝试加载歌单。 */
  void onUserProfileChanged();

  /**
   * 将后台加载结果应用到页面。
   * @param tid 我喜欢歌单 ID
   * @param current_offset 本次加载起始偏移
   * @param songs 本次加载的歌曲
   * @param playlist_found 是否找到我喜欢歌单
   */
  void applyLoadedSongs(
    std::uint64_t tid,
    int current_offset,
    const std::vector<SongInfo>& songs,
    bool playlist_found
  );

  /**
   * 处理歌曲加载失败。
   * @param message 失败信息
   */
  void onLoadFailed(const std::string &message);

  /**
   * 处理歌曲行双击事件。
   * @param item 被双击的歌曲行
   */
  void onSongDoubleClicked(const SongItem *item) const;

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

  /**
   * 获取当前已经加载的歌曲上下文。
   * @return 按页面顺序排列的歌曲信息
   */
  [[nodiscard]] std::vector<SongInfo> collectSongs() const;

  /**
   * 格式化歌曲时长。
   * @param seconds 歌曲时长，单位为秒
   * @return m:ss 格式的时长文本
   */
  static std::string formatDuration(int seconds);

  /**
   * 拼接歌手名称。
   * @param singers 歌手列表
   * @return 使用分隔符拼接后的歌手名称
   */
  static std::string formatSingers(const std::vector<SingerType> &singers);

  /**
   * 从 QQ 音乐歌曲信息构建列表项数据。
   * @param music QQ 音乐歌单歌曲信息
   * @return 歌曲行展示和播放所需的数据
   */
  static SongInfo makeSongInfo(const SonglistType &music);

  SongItem *selected_item{};            // 当前页面选中的歌曲项
  ScrollArea *items_{};                 // 歌曲列表
  std::vector<SongItem *> song_items{}; // 按列表顺序保存歌曲项
  std::uint64_t tid_{};                 // 当前歌单 ID
  int offset_{};                        // 当前加载偏移
  bool loading_ = false;                // 是否正在加载
  bool has_more = true;                 // 是否还有更多数据
};

FavoritesPage::FavoritesPage(Widget *parent) : Widget(parent) {
  setLayout<VBoxLayout<Widget>>();
  layout()->setSpacing(8);
  setPadding(Insets(28, 24, 34, 12));

  const auto title = new Text("我喜欢", this);
  title->setFontSize(30);
  title->setColor(ColorFromARGB(255, 20, 26, 36));
  title->setAlignment(Alignment::CenterLeft);
  title->setMaxHeight(42.0f);

  const auto subtitle = new Text("双击歌曲开始播放，列表会在滚动到底部时继续加载", this);
  subtitle->setFontSize(12.5f);
  subtitle->setColor(ColorFromARGB(170, 56, 68, 86));
  subtitle->setAlignment(Alignment::CenterLeft);
  subtitle->setMaxHeight(24.0f);

  items_ = new ScrollArea(this);
  items_->scrollChanged.connect<&FavoritesPage::checkLoadMore>(this);
  store::user_profile_store.status_changed.connect<&FavoritesPage::onUserProfileChanged>(this);
  playback::controller.currentSongChanged.connect<&FavoritesPage::onCurrentSongChanged>(this);
}

void FavoritesPage::paint(SkCanvas *canvas) {
  SkPaint fill;
  fill.setAntiAlias(true);
  fill.setColor(ColorFromARGB(110, 255, 255, 255));
  canvas->drawRect(borderRect(), fill);

  SkPaint top_light;
  top_light.setAntiAlias(true);
  top_light.setColor(ColorFromARGB(34, 255, 255, 255));
  canvas->drawRect(SkRect::MakeXYWH(0.0f, 0.0f, width_, 92.0f), top_light);
}

void FavoritesPage::checkLoadMore(const float scroll_offset) {
  if (loading_ || !has_more) {
    return;
  }

  const auto item_count = static_cast<int>(items_->children().size());
  if (item_count == 0) {
    return;
  }

  constexpr float kSongItemHeight = 68.0f;
  const int bottom_index =
    static_cast<int>((scroll_offset + items_->contentHeight()) / kSongItemHeight);
  if (item_count - bottom_index <= 5) {
    loadMore();
  }
}

void FavoritesPage::loadMore() {
  if (loading_ || !has_more) {
    return;
  }

  loading_ = true;
  const int current_offset = offset_;
  const std::uint64_t current_tid = tid_;
  thread_manager->addTask([this, current_offset, current_tid] {
    using namespace qqmusic_api::playlist;
    try {
      std::uint64_t resolved_tid = current_tid;
      if (resolved_tid == 0) {
        for (const auto &value : get_user_playlists().data.disslist) {
          if (value.diss_name == "我喜欢") {
            resolved_tid = value.tid;
            break;
          }
        }
      }

      if (resolved_tid == 0) {
        ui::dispatcher.post([this, current_offset] {
          applyLoadedSongs(0, current_offset, {}, false);
        });
        return;
      }

      const auto result = get_user_playlists_detail(resolved_tid, current_offset, 30).req_1.data;
      std::vector<SongInfo> songs;
      songs.reserve(result.songlist.size());
      for (const auto &music : result.songlist) {
        songs.emplace_back(makeSongInfo(music));
      }

      ui::dispatcher.post([this, resolved_tid, current_offset, songs = std::move(songs)]() mutable {
        applyLoadedSongs(resolved_tid, current_offset, songs, true);
      });
    } catch (const std::exception &e) {
      const std::string message = e.what();
      ui::dispatcher.post([this, message] {
        onLoadFailed(message);
      });
    }
  });
}

void FavoritesPage::onUserProfileChanged() {
  if (tid_ == 0 && song_items.empty()) {
    has_more = true;
  }
  loadMore();
}

void FavoritesPage::applyLoadedSongs(
  const std::uint64_t tid,
  const int current_offset,
  const std::vector<SongInfo>& songs,
  const bool playlist_found
) {
  loading_ = false;
  if (!playlist_found) {
    has_more = false;
    yuri::warn("未找到“我喜欢”歌单");
    return;
  }

  tid_ = tid;
  if (songs.empty()) {
    has_more = false;
    return;
  }

  const auto loaded_count = static_cast<int>(songs.size());
  int index = current_offset;
  for (auto &song : songs) {
    auto *const item = new SongItem(index++, song, false, items_);
    item->doubleClicked.connect<&FavoritesPage::onSongDoubleClicked>(this);
    item->contextMenuRequested.connect<&FavoritesPage::onSongContextMenuRequested>(this);
    song_items.push_back(item);
  }

  offset_ = current_offset + loaded_count;
  playback::controller.updateContext(kFavoritesContextId, collectSongs());
  markLayoutDirty();
}

void FavoritesPage::onLoadFailed(const std::string &message) {
  loading_ = false;
  yuri::warn("加载我喜欢歌单失败: {}", message);
}

void FavoritesPage::onSongDoubleClicked(const SongItem *item) const {
  if (item == nullptr) {
    return;
  }
  playback::controller.playFromContext(
    std::string(kFavoritesContextId), collectSongs(), item->info().mid
  );
}

void FavoritesPage::onSongContextMenuRequested(const SongItem *item) {
  if (item != nullptr) {
    songContextMenuRequested.emit(item->info());
  }
}

void FavoritesPage::onCurrentSongChanged(const SongInfo &info) {
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

std::vector<SongInfo> FavoritesPage::collectSongs() const {
  std::vector<SongInfo> songs;
  songs.reserve(song_items.size());
  for (const auto *item : song_items) {
    songs.push_back(item->info());
  }
  return songs;
}

std::string FavoritesPage::formatDuration(const int seconds) {
  char buffer[8];
  auto [end, ec] =
    std::format_to_n(buffer, sizeof(buffer) - 1, "{}:{:02}", seconds / 60, seconds % 60);
  *end = '\0';
  return { buffer, static_cast<std::size_t>(end - buffer) };
}

std::string FavoritesPage::formatSingers(const std::vector<SingerType> &singers) {
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

SongInfo FavoritesPage::makeSongInfo(const SonglistType &music) {
  return SongInfo {
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

} // namespace pages
