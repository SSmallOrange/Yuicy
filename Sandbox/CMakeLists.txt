yuicy_collect_sources(SANDBOX_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_executable(Sandbox ${SANDBOX_SOURCES})

target_link_libraries(Sandbox PRIVATE Yuicy)

yuicy_configure_target(Sandbox)
yuicy_configure_app(Sandbox)
