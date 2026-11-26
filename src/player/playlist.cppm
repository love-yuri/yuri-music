export module player:playlist;

import std;
import models;

namespace player {

/** 管理当前歌单和切歌顺序。 */
export class Playlist {
public:
  /**
   * 选中歌单中的指定歌曲。
   * @param mid 选中歌曲的 mid
   * @return 选中的歌曲，未找到时为空
   */
  const SongInfo *select_song(std::string_view mid);

  /**
   * 追加同一歌单中后续加载的歌曲。
   * @param songs 新加载的歌曲
   */
  void append_songs(std::vector<SongInfo> songs);

  /** 返回歌单中已加载的歌曲。 */
  [[nodiscard]] std::span<const SongInfo> songs() const;

  /**
   * 将歌曲加入一次性的下一首队列。
   * @param song 待播放的歌曲
   */
  void enqueue_next(const SongInfo &song);

  /**
   * 设置下一首的选择模式。
   * @param random 是否随机播放
   */
  void set_random(bool random);

  /** 返回当前选中的歌曲。 */
  [[nodiscard]] const SongInfo *current_song() const;

  /** 选中并返回上一首，随机模式下沿播放历史回退。 */
  const SongInfo *previous();

  /** 选中并返回下一首，优先使用下一首队列。 */
  const SongInfo *next();

private:
  struct Entry {
    SongInfo song;                    // 选中的歌曲
    std::optional<std::size_t> index; // 对应的歌单位置，插播时保留原位置
  };

  static constexpr std::size_t kMaxHistory = 100;          // 保留的播放历史数量
  std::vector<SongInfo> songs_;                            // 当前循环的歌单
  std::optional<Entry> current_;                           // 当前选中的歌曲
  std::deque<Entry> history_;                              // 已播放的歌曲
  std::deque<SongInfo> next_songs_;                        // 一次性插播队列
  std::mt19937 random_engine_ { std::random_device {}() }; // 随机播放引擎
  bool random_ = false;                                    // 是否启用随机播放
};

const SongInfo *Playlist::select_song(const std::string_view mid) {
  const auto selected = std::ranges::find(songs_, mid, &SongInfo::mid);
  if (selected == songs_.end()) {
    return nullptr;
  }
  history_.clear();
  current_ = Entry { *selected, static_cast<std::size_t>(selected - songs_.begin()) };
  return current_song();
}

void Playlist::append_songs(std::vector<SongInfo> songs) {
  songs_.append_range(songs | std::views::as_rvalue);
}

std::span<const SongInfo> Playlist::songs() const {
  return songs_;
}

void Playlist::enqueue_next(const SongInfo &song) {
  next_songs_.push_back(song);
}

void Playlist::set_random(const bool random) {
  random_ = random;
}

const SongInfo *Playlist::current_song() const {
  return current_ ? &current_->song : nullptr;
}

const SongInfo *Playlist::previous() {
  if (!history_.empty()) {
    current_ = std::move(history_.back());
    history_.pop_back();
    return current_song();
  }
  if (random_ || songs_.empty() || !current_ || !current_->index) {
    return nullptr;
  }

  const auto index = *current_->index == 0 ? songs_.size() - 1 : *current_->index - 1;
  current_ = Entry { songs_[index], index };
  return current_song();
}

const SongInfo *Playlist::next() {
  if (next_songs_.empty() && songs_.empty()) {
    return nullptr;
  }

  const auto index = current_ ? current_->index : std::nullopt;
  if (current_) {
    history_.push_back(*current_);
    if (history_.size() > kMaxHistory) {
      history_.pop_front();
    }
  }

  if (!next_songs_.empty()) {
    current_ = Entry { std::move(next_songs_.front()), index };
    next_songs_.pop_front();
    return current_song();
  }

  std::size_t next_index = index ? (*index + 1) % songs_.size() : 0;
  if (random_ && songs_.size() > 1) {
    const auto current =
      current_ ? std::ranges::find(songs_, current_->song.mid, &SongInfo::mid) : songs_.end();
    const auto excluded = static_cast<std::size_t>(current - songs_.begin());
    const auto last = songs_.size() - (current == songs_.end() ? 1 : 2);
    next_index = std::uniform_int_distribution<std::size_t>(0, last)(random_engine_);
    if (next_index >= excluded) {
      ++next_index;
    }
  }
  current_ = Entry { songs_[next_index], next_index };
  return current_song();
}

} // namespace player
