// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/platform/platform_handle.h"

#include "base/logging.h"
#include "build/build_config.h"

#if defined(OS_WIN)
#include <windows.h>
#elif defined(OS_FUCHSIA)
#include <fdio/limits.h>
#include <unistd.h>
#include <zircon/status.h>
#include <zircon/syscalls.h>
#elif defined(OS_MACOSX) && !defined(OS_IOS)
#include <mach/mach_vm.h>

#include "base/mac/mach_logging.h"
#endif

#if defined(OS_POSIX)
#include <unistd.h>
#endif

namespace mojo {

namespace {

#if defined(OS_WIN)
base::win::ScopedHandle CloneHandle(const base::win::ScopedHandle& handle) {
  DCHECK(handle.IsValid());

  HANDLE dupe;
  BOOL result = ::DuplicateHandle(::GetCurrentProcess(), handle.Get(),
                                  ::GetCurrentProcess(), &dupe, 0, FALSE,
                                  DUPLICATE_SAME_ACCESS);
  if (!result)
    return base::win::ScopedHandle();
  DCHECK_NE(dupe, INVALID_HANDLE_VALUE);
  return base::win::ScopedHandle(dupe);
}
#elif defined(OS_FUCHSIA)
base::ScopedZxHandle CloneHandle(const base::ScopedZxHandle& handle) {
  DCHECK(handle.is_valid());

  zx_handle_t dupe;
  zx_status_t result =
      zx_handle_duplicate(handle.get(), ZX_RIGHT_SAME_RIGHTS, &dupe);
  DLOG_IF(ERROR, result != ZX_OK) << "zx_duplicate_handle failed: " << result;
  return base::ScopedZxHandle(dupe);
}
#elif defined(OS_MACOSX) && !defined(OS_IOS)
base::mac::ScopedMachSendRight CloneMachPort(
    const base::mac::ScopedMachSendRight& mach_port) {
  DCHECK(mach_port.is_valid());

  kern_return_t kr = mach_port_mod_refs(mach_task_self(), mach_port.get(),
                                        MACH_PORT_RIGHT_SEND, 1);
  if (kr != KERN_SUCCESS) {
    MACH_DLOG(ERROR, kr) << "mach_port_mod_refs";
    return base::mac::ScopedMachSendRight();
  }
  return base::mac::ScopedMachSendRight(mach_port.get());
}
#endif

#if defined(OS_POSIX) || defined(OS_FUCHSIA)
base::ScopedFD CloneFD(const base::ScopedFD& fd) {
  DCHECK(fd.is_valid());
  return base::ScopedFD(dup(fd.get()));
}
#endif

}  // namespace

PlatformHandle::PlatformHandle() = default;

PlatformHandle::PlatformHandle(PlatformHandle&& other) = default;

#if defined(OS_WIN)
PlatformHandle::PlatformHandle(base::win::ScopedHandle handle)
    : handle_(std::move(handle)) {}
#elif defined(OS_FUCHSIA)
PlatformHandle::PlatformHandle(base::ScopedZxHandle handle)
    : handle_(std::move(handle)) {}
#elif defined(OS_MACOSX) && !defined(OS_IOS)
PlatformHandle::PlatformHandle(base::mac::ScopedMachSendRight mach_port)
    : mach_port_(std::move(mach_port)) {}
#endif

#if defined(OS_POSIX) || defined(OS_FUCHSIA)
PlatformHandle::PlatformHandle(base::ScopedFD fd) : fd_(std::move(fd)) {
#if defined(OS_FUCHSIA)
  DCHECK_LT(fd_.get(), FDIO_MAX_FD);
#endif
}
#endif

PlatformHandle::~PlatformHandle() = default;

PlatformHandle& PlatformHandle::operator=(PlatformHandle&& other) = default;

void PlatformHandle::reset() {
#if defined(OS_WIN)
  handle_.Close();
#elif defined(OS_FUCHSIA)
  handle_.reset();
#elif defined(OS_MACOSX) && !defined(OS_IOS)
  mach_port_.reset();
#endif

#if defined(OS_POSIX) || defined(OS_FUCHSIA)
  fd_.reset();
#endif
}

PlatformHandle PlatformHandle::Clone() const {
#if defined(OS_WIN)
  return PlatformHandle(CloneHandle(handle_));
#elif defined(OS_FUCHSIA)
  if (is_valid_handle())
    return PlatformHandle(CloneHandle(handle_));
  return PlatformHandle(CloneFD(fd_));
#elif defined(OS_MACOSX) && !defined(OS_IOS)
  if (is_valid_mach_port())
    return PlatformHandle(CloneMachPort(mach_port_));
  return PlatformHandle(CloneFD(fd_));
#elif defined(OS_POSIX)
  return PlatformHandle(CloneFD(fd_));
#endif
}

}  // namespace mojo
