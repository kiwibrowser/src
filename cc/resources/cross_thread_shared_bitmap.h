// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_RESOURCES_CROSS_THREAD_SHARED_BITMAP_H_
#define CC_RESOURCES_CROSS_THREAD_SHARED_BITMAP_H_

#include <memory>

#include "base/memory/ref_counted.h"
#include "base/memory/shared_memory.h"
#include "cc/cc_export.h"
#include "components/viz/common/quads/shared_bitmap.h"
#include "components/viz/common/resources/resource_format.h"
#include "ui/gfx/geometry/size.h"

namespace cc {

// This class holds ownership of a base::SharedMemory segment for use as a
// composited resource, and is refcounted in order to share ownership with the
// LayerTreeHost, via TextureLayer, which needs access to the base::SharedMemory
// from the compositor thread.
// Because all the fields exposed are const, they can be used from any thread
// without conflict, as they only read existing states.
class CC_EXPORT CrossThreadSharedBitmap
    : public base::RefCountedThreadSafe<CrossThreadSharedBitmap> {
 public:
  CrossThreadSharedBitmap(const viz::SharedBitmapId& id,
                          std::unique_ptr<base::SharedMemory> memory,
                          const gfx::Size& size,
                          viz::ResourceFormat format);

  const viz::SharedBitmapId& id() const { return id_; }
  const base::SharedMemory* shared_memory() const { return memory_.get(); }
  const gfx::Size& size() const { return size_; }
  viz::ResourceFormat format() const { return format_; }

 private:
  friend base::RefCountedThreadSafe<CrossThreadSharedBitmap>;

  ~CrossThreadSharedBitmap();

  const viz::SharedBitmapId id_;
  const std::unique_ptr<const base::SharedMemory> memory_;
  const gfx::Size size_;
  const viz::ResourceFormat format_;
};

}  // namespace cc

#endif  // CC_RESOURCES_CROSS_THREAD_SHARED_BITMAP_H_
