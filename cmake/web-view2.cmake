# ============================================================================
# 网页登录窗口配置：Windows 使用 WebView2，Linux 使用 WebKitGTK
# ============================================================================

if(WIN32)
    find_package(unofficial-webview2 CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE unofficial::webview2::webview2)
elseif(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(WebKit2GTK REQUIRED IMPORTED_TARGET webkit2gtk-4.1)
    target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::WebKit2GTK)
endif()
