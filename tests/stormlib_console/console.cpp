#include <StormLib.h>

#include <iostream>
#include <string>
#include <stdexcept>

namespace MPQ
{
  class archive
  {
  private:
    HANDLE _internal;
    std::string _archive_name;

  public:
    archive (const std::string& archive_name)
      : _archive_name (archive_name)
    {
      if (!SFileOpenArchive (archive_name.c_str(), 0, 0, &_internal))
      {
        throw std::runtime_error ("opening archive \"" + archive_name + "\" failed.");
      }
    }

    void list_files() const
    {
      SFILE_FIND_DATA file_data;
      HANDLE find_handle (SFileFindFirstFile (_internal, "*", &file_data, NULL));
      if (!find_handle)
      {
        throw std::runtime_error ("there are no files or some error happened. idk");
      }
      do
      {
        std::cout << file_data.cFileName << " ||| " << file_data.dwFileTimeLo << ", " << file_data.dwFileTimeLo << std::endl;
      }
      while (SFileFindNextFile (find_handle, &file_data));
    }
  };
};

int main(int argc, char** argv)
{
  if (argc != 2)
  {
    std::cout << argv[0] << " <mpqfile>\n";
  }

  try
  {
    MPQ::archive archive (argv[1]);

    archive.list_files();
  }
  catch (std::exception& e)
  {
    std::cerr << "EXCEPTION: " << e.what() << std::endl;
  }
  return 0;
}
