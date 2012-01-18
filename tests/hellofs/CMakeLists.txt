# CMakeLists.txt is part of mpqfs-stormlib, licensed via GNU General Public License (version 3).
# Bernd Lörwald <bloerwald+mpqfs@googlemail.com>

set (hellofs_src hello.c)

add_executable (hellofs ${hellofs_src})
target_link_libraries (hellofs ${FUSE_LIBRARIES})
