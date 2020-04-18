// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/embedder/named_platform_handle_utils.h"

#include <sddl.h>
#include <windows.h>

#include <memory>

#include "base/logging.h"
#include "base/win/windows_version.h"
#include "mojo/edk/embedder/named_platform_handle.h"

namespace mojo {
namespace edk {
namespace {

// A DACL to grant:
// GA = Generic All
// access to:
// SY = LOCAL_SYSTEM
// BA = BUILTIN_ADMINISTRATORS
// OW = OWNER_RIGHTS
constexpr base::char16 kDefaultSecurityDescriptor[] =
    L"D:(A;;GA;;;SY)(A;;GA;;;BA)(A;;GA;;;OW)";

}  // namespace

ScopedInternalPlatformHandle CreateClientHandle(
    const NamedPlatformHandle& named_handle) {
  if (!named_handle.is_valid())
    return ScopedInternalPlatformHandle();

  base::string16 pipe_name = named_handle.pipe_name();

  // Note: This may block.
  if (!WaitNamedPipeW(pipe_name.c_str(), NMPWAIT_USE_DEFAULT_WAIT))
    return ScopedInternalPlatformHandle();

  const DWORD kDesiredAccess = GENERIC_READ | GENERIC_WRITE;
  // The SECURITY_ANONYMOUS flag means that the server side cannot impersonate
  // the client.
  const DWORD kFlags =
      SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS | FILE_FLAG_OVERLAPPED;
  ScopedInternalPlatformHandle handle(
      InternalPlatformHandle(CreateFileW(pipe_name.c_str(), kDesiredAccess,
                                         0,  // No sharing.
                                         nullptr, OPEN_EXISTING, kFlags,
                                         nullptr)));  // No template file.
  // The server may have stopped accepting a connection between the
  // WaitNamedPipe() and CreateFile(). If this occurs, an invalid handle is
  // returned.
  DPLOG_IF(ERROR, !handle.is_valid())
      << "Named pipe " << named_handle.pipe_name()
      << " could not be opened after WaitNamedPipe succeeded";
  return handle;
}

ScopedInternalPlatformHandle CreateServerHandle(
    const NamedPlatformHandle& named_handle,
    const CreateServerHandleOptions& options) {
  if (!named_handle.is_valid())
    return ScopedInternalPlatformHandle();

  PSECURITY_DESCRIPTOR security_desc = nullptr;
  ULONG security_desc_len = 0;
  PCHECK(ConvertStringSecurityDescriptorToSecurityDescriptor(
      options.security_descriptor.empty() ? kDefaultSecurityDescriptor
                                          : options.security_descriptor.c_str(),
      SDDL_REVISION_1, &security_desc, &security_desc_len));
  std::unique_ptr<void, decltype(::LocalFree)*> p(security_desc, ::LocalFree);
  SECURITY_ATTRIBUTES security_attributes = {sizeof(SECURITY_ATTRIBUTES),
                                             security_desc, FALSE};

  const DWORD kOpenMode = options.enforce_uniqueness
                              ? PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED |
                                    FILE_FLAG_FIRST_PIPE_INSTANCE
                              : PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED;
  const DWORD kPipeMode =
      PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_REJECT_REMOTE_CLIENTS;
  InternalPlatformHandle handle(
      CreateNamedPipeW(named_handle.pipe_name().c_str(), kOpenMode, kPipeMode,
                       options.enforce_uniqueness ? 1 : 255,  // Max instances.
                       4096,  // Out buffer size.
                       4096,  // In buffer size.
                       5000,  // Timeout in milliseconds.
                       &security_attributes));
  handle.needs_connection = true;
  return ScopedInternalPlatformHandle(handle);
}

}  // namespace edk
}  // namespace mojo
