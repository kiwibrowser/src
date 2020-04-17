// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_RESOURCES_SCOPED_RESOURCE_H_
#define CC_RESOURCES_SCOPED_RESOURCE_H_

#include <memory>

#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "cc/cc_export.h"
#include "cc/resources/resource.h"

#if DCHECK_IS_ON()
#include "base/threading/platform_thread.h"
#endif

namespace cc {

class CC_EXPORT ScopedResource : public Resource {
 public:
  explicit ScopedResource(ResourceProvider* provider);
  virtual ~ScopedResource();

  void Allocate(const gfx::Size& size,
                ResourceProvider::TextureHint hint,
                viz::ResourceFormat format,
                const gfx::ColorSpace& color_space);
  void AllocateWithGpuMemoryBuffer(const gfx::Size& size,
                                   viz::ResourceFormat format,
                                   gfx::BufferUsage usage,
                                   const gfx::ColorSpace& color_space);
  void Free();

  ResourceProvider::TextureHint hint() const { return hint_; }

#if defined(OS_TIZEN)
  bool LockedForPreviousFrame();
#endif

 private:
  ResourceProvider* resource_provider_;
  ResourceProvider::TextureHint hint_ =
      ResourceProvider::TextureHint::TEXTURE_HINT_DEFAULT;

#if DCHECK_IS_ON()
  base::PlatformThreadId allocate_thread_id_;
#endif

  DISALLOW_COPY_AND_ASSIGN(ScopedResource);
};

}  // namespace cc

#endif  // CC_RESOURCES_SCOPED_RESOURCE_H_
