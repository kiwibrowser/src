// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/test/test_utils.h"

#include <fcntl.h>
#include <stddef.h>
#include <unistd.h>

#include "base/posix/eintr_wrapper.h"

namespace mojo {
namespace edk {
namespace test {

bool BlockingWrite(const InternalPlatformHandle& handle,
                   const void* buffer,
                   size_t bytes_to_write,
                   size_t* bytes_written) {
  int original_flags = fcntl(handle.handle, F_GETFL);
  if (original_flags == -1 ||
      fcntl(handle.handle, F_SETFL, original_flags & (~O_NONBLOCK)) != 0) {
    return false;
  }

  ssize_t result = HANDLE_EINTR(write(handle.handle, buffer, bytes_to_write));

  fcntl(handle.handle, F_SETFL, original_flags);

  if (result < 0)
    return false;

  *bytes_written = result;
  return true;
}

bool BlockingRead(const InternalPlatformHandle& handle,
                  void* buffer,
                  size_t buffer_size,
                  size_t* bytes_read) {
  int original_flags = fcntl(handle.handle, F_GETFL);
  if (original_flags == -1 ||
      fcntl(handle.handle, F_SETFL, original_flags & (~O_NONBLOCK)) != 0) {
    return false;
  }

  ssize_t result = HANDLE_EINTR(read(handle.handle, buffer, buffer_size));

  fcntl(handle.handle, F_SETFL, original_flags);

  if (result < 0)
    return false;

  *bytes_read = result;
  return true;
}

bool NonBlockingRead(const InternalPlatformHandle& handle,
                     void* buffer,
                     size_t buffer_size,
                     size_t* bytes_read) {
  ssize_t result = HANDLE_EINTR(read(handle.handle, buffer, buffer_size));

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
  return ScopedInternalPlatformHandle(InternalPlatformHandle(rv));
}

base::ScopedFILE FILEFromInternalPlatformHandle(ScopedInternalPlatformHandle h,
                                                const char* mode) {
  CHECK(h.is_valid());
  base::ScopedFILE rv(fdopen(h.release().handle, mode));
  PCHECK(rv) << "fdopen";
  return rv;
}

}  // namespace test
}  // namespace edk
}  // namespace mojo
