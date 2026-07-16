add_subdirectory(libs/qq-music-api)
target_link_libraries(${PROJECT_NAME} PRIVATE qq_music_api)
qq_music_api_copy_scripts(${PROJECT_NAME})

# 构建后自动拷贝 resources 到可执行文件目录
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/resources
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
)
