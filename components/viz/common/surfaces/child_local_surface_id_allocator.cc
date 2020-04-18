// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/surfaces/child_local_surface_id_allocator.h"

#include <stdint.h>

#include "base/rand_util.h"

namespace viz {

ChildLocalSurfaceIdAllocator::ChildLocalSurfaceIdAllocator()
    : current_local_surface_id_(kInvalidParentSequenceNumber,
                                kInitialChildSequenceNumber,
                                base::UnguessableToken()) {}

bool ChildLocalSurfaceIdAllocator::UpdateFromParent(
    const LocalSurfaceId& parent_allocated_local_surface_id) {
  if (parent_allocated_local_surface_id.parent_sequence_number() >
      current_local_surface_id_.parent_sequence_number()) {
    current_local_surface_id_.parent_sequence_number_ =
        parent_allocated_local_surface_id.parent_sequence_number_;
    current_local_surface_id_.embed_token_ =
        parent_allocated_local_surface_id.embed_token_;
    return true;
  }
  return false;
}

const LocalSurfaceId& ChildLocalSurfaceIdAllocator::GenerateId() {
  // UpdateFromParent must be called before we can generate a valid ID.
  DCHECK_NE(current_local_surface_id_.parent_sequence_number(),
            kInvalidParentSequenceNumber);

  ++current_local_surface_id_.child_sequence_number_;
  return current_local_surface_id_;
}

}  // namespace viz
