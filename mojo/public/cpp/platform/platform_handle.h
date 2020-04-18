// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_PLATFORM_PLATFORM_HANDLE_H_
#define MOJO_PUBLIC_CPP_PLATFORM_PLATFORM_HANDLE_H_

#include "base/component_export.h"
#include "base/logging.h"
#include "base/macros.h"
#include "build/build_config.h"

#if defined(OS_WIN)
#include "base/win/scoped_handle.h"
#elif defined(OS_FUCHSIA)
#include "base/fuchsia/scoped_zx_handle.h"
#elif defined(OS_MACOSX) && !defined(OS_IOS)
#include "base/mac/scoped_mach_port.h"
#endif

#if defined(OS_POSIX) || defined(OS_FUCHSIA)
#include "base/files/scoped_file.h"
#endif

namespace mojo {

// A PlatformHandle is a generic wrapper around a platform-specific system
// handle type, e.g. a POSIX file descriptor or Windows HANDLE. This can wrap
// any of various such types depending on the host platform for which it's
// compiled.
//
// This is useful primarily for two reasons:
//
// - Interacting with the Mojo invitation API, which use OS primitives to
//   bootstrap Mojo IPC connections.
// - Interacting with Mojo platform handle wrapping and unwrapping API, which
//   allows handles to OS primitives to be transmitted over Mojo IPC with a
//   stable wire representation via Mojo handles.
//
// NOTE: This assumes ownership if the handle it represents.
class COMPONENT_EXPORT(MOJO_CPP_PLATFORM) PlatformHandle {
 public:
  PlatformHandle();
  PlatformHandle(PlatformHandle&& other);

#if defined(OS_WIN)
  explicit PlatformHandle(base::win::ScopedHandle handle);
#elif defined(OS_FUCHSIA)
  explicit PlatformHandle(base::ScopedZxHandle handle);
#elif defined(OS_MACOSX) && !defined(OS_IOS)
  explicit PlatformHandle(base::mac::ScopedMachSendRight mach_port);
#endif

#if defined(OS_POSIX) || defined(OS_FUCHSIA)
  explicit PlatformHandle(base::ScopedFD fd);
#endif

  ~PlatformHandle();

  PlatformHandle& operator=(PlatformHandle&& other);

  // Closes the underlying platform handle.
  void reset();

  // Duplicates the underlying platform handle, returning a new PlatformHandle
  // which owns it.
  PlatformHandle Clone() const;

#if defined(OS_WIN)
  bool is_valid() const { return handle_.IsValid(); }
  const base::win::ScopedHandle& GetHandle() const { return handle_; }
  base::win::ScopedHandle TakeHandle() { return std::move(handle_); }
  HANDLE ReleaseHandle() WARN_UNUSED_RESULT { return handle_.Take(); }
#elif defined(OS_FUCHSIA)
  bool is_valid() const { return is_valid_fd() || is_valid_handle(); }

  bool is_valid_handle() const { return handle_.is_valid(); }
  const base::ScopedZxHandle& GetHandle() const { return handle_; }
  base::ScopedZxHandle TakeHandle() { return std::move(handle_); }
  zx_handle_t ReleaseHandle() WARN_UNUSED_RESULT { return handle_.release(); }
#elif defined(OS_MACOSX) && !defined(OS_IOS)
  bool is_valid() const { return is_valid_fd() || is_valid_mach_port(); }

  bool is_valid_mach_port() const { return mach_port_.is_valid(); }
  const base::mac::ScopedMachSendRight& GetMachPort() const {
    return mach_port_;
  }
  base::mac::ScopedMachSendRight TakeMachPort() {
    return std::move(mach_port_);
  }
  mach_port_t ReleaseMachPort() WARN_UNUSED_RESULT {
    return mach_port_.release();
  }
#elif defined(OS_POSIX)
  bool is_valid() const { return is_valid_fd(); }
#else
#error "Unsupported platform."
#endif

#if defined(OS_POSIX) || defined(OS_FUCHSIA)
  bool is_valid_fd() const { return fd_.is_valid(); }
  const base::ScopedFD& GetFD() const { return fd_; }
  base::ScopedFD TakeFD() { return std::move(fd_); }
  int ReleaseFD() WARN_UNUSED_RESULT { return fd_.release(); }
#endif

 private:
#if defined(OS_WIN)
  base::win::ScopedHandle handle_;
#elif defined(OS_FUCHSIA)
  base::ScopedZxHandle handle_;
#elif defined(OS_MACOSX) && !defined(OS_IOS)
  base::mac::ScopedMachSendRight mach_port_;
#endif

#if defined(OS_POSIX) || defined(OS_FUCHSIA)
  base::ScopedFD fd_;
#endif

  DISALLOW_COPY_AND_ASSIGN(PlatformHandle);
};

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_PLATFORM_PLATFORM_HANDLE_H_
