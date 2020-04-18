// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/native_pixmap_handle.h"

#if defined(OS_LINUX)
#include <drm_fourcc.h>
#include "base/posix/eintr_wrapper.h"
#endif

namespace gfx {

#if defined(OS_LINUX)
static_assert(NativePixmapPlane::kNoModifier == DRM_FORMAT_MOD_INVALID,
              "gfx::NativePixmapPlane::kNoModifier should be an alias for"
              "DRM_FORMAT_MOD_INVALID");
#endif

NativePixmapPlane::NativePixmapPlane()
    : stride(0), offset(0), size(0), modifier(0) {}

NativePixmapPlane::NativePixmapPlane(int stride,
                                     int offset,
                                     uint64_t size,
                                     uint64_t modifier)
    : stride(stride), offset(offset), size(size), modifier(modifier) {}

NativePixmapPlane::NativePixmapPlane(const NativePixmapPlane& other) = default;

NativePixmapPlane::~NativePixmapPlane() {}

NativePixmapHandle::NativePixmapHandle() {}
NativePixmapHandle::NativePixmapHandle(const NativePixmapHandle& other) =
    default;

NativePixmapHandle::~NativePixmapHandle() {}

#if defined(OS_LINUX)
NativePixmapHandle CloneHandleForIPC(const NativePixmapHandle& handle) {
  NativePixmapHandle clone;
  std::vector<base::ScopedFD> scoped_fds;
  for (auto& fd : handle.fds) {
    base::ScopedFD scoped_fd(HANDLE_EINTR(dup(fd.fd)));
    if (!scoped_fd.is_valid()) {
      PLOG(ERROR) << "dup";
      return NativePixmapHandle();
    }
    scoped_fds.emplace_back(std::move(scoped_fd));
  }
  for (auto& scoped_fd : scoped_fds)
    clone.fds.emplace_back(scoped_fd.release(), true /* auto_close */);
  clone.planes = handle.planes;
  return clone;
}
#endif  // defined(OS_LINUX)

}  // namespace gfx
