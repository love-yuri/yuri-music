module;
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <WebView2.h>
#include <wrl.h>
#endif

export module webview2;

import std;
import core;
import qq_music_api;

namespace {

#ifdef _WIN32

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

constexpr wchar_t kWebViewWindowClass[] = L"YuriMusicQqLoginWebView";         // WebView2 窗口类名
constexpr wchar_t kQqMusicLoginUrl[] = L"https://y.qq.com/n/ryqq_v2/profile"; // QQ 音乐登录入口
constexpr UINT_PTR kCookieCaptureTimerId = 1;   // cookie 捕获定时器 ID
constexpr UINT kCookieCaptureIntervalMs = 2000; // cookie 捕获间隔

std::atomic_bool login_window_open = false; // 登录窗口是否已打开

struct WebViewWindowState {
  ComPtr<ICoreWebView2Controller> controller; // WebView2 控制器
  ComPtr<ICoreWebView2> webview;              // WebView2 实例
  bool cookie_request_pending = false;        // cookie 请求是否进行中
  bool cookie_saved = false;                  // cookie 是否已保存
};

/**
 * 将宽字符字符串转换为 UTF-8。
 */
std::string wideToUtf8(const wchar_t *value);

/**
 * 判断 cookie 集合是否已经包含 QQ 音乐登录态。
 */
bool isQqMusicLoginCookie(const std::map<std::string, std::string> &cookies);

/**
 * 将 cookie 集合序列化为 HTTP Cookie 头格式。
 */
std::string serializeCookies(const std::map<std::string, std::string> &cookies);

/**
 * 保存捕获到的 QQ 音乐 cookie，并同步到 qq_music_api 全局配置。
 */
void applyCapturedCookie(
  const std::string &cookie,
  const std::map<std::string, std::string> &cookies
);

/**
 * 从 WebView2 cookie 列表提取 name/value 映射。
 */
std::map<std::string, std::string> collectCookieMap(ICoreWebView2CookieList *cookie_list);

/**
 * 向 WebView2 请求捕获当前 QQ 音乐 cookie。
 */
void requestQqMusicCookieCapture(HWND hwnd, const std::shared_ptr<WebViewWindowState> &state);

/**
 * 处理 QQ 音乐 WebView2 窗口消息。
 */
LRESULT CALLBACK qqMusicWebViewWndProc(HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param);

/**
 * 注册 QQ 音乐 WebView2 窗口类。
 */
bool registerQqMusicWebViewClass(HINSTANCE instance);

/**
 * 在独立 STA 线程中运行 QQ 音乐登录 WebView2 窗口。
 */
void runQqMusicLoginWebView();

#endif

} // namespace

export namespace webview2 {

/**
 * 启动 QQ 音乐登录 WebView2 窗口。
 */
void launchQQMusicLogin();

} // namespace webview2

namespace {

#ifdef _WIN32

std::string wideToUtf8(const wchar_t *value) {
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

bool isQqMusicLoginCookie(const std::map<std::string, std::string> &cookies) {
  const bool has_uin = cookies.contains("uin");
  const bool has_login_key = cookies.contains("qm_keyst") || cookies.contains("qqmusic_key")
                             || cookies.contains("p_skey") || cookies.contains("skey");
  return has_uin && has_login_key;
}

std::string serializeCookies(const std::map<std::string, std::string> &cookies) {
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

void applyCapturedCookie(
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

std::map<std::string, std::string> collectCookieMap(ICoreWebView2CookieList *cookie_list) {
  std::map<std::string, std::string> cookies;
  if (cookie_list == nullptr) {
    return cookies;
  }

  UINT32 count = 0;
  if (FAILED(cookie_list->get_Count(&count))) {
    return cookies;
  }

  for (UINT32 i = 0; i < count; ++i) {
    ComPtr<ICoreWebView2Cookie> cookie;
    if (FAILED(cookie_list->GetValueAtIndex(i, &cookie)) || !cookie) {
      continue;
    }

    LPWSTR raw_name = nullptr;
    LPWSTR raw_value = nullptr;
    if (SUCCEEDED(cookie->get_Name(&raw_name)) && SUCCEEDED(cookie->get_Value(&raw_value))) {
      const auto name = wideToUtf8(raw_name);
      const auto value = wideToUtf8(raw_value);
      if (!name.empty()) {
        cookies[name] = value;
      }
    }
    CoTaskMemFree(raw_name);
    CoTaskMemFree(raw_value);
  }

  return cookies;
}

void requestQqMusicCookieCapture(
  const HWND hwnd,
  const std::shared_ptr<WebViewWindowState> &state
) {
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
      [hwnd, state](const HRESULT result, ICoreWebView2CookieList *cookie_list) -> HRESULT {
        state->cookie_request_pending = false;

        if (FAILED(result) || cookie_list == nullptr || state->cookie_saved) {
          return result;
        }

        const auto cookies = collectCookieMap(cookie_list);
        if (!isQqMusicLoginCookie(cookies)) {
          return S_OK;
        }

        const auto cookie = serializeCookies(cookies);
        if (cookie.empty()) {
          return S_OK;
        }

        applyCapturedCookie(cookie, cookies);
        state->cookie_saved = true;
        KillTimer(hwnd, kCookieCaptureTimerId);
        return S_OK;
      }
    ).Get()
  );
}

LRESULT CALLBACK qqMusicWebViewWndProc(
  const HWND hwnd,
  const UINT message,
  const WPARAM w_param,
  const LPARAM l_param
) {
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
        RECT bounds{};
        GetClientRect(hwnd, &bounds);
        state->controller->put_Bounds(bounds);
      }
      return 0;
    case WM_TIMER:
      if (w_param == kCookieCaptureTimerId && state_holder) {
        requestQqMusicCookieCapture(hwnd, *state_holder);
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

bool registerQqMusicWebViewClass(const HINSTANCE instance) {
  WNDCLASSEXW wc{};
  wc.cbSize = sizeof(WNDCLASSEXW);
  wc.lpfnWndProc = qqMusicWebViewWndProc;
  wc.hInstance = instance;
  wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
  wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
  wc.lpszClassName = kWebViewWindowClass;

  if (RegisterClassExW(&wc) != 0) {
    return true;
  }

  return GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

void runQqMusicLoginWebView() {
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
  if (!registerQqMusicWebViewClass(instance)) {
    yuri::error("注册 WebView2 窗口类失败: {}", GetLastError());
    reset_open_flag();
    return;
  }

  auto state = std::make_shared<WebViewWindowState>();
  HWND hwnd = CreateWindowExW(
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
      [hwnd, state](const HRESULT result, ICoreWebView2Environment *environment) -> HRESULT {
        if (FAILED(result) || environment == nullptr) {
          yuri::error("创建 WebView2 环境失败: 0x{:08X}", static_cast<unsigned>(result));
          DestroyWindow(hwnd);
          return result;
        }

        return environment->CreateCoreWebView2Controller(
          hwnd,
          Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
            [hwnd, state](const HRESULT controller_result, ICoreWebView2Controller *controller)
              -> HRESULT {
              if (FAILED(controller_result) || controller == nullptr) {
                yuri::error(
                  "创建 WebView2 控制器失败: 0x{:08X}", static_cast<unsigned>(controller_result)
                );
                DestroyWindow(hwnd);
                return controller_result;
              }

              state->controller = controller;
              state->controller->get_CoreWebView2(&state->webview);

              RECT bounds{};
              GetClientRect(hwnd, &bounds);
              state->controller->put_Bounds(bounds);

              if (state->webview) {
                EventRegistrationToken navigation_token{};
                state->webview->add_NavigationCompleted(
                  Callback<ICoreWebView2NavigationCompletedEventHandler>(
                    [hwnd, state](ICoreWebView2 *, ICoreWebView2NavigationCompletedEventArgs *)
                      -> HRESULT {
                      requestQqMusicCookieCapture(hwnd, state);
                      return S_OK;
                    }
                  ).Get(),
                  &navigation_token
                );

                SetTimer(hwnd, kCookieCaptureTimerId, kCookieCaptureIntervalMs, nullptr);
                state->webview->Navigate(kQqMusicLoginUrl);
                requestQqMusicCookieCapture(hwnd, state);
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

  MSG message{};
  while (GetMessageW(&message, nullptr, 0, 0) > 0) {
    TranslateMessage(&message);
    DispatchMessageW(&message);
  }

  reset_open_flag();
}

#endif

} // namespace

export namespace webview2 {

/**
 * 启动qq音乐登录创库
 */
void launchQQMusicLogin() {
#ifdef _WIN32
  bool expected = false;
  if (!login_window_open.compare_exchange_strong(expected, true)) {
    return;
  }

  runQqMusicLoginWebView();
#else
  yuri::warn("QQ 音乐 WebView2 登录窗口仅支持 Windows");
#endif
}

} // namespace webview2
