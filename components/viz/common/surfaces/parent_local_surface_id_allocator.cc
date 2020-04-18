// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/surfaces/parent_local_surface_id_allocator.h"

#include "base/lazy_instance.h"
#include "base/rand_util.h"

namespace viz {

base::LazyInstance<LocalSurfaceId>::Leaky g_invalid_local_surface_id =
    LAZY_INSTANCE_INITIALIZER;

ParentLocalSurfaceIdAllocator::ParentLocalSurfaceIdAllocator()
    : current_local_surface_id_(kInitialParentSequenceNumber,
                                kInitialChildSequenceNumber,
                                base::UnguessableToken::Create()) {}

bool ParentLocalSurfaceIdAllocator::UpdateFromChild(
    const LocalSurfaceId& child_allocated_local_surface_id) {
  if (child_allocated_local_surface_id.child_sequence_number() >
      current_local_surface_id_.child_sequence_number()) {
    current_local_surface_id_.child_sequence_number_ =
        child_allocated_local_surface_id.child_sequence_number_;
    is_invalid_ = false;
    return true;
  }
  return false;
}

void ParentLocalSurfaceIdAllocator::Reset(
    const LocalSurfaceId& local_surface_id) {
  current_local_surface_id_ = local_surface_id;
}

void ParentLocalSurfaceIdAllocator::Invalidate() {
  is_invalid_ = true;
}

const LocalSurfaceId& ParentLocalSurfaceIdAllocator::GenerateId() {
  if (!is_allocation_suppressed_)
    ++current_local_surface_id_.parent_sequence_number_;
  is_invalid_ = false;
  return current_local_surface_id_;
}

const LocalSurfaceId& ParentLocalSurfaceIdAllocator::GetCurrentLocalSurfaceId()
    const {
  if (is_invalid_)
    return g_invalid_local_surface_id.Get();
  return current_local_surface_id_;
}

}  // namespace viz
