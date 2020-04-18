// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_RENDER_THREAD_MANAGER_H_
#define ANDROID_WEBVIEW_BROWSER_RENDER_THREAD_MANAGER_H_

#include <map>

#include "android_webview/browser/compositor_frame_consumer.h"
#include "android_webview/browser/gl_view_renderer_manager.h"
#include "android_webview/browser/parent_compositor_draw_constraints.h"
#include "base/cancelable_callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/lock.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/vector2d.h"

struct AwDrawGLInfo;
namespace android_webview {

namespace internal {
class RequestInvokeGLTracker;
}

class RenderThreadManagerClient;
class ChildFrame;
class CompositorFrameProducer;
class HardwareRenderer;
class InsideHardwareReleaseReset;
struct CompositorID;

// This class is used to pass data between UI thread and RenderThread.
class RenderThreadManager : public CompositorFrameConsumer {
 public:
  RenderThreadManager(
      RenderThreadManagerClient* client,
      const scoped_refptr<base::SingleThreadTaskRunner>& ui_loop);
  ~RenderThreadManager() override;

  // This function can be called from any thread.
  void ClientRequestInvokeGL(bool for_idle);

  // CompositorFrameConsumer methods.
  void SetCompositorFrameProducer(
      CompositorFrameProducer* compositor_frame_producer) override;
  void SetScrollOffsetOnUI(gfx::Vector2d scroll_offset) override;
  std::unique_ptr<ChildFrame> SetFrameOnUI(
      std::unique_ptr<ChildFrame> frame) override;
  void InitializeHardwareDrawIfNeededOnUI() override;
  ParentCompositorDrawConstraints GetParentDrawConstraintsOnUI() const override;
  void SwapReturnedResourcesOnUI(
      ReturnedResourcesMap* returned_resource_map) override;
  bool ReturnedResourcesEmptyOnUI() const override;
  ChildFrameQueue PassUncommittedFrameOnUI() override;
  void DeleteHardwareRendererOnUI() override;

  // Render thread methods.
  gfx::Vector2d GetScrollOffsetOnRT();
  ChildFrameQueue PassFramesOnRT();
  void DrawGL(AwDrawGLInfo* draw_info);
  void PostExternalDrawConstraintsToChildCompositorOnRT(
      const ParentCompositorDrawConstraints& parent_draw_constraints);
  void InsertReturnedResourcesOnRT(
      const std::vector<viz::ReturnedResource>& resources,
      const CompositorID& compositor_id,
      uint32_t layer_tree_frame_sink_id);

 private:
  friend class internal::RequestInvokeGLTracker;
  class InsideHardwareReleaseReset {
   public:
    explicit InsideHardwareReleaseReset(
        RenderThreadManager* render_thread_manager);
    ~InsideHardwareReleaseReset();

   private:
    RenderThreadManager* render_thread_manager_;
  };
  static std::unique_ptr<ChildFrame> GetSynchronousCompositorFrame(
      scoped_refptr<content::SynchronousCompositor::FrameFuture> frame_future,
      std::unique_ptr<ChildFrame> child_frame);

  // RT thread method.
  void DidInvokeGLProcess();
  bool HasFrameForHardwareRendererOnRT() const;

  // UI thread methods.
  void ResetRequestInvokeGLCallback();
  void ClientRequestInvokeGLOnUI();
  void UpdateParentDrawConstraintsOnUI();
  bool IsInsideHardwareRelease() const;
  void SetInsideHardwareRelease(bool inside);

  // Accessed by UI thread.
  scoped_refptr<base::SingleThreadTaskRunner> ui_loop_;
  RenderThreadManagerClient* const client_;
  CompositorFrameProducer* compositor_frame_producer_;
  base::WeakPtr<RenderThreadManager> ui_thread_weak_ptr_;
  base::CancelableClosure request_draw_gl_cancelable_closure_;
  // Whether any frame has been received on the UI thread by
  // RenderThreadManager.
  bool has_received_frame_;

  // Accessed by RT thread.
  std::unique_ptr<HardwareRenderer> hardware_renderer_;

  // This is accessed by both UI and RT now. TODO(hush): move to RT only.
  GLViewRendererManager::Key renderer_manager_key_;

  // Accessed by both UI and RT thread.
  mutable base::Lock lock_;
  gfx::Vector2d scroll_offset_;
  ChildFrameQueue child_frames_;
  bool inside_hardware_release_;
  ParentCompositorDrawConstraints parent_draw_constraints_;
  ReturnedResourcesMap returned_resources_map_;
  base::RepeatingClosure request_draw_gl_closure_;

  base::WeakPtrFactory<RenderThreadManager> weak_factory_on_ui_thread_;

  DISALLOW_COPY_AND_ASSIGN(RenderThreadManager);
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_RENDER_THREAD_MANAGER_H_
