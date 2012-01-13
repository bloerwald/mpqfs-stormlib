#include "fuse_file_system.h"

#include <iostream>

#include "system_error.h"
#include "path_type.h"

namespace fuse
{
  file_system::file_system() { }
  file_system::~file_system() { }

  availability_flags file_system::availability() const
  {
    availability_flags flags;
    memset (&flags, 0, sizeof (availability_flags));
    return flags;
  }

  stat_type file_system::get_attributes (const path_type&)
  {
    std::cerr << "not implemented: get_attributes" << std::endl;
    throw function_not_implemented();
  }

  stat_type file_system::get_attributes (const path_type&, file_info*)
  {
    std::cerr << "not implemented: get_attributes with info" << std::endl;
    throw function_not_implemented();
  }

  void file_system::open (const path_type&, file_info*)
  {
    std::cerr << "not implemented: open" << std::endl;
    throw function_not_implemented();
  }

  size_t file_system::read (const path_type&, file_info*, char*, off_t, size_t)
  {
    std::cerr << "not implemented: read" << std::endl;
    throw function_not_implemented();
  }

  void file_system::write (const path_type&, file_info*, const char*, off_t, size_t)
  {
    std::cerr << "not implemented: write" << std::endl;
    throw function_not_implemented();
  }

  void file_system::list_files (const path_type&, fuse::file_lister_helper&)
  {
    std::cerr << "not implemented: list_files" << std::endl;
    throw function_not_implemented();
  }

  void file_system::make_directory (const path_type&, mode_t mode)
  {
    std::cerr << "not implemented: make_directory" << std::endl;
    throw function_not_implemented();
  }

  void file_system::remove_directory (const path_type&)
  {
    std::cerr << "not implemented: remove_directory" << std::endl;
    throw function_not_implemented();
  }

  void file_system::remove_file (const path_type&)
  {
    std::cerr << "not implemented: remove_file" << std::endl;
    throw function_not_implemented();
  }

  void file_system::create_file (const path_type&, file_info*, mode_t)
  {
    std::cerr << "not implemented: create_file" << std::endl;
    throw function_not_implemented();
  }

  void file_system::release_file_handle (const path_type&, file_info*)
  {
    std::cerr << "not implemented: release_file_handle" << std::endl;
    throw function_not_implemented();
  }

  void file_system::rename_file (const path_type&, const path_type&)
  {
    std::cerr << "not implemented: rename_file" << std::endl;
    throw function_not_implemented();
  }

  void file_system::get_statistics (const path_type& path, statvfs_type* buf)
  {
    std::cerr << "not implemented: get_statistics" << std::endl;
    //throw function_not_implemented();
  }

  void file_system::change_owner (const path_type& path, uid_t uid, gid_t gid)
  {
    std::cerr << "not implemented: change_owner" << std::endl;
    throw function_not_implemented();
  }

  void file_system::change_mode (const path_type& path, mode_t mode)
  {
    std::cerr << "not implemented: change_mode" << std::endl;
    throw function_not_implemented();
  }

  void file_system::create_link (const path_type& oldpath, const path_type& newpath)
  {
    std::cerr << "not implemented: create_link" << std::endl;
    throw function_not_implemented();
  }

  void file_system::create_sym_link (const path_type& linkname, const path_type& path)
  {
    std::cerr << "not implemented: create_sym_link" << std::endl;
    throw function_not_implemented();
  }

  void file_system::make_node (const path_type& path, mode_t mode, dev_t rdev)
  {
    std::cerr << "not implemented: make_node" << std::endl;
    throw function_not_implemented();
  }

  path_type file_system::resolve_link (const path_type& path)
  {
    std::cerr << "not implemented: resolve_link" << std::endl;
    throw function_not_implemented();
  }

  void file_system::truncate_file (const path_type& path, off_t size)
  {
    std::cerr << "not implemented: truncate_file" << std::endl;
    throw function_not_implemented();
  }

  void file_system::truncate_open_file (const path_type&, file_info*, off_t)
  {
    std::cerr << "not implemented: truncate_open_file" << std::endl;
    throw function_not_implemented();
  }

  void file_system::sync_metadata (const path_type& path, bool only_sync_userdata, file_info* info)
  {
    std::cerr << "not implemented: sync_metadata" << std::endl;
    throw function_not_implemented();
  }

  void file_system::flush_data (const path_type& path, file_info* info)
  {
    std::cerr << "not implemented: flush_data" << std::endl;
    throw function_not_implemented();
  }

  void file_system::set_timestamps (const path_type& path, const timespec& tv0, const timespec& tv1)
  {
    std::cerr << "not implemented: set_timestamps" << std::endl;
    throw function_not_implemented();
  }

  namespace callbacks
  {
    extern c::fuse_operations* operations (file_system*);
  }

  //! \todo Threaded, own loop, not fuse_main, to be able to do more.
  int start_file_system (int argc, char** argv, file_system* file_system)
  {
    try
    {
      return c::fuse_main ( argc
                          , argv
                          , callbacks::operations (file_system)
                          , file_system
                          );
    }
    catch (const std::exception& exception)
    {
      std::cerr << exception.what() << "\n";
      return -1;
    }
  }
}
