// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/surface/transport_dib.h"

#include <limits.h>

#include "skia/ext/platform_canvas.h"

// static
bool TransportDIB::VerifyCanvasSize(int w, int h) {
  static const size_t kMaxSize = static_cast<size_t>(INT_MAX);
  const size_t one_stride = skia::PlatformCanvasStrideForWidth(1);
  const size_t stride = skia::PlatformCanvasStrideForWidth(w);
  if (w <= 0 || h <= 0 || static_cast<size_t>(w) > (kMaxSize / one_stride) ||
      static_cast<size_t>(h) > (kMaxSize / stride)) {
    return false;
  }

  return (stride * h) <= size_;
}

base::SharedMemory* TransportDIB::shared_memory() {
  return &shared_memory_;
}
