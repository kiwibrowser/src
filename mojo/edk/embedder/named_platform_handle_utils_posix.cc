// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/embedder/named_platform_handle_utils.h"

#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"
#include "mojo/edk/embedder/named_platform_handle.h"

namespace mojo {
namespace edk {
namespace {

// This function fills in |unix_addr| with the appropriate data for the socket,
// and sets |unix_addr_len| to the length of the data therein.
// Returns true on success, or false on failure (typically because |handle.name|
// violated the naming rules).
bool MakeUnixAddr(const NamedPlatformHandle& handle,
                  struct sockaddr_un* unix_addr,
                  size_t* unix_addr_len) {
  DCHECK(unix_addr);
  DCHECK(unix_addr_len);
  DCHECK(handle.is_valid());

  // We reject handle.name.length() == kMaxSocketNameLength to make room for the
  // NUL terminator at the end of the string.
  if (handle.name.length() >= kMaxSocketNameLength) {
    LOG(ERROR) << "Socket name too long: " << handle.name;
    return false;
  }

  // Create unix_addr structure.
  memset(unix_addr, 0, sizeof(struct sockaddr_un));
  unix_addr->sun_family = AF_UNIX;
  strncpy(unix_addr->sun_path, handle.name.c_str(), kMaxSocketNameLength);
  *unix_addr_len =
      offsetof(struct sockaddr_un, sun_path) + handle.name.length();
  return true;
}

// This function creates a unix domain socket, and set it as non-blocking.
// If successful, this returns a ScopedInternalPlatformHandle containing the
// socket. Otherwise, this returns an invalid ScopedInternalPlatformHandle.
ScopedInternalPlatformHandle CreateUnixDomainSocket(bool needs_connection) {
  // Create the unix domain socket.
  InternalPlatformHandle socket_handle(socket(AF_UNIX, SOCK_STREAM, 0));
  socket_handle.needs_connection = needs_connection;
  ScopedInternalPlatformHandle handle(socket_handle);
  if (!handle.is_valid()) {
    PLOG(ERROR) << "Failed to create AF_UNIX socket.";
    return ScopedInternalPlatformHandle();
  }

  // Now set it as non-blocking.
  if (!base::SetNonBlocking(handle.get().handle)) {
    PLOG(ERROR) << "base::SetNonBlocking() failed " << handle.get().handle;
    return ScopedInternalPlatformHandle();
  }
  return handle;
}

}  // namespace

ScopedInternalPlatformHandle CreateClientHandle(
    const NamedPlatformHandle& named_handle) {
  if (!named_handle.is_valid())
    return ScopedInternalPlatformHandle();

  struct sockaddr_un unix_addr;
  size_t unix_addr_len;
  if (!MakeUnixAddr(named_handle, &unix_addr, &unix_addr_len))
    return ScopedInternalPlatformHandle();

  ScopedInternalPlatformHandle handle = CreateUnixDomainSocket(false);
  if (!handle.is_valid())
    return ScopedInternalPlatformHandle();

  if (HANDLE_EINTR(connect(handle.get().handle,
                           reinterpret_cast<sockaddr*>(&unix_addr),
                           unix_addr_len)) < 0) {
    PLOG(ERROR) << "connect " << named_handle.name;
    return ScopedInternalPlatformHandle();
  }

  return handle;
}

ScopedInternalPlatformHandle CreateServerHandle(
    const NamedPlatformHandle& named_handle,
    const CreateServerHandleOptions& options) {
  if (!named_handle.is_valid())
    return ScopedInternalPlatformHandle();

  // Make sure the path we need exists.
  base::FilePath socket_dir = base::FilePath(named_handle.name).DirName();
  if (!base::CreateDirectory(socket_dir)) {
    LOG(ERROR) << "Couldn't create directory: " << socket_dir.value();
    return ScopedInternalPlatformHandle();
  }

  // Delete any old FS instances.
  if (unlink(named_handle.name.c_str()) < 0 && errno != ENOENT) {
    PLOG(ERROR) << "unlink " << named_handle.name;
    return ScopedInternalPlatformHandle();
  }

  struct sockaddr_un unix_addr;
  size_t unix_addr_len;
  if (!MakeUnixAddr(named_handle, &unix_addr, &unix_addr_len))
    return ScopedInternalPlatformHandle();

  ScopedInternalPlatformHandle handle = CreateUnixDomainSocket(true);
  if (!handle.is_valid())
    return ScopedInternalPlatformHandle();

  // Bind the socket.
  if (bind(handle.get().handle, reinterpret_cast<const sockaddr*>(&unix_addr),
           unix_addr_len) < 0) {
    PLOG(ERROR) << "bind " << named_handle.name;
    return ScopedInternalPlatformHandle();
  }

  // Start listening on the socket.
  if (listen(handle.get().handle, SOMAXCONN) < 0) {
    PLOG(ERROR) << "listen " << named_handle.name;
    unlink(named_handle.name.c_str());
    return ScopedInternalPlatformHandle();
  }
  return handle;
}

}  // namespace edk
}  // namespace mojo
