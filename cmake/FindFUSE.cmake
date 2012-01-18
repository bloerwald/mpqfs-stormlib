# FindFUSE.cmake is part of mpqfs-stormlib, licensed via GNU General Public License (version 3).
# Bernd Lörwald <bloerwald+mpqfs@googlemail.com>

FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(PC_FUSE fuse)

FIND_PATH( FUSE_INCLUDE_DIR fuse/fuse.h
           HINTS
           ${PC_FUSE_INCLUDE_DIRS}
           ${PC_FUSE_INCLUDEDIR}
           PATH_SUFFIXES fuse
          )

FOREACH (lib ${PC_FUSE_LIBRARIES})
  FIND_LIBRARY( FUSE_LIBRARY_${lib} NAMES ${lib}
                HINTS
                ${PC_FUSE_LIBDIR}
                ${PC_FUSE_LIBRARY_DIRS}
        )

  SET(FUSE_LIBRARIES ${FUSE_LIBRARIES} ${FUSE_LIBRARY_${lib}})
ENDFOREACH (lib ${PC_FUSE_LIBRARIES})

if (NOT FUSE_INCLUDE_DIR)
  message (FATAL_ERROR "FUSE NOT FOUND!")
endif (NOT FUSE_INCLUDE_DIR)

