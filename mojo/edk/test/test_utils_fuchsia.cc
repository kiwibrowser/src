// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/test/test_utils.h"

#include <fcntl.h>
#include <stddef.h>
#include <unistd.h>

#include "base/files/scoped_file.h"
#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"

namespace mojo {
namespace edk {
namespace test {

// TODO(fuchsia): Merge Fuchsia's InternalPlatformHandle with the POSIX one and
// use the POSIX-generic versions of these. See crbug.com/754029.

bool BlockingWrite(const InternalPlatformHandle& handle,
                   const void* buffer,
                   size_t bytes_to_write,
                   size_t* bytes_written) {
  int original_flags = fcntl(handle.as_fd(), F_GETFL);
  if (original_flags == -1 ||
      fcntl(handle.as_fd(), F_SETFL, original_flags & (~O_NONBLOCK)) != 0) {
    return false;
  }

  ssize_t result = HANDLE_EINTR(write(handle.as_fd(), buffer, bytes_to_write));

  fcntl(handle.as_fd(), F_SETFL, original_flags);

  if (result < 0)
    return false;

  *bytes_written = result;
  return true;
}

bool BlockingRead(const InternalPlatformHandle& handle,
                  void* buffer,
                  size_t buffer_size,
                  size_t* bytes_read) {
  int original_flags = fcntl(handle.as_fd(), F_GETFL);
  if (original_flags == -1 ||
      fcntl(handle.as_fd(), F_SETFL, original_flags & (~O_NONBLOCK)) != 0) {
    return false;
  }

  ssize_t result = HANDLE_EINTR(read(handle.as_fd(), buffer, buffer_size));

  fcntl(handle.as_fd(), F_SETFL, original_flags);

  if (result < 0)
    return false;

  *bytes_read = result;
  return true;
}

bool NonBlockingRead(const InternalPlatformHandle& handle,
                     void* buffer,
                     size_t buffer_size,
                     size_t* bytes_read) {
  ssize_t result = HANDLE_EINTR(read(handle.as_fd(), buffer, buffer_size));

  if (result < 0) {
    if (errno != EAGAIN && errno != EWOULDBLOCK)
      return false;

    *bytes_read = 0;
  } else {
    *bytes_read = result;
  }

  return true;
}

ScopedInternalPlatformHandle InternalPlatformHandleFromFILE(
    base::ScopedFILE fp) {
  CHECK(fp);
  int rv = HANDLE_EINTR(dup(fileno(fp.get())));
  PCHECK(rv != -1) << "dup";
  return ScopedInternalPlatformHandle(InternalPlatformHandle::ForFd(rv));
}

base::ScopedFILE FILEFromInternalPlatformHandle(ScopedInternalPlatformHandle h,
                                                const char* mode) {
  CHECK(h.get().is_valid_fd());
  base::ScopedFILE rv(fdopen(h.release().as_fd(), mode));
  PCHECK(rv) << "fdopen";
  return rv;
}

}  // namespace test
}  // namespace edk
}  // namespace mojo
