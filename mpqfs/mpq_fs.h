#include "fuse_file_system.h"
#include "path_type.h"

#include <StormLib.h>

#include <string>
#include <map>

class mpq_fs : public fuse::file_system
{
private:
  typedef std::map<std::string, fuse::stat_type> file_list_type;

  struct directory_entry
  {
    file_list_type _files;
    time_t _min_time;
    time_t _max_time;
  };

  typedef std::map<path_type, directory_entry> directories_type;

public:
  mpq_fs (const std::string& archive_name);
  virtual ~mpq_fs();

  virtual fuse::availability_flags availability() const;

  virtual fuse::stat_type get_attributes (const path_type& path);
  virtual void open (const path_type& path, fuse::file_info* file_info);
  virtual size_t read ( const path_type& path
                      , fuse::file_info* file_info
                      , char* result_buffer
                      , off_t offset
                      , size_t size
                      );
/*
  virtual size_t write ( const std::string& dir
               , const std::string& file
               , fuse::file_info* file_info
               , const char* data
               , off_t offset
               , size_t size
               );
*/

  virtual void list_files ( const path_type& path
                          , fuse::file_lister_helper& list
                          );
  virtual void make_directory (const path_type& path, mode_t mode);
  virtual void remove_directory (const path_type& path);
  virtual void remove_file (const path_type& path);
  virtual void create_file ( const path_type& path
                           , fuse::file_info* file_info
                           , mode_t mode);
  virtual void release_file_handle (const path_type&, fuse::file_info* info);
  virtual void rename_file (const path_type& source, const path_type& target);

private:
  file_list_type::iterator get_file (const path_type& path);

  HANDLE _internal;
  std::string _archive_name;
  directories_type _directories;
};
