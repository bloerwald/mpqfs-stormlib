#include "storm_error_code_thrower.h"

#include <StormPort.h>

#include "system_error.h"

void throw_error_if_available()
{
  switch (GetLastError())
  {
    case ERROR_FILE_NOT_FOUND:
      throw no_such_file();
    case ERROR_ACCESS_DENIED:
      throw operation_permitted();
    case ERROR_INVALID_HANDLE:
      throw bad_file_descriptor();
    case ERROR_NOT_ENOUGH_MEMORY:
      throw no_memory();
    case ERROR_NOT_SUPPORTED:
      throw not_supported();
    case ERROR_INVALID_PARAMETER:
      throw invalid_argument();
    case ERROR_DISK_FULL:
      throw no_space_on_device();
    case ERROR_ALREADY_EXISTS:
      throw file_exists();
    case ERROR_INSUFFICIENT_BUFFER:
      throw no_buffer_space();

    //! \note These do not have a unix equivalent.
    case ERROR_BAD_FORMAT:
    case ERROR_NO_MORE_FILES:
    case ERROR_HANDLE_EOF:
    case ERROR_CAN_NOT_COMPLETE:
    case ERROR_FILE_CORRUPT:
      throw io_error();

    case ERROR_SUCCESS:
    default:
      return;
  }
}
