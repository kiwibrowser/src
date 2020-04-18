// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/skia_bindings/grcontext_for_gles2_interface.h"

#include <stddef.h>
#include <string.h>
#include <utility>

#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/sys_info.h"
#include "base/trace_event/trace_event.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/common/capabilities.h"
#include "gpu/skia_bindings/gl_bindings_skia_cmd_buffer.h"
#include "gpu/skia_bindings/gles2_implementation_with_grcontext_support.h"
#include "third_party/skia/include/gpu/GrContext.h"
#include "third_party/skia/include/gpu/GrContextOptions.h"
#include "third_party/skia/include/gpu/gl/GrGLInterface.h"

namespace skia_bindings {

void GrContextForGLES2Interface::DetermineCacheLimitsFromAvailableMemory(
    size_t* max_resource_cache_bytes,
    size_t* max_glyph_cache_texture_bytes) {
  // The limit of the bytes allocated toward GPU resources in the GrContext's
  // GPU cache.
  static const size_t kMaxLowEndGaneshResourceCacheBytes = 48 * 1024 * 1024;
  static const size_t kMaxGaneshResourceCacheBytes = 96 * 1024 * 1024;
  static const size_t kMaxHighEndGaneshResourceCacheBytes = 256 * 1024 * 1024;
  // Limits for glyph cache textures.
  static const size_t kMaxDefaultGlyphCacheTextureBytes = 2048 * 1024 * 4;
  static const size_t kMaxLowEndGlyphCacheTextureBytes = 1024 * 512 * 4;
  // High-end / low-end memory cutoffs.
  static const int64_t kHighEndMemoryThreshold = (int64_t)4096 * 1024 * 1024;
  static const int64_t kLowEndMemoryThreshold = (int64_t)512 * 1024 * 1024;

  int64_t amount_of_physical_memory = base::SysInfo::AmountOfPhysicalMemory();

  *max_resource_cache_bytes = kMaxGaneshResourceCacheBytes;
  *max_glyph_cache_texture_bytes = kMaxDefaultGlyphCacheTextureBytes;
  if (amount_of_physical_memory <= kLowEndMemoryThreshold) {
    *max_resource_cache_bytes = kMaxLowEndGaneshResourceCacheBytes;
    *max_glyph_cache_texture_bytes = kMaxLowEndGlyphCacheTextureBytes;
  } else if (amount_of_physical_memory >= kHighEndMemoryThreshold) {
    *max_resource_cache_bytes = kMaxHighEndGaneshResourceCacheBytes;
  }
}

void GrContextForGLES2Interface::DefaultCacheLimitsForTests(
    size_t* max_resource_cache_bytes,
    size_t* max_glyph_cache_texture_bytes) {
  static const size_t kDefaultGlyphCacheTextureBytes = 2048 * 1024 * 4;
  static const size_t kDefaultGaneshResourceCacheBytes = 96 * 1024 * 1024;
  *max_resource_cache_bytes = kDefaultGaneshResourceCacheBytes;
  *max_glyph_cache_texture_bytes = kDefaultGlyphCacheTextureBytes;
}

GrContextForGLES2Interface::GrContextForGLES2Interface(
    gpu::gles2::GLES2Interface* gl,
    gpu::ContextSupport* context_support,
    const gpu::Capabilities& capabilities,
    size_t max_resource_cache_bytes,
    size_t max_glyph_cache_texture_bytes)
    : context_support_(context_support) {
  // The limit of the number of GPU resources we hold in the GrContext's
  // GPU cache.
  static const int kMaxGaneshResourceCacheCount = 16384;

  GrContextOptions options;
  options.fGlyphCacheTextureMaximumBytes = max_glyph_cache_texture_bytes;
  options.fAvoidStencilBuffers = capabilities.avoid_stencil_buffers;
  options.fAllowPathMaskCaching = false;
  options.fSharpenMipmappedTextures = true;
  sk_sp<GrGLInterface> interface(
      skia_bindings::CreateGLES2InterfaceBindings(gl, context_support));
  gr_context_ = GrContext::MakeGL(std::move(interface), options);
  if (gr_context_) {
    gr_context_->setResourceCacheLimits(kMaxGaneshResourceCacheCount,
                                        max_resource_cache_bytes);
    context_support_->SetGrContext(gr_context_.get());
  }
}

GrContextForGLES2Interface::~GrContextForGLES2Interface() {
  // At this point the GLES2Interface is going to be destroyed, so have
  // the GrContext clean up and not try to use it anymore.
  if (gr_context_) {
    gr_context_->releaseResourcesAndAbandonContext();
    context_support_->SetGrContext(nullptr);
  }
}

void GrContextForGLES2Interface::OnLostContext() {
  if (gr_context_)
    gr_context_->abandonContext();
}

void GrContextForGLES2Interface::FreeGpuResources() {
  if (gr_context_) {
    TRACE_EVENT_INSTANT0("gpu", "GrContext::freeGpuResources",
                         TRACE_EVENT_SCOPE_THREAD);
    gr_context_->freeGpuResources();
  }
}

GrContext* GrContextForGLES2Interface::get() {
  return gr_context_.get();
}

}  // namespace skia_bindings
