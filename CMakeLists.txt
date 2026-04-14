cmake_minimum_required(VERSION 4.0)

project(yuri-music LANGUAGES CXX)

# 收集模块文件
file(GLOB CXX_MODULES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/*.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/*.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/pages/*.cppm
)

add_executable(${PROJECT_NAME} ${CXX_MODULES} src/main.cpp)

# 配置模块支持
target_sources(${PROJECT_NAME}
    PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
    ${CXX_MODULES}
)

target_link_libraries(${PROJECT_NAME} yuri_lib)

# 构建后自动拷贝 resources 到可执行文件目录
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/resources
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
    COMMENT "拷贝资源文件到指定目录..."
)

