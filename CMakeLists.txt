cmake_minimum_required(VERSION 4.0)

include(libs/pixel-journey/cmake/toolchain.cmake)
include(libs/pixel-journey/cmake/cxx_modules.cmake)

project(yuri-music LANGUAGES C CXX)

# 收集模块文件
file(GLOB CXX_MODULES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/*.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/*.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/controls/player_bar/*.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/*.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/pages/*.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models/*.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/store/*.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/player/*.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/*.cppm
)

add_executable(${PROJECT_NAME} src/main.cpp libs/sqlite3/sqlite3.c)

# 配置模块支持
target_sources(${PROJECT_NAME}
    PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
    ${CXX_MODULES}
)

# 链接所需依赖
include(cmake/yuri-lib.cmake)
include(cmake/web-view2.cmake)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    pkg_check_modules(GIO REQUIRED IMPORTED_TARGET gio-2.0)
    target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::GIO)

    add_executable(yuri-music-waybar src/tools/waybar_lyrics_client.cpp)
    set_target_properties(yuri-music-waybar PROPERTIES CXX_SCAN_FOR_MODULES OFF CXX_MODULE_STD OFF)
    target_link_libraries(yuri-music-waybar PRIVATE PkgConfig::GIO)
    add_dependencies(${PROJECT_NAME} yuri-music-waybar)
endif()
include(cmake/bass.cmake)
include(cmake/qq-music-api.cmake)
include(cmake/ico.cmake)
