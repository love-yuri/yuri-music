#include <cstdio>
#include <gio/gio.h>

#include "waybar_lyrics_dbus.hpp"

/** 将当前歌词作为单行文本输出给 Waybar，应用不在线时输出空行。 */
int main() {
  auto *const connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr);
  if (!connection) {
    std::puts("");
    return 0;
  }

  constexpr int kTimeoutMs = 500; // 歌词查询超时（毫秒）
  auto *const reply = g_dbus_connection_call_sync(
    connection,
    waybar_lyrics::kBusName,
    waybar_lyrics::kPath,
    waybar_lyrics::kInterface,
    waybar_lyrics::kGetLine,
    nullptr,
    G_VARIANT_TYPE("(s)"),
    G_DBUS_CALL_FLAGS_NO_AUTO_START,
    kTimeoutMs,
    nullptr,
    nullptr
  );
  if (reply) {
    const char *line = nullptr;
    g_variant_get(reply, "(&s)", &line);
    std::puts(line);
    g_variant_unref(reply);
  } else {
    std::puts("");
  }
  g_object_unref(connection);
}
