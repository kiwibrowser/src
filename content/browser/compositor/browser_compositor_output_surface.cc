// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/compositor/browser_compositor_output_surface.h"

#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/location.h"
#include "base/strings/string_number_conversions.h"
#include "components/viz/common/frame_sinks/begin_frame_source.h"
#include "components/viz/service/display/output_surface_client.h"
#include "components/viz/service/display_embedder/compositor_overlay_candidate_validator.h"
#include "content/browser/compositor/reflector_impl.h"
#include "services/ui/public/cpp/gpu/context_provider_command_buffer.h"

namespace content {

BrowserCompositorOutputSurface::BrowserCompositorOutputSurface(
    scoped_refptr<viz::ContextProvider> context_provider,
    const UpdateVSyncParametersCallback& update_vsync_parameters_callback,
    std::unique_ptr<viz::CompositorOverlayCandidateValidator>
        overlay_candidate_validator)
    : OutputSurface(std::move(context_provider)),
      update_vsync_parameters_callback_(update_vsync_parameters_callback),
      reflector_(nullptr) {
  overlay_candidate_validator_ = std::move(overlay_candidate_validator);
}

BrowserCompositorOutputSurface::BrowserCompositorOutputSurface(
    std::unique_ptr<viz::SoftwareOutputDevice> software_device,
    const UpdateVSyncParametersCallback& update_vsync_parameters_callback)
    : OutputSurface(std::move(software_device)),
      update_vsync_parameters_callback_(update_vsync_parameters_callback),
      reflector_(nullptr) {}

#if BUILDFLAG(ENABLE_VULKAN)
BrowserCompositorOutputSurface::BrowserCompositorOutputSurface(
    const scoped_refptr<viz::VulkanContextProvider>& vulkan_context_provider,
    const UpdateVSyncParametersCallback& update_vsync_parameters_callback)
    : OutputSurface(std::move(vulkan_context_provider)),
      update_vsync_parameters_callback_(update_vsync_parameters_callback),
      reflector_(nullptr) {}
#endif

BrowserCompositorOutputSurface::~BrowserCompositorOutputSurface() {
  if (reflector_)
    reflector_->DetachFromOutputSurface();
  DCHECK(!reflector_);
}

void BrowserCompositorOutputSurface::SetReflector(ReflectorImpl* reflector) {
  // Software mirroring is done by doing a GL copy out of the framebuffer - if
  // we have overlays then that data will be missing.
  if (overlay_candidate_validator_) {
    overlay_candidate_validator_->SetSoftwareMirrorMode(reflector != nullptr);
  }
  reflector_ = reflector;

  OnReflectorChanged();
}

void BrowserCompositorOutputSurface::OnReflectorChanged() {
}

viz::OverlayCandidateValidator*
BrowserCompositorOutputSurface::GetOverlayCandidateValidator() const {
  return overlay_candidate_validator_.get();
}

bool BrowserCompositorOutputSurface::HasExternalStencilTest() const {
  return false;
}

void BrowserCompositorOutputSurface::ApplyExternalStencil() {}

}  // namespace content
