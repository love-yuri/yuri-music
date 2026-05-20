cmake_minimum_required(VERSION 4.0)

project(yuri-music LANGUAGES CXX)

# 收集模块文件
file(GLOB CXX_MODULES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/*.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/*.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/pages/*.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/*.cppm
)

add_executable(${PROJECT_NAME} ${CXX_MODULES} src/main.cpp)

# 配置模块支持
target_sources(${PROJECT_NAME}
    PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
    ${CXX_MODULES}
)

add_subdirectory(libs/qq-music-api)
target_link_libraries(${PROJECT_NAME} yuri_lib)
target_link_libraries(${PROJECT_NAME} qq_music_api)
qq_music_api_copy_scripts(${PROJECT_NAME})

set(BASS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/libs/bass24")
set(BASS_INCLUDE_DIR "${BASS_ROOT}/c")

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "yuri-music only supports 64-bit BASS builds.")
endif()

if(WIN32)
    set(BASS_LIBRARY "${BASS_ROOT}/c/x64/bass.lib")
    set(BASS_RUNTIME_LIBRARY "${BASS_ROOT}/x64/bass.dll")
elseif(LINUX)
    set(BASS_LIBRARY "${BASS_ROOT}/linux/x64/libbass.so")
    set(BASS_RUNTIME_LIBRARY "${BASS_LIBRARY}")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN"
    )
endif()

if(NOT WIN32 AND NOT LINUX)
    message(FATAL_ERROR "yuri-music only supports BASS on 64-bit Windows and 64-bit Linux.")
endif()

if(NOT EXISTS "${BASS_INCLUDE_DIR}/bass.h")
    message(FATAL_ERROR "BASS header not found: ${BASS_INCLUDE_DIR}/bass.h")
endif()

if(NOT EXISTS "${BASS_LIBRARY}")
    message(FATAL_ERROR "BASS link library not found: ${BASS_LIBRARY}")
endif()

if(NOT EXISTS "${BASS_RUNTIME_LIBRARY}")
    message(FATAL_ERROR "BASS runtime library not found: ${BASS_RUNTIME_LIBRARY}")
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${BASS_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${BASS_LIBRARY})

# 构建后自动拷贝 resources 到可执行文件目录
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/resources
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${BASS_RUNTIME_LIBRARY}
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
)
