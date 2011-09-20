set (hellofs_src hello.c)

add_executable (hellofs ${hellofs_src})
target_link_libraries (hellofs ${FUSE_LIBRARIES})
