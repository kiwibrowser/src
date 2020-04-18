// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_QUADS_SHARED_BITMAP_H_
#define COMPONENTS_VIZ_COMMON_QUADS_SHARED_BITMAP_H_

#include <stddef.h>
#include <stdint.h>

#include "base/hash.h"
#include "base/macros.h"
#include "base/trace_event/memory_allocator_dump.h"
#include "components/viz/common/resources/resource_format.h"
#include "components/viz/common/viz_common_export.h"
#include "gpu/command_buffer/common/mailbox.h"
#include "ui/gfx/geometry/size.h"

namespace base {
class UnguessableToken;
}

namespace viz {
using SharedBitmapId = gpu::Mailbox;

struct SharedBitmapIdHash {
  size_t operator()(const SharedBitmapId& id) const {
    return base::Hash(id.name, sizeof(id.name));
  }
};

VIZ_COMMON_EXPORT base::trace_event::MemoryAllocatorDumpGuid
GetSharedBitmapGUIDForTracing(const SharedBitmapId& bitmap_id);

class VIZ_COMMON_EXPORT SharedBitmap {
 public:
  SharedBitmap(uint8_t* pixels,
               const SharedBitmapId& id,
               uint32_t sequence_number);

  virtual ~SharedBitmap();

  uint8_t* pixels() { return pixels_; }
  const SharedBitmapId& id() { return id_; }

  // The sequence number that ClientSharedBitmapManager assigned to this
  // SharedBitmap.
  uint32_t sequence_number() const { return sequence_number_; }

  // Returns the GUID for tracing when the SharedBitmap supports cross-process
  // use via shared memory. Otherwise, this returns empty.
  virtual base::UnguessableToken GetCrossProcessGUID() const = 0;

  static SharedBitmapId GenerateId();

 private:
  uint8_t* pixels_;
  SharedBitmapId id_;
  const uint32_t sequence_number_;

  DISALLOW_COPY_AND_ASSIGN(SharedBitmap);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_QUADS_SHARED_BITMAP_H_
