#include "fuse_file_system.h"

#include <iostream>

//! \todo Remove again.
#include <errno.h>

#include "path_type.h"
#include "system_error.h"

namespace fuse
{
  namespace callbacks
  {
    static inline file_system* get_file_system_from_fuse_context()
    {
      return static_cast <file_system*> (c::fuse_get_context()->private_data);
    }

    static int getattr (const char* path, struct c::stat* stat_buf)
    {
      try
      {
        *stat_buf = get_file_system_from_fuse_context()->get_attributes (path);
        return 0;
      }
      CATCH_AND_RETURN_ERROR;
    }

    static int readdir ( const char* path
                       , void* result_buffer
                       , c::fuse_fill_dir_t filler
                       , off_t
                       , c::fuse_file_info*
                       )
    {
      //! \todo Use offset for faster stuff.

      file_lister_helper helper (result_buffer, filler);
      helper.add (".");
      helper.add ("..");

      try
      {
        get_file_system_from_fuse_context()->list_files (path, helper);
        return 0;
      }
      CATCH_AND_RETURN_ERROR;
    }

    static int open (const char* path, c::fuse_file_info* file_info)
    {
      try
      {
        get_file_system_from_fuse_context()->open (path, file_info);
        return 0;
      }
      CATCH_AND_RETURN_ERROR;
    }

    static int read ( const char* path
                    , char* result_buffer
                    , size_t size
                    , off_t offset
                    , c::fuse_file_info* file_info
                    )
    {
      try
      {
        return get_file_system_from_fuse_context()->read (path, file_info, result_buffer, offset, size);
      }
      CATCH_AND_RETURN_ERROR;
    }

    static int mkdir (const char* path, mode_t mode)
    {
      try
      {
        get_file_system_from_fuse_context()->make_directory (path, mode);
        return 0;
      }
      CATCH_AND_RETURN_ERROR;
    }

    static int rmdir (const char* path)
    {
      try
      {
        get_file_system_from_fuse_context()->remove_directory (path);
        return 0;
      }
      CATCH_AND_RETURN_ERROR;
    }

    static int create (const char* path, mode_t mode, struct c::fuse_file_info* file_info)
    {
      try
      {
        get_file_system_from_fuse_context()->create_file (path, file_info, mode);
        return 0;
      }
      CATCH_AND_RETURN_ERROR;
    }

    static int release (const char* path, c::fuse_file_info* file_info)
    {
      try
      {
        get_file_system_from_fuse_context()->release_file_handle (path, file_info);
        return 0;
      }
      CATCH_AND_RETURN_ERROR;
    }

    static int unlink (const char* path)
    {
      try
      {
        get_file_system_from_fuse_context()->remove_file (path);
        return 0;
      }
      CATCH_AND_RETURN_ERROR;
    }

    static int write (const char* path, const char* data, size_t size, off_t offset, c::fuse_file_info* file_info)
    {
      try
      {
        //! \note Either write it all or throw.
        get_file_system_from_fuse_context()->write (path, file_info, data, offset, size);
        return size;
      }
      CATCH_AND_RETURN_ERROR;
    }

    static int rename (const char* source, const char* target)
    {
      try
      {
        get_file_system_from_fuse_context()->rename_file (source, target);
        return 0;
      }
      CATCH_AND_RETURN_ERROR;
    }

    static void* init (c::fuse_conn_info*)
    {
      return c::fuse_get_context()->private_data;
    }

    static void destroy (void* private_data)
    {
      delete static_cast<fuse::file_system*> (private_data);
    }

    static int utimens (const char* path, const timespec tv[2])
    {
      try
      {
        get_file_system_from_fuse_context()->set_timestamps (path, tv[0], tv[1]);
        return 0;
      }
      CATCH_AND_RETURN_ERROR;
    }
    static int flush (const char* path, c::fuse_file_info* file_info)
    {
      try
      {
        get_file_system_from_fuse_context()->flush_data (path, file_info);
        return 0;
      }
      CATCH_AND_RETURN_ERROR;
    }
    static int fsync (const char* path, int only_sync_userdata, c::fuse_file_info* file_info)
    {
      try
      {
        get_file_system_from_fuse_context()->sync_metadata (path, only_sync_userdata, file_info);
        return 0;
      }
      CATCH_AND_RETURN_ERROR;
    }
    static int truncate (const char* path, off_t size)
    {
      try
      {
        get_file_system_from_fuse_context()->truncate_file (path, size);
        return 0;
      }
      CATCH_AND_RETURN_ERROR;
    }
    static int readlink (const char* path, char* buf, size_t len)
    {
      try
      {
        const std::string target (get_file_system_from_fuse_context()->resolve_link (path).full_path());
        memcpy (buf, target.c_str(), std::min (target.size(), len));
        return 0;
      }
      CATCH_AND_RETURN_ERROR;
    }
    static int mknod (const char* path, mode_t mode, dev_t rdev)
    {
      try
      {
        get_file_system_from_fuse_context()->make_node (path, mode, rdev);
        return 0;
      }
      CATCH_AND_RETURN_ERROR;
    }
    static int symlink (const char* linkname, const char* path)
    {
      try
      {
        get_file_system_from_fuse_context()->create_sym_link (linkname, path);
        return 0;
      }
      CATCH_AND_RETURN_ERROR;
    }
    static int link (const char* oldpath, const char* newpath)
    {
      try
      {
        get_file_system_from_fuse_context()->create_link (oldpath, newpath);
        return 0;
      }
      CATCH_AND_RETURN_ERROR;
    }
    static int chmod (const char* path, mode_t mode)
    {
      try
      {
        get_file_system_from_fuse_context()->change_mode (path, mode);
        return 0;
      }
      CATCH_AND_RETURN_ERROR;
    }
    static int chown (const char* path, uid_t uid, gid_t gid)
    {
      try
      {
        get_file_system_from_fuse_context()->change_owner (path, uid, gid);
        return 0;
      }
      CATCH_AND_RETURN_ERROR;
    }
    static int statfs (const char* path, struct c::statvfs* buf)
    {
      try
      {
        get_file_system_from_fuse_context()->get_statistics (path, buf);
        return 0;
      }
      CATCH_AND_RETURN_ERROR;
    }
#if (__FreeBSD__ >= 10)
    static int setxattr (const char* path, const char* name, const char* value, size_t size, int flags, uint32_t position)
    {
      std::cerr << "setxattr" << std::endl;
      return -ENOSYS;
    }
    static int getxattr (const char* path, const char* name, char* value, size_t size, uint32_t position)
    {
      std::cerr << "getxattr" << std::endl;
      return -ENOSYS;
    }
#else
    static int setxattr (const char* path, const char* name, const char* value, size_t size, int flags)
    {
      std::cerr << "setxattr" << std::endl;
      return -ENOSYS;
    }
    static int getxattr (const char* path, const char* name, char* value, size_t size)
    {
      std::cerr << "getxattr" << std::endl;
      return -ENOSYS;
    }
#endif
    static int listxattr (const char* path, char* list, size_t size)
    {
      std::cerr << "listxattr" << std::endl;
      return -ENOSYS;
    }
    static int removexattr (const char* path, const char* name)
    {
      std::cerr << "removexattr" << std::endl;
      return -ENOSYS;
    }
    static int opendir (const char* path, struct c::fuse_file_info * info)
    {
      std::cerr << "opendir" << std::endl;
      return -ENOSYS;
    }
    static int releasedir (const char* path, struct c::fuse_file_info * info)
    {
      std::cerr << "releasedir" << std::endl;
      return -ENOSYS;
    }
    static int access (const char* path, int mask)
    {
      std::cerr << "access" << std::endl;
      return -ENOSYS;
    }
    static int ftruncate (const char* path, off_t size, struct c::fuse_file_info *info)
    {
      try
      {
        get_file_system_from_fuse_context()->truncate_open_file (path, info, size);
        return 0;
      }
      CATCH_AND_RETURN_ERROR;
    }
    static int fgetattr (const char* path, struct c::stat *buf, struct c::fuse_file_info *info)
    {
      try
      {
        *buf = get_file_system_from_fuse_context()->get_attributes (path, info);
        return 0;
      }
      CATCH_AND_RETURN_ERROR;
    }
    static int lock (const char* path, struct c::fuse_file_info * info, int cmd, struct c::flock * lock)
    {
      std::cerr << "lock" << std::endl;
      return -ENOSYS;
    }
    static int bmap (const char* path, size_t blocksize, uint64_t *idx)
    {
      std::cerr << "bmap" << std::endl;
      return -ENOSYS;
    }
    static int fsyncdir (const char* path, int datasync, struct c::fuse_file_info *fi)
    {
      std::cerr << "fsyncdir" << std::endl;
      return -ENOSYS;
    }

#define MAYBE_SET(x) if (availability.x == 1) table->x = x; else table->x = NULL

    c::fuse_operations* operations (file_system* fs)
    {
      c::fuse_operations* table = new c::fuse_operations;
      memset (table, 0, sizeof (c::fuse_operations));

      const availability_flags& availability (fs->availability());

      // attributes
      MAYBE_SET (getattr);
      MAYBE_SET (fgetattr);
      MAYBE_SET (statfs);
      MAYBE_SET (utimens);
      MAYBE_SET (rename);
      MAYBE_SET (setxattr);
      MAYBE_SET (getxattr);
      MAYBE_SET (listxattr);
      MAYBE_SET (removexattr);

      // (sym)links
      MAYBE_SET (symlink);
      MAYBE_SET (link);
      MAYBE_SET (readlink);

      // directories
      MAYBE_SET (mkdir);
      MAYBE_SET (rmdir);
      MAYBE_SET (opendir);
      MAYBE_SET (readdir);
      MAYBE_SET (releasedir);
      MAYBE_SET (fsyncdir);

      // file
      MAYBE_SET (mknod);
      MAYBE_SET (create);
      MAYBE_SET (unlink);

      MAYBE_SET (open);
      MAYBE_SET (read);
      MAYBE_SET (write);
      MAYBE_SET (release);

      MAYBE_SET (flush);
      MAYBE_SET (fsync);
      MAYBE_SET (truncate);
      MAYBE_SET (ftruncate);

      MAYBE_SET (bmap);

      // permissions
      MAYBE_SET (access);
      MAYBE_SET (chmod);
      MAYBE_SET (chown);
      MAYBE_SET (lock);

      // general
      table->init = init;
      table->destroy = destroy;

      return table;
    }
#undef MAYBE_SET
  }
}
