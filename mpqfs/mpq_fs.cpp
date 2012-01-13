#include "mpq_fs.h"

#include "system_error.h"
#include "storm_error_code_thrower.h"
#include "platform_dependence_helpers.h"

#include <iostream>

mpq_fs::mpq_fs (const std::string& archive_name)
  : fuse::file_system()
  , _archive_name (archive_name)
{
  if (!SFileOpenArchive (archive_name.c_str(), 0, 0, &_internal))
  {
    throw_error_if_available();
    throw invalid_argument();
  }

  //! \note Dummy.
  _directories[path_type()]._min_time = 0;

  SFILE_FIND_DATA file_data;
  for ( HANDLE find_handle ( SFileFindFirstFile ( _internal
                                                , "*"
                                                , &file_data
                                                , NULL
                                                )
                           )
      ; GetLastError() != ERROR_NO_MORE_FILES
      ; SFileFindNextFile (find_handle, &file_data)
      )
  {
    if ( strcmp (file_data.cFileName, LISTFILE_NAME) == 0
      || strcmp (file_data.cFileName, SIGNATURE_NAME) == 0
      || strcmp (file_data.cFileName, ATTRIBUTES_NAME) == 0
       )
    {
      continue;
    }

    fuse::stat_type stat;
    memset (&stat, 0, sizeof (stat));
    //! \todo Write permission?
    stat.st_mode = S_IFREG
                 | S_IRUSR | S_IWUSR
                 | S_IRGRP | S_IWGRP
                 | S_IROTH | S_IWOTH;
    stat.st_nlink = 1;
    stat.st_size = file_data.dwFileSize;
    std::cout << "path: " << file_data.cFileName << ": " << stat.st_size << std::endl;

    using platform_dependence_helpers::filetime_to_time_t;
    const time_t unix_time ( filetime_to_time_t ( file_data.dwFileTimeLo
                                                , file_data.dwFileTimeHi
                                                )
                           );

    //! \todo access and modification dynamic?
    stat.st_atime = stat.st_mtime = stat.st_ctime
                  = stat.st_birthtime = unix_time;

    const path_type path (file_data.cFileName);

    directory_entry& directory (_directories[path.parent()]);
    directory._max_time = std::max (unix_time, directory._max_time);
    directory._min_time = std::min (unix_time, directory._min_time);
    directory._files[path.file()] = stat;
  }

  //! \note insert dummy directories for directories without
  // files and only sub directories.
  for ( directories_type::iterator it (_directories.begin())
      ; it != _directories.end()
      ; ++it
      )
  {
    for (path_type path (it->first); !path.is_root(); path = path.parent())
    {
      if (_directories.find (path) == _directories.end())
      {
        _directories[path]._min_time = 0;
      }
    }
  }
}

mpq_fs::~mpq_fs()
{
  //! \todo close files? Are all file handles closed earlier?
  SFileCloseArchive (_internal);
}

fuse::availability_flags mpq_fs::availability() const
{
  fuse::availability_flags flags (fuse::file_system::availability());

  flags.getattr = 1;
  //flags.statfs = 1;
  //flags.utimens = 1;
  flags.rename = 1;
  flags.mkdir = 1;
  flags.rmdir = 1;
  flags.readdir = 1;
  flags.create = 1;
  flags.unlink = 1;
  flags.open = 1;
  flags.read = 1;
  //flags.write = 1;
  flags.release = 1;
  flags.flush = 1;
  flags.truncate = 1;
  //?flags.access = 1;

  return flags;
}

fuse::stat_type mpq_fs::get_attributes (const path_type& path)
{
  directories_type::iterator dir_it (_directories.find (path));
  if (dir_it != _directories.end())
  {
    fuse::stat_type directory;
    memset (&directory, 0, sizeof (directory));
    directory.st_mode = S_IFDIR
                      | S_IRUSR | S_IWUSR | S_IXUSR
                      | S_IRGRP | S_IWGRP | S_IXGRP
                      | S_IROTH | S_IWOTH | S_IXOTH;
    directory.st_nlink = 3;
    directory.st_atime = dir_it->second._max_time;
    directory.st_mtime = dir_it->second._max_time;
    directory.st_ctime = dir_it->second._min_time;
    directory.st_birthtime = dir_it->second._min_time;
    return directory;
  }

  return get_file (path)->second;
}

void mpq_fs::open (const path_type& path, fuse::file_info* file_info)
{
  file_list_type::iterator file_it (get_file (path));

  if ( !SFileOpenFileEx ( _internal
                        , path.full_path ("\\", false).c_str()
                        , SFILE_OPEN_FROM_MPQ
                        , reinterpret_cast<HANDLE*> (&file_info->fh)
                        )
     )
  {
    throw no_such_file();
  }
}

size_t mpq_fs::read ( const path_type& path
                    , fuse::file_info* file_info
                    , char* result_buffer
                    , off_t offset
                    , size_t size
                    )
{
  file_list_type::iterator file_it (get_file (path));

  if (offset < file_it->second.st_size)
  {
    if (offset + size > file_it->second.st_size)
    {
      size = file_it->second.st_size - offset;
    }

    //! \todo not null but size_hi?
    SFileSetFilePointer ( reinterpret_cast<HANDLE> (file_info->fh)
                        , offset
                        , NULL
                        , FILE_BEGIN
                        );

    DWORD read_bytes (size);
    SFileReadFile ( reinterpret_cast<HANDLE> (file_info->fh)
                  , result_buffer
                  , size
                  , &read_bytes
                  );

    return read_bytes;
  }

  throw access_denied();
}

/*
size_t mpq_fs::write ( const path_type& path
                     , fuse::file_info* file_info
                     , const char* data
                     , off_t offset
                     , size_t size
                     )
{
  std::cout << "write (" << dir << ", " << file << ", file_info, " << data << ", " << offset << ");\n";

  file_list_type::iterator file_it (get_file (dir, file));

  if (offset <= file_it->second.st_size)
  {
    std::cout << "offset < statsize\n";
    if (offset + size > file_it->second.st_size)
    {
      file_it->second.st_size = size + offset;
      std::cout << file_it->second.st_size << " = " << size<<" + "<<offset<<"\n";
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
    std::cout << "offset > statsize ( " << file_it->second.st_size << ")\n";
    throw access_denied();
  }

  return size;
}
*/

void mpq_fs::list_files ( const path_type& path
                        , fuse::file_lister_helper& list
                        )
{
  directories_type::iterator dir (_directories.find (path));
  if (dir == _directories.end())
  {
    throw no_such_file();
  }

  for ( directories_type::iterator it (_directories.begin())
      ; it != _directories.end()
      ; ++it
      )
  {
    const path_type parent (it->first.parent());
    if (it != dir && it->first.parent() == dir->first)
    {
      list.add (it->first.file());
    }
  }

  for ( file_list_type::const_iterator it (dir->second._files.begin())
      ; it != dir->second._files.end()
      ; ++it
      )
  {
    list.add (it->first);
  }
}

void mpq_fs::make_directory (const path_type& path, mode_t mode)
{
  if (_directories.find (path) != _directories.end())
  {
    throw file_exists();
  }

  _directories[path]._min_time = 0;
  time (&_directories[path]._min_time);
  _directories[path]._max_time = _directories[path]._min_time;
}

void mpq_fs::remove_directory (const path_type& path)
{
  directories_type::iterator dir_it (_directories.find (path));
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
    //! \todo path/sub/sub2?
    if (it->first.parent() == path)
    {
      throw not_empty();
    }
  }

  _directories.erase (dir_it);
}

void mpq_fs::remove_file (const path_type& path)
{
  //! \todo Do all files get closed before this is called?

  if ( !SFileRemoveFile ( _internal
                        , path.full_path ("\\", false).c_str()
                        )
     )
  {
    throw_error_if_available();
  }

  _directories[path.parent()]._files.erase (get_file (path));
}

void mpq_fs::create_file ( const path_type& path
                         , fuse::file_info* file_info
                         , mode_t mode)
{
  //! \todo Does directory exist? Oo

  time_t current_time;
  time (&current_time);

  using platform_dependence_helpers::time_t_to_filetime;

  if ( !SFileCreateFile ( _internal
                        , path.full_path ("\\", false).c_str()
                        , time_t_to_filetime (current_time)
                        //! \todo Already have some buffer size? One block?
                        , 0 //size
                        , 0 //locale
                        , 0 //flags
                        , reinterpret_cast<HANDLE*> (&file_info->fh)
                        )
     )
  {
    throw_error_if_available();
  }

  fuse::stat_type stat;
  memset (&stat, 0, sizeof (stat));
  //! \todo Write permission?
  stat.st_mode = S_IFREG
               | S_IRUSR | S_IWUSR
               | S_IRGRP | S_IWGRP
               | S_IROTH | S_IWOTH;
  stat.st_nlink = 1;
  stat.st_size = 0;
  stat.st_atime = stat.st_mtime = stat.st_ctime
                = stat.st_birthtime = current_time;

  directory_entry& directory (_directories[path.parent()]);
  directory._max_time = std::max (current_time, directory._max_time);
  directory._min_time = std::min (current_time, directory._max_time);
  directory._files[path.file()] = stat;
}

void mpq_fs::release_file_handle (const path_type&, fuse::file_info* info)
{
  if (!SFileFinishFile (reinterpret_cast<HANDLE*> (&info->fh)))
  {
    throw_error_if_available();
  }
  if (!SFileCloseFile (reinterpret_cast<HANDLE*> (&info->fh)))
  {
    throw_error_if_available();
  }

  //! \todo Flush MPQ?
}

void mpq_fs::rename_file (const path_type& source, const path_type& target)
{
  if (source == target)
  {
    //! \todo Other errors.
    throw no_such_file();
  }

  if (_directories.find (source) != _directories.end())
  {
    //! \todo Implement recursive moving of directories.
    throw operation_permitted();
  }

  directories_type::iterator source_dir (_directories.find (source.parent()));
  if (source_dir == _directories.end())
  {
    throw no_such_file();
  }

  file_list_type::iterator source_file (get_file (source));

  //! \todo In MPQs, there are no directories, therefore no need for
  // checking if directory exists. Maybe just create?
  directories_type::iterator target_dir (_directories.find (target.parent()));
  if (target_dir == _directories.end())
  {
    throw no_such_file();
  }

  if ( target_dir->second._files.find (target.file())
         != target_dir->second._files.end()
     )
  {
    throw file_exists();
  }

  if ( !SFileRenameFile ( _internal
                        , source.full_path ("\\", false).c_str()
                        , target.full_path ("\\", false).c_str()
                        )
     )
  {
    throw_error_if_available();
  }

  target_dir->second._files[target.file()] = source_file->second;
  source_dir->second._files.erase (source_file);
}

mpq_fs::file_list_type::iterator mpq_fs::get_file (const path_type& path)
{
  directories_type::iterator dir_it (_directories.find (path.parent()));
  if (dir_it == _directories.end())
  {
    throw no_such_file();
  }

  file_list_type::iterator file_it (dir_it->second._files.find (path.file()));
  if (file_it == dir_it->second._files.end())
  {
    throw no_such_file();
  }

  return file_it;
}
