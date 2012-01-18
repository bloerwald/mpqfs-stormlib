# CMakeLists.txt is part of mpqfs-stormlib, licensed via GNU General Public License (version 3).
# Bernd Lörwald <bloerwald+mpqfs@googlemail.com>

set (stormlib_console_src console.cpp)

add_executable (stormlib_console ${stormlib_console_src})
target_link_libraries (stormlib_console ${STORM_LIBRARIES})
