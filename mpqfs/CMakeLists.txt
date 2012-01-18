# CMakeLists.txt is part of mpqfs-stormlib, licensed via GNU General Public License (version 3).
# Bernd Lörwald <bloerwald+mpqfs@googlemail.com>

set ( mpqfs_src
      main.cpp
      mpq_fs.cpp
      system_error.cpp
      storm_error_code_thrower.cpp
      fuse_file_system.cpp
      fuse_file_system.callbacks.cpp
      path_type.cpp
    )

add_executable (mpqfs ${mpqfs_src})
target_link_libraries (mpqfs ${FUSE_LIBRARIES} ${STORM_LIBRARIES})
