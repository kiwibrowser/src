// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/wayland/wayland_cursor.h"

#include <sys/mman.h>
#include <vector>

#include "base/memory/shared_memory.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "ui/gfx/skia_util.h"
#include "ui/ozone/platform/wayland/wayland_connection.h"
#include "ui/ozone/platform/wayland/wayland_pointer.h"

namespace ui {

namespace {

const uint32_t kShmFormat = WL_SHM_FORMAT_ARGB8888;
const SkColorType kColorType = kBGRA_8888_SkColorType;

}  // namespace

WaylandCursor::WaylandCursor() : shared_memory_(new base::SharedMemory()) {}

void WaylandCursor::Init(wl_pointer* pointer, WaylandConnection* connection) {
  if (input_pointer_ == pointer)
    return;

  input_pointer_ = pointer;

  DCHECK(connection);
  shm_ = connection->shm();
  pointer_surface_.reset(
      wl_compositor_create_surface(connection->compositor()));
}

WaylandCursor::~WaylandCursor() {
  pointer_surface_.reset();
  buffer_.reset();

  if (shared_memory_->handle().GetHandle()) {
    shared_memory_->Unmap();
    shared_memory_->Close();
  }
}

void WaylandCursor::UpdateBitmap(const std::vector<SkBitmap>& cursor_image,
                                 const gfx::Point& location,
                                 uint32_t serial) {
  if (!input_pointer_)
    return;

  if (!cursor_image.size()) {
    HideCursor(serial);
    return;
  }

  const SkBitmap& image = cursor_image[0];
  SkISize size = image.dimensions();
  if (size.isEmpty()) {
    HideCursor(serial);
    return;
  }

  if (!CreateSHMBuffer(gfx::SkISizeToSize(size))) {
    LOG(ERROR) << "Failed to create SHM buffer for Cursor Bitmap.";
    wl_pointer_set_cursor(input_pointer_, serial, nullptr, 0, 0);
    return;
  }

  // The |bitmap| contains ARGB image, so update our wl_buffer, which is
  // backed by a SkSurface.
  SkRect damage;
  image.getBounds(&damage);

  // Clear to transparent in case |image| is smaller than the canvas.
  SkCanvas* canvas = sk_surface_->getCanvas();
  canvas->clear(SK_ColorTRANSPARENT);
  canvas->drawBitmapRect(image, damage, nullptr);

  wl_pointer_set_cursor(input_pointer_, serial, pointer_surface_.get(),
                        location.x(), location.y());
  wl_surface_attach(pointer_surface_.get(), buffer_.get(), 0, 0);
  wl_surface_damage(pointer_surface_.get(), 0, 0, size_.width(),
                    size_.height());
  wl_surface_commit(pointer_surface_.get());
}

bool WaylandCursor::CreateSHMBuffer(const gfx::Size& size) {
  if (size == size_)
    return true;

  size_ = size;

  SkImageInfo info = SkImageInfo::MakeN32Premul(size_.width(), size_.height());
  int stride = info.minRowBytes();
  size_t image_buffer_size = info.computeByteSize(stride);
  if (image_buffer_size == SK_MaxSizeT)
    return false;

  if (shared_memory_->handle().GetHandle()) {
    shared_memory_->Unmap();
    shared_memory_->Close();
  }

  if (!shared_memory_->CreateAndMapAnonymous(image_buffer_size)) {
    LOG(ERROR) << "Create and mmap failed.";
    return false;
  }

  // TODO(tonikitoo): Use SharedMemory::requested_size instead of
  // 'image_buffer_size'?
  wl::Object<wl_shm_pool> pool;
  pool.reset(wl_shm_create_pool(shm_, shared_memory_->handle().GetHandle(),
                                image_buffer_size));
  buffer_.reset(wl_shm_pool_create_buffer(pool.get(), 0, size_.width(),
                                          size_.height(), stride, kShmFormat));

  sk_surface_ = SkSurface::MakeRasterDirect(
      SkImageInfo::Make(size_.width(), size_.height(), kColorType,
                        kOpaque_SkAlphaType),
      static_cast<uint8_t*>(shared_memory_->memory()), stride);
  return true;
}

void WaylandCursor::HideCursor(uint32_t serial) {
  size_ = gfx::Size();
  wl_pointer_set_cursor(input_pointer_, serial, nullptr, 0, 0);

  buffer_.reset();

  if (shared_memory_->handle().GetHandle()) {
    shared_memory_->Unmap();
    shared_memory_->Close();
  }
}

}  // namespace ui
