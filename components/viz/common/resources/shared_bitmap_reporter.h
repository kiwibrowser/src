// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_RESOURCES_SHARED_BITMAP_REPORTER_H_
#define COMPONENTS_VIZ_COMMON_RESOURCES_SHARED_BITMAP_REPORTER_H_

#include "components/viz/common/quads/shared_bitmap.h"
#include "components/viz/common/viz_common_export.h"
#include "mojo/public/cpp/system/buffer.h"

namespace viz {

// Used by clients to notify the display compositor about SharedMemory allocated
// for shared bitmaps.
// TODO(kylechar): This should be //components/viz/client but because of deps
// issues that isn't possible. Fix and move there.
class VIZ_COMMON_EXPORT SharedBitmapReporter {
 public:
  // Associates a SharedBitmapId with a shared buffer handle.
  virtual void DidAllocateSharedBitmap(mojo::ScopedSharedBufferHandle buffer,
                                       const SharedBitmapId& id) = 0;

  // Disassociates a SharedBitmapId previously passed to
  // DidAllocateSharedBitmap.
  virtual void DidDeleteSharedBitmap(const SharedBitmapId& id) = 0;

 protected:
  virtual ~SharedBitmapReporter();
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_RESOURCES_SHARED_BITMAP_REPORTER_H_
