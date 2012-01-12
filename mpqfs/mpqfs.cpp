#include "system_error.h"

#include "storm_error_code_thrower.h"

#include <fuse.h>

#include <StormLib.h>

#include <vector>
#include <iostream>
#include <string>
#include <sstream>
#include <map>

typedef struct stat stat_type;

struct file_entry_type
{
  stat_type _stat;
  std::string _filename;

  file_entry_type (const stat_type& stat, const std::string& filename)
    : _stat (stat)
    , _filename (filename)
  { }

  static file_entry_type dummy (const std::string& filename)
  {
    return file_entry_type (stat_type(), filename);
  }

  bool operator == (const file_entry_type& other) const
  {
    return _filename == other._filename;
  }
};

typedef std::vector<file_entry_type> file_list_type;

struct directory_entry
{
  file_list_type _files;
  time_t _min_time;
  time_t _max_time;
};

typedef std::map<std::string, directory_entry> directories_type;

void replace ( std::string& string
             , const std::string& old
             , const std::string& with
             )
{
  size_t pos = string.find (old);
  while (pos != std::string::npos)
  {
    string.replace (pos, old.length(), with);
    pos = string.find (old);
  }
}

time_t filetime_to_time_t (const long long int& lo, const long long int& hi)
{
  return time_t ((hi << 32 | lo) / 10000000LL - 11644473600LL);
}

ULONGLONG time_t_to_filetime (const time_t& time)
{
  return ULONGLONG ((time + 11644473600LL) * 10000000LL);
}

class file_lister_helper
{
public:
  file_lister_helper (void* result_buffer, fuse_fill_dir_t filler)
    : _result_buffer (result_buffer)
    , _filler (filler)
  { }

  void add (const std::string& name)
  {
    _filler (_result_buffer, name.c_str(), NULL, 0);
  }

  void* _result_buffer;
  fuse_fill_dir_t _filler;
};




inline void split_into_dir_and_file ( const std::string& in
                                    , std::string& path
                                    , std::string& file
                                    )
{
  const size_t slash_pos (in.find_last_of ('/'));
  path = in.substr (0, slash_pos + 1);
  file = in.substr (slash_pos + 1);
}

class path_type
{
  typedef std::vector<std::string> elements_type;

public:
  path_type (const char* c_str)
    : _elements (0)
  {
    std::string s (c_str);
    replace (s, "\\", "/");

    std::stringstream ss (s);
    std::string item;
    while (std::getline (ss, item, '/'))
    {
      if (item != "")
      {
        _elements.push_back (item);
      }
    }
  }

  const std::string& file() const
  {
    return _elements.back();
  }
  std::string directory ( const std::string& delimiter = "/"
                        , const bool& start_with_delimiter = true
                        ) const
  {
    return join_path (_elements.end() - 1, delimiter, start_with_delimiter) + delimiter;
  }

  std::string full_path ( const bool& is_a_directory = false
                        , const std::string& delimiter = "/"
                        , const bool& start_with_delimiter = true
                        ) const
  {
    if (is_a_directory)
    {
      return join_path (_elements.end(), delimiter, start_with_delimiter);
    }
    else
    {
      return join_path (_elements.end(), delimiter, start_with_delimiter) + delimiter;
    }
  }

private:
  inline std::string join_path ( const elements_type::const_iterator& end
                               , const std::string& delimiter = "/"
                               , const bool& start_with_delimiter = true
                               ) const
  {
    std::string return_value;
    for (elements_type::const_iterator it (_elements.begin()); it != end; ++it)
    {
      return_value += delimiter + *it;
    }
    if (!start_with_delimiter)
    {
      return return_value.substr (1);
    }
    else
    {
      if (_elements.begin() == end)
      {
        return_value = delimiter;
      }
      return return_value;
    }
  }

  elements_type _elements;
};

class fuse_file_system
{
public:
  fuse_file_system() { }
  virtual ~fuse_file_system() { }

  virtual stat_type get_attributes ( const std::string& dir
                                   , const std::string& file
                                   )
  {
    throw function_not_implemented();
  }
  virtual void open ( const std::string& dir
                    , const std::string& file
                    , fuse_file_info* file_info
                    )
  {
    throw function_not_implemented();
  }
  virtual size_t read ( const std::string& dir
                      , const std::string& file
                      , fuse_file_info* file_info
                      , char* result_buffer
                      , off_t offset
                      , size_t size
                      )
  {
    throw function_not_implemented();
  }
  virtual void list_files ( const std::string& directory
                          , file_lister_helper& list
                          )
  {
    throw function_not_implemented();
  }
  virtual void make_directory (std::string directory)
  {
    throw function_not_implemented();
  }
  virtual void remove_directory (std::string directory)
  {
    throw function_not_implemented();
  }
  virtual void remove_file (const path_type& path)
  {
    throw function_not_implemented();
  }
  virtual void create_file ( const path_type& path
                           , fuse_file_info* file_info
                           , mode_t mode
                           )
  {
    throw function_not_implemented();
  }
  virtual void release_file_handle ( const std::string& dir
                                   , const std::string& file
                                   , fuse_file_info* file_info
                                   )
  {
    throw function_not_implemented();
  }
  virtual void rename_file ( const std::string& source_directory
                           , const std::string& source_file
                           , const std::string& target_directory
                           , const std::string& target_file
                           )
  {
    throw function_not_implemented();
  }
};

class mpq_fs : public fuse_file_system
{
public:
  mpq_fs (const std::string& archive_name)
    : fuse_file_system()
    , _archive_name (archive_name)
  {
    if (!SFileOpenArchive (archive_name.c_str(), 0, 0, &_internal))
    {
      throw_error_if_available();
    }

    //! \note Dummy.
    _directories["/"]._min_time = 0;

    SFILE_FIND_DATA file_data;
    for ( HANDLE find_handle (SFileFindFirstFile (_internal, "*", &file_data, NULL))
        ; GetLastError() != ERROR_NO_MORE_FILES
        ; SFileFindNextFile (find_handle, &file_data)
        )
    {
      std::string filename (file_data.cFileName);

      std::cout << "+ " << filename << std::endl;

      if (filename == "(listfile)" || filename == "(attributes)")
      {
        continue;
      }

      filename = "/" + filename;
      replace (filename, "\\", "/");
      size_t pos (filename.find_last_of ('/'));
      std::string directory (filename.substr (0, pos + 1));
      std::string file (filename.substr (pos + 1));

      stat_type stat;
      memset (&stat, 0, sizeof (stat));
      //! \todo Write permission?
      stat.st_mode = S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
      stat.st_nlink = 1;
      stat.st_size = file_data.dwFileSize;

      const time_t unix_time ( filetime_to_time_t ( file_data.dwFileTimeLo
                                                  , file_data.dwFileTimeHi
                                                  )
                             );

      //! \todo access and modification dynamic?
      stat.st_atime = unix_time;
      stat.st_mtime = unix_time;
      stat.st_ctime = unix_time;
      stat.st_birthtime = unix_time;

      _directories[directory]._max_time = std::max ( unix_time
                                                  , _directories[directory]._max_time
                                                  );
      _directories[directory]._min_time = std::min ( unix_time
                                                  , _directories[directory]._max_time
                                                  );

      _directories[directory]._files.push_back (file_entry_type (stat, file));
    }

    //! \note insert dummy directories for directories without files and only sub directories.
    for ( directories_type::iterator it (_directories.begin())
        ; it != _directories.end()
        ; ++it
        )
    {
      const std::string& sub_dir_path (it->first);

      size_t f (sub_dir_path.find_first_of ('/'));
      do
      {
        _directories[sub_dir_path.substr (0, f + 1)]._min_time = 0;
        f = sub_dir_path.find_first_of ('/', f + 1);
      }
      while (f != std::string::npos);
    }
  }

  ~mpq_fs()
  {
    //! \todo close files? Are all file handles closed earlier?
    SFileCloseArchive (_internal);
  }

  virtual stat_type get_attributes (const std::string& dir, const std::string& file)
  {
    std::cout << "get_attributes ( " << dir << ", " << file << " ..._ >" << assemble_directory (dir, file) << std::endl;
    directories_type::iterator dir_it (_directories.find (assemble_directory (dir, file)));
    if (dir_it != _directories.end())
    {
      stat_type directory;
      memset (&directory, 0, sizeof (directory));
      directory.st_mode = S_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH;
      directory.st_nlink = 3;
      directory.st_atime = dir_it->second._max_time;
      directory.st_mtime = dir_it->second._max_time;
      directory.st_ctime = dir_it->second._min_time;
      directory.st_birthtime = dir_it->second._min_time;
      return directory;
    }

    return get_file (dir, file)->_stat;
  }

  virtual void open (const std::string& dir, const std::string& file, fuse_file_info* file_info)
  {

    file_list_type::iterator file_it (get_file (dir, file));

    std::string path ((dir + file).substr (1));
    replace (path, "/", "\\");

    if (!SFileOpenFileEx (_internal, path.c_str(), SFILE_OPEN_FROM_MPQ, reinterpret_cast<HANDLE*> (&file_info->fh)))
    {
      throw no_such_file();
    }
  }

  virtual size_t read ( const std::string& dir
              , const std::string& file
              , fuse_file_info* file_info
              , char* result_buffer
              , off_t offset
              , size_t size
              )
  {
    file_list_type::iterator file_it (get_file (dir, file));

    if (offset < file_it->_stat.st_size)
    {
      if (offset + size > file_it->_stat.st_size)
      {
        size = file_it->_stat.st_size - offset;
      }

      //! \todo not null but size_hi?
      SFileSetFilePointer (reinterpret_cast<HANDLE> (file_info->fh), offset, NULL, FILE_BEGIN);

      DWORD read_bytes (size);
      SFileReadFile (reinterpret_cast<HANDLE> (file_info->fh), result_buffer, size, &read_bytes);

      return read_bytes;
    }

    throw access_denied();
  }

/*
  virtual size_t write ( const std::string& dir
               , const std::string& file
               , fuse_file_info* file_info
               , const char* data
               , off_t offset
               , size_t size
               )
  {
    std::cout << "write (" << dir << ", " << file << ", file_info, " << data << ", " << offset << ");\n";

    file_list_type::iterator file_it (get_file (dir, file));

    if (offset <= file_it->_stat.st_size)
    {
      std::cout << "offset < statsize\n";
      if (offset + size > file_it->_stat.st_size)
      {
        file_it->_stat.st_size = size + offset;
        std::cout << file_it->_stat.st_size << " = " << size<<" + "<<offset<<"\n";
      }

      //! \todo not null but size_hi?
      SFileSetFilePointer (reinterpret_cast<HANDLE> (file_info->fh), offset, NULL, FILE_BEGIN);

      if (!SFileWriteFile (reinterpret_cast<HANDLE> (file_info->fh), data, size, 0))
      {
        std::cout << "writefail " << GetLastError() << "\n";
        throw access_denied();
      }
    }
    else
    {
      std::cout << "offset > statsize ( " << file_it->_stat.st_size << ")\n";
      throw access_denied();
    }

    return size;
  }
*/

  virtual void list_files (const std::string& directory, file_lister_helper& list)
  {

    std::cout << "dirlist: " << directory << std::endl;

    directories_type::iterator dir (_directories.find (directory));
    if (dir == _directories.end())
    {
      throw no_such_file();
    }

    for ( directories_type::iterator it (_directories.begin())
        ; it != _directories.end()
        ; ++it
        )
    {
      const std::string& sub_dir (it->first);
      if ( it == dir
        || !std::equal (directory.begin(), directory.end(), sub_dir.begin())
         )
      {
        continue;
      }

      const std::string proper_sub_dir (sub_dir.substr (directory.length()));
      if (proper_sub_dir.find_first_of ('/') == proper_sub_dir.length() - 1)
      {
        list.add (proper_sub_dir.substr (0, proper_sub_dir.length() - 1));
      }
    }

    for ( file_list_type::const_iterator it (dir->second._files.begin())
        ; it != dir->second._files.end()
        ; ++it
        )
    {
      list.add (it->_filename);
    }
  }

  virtual void make_directory (std::string directory)
  {
    directory += "/";
    _directories[directory]._min_time = 0;
    time (&_directories[directory]._min_time);
    _directories[directory]._max_time = _directories[directory]._min_time;
  }

  virtual void remove_directory (std::string directory)
  {
    directory += "/";

    directories_type::iterator dir_it (_directories.find (directory));
    if (dir_it == _directories.end())
    {
      throw no_such_file();
    }

    if (!dir_it->second._files.empty())
    {
      throw not_empty();
    }

    for ( directories_type::iterator it (_directories.begin())
        ; it != _directories.end()
        ; ++it
        )
    {
      if ( it != dir_it
        && std::equal (directory.begin(), directory.end(), it->first.begin())
         )
      {
        throw not_empty();
      }
    }

    _directories.erase (dir_it);
  }

  virtual void remove_file (const path_type& path)
  {
    //! \todo Do all files get closed before this is called?

    if (!SFileRemoveFile (_internal, path.full_path (false, "\\", false).c_str()))
    {
      throw_error_if_available();
    }

    _directories[path.directory()]._files.erase (get_file (path));
  }

  virtual void create_file (const path_type& path, fuse_file_info* file_info, mode_t mode)
  {
    //! \todo Does directory exist? Oo

    time_t current_time;
    time (&current_time);

    if ( !SFileCreateFile ( _internal
                          , path.full_path (false, "\\", false).c_str()
                          , time_t_to_filetime (current_time)
                          , 0 //size
                          , 0 //locale
                          , 0 //flags
                          , reinterpret_cast<HANDLE*> (&file_info->fh)
                          )
       )
    {
      throw_error_if_available();
    }

    stat_type stat;
    memset (&stat, 0, sizeof (stat));
    //! \todo Write permission?
    stat.st_mode = S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    stat.st_nlink = 1;
    stat.st_size = 0;
    stat.st_atime = current_time;
    stat.st_mtime = current_time;
    stat.st_ctime = current_time;
    stat.st_birthtime = current_time;

    const std::string dir (path.directory());

    std::cout << "dir: " << dir << ", file: " << path.file() << std::endl;

    _directories[dir]._max_time = std::max ( current_time
                                           , _directories[dir]._max_time
                                           );
    _directories[dir]._min_time = std::min ( current_time
                                           , _directories[dir]._max_time
                                           );

    _directories[dir]._files.push_back (file_entry_type (stat, path.file()));
  }

  virtual void release_file_handle (const std::string& dir, const std::string& file, fuse_file_info* file_info)
  {
    if (!SFileFinishFile (reinterpret_cast<HANDLE*> (&file_info->fh)))
    {
      throw_error_if_available();
    }
    if (!SFileCloseFile (reinterpret_cast<HANDLE*> (&file_info->fh)))
    {
      throw_error_if_available();
    }

    //! \todo Flush MPQ?
  }

  virtual void rename_file ( const std::string& source_directory
                   , const std::string& source_file
                   , const std::string& target_directory
                   , const std::string& target_file
                   )
  {
    std::string source ((source_directory + source_file).substr (1));
    std::string target ((target_directory + target_file).substr (1));

    replace (source, "/", "\\");
    replace (target, "/", "\\");

    if (source == target)
    {
      //! \todo Other errors.
      throw no_such_file();
    }

    if (!SFileRenameFile (_internal, source.c_str(), target.c_str()))
    {
      throw_error_if_available();
    }

    file_list_type::iterator old_file (get_file (source_directory, source_file));

    //! \todo In MPQs, there are no directories, therefore no need for checking if directory exists. Maybe just create?
    directories_type::iterator target_dir_it (_directories.find (target_directory));
    if (target_dir_it == _directories.end())
    {
      throw no_such_file();
    }

    if ( std::find ( target_dir_it->second._files.begin()
                   , target_dir_it->second._files.end()
                   , file_entry_type::dummy (target_file)
                   )
       != target_dir_it->second._files.end()
       )
    {
      //! \todo Not this one, but "already exists.".
      throw no_such_file();
    }

    file_entry_type copy (*old_file);
    copy._filename = target_file;

    target_dir_it->second._files.push_back (copy);

    directories_type::iterator source_dir_it (_directories.find (source_directory));
    if (source_dir_it == _directories.end())
    {
      throw no_such_file();
    }
    source_dir_it->second._files.erase ( std::find ( source_dir_it->second._files.begin()
                                                   , source_dir_it->second._files.end()
                                                   , file_entry_type::dummy (source_file)
                                                   )
                                       );
  }

  static std::string assemble_directory ( const std::string& dir
                                        , const std::string& file
                                        )
  {
    return dir + file + (file == "" ? "" : "/");
  }

private:
  file_list_type::iterator get_file (const path_type& path)
  {
    return get_file (path.directory(), path.file());
  }

  file_list_type::iterator get_file ( const std::string& dir
                                    , const std::string& file
                                    )
  {
    directories_type::iterator dir_it (_directories.find (dir));
    if (dir_it == _directories.end())
    {
      throw no_such_file();
    }

    file_list_type::iterator file_it
      ( std::find ( dir_it->second._files.begin()
                  , dir_it->second._files.end()
                  , file_entry_type::dummy (file)
                  )
      );
    if (file_it == dir_it->second._files.end())
    {
      throw no_such_file();
    }

    return file_it;
  }

  HANDLE _internal;
  std::string _archive_name;
  directories_type _directories;
};

inline fuse_file_system* get_file_system_from_fuse_context()
{
  return static_cast <mpq_fs*> (fuse_get_context()->private_data);
}

int fs_getattr (const char* path, stat_type* stat_buf)
{
  std::string directory;
  std::string file;
  split_into_dir_and_file (path, directory, file);

  try
  {
    *stat_buf = get_file_system_from_fuse_context()->get_attributes (directory, file);
    return 0;
  }
  CATCH_AND_RETURN_ERROR;
}

int fs_readdir ( const char* path
               , void* result_buffer
               , fuse_fill_dir_t filler
               , off_t
               , fuse_file_info*
               )
{
  //! \todo Use offset for faster stuff.

  file_lister_helper helper (result_buffer, filler);
  std::string directory (path);
  if (directory[directory.length() - 1] != '/')
  {
    directory += '/';
  }

  helper.add (".");
  helper.add ("..");

  try
  {
    get_file_system_from_fuse_context()->list_files (directory, helper);
    return 0;
  }
  CATCH_AND_RETURN_ERROR;
}

int fs_open (const char* path, fuse_file_info* file_info)
{
  std::string directory;
  std::string file;
  split_into_dir_and_file (path, directory, file);

  try
  {
    get_file_system_from_fuse_context()->open (directory, file, file_info);
    return 0;
  }
  CATCH_AND_RETURN_ERROR;
}

int fs_read ( const char* path
            , char* result_buffer
            , size_t size
            , off_t offset
            , fuse_file_info* file_info
            )
{
  std::string directory;
  std::string file;
  split_into_dir_and_file (path, directory, file);

  try
  {
    return get_file_system_from_fuse_context()->read (directory, file, file_info, result_buffer, offset, size);
  }
  CATCH_AND_RETURN_ERROR;
}

int fs_mkdir (const char* path, mode_t)
{
  try
  {
    get_file_system_from_fuse_context()->make_directory (path);
    return 0;
  }
  CATCH_AND_RETURN_ERROR;
}

int fs_rmdir (const char* path)
{
  try
  {
    get_file_system_from_fuse_context()->remove_directory (path);
    return 0;
  }
  CATCH_AND_RETURN_ERROR;
}

int fs_create (const char* path, mode_t mode, struct fuse_file_info* file_info)
{
  try
  {
    get_file_system_from_fuse_context()->create_file (path, file_info, mode);
    return 0;
  }
  CATCH_AND_RETURN_ERROR;
}

int fs_release (const char* path, fuse_file_info* file_info)
{
  std::string directory;
  std::string file;
  split_into_dir_and_file (path, directory, file);

  try
  {
    get_file_system_from_fuse_context()->release_file_handle (directory, file, file_info);
    return 0;
  }
  CATCH_AND_RETURN_ERROR;
}

int fs_unlink (const char* path)
{
  try
  {
    get_file_system_from_fuse_context()->remove_file (path);
    return 0;
  }
  CATCH_AND_RETURN_ERROR;
}

/*
int fs_write (const char* path, const char* data, size_t size, off_t offset, fuse_file_info* file_info)
{
  std::string directory;
  std::string file;
  split_into_dir_and_file (path, directory, file);

  try
  {
    return get_file_system_from_fuse_context()->write (directory, file, file_info, data, offset, size);
  }
  CATCH_AND_RETURN_ERROR;
}
*/

int fs_rename (const char* source, const char* target)
{
  std::string source_directory;
  std::string source_file;
  std::string target_directory;
  std::string target_file;
  split_into_dir_and_file (source, source_directory, source_file);
  split_into_dir_and_file (target, target_directory, target_file);

  try
  {
    get_file_system_from_fuse_context()->rename_file (source_directory, source_file, target_directory, target_file);
    return 0;
  }
  CATCH_AND_RETURN_ERROR;
}

void* fs_init (fuse_conn_info*)
{
  return new mpq_fs (static_cast<const char*> (fuse_get_context()->private_data));
}

void fs_destroy (void* private_data)
{
  delete static_cast<mpq_fs*> (private_data);
}

/*
int fs_utimens (const char* path, const timespec tv[2])
{
  return 0;
}

int fs_flush (const char* path, fuse_file_info* file_info)
{
  return 0;
}

int fs_fsync (const char* path, int only_sync_userdata, fuse_file_info* file_info)
{
  return 0;
}

int fs_truncate (const char* path, off_t size)
{
  return 0;
}
*/

static fuse_operations hello_oper =
{
  // metadata
  .getattr  = fs_getattr,
  .readdir  = fs_readdir,

  // file
  .open = fs_open,
  .create = fs_create,
  .release = fs_release,

  .unlink = fs_unlink,

  .read = fs_read,
  //.write = fs_write,
  //.truncate = fs_truncate,

  //.flush = fs_flush,
  //.fsync = fs_fsync,

  //.utimens = fs_utimens,
  .rename = fs_rename,

  // directories
  .mkdir = fs_mkdir,
  .rmdir = fs_rmdir,

  // global
  .init = fs_init,
  .destroy = fs_destroy,
};


int main (int argc, char** argv)
{
  try
  {
    return fuse_main (argc - 1, argv, &hello_oper, argv[argc - 1]);
  }
  catch (const std::exception& exception)
  {
    std::cerr << exception.what() << "\n";
  }
}
