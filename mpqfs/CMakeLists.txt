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
