#include "system_error.h"

#include <errno.h>

#define ERROR_CLASS_IMPL(NAME, ID) NAME::NAME()                                \
  : system_error (ID, #NAME " (" #ID ")") { }

ERROR_CLASS_IMPL (access_denied, EACCES);
ERROR_CLASS_IMPL (temporarily_unavailable, EAGAIN);
ERROR_CLASS_IMPL (bad_file_descriptor, EBADF);
ERROR_CLASS_IMPL (bad_message, EBADMSG);
ERROR_CLASS_IMPL (resource_busy, EBUSY);
ERROR_CLASS_IMPL (canceled, ECANCELED);
ERROR_CLASS_IMPL (deadlock, EDEADLK);
ERROR_CLASS_IMPL (file_exists, EEXIST);
ERROR_CLASS_IMPL (bad_address, EFAULT);
ERROR_CLASS_IMPL (file_too_big, EFBIG);
ERROR_CLASS_IMPL (operation_in_progress, EINPROGRESS);
ERROR_CLASS_IMPL (invalid_argument, EINVAL);
ERROR_CLASS_IMPL (io_error, EIO);
ERROR_CLASS_IMPL (is_directory, EISDIR);
ERROR_CLASS_IMPL (symbolic_link_loop, ELOOP);
ERROR_CLASS_IMPL (too_many_open_files, EMFILE);
ERROR_CLASS_IMPL (filename_too_long, ENAMETOOLONG);
ERROR_CLASS_IMPL (no_buffer_space, ENOBUFS);
ERROR_CLASS_IMPL (no_such_file, ENOENT);
ERROR_CLASS_IMPL (no_space_on_device, ENOSPC);
ERROR_CLASS_IMPL (function_not_implemented, ENOSYS);
ERROR_CLASS_IMPL (not_a_directory, ENOTDIR);
ERROR_CLASS_IMPL (not_empty, ENOTEMPTY);
ERROR_CLASS_IMPL (not_supported, ENOTSUP);
ERROR_CLASS_IMPL (operation_permitted, EPERM);
ERROR_CLASS_IMPL (file_is_read_only, EROFS);
ERROR_CLASS_IMPL (timed_out, ETIMEDOUT);
ERROR_CLASS_IMPL (no_memory, ENOMEM);

#undef ERROR_CLASS_IMPL
