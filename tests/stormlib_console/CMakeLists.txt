set (stormlib_console_src console.cpp)

add_executable (stormlib_console ${stormlib_console_src})
target_link_libraries (stormlib_console ${STORMLIB_LIBRARIES})
