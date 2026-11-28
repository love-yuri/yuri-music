module;
#ifdef __linux__
#include <gio/gio.h>
#include "waybar_lyrics_dbus.hpp"
#endif

export module linux_tray;

#ifdef __linux__
import std;
import models;
import player;
import qq_music_api;
import yuri.core;

constexpr char kWatcherName[] = "org.kde.StatusNotifierWatcher";        // 托盘服务名
constexpr char kWatcherPath[] = "/StatusNotifierWatcher";               // 托盘服务对象路径
constexpr char kItemPath[] = "/StatusNotifierItem";                     // 托盘图标对象路径
constexpr char kMenuPath[] = "/MenuBar";                                // 托盘菜单对象路径
constexpr int kRootId = 0;                                              // 菜单根节点 ID
constexpr int kExitId = 1;                                              // 退出项 ID
constexpr int kRegisterTimeoutMs = 1500;                                // 托盘注册超时
constexpr auto kDoubleClickInterval = std::chrono::milliseconds(400);   // 双击允许的最大间隔
constexpr auto kDuplicateClickInterval = std::chrono::milliseconds(50); // 忽略合成双击事件的时长
constexpr char kTrayXml[] = R"xml(
<node>
  <interface name="org.kde.StatusNotifierItem">
    <method name="Activate">
      <arg type="i" direction="in"/><arg type="i" direction="in"/>
    </method>
    <property name="Category" type="s" access="read"/>
    <property name="Id" type="s" access="read"/>
    <property name="Title" type="s" access="read"/>
    <property name="Status" type="s" access="read"/>
    <property name="IconName" type="s" access="read"/>
    <property name="ItemIsMenu" type="b" access="read"/>
    <property name="Menu" type="o" access="read"/>
  </interface>
  <interface name="com.canonical.dbusmenu">
    <property name="Version" type="u" access="read"/>
    <method name="GetLayout">
      <arg type="i" direction="in"/><arg type="i" direction="in"/>
      <arg type="as" direction="in"/>
      <arg type="u" direction="out"/><arg type="(ia{sv}av)" direction="out"/>
    </method>
    <method name="GetGroupProperties">
      <arg type="ai" direction="in"/><arg type="as" direction="in"/>
      <arg type="a(ia{sv})" direction="out"/>
    </method>
    <method name="AboutToShow">
      <arg type="i" direction="in"/><arg type="b" direction="out"/>
    </method>
    <method name="Event">
      <arg type="i" direction="in"/><arg type="s" direction="in"/>
      <arg type="v" direction="in"/><arg type="u" direction="in"/>
    </method>
  </interface>
  <interface name="io.github.loveyuri.YuriMusic.Lyrics">
    <method name="GetLine">
      <arg name="text" type="s" direction="out"/>
    </method>
  </interface>
</node>)xml"; // 托盘图标、静态菜单与歌词的 D-Bus 接口

/**
 * 查找随可执行文件分发的应用图标。
 * @return 应用图标的绝对路径，缺失时返回系统媒体图标名。
 */
std::string tray_icon_name();

/**
 * 构造指定菜单节点的属性。
 * @param id 菜单节点 ID。
 * @param names 请求的属性名，空数组表示全部属性。
 * @return 菜单节点的属性字典。
 */
GVariant *menu_properties(gint id, GVariant *names);

/**
 * 构造菜单节点及其子节点。
 * @param id 菜单节点 ID。
 * @param depth 最大递归层级，负值表示不限制。
 * @param names 请求的属性名。
 * @return 菜单节点布局。
 */
GVariant *menu_layout(gint id, gint depth, GVariant *names);

export class LinuxTray final {
public:
  /**
   * 在独立的 GLib 上下文中注册 Linux 托盘图标和菜单。
   * @param on_activate 在托盘线程中调用的双击回调。
   * @param on_exit 在托盘线程中调用的退出回调。
   */
  LinuxTray(std::function<void()> on_activate, std::function<void()> on_exit);

  /** 停止托盘线程并注销 D-Bus 对象。 */
  ~LinuxTray();

  /**
   * 查询当前托盘图标的注册状态。
   * @return 图标是否已成功注册到当前托盘服务。
   */
  [[nodiscard]] bool registered() const;

private:
  struct LyricSnapshot {
    std::atomic_bool ready = false;                // 歌词是否加载完成
    std::vector<qqmusic_api::song::LrcLine> lines; // 按时间排序的歌词
  };

  /**
   * 运行托盘线程并监听托盘服务的出现和消失。
   * @param stop 线程退出令牌。
   */
  void run(std::stop_token stop);

  /**
   * 返回托盘图标和菜单的 D-Bus 属性。
   * @param property 属性名。
   * @return 属性值，未知属性返回空指针。
   */
  [[nodiscard]] GVariant *tray_property(std::string_view property) const;

  /** 将托盘主键的连续点击识别为双击。 */
  void handle_activation();

  /**
   * 为当前歌曲后台加载歌词。
   * @param song 当前歌曲信息
   */
  void request_lyrics(const std::shared_ptr<const SongInfo> &song);

  /** 返回当前进度对应的歌词。 */
  [[nodiscard]] std::string current_lyric() const;

  /**
   * 响应 D-Bus 菜单请求。
   * @param method 方法名。
   * @param parameters 请求参数。
   * @param invocation 用于发送方法回复的句柄。
   */
  void handle_menu(
    std::string_view method,
    GVariant *parameters,
    GDBusMethodInvocation *invocation
  ) const;

  std::function<void()> on_activate_;            // 双击回调
  std::function<void()> on_exit_;                // 退出回调
  std::string icon_name_ = tray_icon_name();     // 应用图标路径或主题名
  GMainContext *context_ = g_main_context_new(); // 与 GTK 登录循环独立的事件上下文
  std::atomic_bool registered_ = false;          // 是否已向托盘服务注册
  std::optional<std::chrono::steady_clock::time_point> first_click_; // 等待配对的首次点击时间
  std::chrono::steady_clock::time_point ignore_click_until_;         // 合成事件忽略截止时间
  std::atomic<std::shared_ptr<LyricSnapshot>> lyrics_;               // 当前歌曲的歌词快照
  std::jthread worker_;                                              // D-Bus 托盘线程
};

std::string tray_icon_name() {
  std::error_code error;
  const auto executable = std::filesystem::read_symlink("/proc/self/exe", error);
  if (!error) {
    const auto icon = executable.parent_path() / "resources/icon.svg";
    if (std::filesystem::is_regular_file(icon, error)) {
      return icon.string();
    }
  }
  return "multimedia-player";
}

GVariant *menu_properties(const gint id, GVariant *const names) {
  GVariantBuilder properties;
  g_variant_builder_init(&properties, G_VARIANT_TYPE_VARDICT);
  const char *key = id == kExitId ? "label" : "children-display";
  const char *value = id == kExitId ? "退出" : "submenu";
  bool include_property = g_variant_n_children(names) == 0;
  GVariantIter iter;
  g_variant_iter_init(&iter, names);
  const char *name = nullptr;
  while (!include_property && g_variant_iter_next(&iter, "&s", &name)) {
    include_property = std::string_view(name) == key;
  }
  if (include_property) {
    g_variant_builder_add(&properties, "{sv}", key, g_variant_new_string(value));
  }
  return g_variant_builder_end(&properties);
}

GVariant *menu_layout(const gint id, const gint depth, GVariant *const names) {
  GVariantBuilder children;
  g_variant_builder_init(&children, G_VARIANT_TYPE("av"));
  if (id == kRootId && depth != 0) {
    g_variant_builder_add(&children, "v", menu_layout(kExitId, 0, names));
  }
  return g_variant_new("(i@a{sv}av)", id, menu_properties(id, names), &children);
}

LinuxTray::LinuxTray(std::function<void()> on_activate, std::function<void()> on_exit) :
  on_activate_(std::move(on_activate)), on_exit_(std::move(on_exit)), lyrics_(),
  worker_([this](const std::stop_token stop) { run(stop); }) {
  auto &audio = player::bass24_player();
  audio.song_changed.connect<&LinuxTray::request_lyrics>(this);
  if (const auto song = audio.current_song()) {
    request_lyrics(song);
  }
}

LinuxTray::~LinuxTray() {
  player::bass24_player().song_changed.disconnect(this);
  worker_.request_stop();
  g_main_context_wakeup(context_);
  worker_.join();
  g_main_context_unref(context_);
}

bool LinuxTray::registered() const {
  return registered_.load();
}

void LinuxTray::request_lyrics(const std::shared_ptr<const SongInfo> &song) {
  const auto snapshot = std::make_shared<LyricSnapshot>();
  lyrics_.store(snapshot);
  thread_manager->addTask([mid = song->mid, pending = std::weak_ptr(snapshot)] {
    if (const auto target = pending.lock()) {
      target->lines = qqmusic_api::song::parse_lrc(qqmusic_api::song::get_song_lyrics(mid));
      target->ready.store(true, std::memory_order_release);
    }
  });
}

std::string LinuxTray::current_lyric() const {
  const auto snapshot = lyrics_.load();
  const auto &audio = player::bass24_player();
  const auto state = audio.state();
  if (
    !snapshot || !snapshot->ready.load(std::memory_order_acquire)
    || (state != player::PlaybackState::playing && state != player::PlaybackState::paused)
  ) {
    return {};
  }
  const auto position_ms = static_cast<std::int64_t>(audio.position() * 1000.0);
  if (snapshot != lyrics_.load() || state != audio.state()) {
    return {};
  }
  return std::string(qqmusic_api::song::line_at(snapshot->lines, position_ms));
}

void LinuxTray::run(const std::stop_token stop) {
  // D-Bus 回调由托盘线程分发，不占用登录窗口的 GTK 主循环。
  g_main_context_push_thread_default(context_);
  auto *connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr);
  if (connection != nullptr) {
    static constexpr GDBusInterfaceVTable kCallbacks {
      .method_call =
        [](
          GDBusConnection *,
          const gchar *,
          const gchar *,
          const gchar *interface,
          const gchar *method,
          GVariant *const parameters,
          GDBusMethodInvocation *const invocation,
          const gpointer data
        ) {
          auto *const tray = static_cast<LinuxTray *>(data);
          if (std::string_view(interface) == waybar_lyrics::kInterface) {
            const auto line = tray->current_lyric();
            g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", line.c_str()));
          } else if (std::string_view(method) == "Activate") {
            g_dbus_method_invocation_return_value(invocation, nullptr);
            tray->handle_activation();
          } else {
            tray->handle_menu(method, parameters, invocation);
          }
        },
      .get_property = [](
                        GDBusConnection *,
                        const gchar *,
                        const gchar *,
                        const gchar *,
                        const gchar *property,
                        GError **,
                        const gpointer data
                      ) { return static_cast<LinuxTray *>(data)->tray_property(property); },
    };
    auto *info = g_dbus_node_info_new_for_xml(kTrayXml, nullptr);
    const auto item = g_dbus_connection_register_object(
      connection, kItemPath, info->interfaces[0], &kCallbacks, this, nullptr, nullptr
    );
    const auto menu = g_dbus_connection_register_object(
      connection, kMenuPath, info->interfaces[1], &kCallbacks, this, nullptr, nullptr
    );
    const auto lyric_object = g_dbus_connection_register_object(
      connection,
      waybar_lyrics::kPath,
      g_dbus_node_info_lookup_interface(info, waybar_lyrics::kInterface),
      &kCallbacks,
      this,
      nullptr,
      nullptr
    );
    guint lyrics_owner = 0;
    if (lyric_object) {
      lyrics_owner = g_bus_own_name_on_connection(
        connection,
        waybar_lyrics::kBusName,
        G_BUS_NAME_OWNER_FLAGS_NONE,
        nullptr,
        nullptr,
        nullptr,
        nullptr
      );
    }
    guint watch = 0;
    if (item && menu) {
      // 托盘服务重启后，使用同一对 D-Bus 对象重新注册图标。
      watch = g_bus_watch_name_on_connection(
        connection,
        kWatcherName,
        G_BUS_NAME_WATCHER_FLAGS_NONE,
        [](
          GDBusConnection *const bus, const gchar *, const gchar *name_owner, const gpointer data
        ) {
          auto *reply = g_dbus_connection_call_sync(
            bus,
            name_owner,
            kWatcherPath,
            kWatcherName,
            "RegisterStatusNotifierItem",
            g_variant_new("(s)", kItemPath),
            G_VARIANT_TYPE_UNIT,
            G_DBUS_CALL_FLAGS_NONE,
            kRegisterTimeoutMs,
            nullptr,
            nullptr
          );
          static_cast<LinuxTray *>(data)->registered_.store(reply != nullptr);
          if (reply != nullptr) {
            g_variant_unref(reply);
          }
        },
        [](GDBusConnection *, const gchar *, const gpointer data) {
          static_cast<LinuxTray *>(data)->registered_.store(false);
        },
        this,
        nullptr
      );
    }
    while (!stop.stop_requested()) {
      g_main_context_iteration(context_, TRUE);
    }
    if (watch) {
      g_bus_unwatch_name(watch);
    }
    if (lyrics_owner) {
      g_bus_unown_name(lyrics_owner);
    }
    if (lyric_object) {
      g_dbus_connection_unregister_object(connection, lyric_object);
    }
    if (menu) {
      g_dbus_connection_unregister_object(connection, menu);
    }
    if (item) {
      g_dbus_connection_unregister_object(connection, item);
    }
    g_dbus_node_info_unref(info);
    g_object_unref(connection);
  }
  g_main_context_pop_thread_default(context_);
}

GVariant *LinuxTray::tray_property(const std::string_view property) const {
  if (property == "Category") {
    return g_variant_new_string("ApplicationStatus");
  }
  if (property == "Id" || property == "Title") {
    return g_variant_new_string("yuri-music");
  }
  if (property == "Status") {
    return g_variant_new_string("Active");
  }
  if (property == "IconName") {
    return g_variant_new_string(icon_name_.c_str());
  }
  if (property == "ItemIsMenu") {
    return g_variant_new_boolean(FALSE);
  }
  if (property == "Menu") {
    return g_variant_new_object_path(kMenuPath);
  }
  if (property == "Version") {
    return g_variant_new_uint32(2);
  }
  return nullptr;
}

void LinuxTray::handle_activation() {
  const auto now = std::chrono::steady_clock::now();
  if (now < ignore_click_until_) {
    return;
  }
  if (first_click_ && now - *first_click_ <= kDoubleClickInterval) {
    first_click_.reset();
    // Waybar 可能还会转发 GTK 合成的双击事件，不让它成为下一次点击的起点。
    ignore_click_until_ = now + kDuplicateClickInterval;
    on_activate_();
  } else {
    first_click_ = now;
  }
}

void LinuxTray::handle_menu(
  const std::string_view method,
  GVariant *const parameters,
  GDBusMethodInvocation *const invocation
) const {
  if (method == "GetLayout") {
    gint id = 0;
    gint depth = 0;
    GVariant *names = nullptr;
    g_variant_get(parameters, "(ii@as)", &id, &depth, &names);
    if (id == kRootId || id == kExitId) {
      g_dbus_method_invocation_return_value(
        invocation, g_variant_new("(u@(ia{sv}av))", 1u, menu_layout(id, depth, names))
      );
    } else {
      g_dbus_method_invocation_return_error_literal(
        invocation, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS, "Unknown menu item"
      );
    }
    g_variant_unref(names);
  } else if (method == "GetGroupProperties") {
    GVariantIter *ids = nullptr;
    GVariant *names = nullptr;
    g_variant_get(parameters, "(ai@as)", &ids, &names);
    GVariantBuilder properties;
    g_variant_builder_init(&properties, G_VARIANT_TYPE("a(ia{sv})"));
    gint id = 0;
    while (g_variant_iter_next(ids, "i", &id)) {
      if (id == kRootId || id == kExitId) {
        g_variant_builder_add(&properties, "(i@a{sv})", id, menu_properties(id, names));
      }
    }
    g_variant_iter_free(ids);
    g_variant_unref(names);
    g_dbus_method_invocation_return_value(invocation, g_variant_new("(a(ia{sv}))", &properties));
  } else if (method == "AboutToShow") {
    g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", FALSE));
  } else if (method == "Event") {
    gint id = 0;
    const char *event = nullptr;
    g_variant_get_child(parameters, 0, "i", &id);
    g_variant_get_child(parameters, 1, "&s", &event);
    const bool clicked_exit = id == kExitId && std::string_view(event) == "clicked";
    // 先回复菜单客户端，再通知主窗口退出。
    g_dbus_method_invocation_return_value(invocation, nullptr);
    if (clicked_exit) {
      on_exit_();
    }
  }
}
#endif
