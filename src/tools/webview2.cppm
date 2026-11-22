module;
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <WebView2.h>
#include <wrl.h>
#elif defined(__linux__)
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#endif

export module webview2;

import std;
import yuri.core;
import qq_music_api;

std::atomic_bool login_window_open = false; // 登录窗口是否已打开

/**
 * 判断 cookie 集合是否已经包含 QQ 音乐登录态。
 * @param cookies 捕获的 cookie 集合。
 * @return 是否具备登录所需的字段。
 */
bool has_qq_music_login_cookie(const std::map<std::string, std::string> &cookies);

/**
 * 将 cookie 集合序列化为 HTTP Cookie 头格式。
 * @param cookies 捕获的 cookie 集合。
 * @return 序列化后的 Cookie 头内容。
 */
std::string serialize_cookies(const std::map<std::string, std::string> &cookies);

/**
 * 保存捕获到的 QQ 音乐 cookie，并同步到 qq_music_api 全局配置。
 * @param cookie 序列化后的 Cookie 头内容。
 * @param cookies 捕获的 cookie 集合。
 */
void apply_captured_cookie(
  const std::string &cookie,
  const std::map<std::string, std::string> &cookies
);

#ifdef _WIN32

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

constexpr wchar_t kWebViewWindowClass[] = L"YuriMusicQqLoginWebView";         // WebView2 窗口类名
constexpr wchar_t kQqMusicLoginUrl[] = L"https://y.qq.com/n/ryqq_v2/profile"; // QQ 音乐登录入口
constexpr UINT_PTR kCookieCaptureTimerId = 1;   // cookie 捕获定时器 ID
constexpr UINT kCookieCaptureIntervalMs = 2000; // cookie 捕获间隔

struct WebViewWindowState {
  ComPtr<ICoreWebView2Controller> controller; // WebView2 控制器
  ComPtr<ICoreWebView2> webview;              // WebView2 实例
  bool cookie_request_pending = false;        // cookie 请求是否进行中
  bool cookie_saved = false;                  // cookie 是否已保存
};

/**
 * 将宽字符字符串转换为 UTF-8。
 * @param value 原始宽字符字符串。
 * @return UTF-8 字符串；转换失败时为空。
 */
std::string wide_to_utf8(const wchar_t *value);

/**
 * 从 WebView2 cookie 列表提取 name/value 映射。
 * @param cookie_list WebView2 返回的 cookie 列表。
 * @return cookie 名称到值的映射。
 */
std::map<std::string, std::string> collect_cookie_map(ICoreWebView2CookieList *cookie_list);

/**
 * 向 WebView2 请求捕获当前 QQ 音乐 cookie。
 * @param hwnd 登录窗口句柄。
 * @param state 登录窗口状态。
 */
void request_cookie_capture(HWND hwnd, const std::shared_ptr<WebViewWindowState> &state);

/**
 * 处理 QQ 音乐 WebView2 窗口消息。
 * @param hwnd 登录窗口句柄。
 * @param message Windows 消息编号。
 * @param w_param 消息的 WPARAM 参数。
 * @param l_param 消息的 LPARAM 参数。
 * @return Windows 消息处理结果。
 */
LRESULT CALLBACK login_window_proc(HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param);

/**
 * 注册 QQ 音乐 WebView2 窗口类。
 * @param instance 应用实例句柄。
 * @return 窗口类是否可用。
 */
bool register_login_window_class(HINSTANCE instance);

/**
 * 在独立 STA 线程中运行 QQ 音乐登录 WebView2 窗口。
 */
void run_login_window();

#elif defined(__linux__)

constexpr char kQqMusicLoginUrl[] = "https://y.qq.com/n/ryqq_v2/profile"; // QQ 音乐登录入口
constexpr guint kCookieCaptureIntervalMs = 2000;                          // cookie 捕获间隔
std::atomic_bool login_shutdown_requested = false;                        // 主程序是否正在退出

struct LoginWindowState {
  GtkWidget *window {};                   // 登录窗口
  WebKitWebView *webview {};              // WebKit 视图
  WebKitCookieManager *cookie_manager {}; // cookie 管理器
  GCancellable *cookie_cancel {};         // cookie 请求取消句柄
  guint capture_timer_id = 0;             // cookie 捕获定时器 ID
  bool cookie_request_pending = false;    // cookie 请求是否进行中
  bool cookie_saved = false;              // cookie 是否已保存
};

/**
 * 释放 WebKit 返回的 cookie 列表。
 * @param cookie_list WebKit 返回的 cookie 列表。
 */
void free_cookie_list(GList *cookie_list);

/**
 * 从 WebKit cookie 列表提取 name/value 映射。
 * @param cookie_list WebKit 返回的 cookie 列表。
 * @return cookie 名称到值的映射。
 */
std::map<std::string, std::string> collect_cookie_map(GList *cookie_list);

/**
 * 关闭登录窗口并结束 GTK 主循环。
 * @param state 登录窗口状态。
 */
void close_login_window(const LoginWindowState *state);

/**
 * 向 WebKit 请求捕获当前 QQ 音乐 cookie。
 * @param state 登录窗口状态。
 */
void request_cookie_capture(LoginWindowState *state);

/**
 * 处理 QQ 音乐 cookie 捕获结果。
 * @param source 发起请求的 WebKit cookie 管理器。
 * @param result 异步请求结果。
 * @param user_data 登录窗口状态。
 */
void on_cookie_captured(GObject *source, GAsyncResult *result, gpointer user_data);

/**
 * 定时捕获 QQ 音乐 cookie（扫码登录不会触发页面跳转）。
 * @param user_data 登录窗口状态。
 * @return 是否保留定时器。
 */
gboolean on_cookie_capture_timer(gpointer user_data);

/**
 * 处理 WebKit 页面加载状态变化。
 * @param webview 发出信号的 WebKit 视图。
 * @param event 当前页面加载事件。
 * @param user_data 登录窗口状态。
 */
void on_load_changed(WebKitWebView *webview, WebKitLoadEvent event, gpointer user_data);

/**
 * 将新窗口请求交由当前视图处理。
 * @param webview 发出请求的 WebKit 视图。
 * @param action 新窗口的导航操作。
 * @param user_data 信号携带的数据，此处未使用。
 * @return 当前 WebKit 视图控件。
 */
GtkWidget *
on_create_web_view(WebKitWebView *webview, WebKitNavigationAction *action, gpointer user_data);

/**
 * 忽略网页发起的 window.close() 请求。
 * @param webview 发出关闭请求的 WebKit 视图。
 * @param user_data 信号携带的数据，此处未使用。
 */
void on_close_web_view(WebKitWebView *webview, gpointer user_data);

/**
 * 处理登录窗口销毁事件。
 * @param widget 发出销毁信号的窗口控件。
 * @param user_data 登录窗口状态。
 */
void on_window_destroy(GtkWidget *widget, gpointer user_data);

/**
 * 在 GTK 主循环中运行 QQ 音乐登录窗口。
 * @return GTK 初始化失败时返回 1，否则返回 0。
 */
int run_login_window();

#endif

export namespace webview2 {

/**
 * 启动 QQ 音乐登录窗口。
 */
void launch_qq_music_login();

#ifdef __linux__
/** 请求登录线程关闭窗口，避免退出时等待登录。 */
void request_login_shutdown();
#endif

} // namespace webview2

bool has_qq_music_login_cookie(const std::map<std::string, std::string> &cookies) {
  const bool has_uin = cookies.contains("uin");
  const bool has_login_key = cookies.contains("qm_keyst") || cookies.contains("qqmusic_key")
                             || cookies.contains("p_skey") || cookies.contains("skey");
  return has_uin && has_login_key;
}

std::string serialize_cookies(const std::map<std::string, std::string> &cookies) {
  std::string result;
  for (const auto &[name, value] : cookies) {
    if (!result.empty()) {
      result += "; ";
    }
    result += name;
    result += '=';
    result += value;
  }
  return result;
}

void apply_captured_cookie(
  const std::string &cookie,
  const std::map<std::string, std::string> &cookies
) {
  std::ofstream file("cookie.txt", std::ios::binary | std::ios::trunc);
  if (!file.is_open()) {
    yuri::error("保存 QQ 音乐 cookie 失败: 无法打开 cookie.txt");
    return;
  }

  file.write(cookie.data(), static_cast<std::streamsize>(cookie.size()));
  file.close();

  qqmusic_api_config.cookie = cookie;
  qqmusic_api_config.has_login = true;

  if (auto it = cookies.find("uin"); it != cookies.end()) {
    qqmusic_api_config.qq.clear();
    for (const char ch : it->second) {
      if (std::isdigit(static_cast<unsigned char>(ch))) {
        qqmusic_api_config.qq.push_back(ch);
      }
    }
  }

  yuri::info("QQ 音乐登录 cookie 已保存到 cookie.txt");
}

#ifdef _WIN32

std::string wide_to_utf8(const wchar_t *const value) {
  if (value == nullptr || value[0] == L'\0') {
    return {};
  }

  const int size = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
  if (size <= 1) {
    return {};
  }

  std::string result(static_cast<std::size_t>(size - 1), '\0');
  WideCharToMultiByte(CP_UTF8, 0, value, -1, result.data(), size, nullptr, nullptr);
  return result;
}

std::map<std::string, std::string> collect_cookie_map(ICoreWebView2CookieList *const cookie_list) {
  std::map<std::string, std::string> cookies;
  if (cookie_list == nullptr) {
    return cookies;
  }

  UINT32 count = 0;
  if (FAILED(cookie_list->get_Count(&count))) {
    return cookies;
  }

  for (UINT32 index = 0; index < count; ++index) {
    ComPtr<ICoreWebView2Cookie> cookie;
    if (FAILED(cookie_list->GetValueAtIndex(index, &cookie)) || !cookie) {
      continue;
    }

    LPWSTR raw_name = nullptr;
    LPWSTR raw_value = nullptr;
    if (SUCCEEDED(cookie->get_Name(&raw_name)) && SUCCEEDED(cookie->get_Value(&raw_value))) {
      const auto name = wide_to_utf8(raw_name);
      const auto value = wide_to_utf8(raw_value);
      if (!name.empty()) {
        cookies[name] = value;
      }
    }
    CoTaskMemFree(raw_name);
    CoTaskMemFree(raw_value);
  }

  return cookies;
}

void request_cookie_capture(const HWND hwnd, const std::shared_ptr<WebViewWindowState> &state) {
  if (!state || !state->webview || state->cookie_saved || state->cookie_request_pending) {
    return;
  }

  ComPtr<ICoreWebView2_2> webview2;
  if (FAILED(state->webview.As(&webview2)) || !webview2) {
    yuri::warn("当前 WebView2 运行时不支持 CookieManager");
    return;
  }

  ComPtr<ICoreWebView2CookieManager> cookie_manager;
  if (FAILED(webview2->get_CookieManager(&cookie_manager)) || !cookie_manager) {
    return;
  }

  state->cookie_request_pending = true;
  cookie_manager->GetCookies(
    kQqMusicLoginUrl,
    Callback<ICoreWebView2GetCookiesCompletedHandler>(
      [hwnd, state](const HRESULT result, ICoreWebView2CookieList *const cookie_list) -> HRESULT {
        state->cookie_request_pending = false;

        if (FAILED(result) || cookie_list == nullptr || state->cookie_saved) {
          return result;
        }

        const auto cookies = collect_cookie_map(cookie_list);
        if (!has_qq_music_login_cookie(cookies)) {
          return S_OK;
        }

        const auto cookie = serialize_cookies(cookies);
        if (cookie.empty()) {
          return S_OK;
        }

        apply_captured_cookie(cookie, cookies);
        state->cookie_saved = true;
        KillTimer(hwnd, kCookieCaptureTimerId);
        return S_OK;
      }
    ).Get()
  );
}

LRESULT CALLBACK
login_window_proc(const HWND hwnd, const UINT message, const WPARAM w_param, const LPARAM l_param) {
  auto *state_holder =
    reinterpret_cast<std::shared_ptr<WebViewWindowState> *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
  WebViewWindowState *state = state_holder ? state_holder->get() : nullptr;

  switch (message) {
    case WM_NCCREATE: {
      const auto *create_struct = reinterpret_cast<CREATESTRUCTW *>(l_param);
      SetWindowLongPtrW(
        hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create_struct->lpCreateParams)
      );
      return TRUE;
    }
    case WM_SIZE:
      if (state && state->controller) {
        RECT bounds {};
        GetClientRect(hwnd, &bounds);
        state->controller->put_Bounds(bounds);
      }
      return 0;
    case WM_TIMER:
      if (w_param == kCookieCaptureTimerId && state_holder) {
        request_cookie_capture(hwnd, *state_holder);
      }
      return 0;
    case WM_DESTROY:
      KillTimer(hwnd, kCookieCaptureTimerId);
      SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
      PostQuitMessage(0);
      return 0;
    default: return DefWindowProcW(hwnd, message, w_param, l_param);
  }
}

bool register_login_window_class(const HINSTANCE instance) {
  WNDCLASSEXW window_class {};
  window_class.cbSize = sizeof(WNDCLASSEXW);
  window_class.lpfnWndProc = login_window_proc;
  window_class.hInstance = instance;
  window_class.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
  window_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
  window_class.lpszClassName = kWebViewWindowClass;

  if (RegisterClassExW(&window_class) != 0) {
    return true;
  }

  return GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

void run_login_window() {
  const HRESULT com_result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  if (FAILED(com_result)) {
    yuri::error("初始化 WebView2 COM 失败: 0x{:08X}", static_cast<unsigned>(com_result));
    login_window_open = false;
    return;
  }

  const auto reset_open_flag = [] {
    login_window_open = false;
    CoUninitialize();
  };

  const HINSTANCE instance = GetModuleHandleW(nullptr);
  if (!register_login_window_class(instance)) {
    yuri::error("注册 WebView2 窗口类失败: {}", GetLastError());
    reset_open_flag();
    return;
  }

  auto state = std::make_shared<WebViewWindowState>();
  const HWND hwnd = CreateWindowExW(
    0,
    kWebViewWindowClass,
    L"QQ Music Login",
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    1100,
    760,
    nullptr,
    nullptr,
    instance,
    &state
  );

  if (!hwnd) {
    yuri::error("创建 WebView2 窗口失败: {}", GetLastError());
    reset_open_flag();
    return;
  }

  ShowWindow(hwnd, SW_SHOW);
  UpdateWindow(hwnd);

  const HRESULT webview_result = CreateCoreWebView2EnvironmentWithOptions(
    nullptr,
    nullptr,
    nullptr,
    Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
      [hwnd, state](const HRESULT result, ICoreWebView2Environment *const environment) -> HRESULT {
        if (FAILED(result) || environment == nullptr) {
          yuri::error("创建 WebView2 环境失败: 0x{:08X}", static_cast<unsigned>(result));
          DestroyWindow(hwnd);
          return result;
        }

        return environment->CreateCoreWebView2Controller(
          hwnd,
          Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
            [hwnd, state](
              const HRESULT controller_result, ICoreWebView2Controller *const controller
            ) -> HRESULT {
              if (FAILED(controller_result) || controller == nullptr) {
                yuri::error(
                  "创建 WebView2 控制器失败: 0x{:08X}", static_cast<unsigned>(controller_result)
                );
                DestroyWindow(hwnd);
                return controller_result;
              }

              state->controller = controller;
              state->controller->get_CoreWebView2(&state->webview);

              RECT bounds {};
              GetClientRect(hwnd, &bounds);
              state->controller->put_Bounds(bounds);

              if (state->webview) {
                EventRegistrationToken navigation_token {};
                state->webview->add_NavigationCompleted(
                  Callback<ICoreWebView2NavigationCompletedEventHandler>(
                    [hwnd, state](ICoreWebView2 *, ICoreWebView2NavigationCompletedEventArgs *)
                      -> HRESULT {
                      request_cookie_capture(hwnd, state);
                      return S_OK;
                    }
                  ).Get(),
                  &navigation_token
                );

                SetTimer(hwnd, kCookieCaptureTimerId, kCookieCaptureIntervalMs, nullptr);
                state->webview->Navigate(kQqMusicLoginUrl);
                request_cookie_capture(hwnd, state);
              }
              return S_OK;
            }
          ).Get()
        );
      }
    ).Get()
  );

  if (FAILED(webview_result)) {
    yuri::error("启动 WebView2 失败: 0x{:08X}", static_cast<unsigned>(webview_result));
    DestroyWindow(hwnd);
  }

  MSG message {};
  while (GetMessageW(&message, nullptr, 0, 0) > 0) {
    TranslateMessage(&message);
    DispatchMessageW(&message);
  }

  reset_open_flag();
}

#elif defined(__linux__)

void free_cookie_list(GList *const cookie_list) {
  if (cookie_list != nullptr) {
    // cookie 列表由 WebKit 转移所有权，需要逐个释放
    g_list_free_full(cookie_list, reinterpret_cast<GDestroyNotify>(soup_cookie_free));
  }
}

std::map<std::string, std::string> collect_cookie_map(GList *const cookie_list) {
  std::map<std::string, std::string> cookies;

  for (GList *node = cookie_list; node != nullptr; node = node->next) {
    auto *cookie = static_cast<SoupCookie *>(node->data);
    if (cookie == nullptr) {
      continue;
    }

    const char *raw_name = soup_cookie_get_name(cookie);
    const char *raw_value = soup_cookie_get_value(cookie);
    if (raw_name == nullptr || raw_value == nullptr || raw_name[0] == '\0') {
      continue;
    }

    cookies[raw_name] = raw_value;
  }

  return cookies;
}

void close_login_window(const LoginWindowState *const state) {
  if (state->window != nullptr) {
    gtk_widget_destroy(state->window);
  }
}

void request_cookie_capture(LoginWindowState *const state) {
  if (
    state == nullptr || state->window == nullptr || state->cookie_saved
    || state->cookie_request_pending
  ) {
    return;
  }

  state->cookie_request_pending = true;
  webkit_cookie_manager_get_cookies(
    state->cookie_manager, kQqMusicLoginUrl, state->cookie_cancel, on_cookie_captured, state
  );
}

void on_cookie_captured(
  GObject *const source,
  GAsyncResult *const result,
  const gpointer user_data
) {
  auto *state = static_cast<LoginWindowState *>(user_data);
  state->cookie_request_pending = false;

  GError *error = nullptr;
  GList *cookie_list =
    webkit_cookie_manager_get_cookies_finish(WEBKIT_COOKIE_MANAGER(source), result, &error);
  if (state->window == nullptr) {
    g_clear_error(&error);
    free_cookie_list(cookie_list);
    gtk_main_quit();
    return;
  }
  if (error != nullptr) {
    yuri::warn("读取 QQ 音乐 cookie 失败: {}", error->message);
    g_error_free(error);
  }

  if (cookie_list == nullptr || state->cookie_saved) {
    free_cookie_list(cookie_list);
    return;
  }

  const auto cookies = collect_cookie_map(cookie_list);
  free_cookie_list(cookie_list);

  if (!has_qq_music_login_cookie(cookies)) {
    return;
  }

  const auto cookie = serialize_cookies(cookies);
  if (cookie.empty()) {
    return;
  }

  apply_captured_cookie(cookie, cookies);
  state->cookie_saved = true;
  close_login_window(state);
}

gboolean on_cookie_capture_timer(const gpointer user_data) {
  auto *state = static_cast<LoginWindowState *>(user_data);
  if (login_shutdown_requested) {
    close_login_window(state);
    return G_SOURCE_REMOVE;
  }
  request_cookie_capture(state);
  return G_SOURCE_CONTINUE;
}

void on_load_changed(WebKitWebView *, const WebKitLoadEvent event, const gpointer user_data) {
  // 页面加载完成时立即尝试捕获，扫码登录不跳转页面则依赖定时器捕获
  if (event == WEBKIT_LOAD_FINISHED) {
    request_cookie_capture(static_cast<LoginWindowState *>(user_data));
  }
}

GtkWidget *on_create_web_view(WebKitWebView *const webview, WebKitNavigationAction *, gpointer) {
  // 新窗口请求在当前视图中打开，保证登录流程始终在同一个窗口内完成
  return GTK_WIDGET(webview);
}

void on_close_web_view(WebKitWebView *const webview, gpointer) {
  // 屏蔽 WebKit 默认的销毁顶层窗口行为，登录窗口只由用户或捕获完成后关闭
  g_signal_stop_emission_by_name(G_OBJECT(webview), "close");
  yuri::info("已忽略网页发起的关闭窗口请求");
}

void on_window_destroy(GtkWidget *, const gpointer user_data) {
  auto *state = static_cast<LoginWindowState *>(user_data);
  state->window = nullptr;
  if (state->capture_timer_id != 0) {
    g_source_remove(state->capture_timer_id);
    state->capture_timer_id = 0;
  }

  if (!state->cookie_saved && !login_shutdown_requested) {
    yuri::warn("QQ 音乐登录窗口已关闭，未捕获到登录 cookie");
  }

  if (state->cookie_request_pending) {
    // 让异步回调先完成，避免 GTK 循环结束后仍访问栈上的窗口状态。
    g_cancellable_cancel(state->cookie_cancel);
  } else {
    gtk_main_quit();
  }
}

int run_login_window() {
  if (login_shutdown_requested) {
    return 0;
  }
  if (gdk_display_get_default() == nullptr) {
    gdk_set_allowed_backends("wayland");
  }
  if (gtk_init_check(nullptr, nullptr) == FALSE) {
    yuri::error("初始化 GTK 失败: 无法连接到 Wayland 合成器");
    return 1;
  }

  LoginWindowState state;
  WebKitWebsiteDataManager *data_manager = webkit_website_data_manager_new_ephemeral(); // 临时会话
  WebKitWebContext *context = webkit_web_context_new_with_website_data_manager(data_manager);
  g_object_unref(data_manager); // 上下文已持有引用

  state.webview = WEBKIT_WEB_VIEW(webkit_web_view_new_with_context(context));
  state.cookie_manager = webkit_web_context_get_cookie_manager(context);
  state.cookie_cancel = g_cancellable_new();
  state.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(state.window), "QQ Music Login");
  gtk_window_set_default_size(GTK_WINDOW(state.window), 1100, 760);
  gtk_container_add(GTK_CONTAINER(state.window), GTK_WIDGET(state.webview));

  g_signal_connect(state.window, "destroy", G_CALLBACK(on_window_destroy), &state);
  g_signal_connect(state.webview, "load-changed", G_CALLBACK(on_load_changed), &state);
  g_signal_connect(state.webview, "create", G_CALLBACK(on_create_web_view), nullptr);
  g_signal_connect(state.webview, "close", G_CALLBACK(on_close_web_view), nullptr);
  state.capture_timer_id = g_timeout_add(kCookieCaptureIntervalMs, on_cookie_capture_timer, &state);

  gtk_widget_show_all(state.window);
  webkit_web_view_load_uri(state.webview, kQqMusicLoginUrl);
  request_cookie_capture(&state);

  gtk_main(); // 阻塞直到登录成功或用户关闭窗口
  g_object_unref(state.cookie_cancel);
  return 0;
}

#endif

void webview2::launch_qq_music_login() {
#ifdef _WIN32
  bool expected = false;
  if (!login_window_open.compare_exchange_strong(expected, true)) {
    return;
  }

  run_login_window();
#elif defined(__linux__)
  bool expected = false;
  if (!login_window_open.compare_exchange_strong(expected, true)) {
    return;
  }

  // WebKitGTK 的窗口与事件循环在调用线程内运行，需要由后台线程调用
  yuri::info("正在打开 QQ 音乐登录窗口");
  run_login_window();

  login_window_open = false;
#else
  yuri::warn("QQ 音乐网页登录暂不支持当前平台");
#endif
}

#ifdef __linux__
void webview2::request_login_shutdown() {
  login_shutdown_requested = true;
}
#endif
