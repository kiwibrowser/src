// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/resources/resource.h"

#include "build/build_config.h"
#include "components/viz/common/quads/shared_bitmap.h"

namespace viz {
namespace internal {

Resource::Resource(const gfx::Size& size,
                   ResourceType type,
                   ResourceFormat format,
                   const gfx::ColorSpace& color_space)
    : locked_for_external_use(false),
      marked_for_deletion(false),
      read_lock_fences_enabled(false),
      has_shared_bitmap_id(false),
      is_overlay_candidate(false),
#if defined(OS_ANDROID)
      is_backed_by_surface_texture(false),
      wants_promotion_hint(false),
#endif
      size(size),
      type(type),
      format(format),
      color_space(color_space) {
}

Resource::Resource(Resource&& other) = default;
Resource::~Resource() = default;
Resource& Resource::operator=(Resource&& other) = default;

void Resource::SetSharedBitmap(SharedBitmap* bitmap) {
  DCHECK(bitmap);
  DCHECK(bitmap->pixels());
  shared_bitmap = bitmap;
  pixels = bitmap->pixels();
  has_shared_bitmap_id = true;
  shared_bitmap_id = bitmap->id();
}

void Resource::SetLocallyUsed() {
  synchronization_state_ = LOCALLY_USED;
  sync_token_.Clear();
}

void Resource::SetSynchronized() {
  synchronization_state_ = SYNCHRONIZED;
}

void Resource::UpdateSyncToken(const gpu::SyncToken& sync_token) {
  DCHECK(is_gpu_resource_type());
  // An empty sync token may be used if commands are guaranteed to have run on
  // the gpu process or in case of context loss.
  sync_token_ = sync_token;
  synchronization_state_ = sync_token.HasData() ? NEEDS_WAIT : SYNCHRONIZED;
}

int8_t* Resource::GetSyncTokenData() {
  return sync_token_.GetData();
}

bool Resource::ShouldWaitSyncToken() const {
  return synchronization_state_ == NEEDS_WAIT;
}

}  // namespace internal
}  // namespace viz
