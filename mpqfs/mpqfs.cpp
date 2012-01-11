#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include <StormLib.h>

#include <vector>
#include <iostream>
#include <string>
#include <stdexcept>
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

time_t filetime_to_time_t (long long int lo, long long int hi)
{
  return time_t ((hi << 32 | lo) / 10000000LL - 11644473600LL);
}

//! \todo file already exists und so

class system_error : public std::runtime_error
{
public:
  system_error (size_t error_id, const std::string& what)
    : std::runtime_error (what)
    , _error_id (error_id)
  { }

  size_t _error_id;
};

#define ERROR_CLASS(NAME, ID) \
  class NAME : public system_error \
  { public: NAME() : system_error (ID, #NAME "(" #ID ")") { } };

ERROR_CLASS (no_such_file, ENOENT);
ERROR_CLASS (access_denied, EACCES);
ERROR_CLASS (not_empty, ENOTEMPTY);

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

class mpq_fs
{
public:
  mpq_fs (const std::string& archive_name)
    : _archive_name (archive_name)
  {
    if (!SFileOpenArchive (archive_name.c_str(), 0, 0, &_internal))
    {
      throw std::runtime_error ("opening archive \"" + archive_name + "\" failed.");
    }

    //! \note Dummy.
    _directories["/"]._min_time = 0;

    create_file_lists();
  }

  ~mpq_fs()
  {
    //! \todo close files? Are all file handles closed earlier?
    SFileCloseArchive (_internal);
  }

  void create_file_lists()
  {
    SFILE_FIND_DATA file_data;
    HANDLE find_handle (SFileFindFirstFile (_internal, "*", &file_data, NULL));
    if (!find_handle)
    {
      throw std::runtime_error ("there are no files or some error happened. idk");
    }
    do
    {
      std::string filename (file_data.cFileName);
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

      time_t unix_time ( filetime_to_time_t ( file_data.dwFileTimeLo
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
    while (SFileFindNextFile (find_handle, &file_data));

    SFileFindClose (find_handle);

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

  stat_type get_attributes (const std::string& dir, const std::string& file)
  {
    std::cout << "get_attributes (" << dir << ", " << file << ");\n";
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

  void open (const std::string& dir, const std::string& file, fuse_file_info* file_info)
  {
    std::cout << "open (" << dir << ", " << file << ", file_info);\n";

    file_list_type::iterator file_it (get_file (dir, file));

    std::string path ((dir + file).substr (1));
    replace (path, "/", "\\");

    if (!SFileOpenFileEx (_internal, path.c_str(), SFILE_OPEN_FROM_MPQ, reinterpret_cast<HANDLE*> (&file_info->fh)))
    {
      throw no_such_file();
    }
  }

  size_t read ( const std::string& dir
              , const std::string& file
              , fuse_file_info* file_info
              , char* result_buffer
              , off_t offset
              , size_t size
              )
  {
    std::cout << "read (" << dir << ", " << file << ", file_info, result_buffer, " << offset << ", " << size << ");\n";

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
  size_t write ( const std::string& dir
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

  void list_files (const std::string& directory, file_lister_helper& list)
  {
    std::cout << "list_files (" << directory << ", list);\n";

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

  void make_directory (std::string directory)
  {
    directory += "/";
    std::cout << "make_directory (" << directory << ");\n";
    _directories[directory]._min_time = 0;
    time (&_directories[directory]._min_time);
    _directories[directory]._max_time = _directories[directory]._min_time;
  }

  void remove_directory (std::string directory)
  {
    directory += "/";
    std::cout << "remove_directory (" << directory << ");\n";

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

  void remove_file (const std::string& directory, const std::string& file)
  {
    //! \todo Do all files get closed before this is called?

    std::string path ((directory + file).substr (1));
    replace (path, "/", "\\");

    if (!SFileRemoveFile (_internal, path.c_str(), SFILE_OPEN_FROM_MPQ))
    {
      //! \todo correct error
      throw no_such_file();
    }

    _directories[directory]._files.erase (get_file (directory, file));
  }

  void create_file (const std::string& dir, const std::string& file, fuse_file_info* file_info, mode_t mode)
  {
    //! \todo DOes directory exist? Oo
    std::cout << "create_file (" << dir << ", " << file << ", file_info, mode);\n";

    time_t current_time;
    time (&current_time);

    //! \todo reverse!
    //time_t unix_time ( filetime_to_time_t ( file_data.dwFileTimeLo
    //                                     , file_data.dwFileTimeHi
     //                                     )
     //                );

    std::string path ((dir + file).substr (1));
    replace (path, "/", "\\");

    if ( !SFileCreateFile ( _internal
                          , path.c_str()
                          , 0 //! \todo time
                          , 0 //size
                          , 0 //locale
                          , 0 //flags
                          , reinterpret_cast<HANDLE*> (&file_info->fh)
                          )
       )
    {
      //! \todo anderer fehler.
      throw no_such_file();
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

    _directories[dir]._max_time = std::max ( current_time
                                           , _directories[dir]._max_time
                                           );
    _directories[dir]._min_time = std::min ( current_time
                                           , _directories[dir]._max_time
                                           );

    _directories[dir]._files.push_back (file_entry_type (stat, file));

//bool   WINAPI SFileWriteFile(HANDLE hFile, const void * pvData, DWORD dwSize, DWORD dwCompression);
  }

  void release_file_handle (const std::string& dir, const std::string& file, fuse_file_info* file_info)
  {
    std::cout << "release_file_handle (" << dir << ", " << file << ", file_info);\n";

    if (!SFileFinishFile (reinterpret_cast<HANDLE*> (&file_info->fh)))
    {
      //! \todo anderer fehler.
      throw no_such_file();
    }
    if (!SFileCloseFile (reinterpret_cast<HANDLE*> (&file_info->fh)))
    {
      //! \todo anderer fehler.
      throw no_such_file();
    }

    //! \todo Flush MPQ?
  }

  void rename_file ( const std::string& source_directory
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
      //! \todo Other errors.
      throw no_such_file();
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

mpq_fs* get_file_system_from_fuse_context();

inline void split_into_dir_and_file ( const std::string& in
                                    , std::string& path
                                    , std::string& file
                                    )
{
  const size_t slash_pos (in.find_last_of ('/'));
  path = in.substr (0, slash_pos + 1);
  file = in.substr (slash_pos + 1);
}

inline mpq_fs* get_file_system_from_fuse_context()
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
  catch (const system_error& error)
  {
    return -error._error_id;
  }
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
  catch (const system_error& error)
  {
    return -error._error_id;
  }
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
  catch (const system_error& error)
  {
    return -error._error_id;
  }
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
  catch (const system_error& error)
  {
    return -error._error_id;
  }
}

int fs_mkdir (const char* path, mode_t)
{
  try
  {
    get_file_system_from_fuse_context()->make_directory (path);
    return 0;
  }
  catch (const system_error& error)
  {
    return -error._error_id;
  }
}

int fs_rmdir (const char* path)
{
  try
  {
    get_file_system_from_fuse_context()->remove_directory (path);
    return 0;
  }
  catch (const system_error& error)
  {
    return -error._error_id;
  }
}

int fs_create (const char* path, mode_t mode, struct fuse_file_info* file_info)
{
  std::string directory;
  std::string file;
  split_into_dir_and_file (path, directory, file);

  try
  {
    get_file_system_from_fuse_context()->create_file (directory, file, file_info, mode);
    return 0;
  }
  catch (const system_error& error)
  {
    return -error._error_id;
  }
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
  catch (const system_error& error)
  {
    return -error._error_id;
  }
}

int fs_unlink (const char* path)
{
  std::string directory;
  std::string file;
  split_into_dir_and_file (path, directory, file);

  try
  {
    get_file_system_from_fuse_context()->remove_file (directory, file);
    return 0;
  }
  catch (const system_error& error)
  {
    return -error._error_id;
  }
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
  catch (const system_error& error)
  {
    return -error._error_id;
  }
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
  catch (const system_error& error)
  {
    return -error._error_id;
  }
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
    return fuse_main(argc - 1, argv, &hello_oper, argv[argc - 1]);
  }
  catch (std::exception& e)
  {
    std::cerr << "EXCEPTION: " << e.what() << std::endl;
  }
}
