// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/resources/scoped_resource.h"

namespace cc {

ScopedResource::ScopedResource(ResourceProvider* resource_provider)
    : resource_provider_(resource_provider) {
  DCHECK(resource_provider_);
}

ScopedResource::~ScopedResource() {
  Free();
}

void ScopedResource::Allocate(const gfx::Size& size,
                              ResourceProvider::TextureHint hint,
                              viz::ResourceFormat format,
                              const gfx::ColorSpace& color_space) {
  DCHECK(!id());
  DCHECK(!size.IsEmpty());

  set_dimensions(size, format);
  set_id(resource_provider_->CreateResource(size, hint, format, color_space));
  set_color_space(color_space);
  hint_ = hint;

#if DCHECK_IS_ON()
  allocate_thread_id_ = base::PlatformThread::CurrentId();
#endif
}

void ScopedResource::AllocateWithGpuMemoryBuffer(
    const gfx::Size& size,
    viz::ResourceFormat format,
    gfx::BufferUsage usage,
    const gfx::ColorSpace& color_space) {
  DCHECK(!id());
  DCHECK(!size.IsEmpty());

  set_dimensions(size, format);
  set_id(resource_provider_->CreateGpuMemoryBufferResource(
      size, ResourceProvider::TEXTURE_HINT_DEFAULT, format, usage,
      color_space));
  set_color_space(color_space);
  hint_ = ResourceProvider::TEXTURE_HINT_DEFAULT;

#if DCHECK_IS_ON()
  allocate_thread_id_ = base::PlatformThread::CurrentId();
#endif
}

void ScopedResource::Free() {
  if (id()) {
#if DCHECK_IS_ON()
    DCHECK(allocate_thread_id_ == base::PlatformThread::CurrentId());
#endif
    resource_provider_->DeleteResource(id());
  }
  set_id(0);
}

#if defined(OS_TIZEN)
bool ScopedResource::LockedForPreviousFrame() {
  return resource_provider_->LockedForPreviousFrame(id());
}
#endif

}  // namespace cc
