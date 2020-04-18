// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_DISPLAY_OUTPUT_SURFACE_H_
#define COMPONENTS_VIZ_SERVICE_DISPLAY_OUTPUT_SURFACE_H_

#include <memory>

#include "base/containers/circular_deque.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread_checker.h"
#include "components/viz/common/gpu/context_provider.h"
#include "components/viz/common/resources/returned_resource.h"
#include "components/viz/service/display/overlay_candidate_validator.h"
#include "components/viz/service/display/software_output_device.h"
#include "components/viz/service/viz_service_export.h"
#include "gpu/command_buffer/common/texture_in_use_response.h"
#include "gpu/vulkan/buildflags.h"
#include "ui/gfx/color_space.h"
#include "ui/latency/latency_info.h"

#if BUILDFLAG(ENABLE_VULKAN)
#include "components/viz/common/gpu/vulkan_context_provider.h"
#include "gpu/vulkan/vulkan_surface.h"
#endif

namespace gfx {
class ColorSpace;
class Size;
struct SwapResponse;
}  // namespace gfx

namespace viz {
class OutputSurfaceClient;
class OutputSurfaceFrame;

// This class represents a platform-independent API for presenting
// buffers to display via GPU or software compositing. Implementations
// can provide platform-specific behaviour.
class VIZ_SERVICE_EXPORT OutputSurface {
 public:
  struct Capabilities {
    Capabilities() = default;

    int max_frames_pending = 1;
    // Whether this output surface renders to the default OpenGL zero
    // framebuffer or to an offscreen framebuffer.
    bool uses_default_gl_framebuffer = true;
    // Whether this OutputSurface is flipped or not.
    bool flipped_output_surface = false;
    // Whether this OutputSurface supports stencil operations or not.
    // Note: HasExternalStencilTest() must return false when an output surface
    // has been configured for stencil usage.
    bool supports_stencil = false;
  };

  // Constructor for GL-based compositing.
  explicit OutputSurface(scoped_refptr<ContextProvider> context_provider);
  // Constructor for software compositing.
  explicit OutputSurface(std::unique_ptr<SoftwareOutputDevice> software_device);
#if BUILDFLAG(ENABLE_VULKAN)
  // Constructor for Vulkan-based compositing.
  explicit OutputSurface(
      scoped_refptr<VulkanContextProvider> vulkan_context_provider);
#endif

  virtual ~OutputSurface();

  const Capabilities& capabilities() const { return capabilities_; }

  // Obtain the 3d context or the software device associated with this output
  // surface. Either of these may return a null pointer, but not both.
  // In the event of a lost context, the entire output surface should be
  // recreated.
  ContextProvider* context_provider() const { return context_provider_.get(); }
#if BUILDFLAG(ENABLE_VULKAN)
  VulkanContextProvider* vulkan_context_provider() const {
    return vulkan_context_provider_.get();
  }
#endif
  SoftwareOutputDevice* software_device() const {
    return software_device_.get();
  }

  void set_color_matrix(const SkMatrix44& color_matrix) {
    color_matrix_ = color_matrix;
  }
  const SkMatrix44& color_matrix() const { return color_matrix_; }

  virtual void BindToClient(OutputSurfaceClient* client) = 0;

  virtual void EnsureBackbuffer() = 0;
  virtual void DiscardBackbuffer() = 0;

  // Bind the default framebuffer for drawing to, only valid for GL backed
  // OutputSurfaces.
  virtual void BindFramebuffer() = 0;

  // Marks that the given rectangle will be drawn to on the default, bound
  // framebuffer. Only valid for surfaces with dc_layers in the context
  // capabilities.
  virtual void SetDrawRectangle(const gfx::Rect& rect) = 0;

  // Get the class capable of informing cc of hardware overlay capability.
  virtual OverlayCandidateValidator* GetOverlayCandidateValidator() const = 0;

  // Returns true if a main image overlay plane should be scheduled.
  virtual bool IsDisplayedAsOverlayPlane() const = 0;

  // Get the texture for the main image's overlay.
  virtual unsigned GetOverlayTextureId() const = 0;

  // Get the format for the main image's overlay.
  virtual gfx::BufferFormat GetOverlayBufferFormat() const = 0;

  virtual void Reshape(const gfx::Size& size,
                       float device_scale_factor,
                       const gfx::ColorSpace& color_space,
                       bool has_alpha,
                       bool use_stencil) = 0;

  virtual bool HasExternalStencilTest() const = 0;
  virtual void ApplyExternalStencil() = 0;

  // Gives the GL internal format that should be used for calling CopyTexImage2D
  // when the framebuffer is bound via BindFramebuffer().
  virtual uint32_t GetFramebufferCopyTextureFormat() = 0;

  // Swaps the current backbuffer to the screen. For successful swaps, the
  // implementation must call OutputSurfaceClient::DidReceiveSwapBuffersAck()
  // after returning from this method in order to unblock the next frame.
  virtual void SwapBuffers(OutputSurfaceFrame frame) = 0;

#if BUILDFLAG(ENABLE_VULKAN)
  // Gives the Vulkan surface created when enable_vulkan flag is set.
  virtual gpu::VulkanSurface* GetVulkanSurface() = 0;
#endif

  // Updates the GpuFence associated with this surface. The id of a newly
  // created GpuFence is returned, or if an error occurs, or fences are not
  // supported, the special id of 0 (meaning "no fence") is returned.  In all
  // cases, any previously associated fence is destroyed. The returned fence id
  // corresponds to the GL id used by the CHROMIUM_gpu_fence GL extension and
  // can be passed directly to any related extension functions.
  virtual unsigned UpdateGpuFence() = 0;

  // Returns true if any of the LatencyInfos provided contains a snapshot
  // request.
  static bool LatencyInfoHasSnapshotRequest(
      const std::vector<ui::LatencyInfo>& latency_info);

  // Updates timing info on the provided LatencyInfo when swap completes.
  static void UpdateLatencyInfoOnSwap(
      const gfx::SwapResponse& response,
      std::vector<ui::LatencyInfo>* latency_info);

 protected:
  struct OutputSurface::Capabilities capabilities_;
  scoped_refptr<ContextProvider> context_provider_;
#if BUILDFLAG(ENABLE_VULKAN)
  scoped_refptr<VulkanContextProvider> vulkan_context_provider_;
#endif
  std::unique_ptr<SoftwareOutputDevice> software_device_;

 private:
  SkMatrix44 color_matrix_;

  DISALLOW_COPY_AND_ASSIGN(OutputSurface);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_DISPLAY_OUTPUT_SURFACE_H_
