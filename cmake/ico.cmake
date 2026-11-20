# Windows 程序图标与 Release GUI 子系统配置。
if(WIN32)
    enable_language(RC)
    target_sources(${PROJECT_NAME} PRIVATE resources/yuri_music.rc)
    set_source_files_properties(resources/yuri_music.rc PROPERTIES
            INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/resources"
            OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/resources/yuri_music.ico"
    )
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE "$<CONFIG:Release>")
    if(MSVC)
        target_link_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:Release>:/ENTRY:mainCRTStartup>")
    endif()
endif()