# ============================================================================
# WebView2 配置
# ============================================================================

if(WIN32)
    find_package(unofficial-webview2 CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE unofficial::webview2::webview2)
endif()
