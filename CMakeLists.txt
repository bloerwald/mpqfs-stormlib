# CMakeLists.txt is part of mpqfs-stormlib, licensed via GNU General Public License (version 3).
# Bernd Lörwald <bloerwald+mpqfs@googlemail.com>

cmake_minimum_required (VERSION 2.0)

project (MPQfs)

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/)

find_package (FUSE)
find_package (storm)

include_directories (${FUSE_INCLUDE_DIR})
include_directories (${storm_INCLUDE_DIR})

add_definitions (-D_FILE_OFFSET_BITS=64 -D__FreeBSD__=10 -DFUSE_USE_VERSION=26)

add_subdirectory (mpqfs)
add_subdirectory (tests)
