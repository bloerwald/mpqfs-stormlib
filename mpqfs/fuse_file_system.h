// fuse_file_system.h is part of mpqfs-stormlib, licensed via GNU General Public License (version 3).
// Bernd Lörwald <bloerwald+mpqfs@googlemail.com>

#ifndef __FUSE_FILE_SYSTEM_H
#define __FUSE_FILE_SYSTEM_H

#include <string>

class path_type;

namespace c
{
  #include <fuse.h>
  #include <sys/stat.h>
  #include <sys/statvfs.h>
}

namespace fuse
{
  typedef c::fuse_file_info file_info;
  typedef struct c::stat stat_type;
  typedef struct c::statvfs statvfs_type;

  class file_lister_helper
  {
  public:
    file_lister_helper (void* result_buffer, c::fuse_fill_dir_t filler)
      : _result_buffer (result_buffer)
      , _filler (filler)
    { }

    void add (const std::string& name)
    {
      _filler (_result_buffer, name.c_str(), NULL, 0);
    }

    void* _result_buffer;
    c::fuse_fill_dir_t _filler;
  };

  struct availability_flags
  {
    unsigned long long int getattr : 1;
    unsigned long long int fgetattr : 1;
    unsigned long long int statfs : 1;
    unsigned long long int utimens : 1;
    unsigned long long int rename : 1;
    unsigned long long int setxattr : 1;
    unsigned long long int getxattr : 1;
    unsigned long long int listxattr : 1;
    unsigned long long int removexattr : 1;
    unsigned long long int symlink : 1;
    unsigned long long int link : 1;
    unsigned long long int readlink : 1;

    // directories
    unsigned long long int mkdir : 1;
    unsigned long long int rmdir : 1;
    unsigned long long int opendir : 1;
    unsigned long long int readdir : 1;
    unsigned long long int releasedir : 1;
    unsigned long long int fsyncdir : 1;

    // file
    unsigned long long int mknod : 1;
    unsigned long long int create : 1;
    unsigned long long int unlink : 1;

    unsigned long long int open : 1;
    unsigned long long int read : 1;
    unsigned long long int write : 1;
    unsigned long long int release : 1;

    unsigned long long int flush : 1;
    unsigned long long int fsync : 1;
    unsigned long long int truncate : 1;
    unsigned long long int ftruncate : 1;

    unsigned long long int bmap : 1;

    // permissions
    unsigned long long int access : 1;
    unsigned long long int chmod : 1;
    unsigned long long int chown : 1;
    unsigned long long int lock : 1;
  };

  class file_system
  {
  public:
    file_system();
    virtual ~file_system();

    virtual availability_flags availability() const;

    virtual stat_type get_attributes (const path_type& path);
    virtual stat_type get_attributes (const path_type& path, file_info* info);
    virtual void open (const path_type& path, file_info* info);
    virtual size_t read ( const path_type& path
                        , file_info* info
                        , char* result_buffer
                        , off_t offset
                        , size_t size
                        );
    virtual void write ( const path_type& path
                       , file_info* info
                       , const char* data
                       , off_t offset
                       , size_t size
                       );
    virtual void list_files (const path_type& path, file_lister_helper& list);
    virtual void make_directory (const path_type& directory, mode_t mode);
    virtual void remove_directory (const path_type& directory);
    virtual void remove_file (const path_type& path);
    virtual void create_file (const path_type& path, file_info* info, mode_t);
    virtual void release_file_handle (const path_type& path, file_info* info);
    virtual void rename_file (const path_type& source, const path_type& target);
    //! \todo Return, not pass by *?
    virtual void get_statistics (const path_type& path, statvfs_type* buf);
    virtual void change_owner (const path_type& path, uid_t uid, gid_t gid);
    virtual void change_mode (const path_type& path, mode_t mode);
    virtual void create_link (const path_type& oldpath, const path_type& newpath);
    virtual void create_sym_link (const path_type& linkname, const path_type& path);
    virtual void make_node (const path_type& path, mode_t mode, dev_t rdev);
    virtual path_type resolve_link (const path_type& path);
    virtual void truncate_file (const path_type& path, off_t size);
    virtual void truncate_open_file (const path_type& path, file_info* info, off_t size);
    virtual void sync_metadata (const path_type& path, bool only_sync_userdata, file_info* info);
    virtual void flush_data (const path_type& path, file_info* info);
    virtual void set_timestamps (const path_type& path, const timespec& tv0, const timespec& tv1);
  };

  int start_file_system (int argc, char** argv, file_system* file_system);
}

#endif
