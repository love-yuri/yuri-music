# ============================================================================
# WebView2 配置
# ============================================================================

function(configure_yuri_music_webview2 target_name)
    if(NOT WIN32)
        return()
    endif()

    find_package(unofficial-webview2 CONFIG REQUIRED)
    target_link_libraries(${target_name} PRIVATE unofficial::webview2::webview2)
endfunction()
