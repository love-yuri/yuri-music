# ============================================================================
# BASS 音频库配置
# ============================================================================

set(YURI_MUSIC_BASS_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")

function(configure_yuri_music_bass target_name)
    get_filename_component(YURI_MUSIC_ROOT "${YURI_MUSIC_BASS_CMAKE_DIR}/.." ABSOLUTE)
    set(BASS_ROOT "${YURI_MUSIC_ROOT}/libs/bass24")

    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "yuri-music only supports 64-bit BASS builds.")
    endif()

    if(WIN32)
        set(BASS_LIBRARY "${BASS_ROOT}/bass.lib")
        set(BASS_RUNTIME_LIBRARY "${BASS_ROOT}/bass.dll")
        set(BASS_FLAC_RUNTIME_LIBRARY "${BASS_ROOT}/bassflac.dll")
    elseif(LINUX)
        set(BASS_LIBRARY "${BASS_ROOT}/libbass.so")
        set(BASS_RUNTIME_LIBRARY "${BASS_LIBRARY}")
        set(BASS_FLAC_RUNTIME_LIBRARY "${BASS_ROOT}/libbassflac.so")
        set_target_properties(${target_name} PROPERTIES
            BUILD_RPATH "$ORIGIN"
            INSTALL_RPATH "$ORIGIN"
        )
    else()
        message(FATAL_ERROR "yuri-music only supports BASS on 64-bit Windows and 64-bit Linux.")
    endif()

    target_include_directories(${target_name} PRIVATE ${BASS_ROOT})
    target_link_libraries(${target_name} PRIVATE ${BASS_LIBRARY})

    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${BASS_RUNTIME_LIBRARY}
            ${BASS_FLAC_RUNTIME_LIBRARY}
            $<TARGET_FILE_DIR:${target_name}>
    )
endfunction()
