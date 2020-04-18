// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/surface/transport_dib.h"

#include <windows.h>
#include <stddef.h>
#include <stdint.h>

#include <limits>
#include <memory>

#include "base/logging.h"
#include "base/sys_info.h"
#include "skia/ext/platform_canvas.h"

TransportDIB::TransportDIB()
    : size_(0) {
}

TransportDIB::~TransportDIB() {
}

TransportDIB::TransportDIB(base::SharedMemoryHandle handle)
    : shared_memory_(handle, false /* read write */), size_(0) {}

// static
TransportDIB* TransportDIB::Map(Handle handle) {
  std::unique_ptr<TransportDIB> dib(CreateWithHandle(handle));
  if (!dib->Map())
    return NULL;
  return dib.release();
}

// static
TransportDIB* TransportDIB::CreateWithHandle(Handle handle) {
  return new TransportDIB(handle);
}

// static
bool TransportDIB::is_valid_handle(Handle dib) {
  return dib.IsValid();
}

std::unique_ptr<SkCanvas> TransportDIB::GetPlatformCanvas(int w,
                                                          int h,
                                                          bool opaque) {
  // This DIB already mapped the file into this process, but PlatformCanvas
  // will map it again.
  DCHECK(!memory()) << "Mapped file twice in the same process.";

  // We can't check the canvas size before mapping, but it's safe because
  // Windows will fail to map the section if the dimensions of the canvas
  // are too large.
  std::unique_ptr<SkCanvas> canvas =
      skia::CreatePlatformCanvasWithSharedSection(
          w, h, opaque, shared_memory_.handle().GetHandle(),
          skia::RETURN_NULL_ON_FAILURE);

  // Calculate the size for the memory region backing the canvas.
  if (canvas)
    size_ = skia::PlatformCanvasStrideForWidth(w) * h;

  return canvas;
}

bool TransportDIB::Map() {
  if (!is_valid_handle(shared_memory_.handle()))
    return false;
  if (memory())
    return true;

  if (!shared_memory_.Map(0 /* map whole shared memory segment */)) {
    LOG(ERROR) << "Failed to map transport DIB"
               << " handle:" << shared_memory_.handle().GetHandle()
               << " error:" << ::GetLastError();
    return false;
  }

  size_ = shared_memory_.mapped_size();
  return true;
}

void* TransportDIB::memory() const {
  return shared_memory_.memory();
}
