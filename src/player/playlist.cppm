export module player:playlist;

import std;
import models;
import qq_music_api;
import yuri.core;
import :audio_player;
import :bass24_player;

constexpr int kPageSize = 30; // 每次加载的歌曲数量

namespace player {

/** 在线程池中分页加载 QQ 音乐歌单。使用 shared_ptr 持有。 */
export class PlaylistSource : public std::enable_shared_from_this<PlaylistSource> {
public:
  Signal<std::span<SongInfo>> songs_loaded; // 追加了新一页歌曲的事件

  /**
   * 创建歌单。
   * @param info 歌单信息
   */
  explicit PlaylistSource(PlayListInfo info);

  /** 返回歌单信息。 */
  [[nodiscard]] const PlayListInfo &info() const;

  /** 返回已加载的歌曲数量。 */
  [[nodiscard]] std::size_t size() const;

  /**
   * 返回指定位置的歌曲。
   * @param index 已加载歌曲的下标
   */
  [[nodiscard]] SongInfo song(std::size_t index) const;

  /**
   * 查找已加载歌曲在歌单中的位置。
   * @param mid 歌曲 mid
   */
  [[nodiscard]] std::optional<std::size_t> find_song(std::string_view mid) const;

  /**
   * 在加载前连接分页通知。
   * @param slot 后台回调，歌曲视图只在回调期间有效
   */
  void connect_page_loaded(std::function<void(std::span<SongInfo>)> slot);

  /**
   * 在后台加载下一页歌曲。
   * @return 完成后报告是否追加了歌曲
   */
  std::future<bool> load_more();

  /** 预留加载完整歌单的入口。 */
  void load_all();

private:
  const PlayListInfo info_;                              // 初始歌单信息
  std::vector<SongInfo> songs_;                          // 已加载的歌曲
  int song_count_;                                       // 歌曲总数
  std::atomic_bool loading_ = false;                     // 是否正在加载
  std::function<void(std::span<SongInfo>)> page_loaded_; // 分页追加回调
};

/** 管理歌单的播放顺序、插播队列和当前会话的播放历史。 */
export class PlaylistPlayer {
public:
  /**
   * 创建歌单播放器，并订阅自然播放完成事件。
   * @param audio 负责实际播放的音频播放器
   */
  explicit PlaylistPlayer(AudioPlayer &audio);

  /** 断开音频播放器的订阅。 */
  ~PlaylistPlayer();

  /** 播放器的信号订阅与实例绑定，禁止复制。 */
  PlaylistPlayer(const PlaylistPlayer &) = delete;

  /** 播放器的信号订阅与实例绑定，禁止复制赋值。 */
  PlaylistPlayer &operator=(const PlaylistPlayer &) = delete;

  /**
   * 设置当前歌单。换歌单时重置播放路径。
   * @param source 待播放的歌单
   */
  void set_source(const std::shared_ptr<PlaylistSource> &source);

  /**
   * 播放当前歌单中的一首歌。
   * @param mid 歌曲 mid
   */
  void play(std::string_view mid);

  /** 沿播放历史回退，顺序模式下可从歌单位置选择上一首。 */
  void previous();

  /** 优先播放排队歌曲或历史中的后续歌曲，再按当前模式选曲。 */
  void next();

  /**
   * 将歌曲追加到一次性插播队列。
   * @param song 待播放的歌曲
   */
  void enqueue_next(const SongInfo &song);

  /**
   * 设置下一首的选择模式。
   * @param random 是否启用随机播放
   */
  void set_random(bool random);

  /** 返回当前歌单内最近的播放路径，从最早保留的一首到最新一首。 */
  [[nodiscard]] std::vector<SongInfo> history() const;

private:
  struct Entry {
    SongInfo song;                    // 已选中的歌曲
    std::optional<std::size_t> index; // 歌单位置，插播时沿用原位置
  };

  /**
   * 从当前歌单选择一首歌。
   * @param index 歌曲在歌单中的位置
   */
  void play_at(std::size_t index);

  /**
   * 在当前播放路径中追加歌曲并开始播放。
   * @param entry 本次选中的歌曲和对应的歌单位置
   */
  void play_entry(Entry entry);

  static constexpr std::size_t kMaxHistory = 100;          // 播放历史最多保留的歌曲数
  AudioPlayer &audio_;                                     // 实际播放音频的播放器
  std::shared_ptr<PlaylistSource> source_;                 // 当前播放的歌单
  std::deque<Entry> history_;                              // 当前播放路径
  std::size_t history_pos_ = 0;                            // 当前歌曲在播放路径中的位置
  std::deque<SongInfo> next_songs_;                        // 一次性插播队列
  std::mt19937 random_engine_ { std::random_device {}() }; // 随机播放引擎
  bool random_ = false;                                    // 是否启用随机播放
};

PlaylistSource::PlaylistSource(PlayListInfo info) :
  info_(std::move(info)), song_count_(info_.song_count) {
}

const PlayListInfo &PlaylistSource::info() const {
  return info_;
}

std::size_t PlaylistSource::size() const {
  return songs_.size();
}

SongInfo PlaylistSource::song(const std::size_t index) const {
  return songs_[index];
}

std::optional<std::size_t> PlaylistSource::find_song(const std::string_view mid) const {
  const auto found = std::ranges::find(songs_, mid, &SongInfo::mid);
  if (found == songs_.end()) {
    return std::nullopt;
  }
  return std::optional(static_cast<std::size_t>(found - songs_.begin()));
}

void PlaylistSource::connect_page_loaded(std::function<void(std::span<SongInfo>)> slot) {
  page_loaded_ = std::move(slot);
  songs_loaded.connect(page_loaded_);
}

std::future<bool> PlaylistSource::load_more() {
  using namespace qqmusic_api::playlist;

  if (loading_ || std::cmp_greater_equal(songs_.size(), song_count_)) {
    std::promise<bool> result;
    result.set_value(false);
    return result.get_future();
  }

  loading_ = true;
  return thread_manager->addTask([self = shared_from_this()] {
    bool loaded = false;
    try {
      const auto begin = static_cast<int>(self->songs_.size());
      const auto result = get_user_playlists_detail(self->info_.tid, begin, kPageSize);
      const auto &page = result.req_1.data.songlist;
      if (!page.empty()) {
        self->songs_.reserve(self->songs_.size() + page.size());
        for (const auto &music : page) {
          self->songs_.emplace_back(SongInfo::from_api(music));
        }
        self->songs_loaded.emit(std::span(self->songs_).subspan(begin, page.size()));
        loaded = true;
      }
    } catch (const std::exception &error) {
      yuri::error("加载歌单“{}”失败: {}", self->info_.name, error.what());
    }
    self->loading_ = false;

    return loaded;
  });
}

void PlaylistSource::load_all() {
}

PlaylistPlayer::PlaylistPlayer(AudioPlayer &audio) : audio_(audio) {
  audio_.finished.connect<&PlaylistPlayer::next>(this);
}

PlaylistPlayer::~PlaylistPlayer() {
  audio_.finished.disconnect(this);
}

void PlaylistPlayer::set_source(const std::shared_ptr<PlaylistSource> &source) {
  if (source_ != source) {
    source_ = source;
    history_.clear();
    history_pos_ = 0;
  }
}

void PlaylistPlayer::play(const std::string_view mid) {
  if (const auto index = source_ ? source_->find_song(mid) : std::nullopt) {
    play_at(*index);
  }
}

void PlaylistPlayer::previous() {
  if (history_.empty()) {
    return;
  }

  if (history_pos_ > 0) {
    audio_.play_song_auto(history_[--history_pos_].song);
    return;
  }

  const auto index = history_.front().index;
  if (random_ || !index) {
    return;
  }

  // 历史已回到起点时，沿歌单向前扩展播放路径。
  const auto previous_index = *index == 0 ? source_->size() - 1 : *index - 1;
  history_.push_front({ source_->song(previous_index), previous_index });
  if (history_.size() > kMaxHistory) {
    history_.pop_back();
  }
  audio_.play_song_auto(history_.front().song);
}

void PlaylistPlayer::next() {
  const auto *current = history_.empty() ? nullptr : &history_[history_pos_];
  const auto index = current ? current->index : std::nullopt;
  if (!next_songs_.empty()) {
    auto song = std::move(next_songs_.front());
    next_songs_.pop_front();
    play_entry({ std::move(song), index });
    return;
  }

  if (history_pos_ + 1 < history_.size()) {
    audio_.play_song_auto(history_[++history_pos_].song);
    return;
  }

  const auto count = source_ ? source_->size() : 0;
  if (count == 0) {
    return;
  }
  if (!random_) {
    play_at(index ? (*index + 1) % count : 0);
    return;
  }

  const auto excluded = current ? source_->find_song(current->song.mid) : std::nullopt;
  const bool skip_current = count > 1 && excluded && *excluded < count;
  const auto last = count - (skip_current ? 2 : 1);
  auto next_index = std::uniform_int_distribution<std::size_t>(0, last)(random_engine_);
  if (skip_current && next_index >= *excluded) {
    ++next_index;
  }
  play_at(next_index);
}

void PlaylistPlayer::enqueue_next(const SongInfo &song) {
  next_songs_.push_back(song);
}

void PlaylistPlayer::set_random(const bool random) {
  random_ = random;
}

std::vector<SongInfo> PlaylistPlayer::history() const {
  return history_ | std::views::transform(&Entry::song) | std::ranges::to<std::vector>();
}

void PlaylistPlayer::play_at(const std::size_t index) {
  play_entry({ source_->song(index), index });
}

void PlaylistPlayer::play_entry(Entry entry) {
  if (!history_.empty()) {
    history_.resize(history_pos_ + 1);
  }
  history_.push_back(std::move(entry));
  if (history_.size() > kMaxHistory) {
    history_.pop_front();
  }
  history_pos_ = history_.size() - 1;
  audio_.play_song_auto(history_.back().song);
}

/** 返回应用内共享的歌单播放器。 */
export PlaylistPlayer &playlist_player() {
  static PlaylistPlayer instance(bass24_player());
  return instance;
}

} // namespace player
