// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_HARDWARE_RENDERER_H_
#define ANDROID_WEBVIEW_BROWSER_HARDWARE_RENDERER_H_

#include <memory>

#include "android_webview/browser/child_frame.h"
#include "android_webview/browser/compositor_id.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/viz/common/surfaces/frame_sink_id.h"
#include "components/viz/common/surfaces/surface_id.h"
#include "services/viz/public/interfaces/compositing/compositor_frame_sink.mojom.h"

struct AwDrawGLInfo;

namespace viz {
class CompositorFrameSinkSupport;
class ParentLocalSurfaceIdAllocator;
}

namespace android_webview {

class ChildFrame;
class RenderThreadManager;
class SurfacesInstance;

class HardwareRenderer : public viz::mojom::CompositorFrameSinkClient {
 public:
  // Two rules:
  // 1) Never wait on |new_frame| on the UI thread, or in kModeSync. Otherwise
  //    this defeats the purpose of having a future.
  // 2) Never replace a non-empty frames with an empty frame.
  // The only way to do both is to hold up to two frames here. This is a helper
  // method to do this. General pattern is call this method to prune existing
  // queue, and then append the new frame. Wait on all frames in queue. Then
  // remove all except the latest non-empty frame. If all frames are empty,
  // then the deque is cleared. Return any non-empty frames that are pruned.
  // Return value does not guarantee relative order is maintained.
  static ChildFrameQueue WaitAndPruneFrameQueue(ChildFrameQueue* child_frames);

  explicit HardwareRenderer(RenderThreadManager* state);
  ~HardwareRenderer() override;

  void DrawGL(AwDrawGLInfo* draw_info);
  void CommitFrame();

 private:
  // viz::mojom::CompositorFrameSinkClient implementation.
  void DidReceiveCompositorFrameAck(
      const std::vector<viz::ReturnedResource>& resources) override;
  void DidPresentCompositorFrame(uint32_t presentation_token,
                                 base::TimeTicks time,
                                 base::TimeDelta refresh,
                                 uint32_t flags) override;
  void DidDiscardCompositorFrame(uint32_t presentation_token) override;
  void OnBeginFrame(const viz::BeginFrameArgs& args) override;
  void ReclaimResources(
      const std::vector<viz::ReturnedResource>& resources) override;
  void OnBeginFramePausedChanged(bool paused) override;

  void ReturnChildFrame(std::unique_ptr<ChildFrame> child_frame);
  void ReturnResourcesToCompositor(
      const std::vector<viz::ReturnedResource>& resources,
      const CompositorID& compositor_id,
      uint32_t layer_tree_frame_sink_id);

  void AllocateSurface();
  void DestroySurface();

  void CreateNewCompositorFrameSinkSupport();

  RenderThreadManager* render_thread_manager_;

  typedef void* EGLContext;
  EGLContext last_egl_context_;

  // Information about last delegated frame.
  gfx::Size surface_size_;
  float device_scale_factor_ = 0;

  // Infromation from UI on last commit.
  gfx::Vector2d scroll_offset_;

  ChildFrameQueue child_frame_queue_;

  // This holds the last ChildFrame received. Contains the frame info of the
  // last frame. The |frame| member is always null since frame has already
  // been submitted.
  std::unique_ptr<ChildFrame> child_frame_;

  const scoped_refptr<SurfacesInstance> surfaces_;
  viz::FrameSinkId frame_sink_id_;
  const std::unique_ptr<viz::ParentLocalSurfaceIdAllocator>
      parent_local_surface_id_allocator_;
  std::unique_ptr<viz::CompositorFrameSinkSupport> support_;
  viz::LocalSurfaceId child_id_;
  CompositorID compositor_id_;
  // HardwareRenderer guarantees resources are returned in the order of
  // layer_tree_frame_sink_id, and resources for old output surfaces are
  // dropped.
  uint32_t last_committed_layer_tree_frame_sink_id_;
  uint32_t last_submitted_layer_tree_frame_sink_id_;

  DISALLOW_COPY_AND_ASSIGN(HardwareRenderer);
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_HARDWARE_RENDERER_H_
