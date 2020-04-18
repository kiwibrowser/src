// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/embedder/platform_handle.h"

#include "build/build_config.h"

#if defined(OS_WIN)
#include <windows.h>
#elif defined(OS_FUCHSIA)
#include <unistd.h>
#include <zircon/status.h>
#include <zircon/syscalls.h>
#elif defined(OS_POSIX)
#include <unistd.h>
#endif

#include "base/logging.h"

#if defined(OS_WIN)
#include "base/optional.h"
#include "mojo/edk/system/scoped_process_handle.h"
#endif

namespace mojo {
namespace edk {

void InternalPlatformHandle::CloseIfNecessary() {
#if defined(OS_WIN)
  // Take local ownership of the process handle in |owning_process| if it's
  // a handle to a remote process. We do this before the generic handle validity
  // test below because even if the platform handle has been taken by someone
  // else, we may still own a remote process handle and it needs to be closed
  // before we return.
  base::Optional<ScopedProcessHandle> remote_process_handle;
  if (owning_process != base::GetCurrentProcessHandle()) {
    remote_process_handle.emplace(owning_process);
    owning_process = base::GetCurrentProcessHandle();
  }
#endif

  if (!is_valid())
    return;

#if defined(OS_WIN)
  if (remote_process_handle) {
    // This handle may have been duplicated to a new target process but not yet
    // sent there. In this case CloseHandle should NOT be called. From MSDN
    // documentation for DuplicateHandle[1]:
    //
    //    Normally the target process closes a duplicated handle when that
    //    process is finished using the handle. To close a duplicated handle
    //    from the source process, call DuplicateHandle with the following
    //    parameters:
    //
    //    * Set hSourceProcessHandle to the target process from the
    //      call that created the handle.
    //    * Set hSourceHandle to the duplicated handle to close.
    //    * Set lpTargetHandle [sic] to NULL. (N.B.: This appears to be a
    //      documentation bug; what matters is that hTargetProcessHandle is
    //      NULL.)
    //    * Set dwOptions to DUPLICATE_CLOSE_SOURCE.
    //
    // [1] https://msdn.microsoft.com/en-us/library/windows/desktop/ms724251
    //
    // NOTE: It's possible for this operation to fail if the owning process
    // was terminated or is in the process of being terminated. Either way,
    // there is nothing we can reasonably do about failure, so we ignore it.
    ::DuplicateHandle(remote_process_handle->get(), handle, NULL, &handle, 0,
                      FALSE, DUPLICATE_CLOSE_SOURCE);
    return;
  }

  bool success = !!CloseHandle(handle);
  DPCHECK(success);
  handle = INVALID_HANDLE_VALUE;
#elif defined(OS_FUCHSIA)
  if (handle != ZX_HANDLE_INVALID) {
    zx_status_t result = zx_handle_close(handle);
    DCHECK_EQ(ZX_OK, result) << "CloseIfNecessary(zx_handle_close): "
                             << zx_status_get_string(result);
    handle = ZX_HANDLE_INVALID;
  }
  if (fd >= 0) {
    bool success = (close(fd) == 0);
    DPCHECK(success);
    fd = -1;
  }
#elif defined(OS_POSIX)
  if (type == Type::POSIX) {
    bool success = (close(handle) == 0);
    DPCHECK(success);
    handle = -1;
  }
#if defined(OS_MACOSX) && !defined(OS_IOS)
  else if (type == Type::MACH) {
    kern_return_t rv = mach_port_deallocate(mach_task_self(), port);
    DPCHECK(rv == KERN_SUCCESS);
    port = MACH_PORT_NULL;
  }
#endif  // defined(OS_MACOSX) && !defined(OS_IOS)
#else
#error "Platform not yet supported."
#endif  // defined(OS_WIN)
}

}  // namespace edk
}  // namespace mojo
