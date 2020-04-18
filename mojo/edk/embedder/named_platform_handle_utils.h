// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_EMBEDDER_NAMED_PLATFORM_HANDLE_UTILS_H_
#define MOJO_EDK_EMBEDDER_NAMED_PLATFORM_HANDLE_UTILS_H_

#include "build/build_config.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"
#include "mojo/edk/system/system_impl_export.h"

#if defined(OS_WIN)
#include "base/strings/string16.h"
#endif

namespace mojo {
namespace edk {

struct NamedPlatformHandle;

#if defined(OS_POSIX)

// The maximum length of the name of a unix domain socket. The standard size on
// linux is 108, mac is 104. To maintain consistency across platforms we
// standardize on the smaller value.
const size_t kMaxSocketNameLength = 104;

#endif

struct CreateServerHandleOptions {
#if defined(OS_WIN)
  // If true, creating a server handle will fail if another pipe with the same
  // name exists.
  bool enforce_uniqueness = true;

  // If non-empty, a security descriptor to use when creating the pipe. If
  // empty, a default security descriptor will be used. See
  // kDefaultSecurityDescriptor in named_platform_handle_utils_win.cc.
  base::string16 security_descriptor;
#endif
};

// Creates a client platform handle from |handle|. This may block until |handle|
// is ready to receive connections.
MOJO_SYSTEM_IMPL_EXPORT ScopedInternalPlatformHandle
CreateClientHandle(const NamedPlatformHandle& handle);

// Creates a server platform handle from |handle|.
MOJO_SYSTEM_IMPL_EXPORT ScopedInternalPlatformHandle
CreateServerHandle(const NamedPlatformHandle& handle,
                   const CreateServerHandleOptions& options = {});

}  // namespace edk
}  // namespace mojo

#endif  // MOJO_EDK_EMBEDDER_NAMED_PLATFORM_HANDLE_UTILS_H_
