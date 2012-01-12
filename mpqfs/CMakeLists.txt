set ( mpqfs_src
      mpqfs.cpp
      system_error.cpp
      storm_error_code_thrower.cpp
    )

add_executable (mpqfs ${mpqfs_src})
target_link_libraries (mpqfs ${FUSE_LIBRARIES} ${STORM_LIBRARIES})
