cmake_minimum_required(VERSION 4.0)

include(libs/pixel-journey/cmake/toolchain.cmake)
include(libs/pixel-journey/cmake/cxx_modules.cmake)

project(yuri-music LANGUAGES CXX)

# 收集模块文件
file(GLOB CXX_MODULES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/*.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/*.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/*.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/pages/*.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models/*.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/store/*.cppm
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
include(cmake/yuri-lib.cmake)
include(cmake/web-view2.cmake)
include(cmake/bass.cmake)
include(cmake/qq-music-api.cmake)