yuicy_collect_sources(YUISTUDIO_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_executable(YuiStudio ${YUISTUDIO_SOURCES})

target_precompile_headers(YuiStudio PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.h")
target_include_directories(YuiStudio PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(YuiStudio PRIVATE Yuicy)

yuicy_configure_target(YuiStudio)
yuicy_configure_app(YuiStudio)
