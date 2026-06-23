cmake_minimum_required(VERSION 4.0)

project(yuri-music LANGUAGES CXX)

include(cmake/bass.cmake)
include(cmake/web-view2.cmake)

# 收集模块文件
file(GLOB CXX_MODULES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/*.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/*.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/*.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/pages/*.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models/*.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/*.cppm
)

add_executable(${PROJECT_NAME} ${CXX_MODULES} src/main.cpp)

# 配置模块支持
target_sources(${PROJECT_NAME}
    PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
    ${CXX_MODULES}
)

# 链接所需依赖
target_link_libraries(${PROJECT_NAME} PRIVATE yuri_lib)
configure_yuri_music_webview2(${PROJECT_NAME})
configure_yuri_music_bass(${PROJECT_NAME})

# 链接api依赖
add_subdirectory(libs/qq-music-api)
target_link_libraries(${PROJECT_NAME} PRIVATE qq_music_api)
qq_music_api_copy_scripts(${PROJECT_NAME})

# 构建后自动拷贝 resources 到可执行文件目录
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/resources
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
)
