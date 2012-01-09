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
    //! \todo close files, then mpq.
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

  void list_files (const std::string& directory, file_lister_helper& list)
  {
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

mpq_fs* global_file_system;

inline void split_into_dir_and_file ( const std::string& in
                                    , std::string& path
                                    , std::string& file
                                    )
{
  const size_t slash_pos (in.find_last_of ('/'));
  path = in.substr (0, slash_pos + 1);
  file = in.substr (slash_pos + 1);
}

int fs_getattr (const char* path, stat_type* stat_buf)
{
  std::string directory;
  std::string file;
  split_into_dir_and_file (path, directory, file);

  try
  {
    *stat_buf = global_file_system->get_attributes (directory, file);
  }
  catch (const system_error& error)
  {
    return -error._error_id;
  }

  return 0;
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
    global_file_system->list_files (directory, helper);
  }
  catch (const system_error& error)
  {
    return -error._error_id;
  }

  return 0;
}

int fs_open (const char* path, fuse_file_info* file_info)
{
  std::string directory;
  std::string file;
  split_into_dir_and_file (path, directory, file);

  try
  {
    global_file_system->open (directory, file, file_info);
  }
  catch (const system_error& error)
  {
    return -error._error_id;
  }

  return 0;
}

int fs_read ( const char* path
            , char* result_buffer
            , size_t size
            , off_t offset
            , struct fuse_file_info* file_info
            )
{
  std::string directory;
  std::string file;
  split_into_dir_and_file (path, directory, file);

  try
  {
    return global_file_system->read (directory, file, file_info, result_buffer, offset, size);
  }
  catch (const system_error& error)
  {
    return -error._error_id;
  }
}

static fuse_operations hello_oper =
{
  .getattr  = fs_getattr,
  .readdir  = fs_readdir,
  .open = fs_open,
  .read = fs_read,
};


int main (int argc, char** argv)
{
  try
  {
    global_file_system = new mpq_fs (argv[argc - 1]);

    return fuse_main(argc - 1, argv, &hello_oper, NULL);
  }
  catch (std::exception& e)
  {
    std::cerr << "EXCEPTION: " << e.what() << std::endl;
  }
}
