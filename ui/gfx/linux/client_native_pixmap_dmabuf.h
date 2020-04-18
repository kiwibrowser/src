// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_LINUX_CLIENT_NATIVE_PIXMAP_DMABUF_H_
#define UI_GFX_LINUX_CLIENT_NATIVE_PIXMAP_DMABUF_H_

#include <stdint.h>

#include <memory>

#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "ui/gfx/client_native_pixmap.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/native_pixmap_handle.h"

namespace gfx {

class ClientNativePixmapDmaBuf : public gfx::ClientNativePixmap {
 public:
  static std::unique_ptr<gfx::ClientNativePixmap> ImportFromDmabuf(
      const gfx::NativePixmapHandle& handle,
      const gfx::Size& size);

  ~ClientNativePixmapDmaBuf() override;

  // Overridden from ClientNativePixmap.
  bool Map() override;
  void Unmap() override;

  void* GetMemoryAddress(size_t plane) const override;
  int GetStride(size_t plane) const override;

 private:
  ClientNativePixmapDmaBuf(const gfx::NativePixmapHandle& handle,
                           const gfx::Size& size);

  const gfx::NativePixmapHandle pixmap_handle_;
  const gfx::Size size_;
  base::ScopedFD dmabuf_fd_;
  void* data_;

  DISALLOW_COPY_AND_ASSIGN(ClientNativePixmapDmaBuf);
};

}  // namespace gfx

#endif  // UI_GFX_LINUX_CLIENT_NATIVE_PIXMAP_DMABUF_H_
