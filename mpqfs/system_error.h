// system_error.h is part of mpqfs-stormlib, licensed via GNU General Public License (version 3).
// Bernd Lörwald <bloerwald+mpqfs@googlemail.com>

#ifndef __SYSTEM_ERROR_H
#define __SYSTEM_ERROR_H

#include <stdexcept>

class system_error : public std::runtime_error
{
public:
  system_error (size_t error_id, const std::string& what)
    : std::runtime_error (what)
    , _error_id (error_id)
  { }

  inline const size_t& error_id() const
  {
    return _error_id;
  }

private:
  size_t _error_id;
};

#define ERROR_CLASS(NAME)                                                      \
  class NAME : public system_error                                             \
  { public: NAME(); }

ERROR_CLASS (access_denied);
ERROR_CLASS (temporarily_unavailable);
ERROR_CLASS (bad_file_descriptor);
ERROR_CLASS (bad_message);
ERROR_CLASS (resource_busy);
ERROR_CLASS (canceled);
ERROR_CLASS (deadlock);
ERROR_CLASS (file_exists);
ERROR_CLASS (bad_address);
ERROR_CLASS (file_too_big);
ERROR_CLASS (operation_in_progress);
ERROR_CLASS (invalid_argument);
ERROR_CLASS (io_error);
ERROR_CLASS (is_directory);
ERROR_CLASS (symbolic_link_loop);
ERROR_CLASS (too_many_open_files);
ERROR_CLASS (filename_too_long);
ERROR_CLASS (no_buffer_space);
ERROR_CLASS (no_such_file);
ERROR_CLASS (no_space_on_device);
ERROR_CLASS (function_not_implemented);
ERROR_CLASS (not_a_directory);
ERROR_CLASS (not_empty);
ERROR_CLASS (not_supported);
ERROR_CLASS (operation_permitted);
ERROR_CLASS (file_is_read_only);
ERROR_CLASS (timed_out);
ERROR_CLASS (no_memory);

#undef ERROR_CLASS

#define CATCH_AND_RETURN_ERROR catch (const system_error& error)               \
  { std::cerr << error.what() << "\n"; return -error.error_id(); }

#endif
