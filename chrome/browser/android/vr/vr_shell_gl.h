// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_VR_VR_SHELL_GL_H_
#define CHROME_BROWSER_ANDROID_VR_VR_SHELL_GL_H_

#include <memory>
#include <utility>
#include <vector>

#include "base/cancelable_callback.h"
#include "base/containers/queue.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "chrome/browser/android/vr/android_vsync_helper.h"
#include "chrome/browser/android/vr/vr_controller.h"
#include "chrome/browser/vr/content_input_delegate.h"
#include "chrome/browser/vr/fps_meter.h"
#include "chrome/browser/vr/model/controller_model.h"
#include "chrome/browser/vr/sliding_average.h"
#include "chrome/browser/vr/ui_input_manager.h"
#include "chrome/browser/vr/ui_renderer.h"
#include "chrome/browser/vr/ui_test_input.h"
#include "device/vr/public/mojom/vr_service.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "third_party/gvr-android-sdk/src/libraries/headers/vr/gvr/capi/include/gvr.h"
#include "third_party/gvr-android-sdk/src/libraries/headers/vr/gvr/capi/include/gvr_types.h"
#include "ui/gfx/geometry/quaternion.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/geometry/size_f.h"
#include "ui/gfx/native_widget_types.h"

namespace gl {
class GLContext;
class GLFence;
class GLFenceAndroidNativeFenceSync;
class GLFenceEGL;
class GLImageEGL;
class GLSurface;
class ScopedJavaSurface;
class SurfaceTexture;
}  // namespace gl

namespace gfx {
class GpuFence;
}  // namespace gfx

namespace gpu {
struct MailboxHolder;
struct SyncToken;
class GpuMemoryBufferImplAndroidHardwareBuffer;
}  // namespace gpu

namespace vr {

class BrowserUiInterface;
class FPSMeter;
class GlBrowserInterface;
class MailboxToSurfaceBridge;
class ScopedGpuTrace;
class SlidingTimeDeltaAverage;
class Ui;
class VrController;
class VrShell;

struct WebVrBounds {
  WebVrBounds(const gfx::RectF& left,
              const gfx::RectF& right,
              const gfx::Size& size)
      : left_bounds(left), right_bounds(right), source_size(size) {}
  gfx::RectF left_bounds;
  gfx::RectF right_bounds;
  gfx::Size source_size;
};

// WebVR/WebXR frames go through a three-stage pipeline: Animating, Processing,
// and Rendering. There's also an Idle state used as the starting state before
// Animating and ending state after Rendering.
//
// The stages can overlap, but we enforce that there isn't more than one
// frame in a given non-Idle state at any one time.
//
//       <- GetVSync
//   Idle
//       SendVSync
//   Animating
//       <- UpdateLayerBounds (optional)
//       <- GetVSync
//       <- SubmitFrame
//       ProcessWebVrFrame
//   Processing
//       <- OnWebVrFrameAvailable
//       DrawFrame
//       DrawFrameSubmitWhenReady
//       <= poll prev_frame_completion_fence_
//       DrawFrameSubmitNow
//   Rendering
//       <= prev_frame_completion_fence_ signals
//       DrawFrameSubmitNow (of next frame)
//   Idle
//
// Note that the frame is considered to still be in "Animating" state until
// ProcessWebVrFrame is called. If the current processing frame isn't done yet
// at the time the incoming SubmitFrame arrives, we defer ProcessWebVrFrame
// until that finishes.
//
// The renderer may call SubmitFrameMissing instead of SubmitFrame. In that
// case, the frame transitions from Animating back to Idle.
//
//       <- GetVSync
//   Idle
//       SendVSync
//   Animating
//       <- UpdateLayerBounds (optional)
//       <- GetVSync
//       <- SubmitFrameMissing
//   Idle

struct WebXrSharedBuffer {
  WebXrSharedBuffer();
  ~WebXrSharedBuffer();

  gfx::Size size = {0, 0};

  // Shared GpuMemoryBuffer
  std::unique_ptr<gpu::GpuMemoryBufferImplAndroidHardwareBuffer> gmb;

  // Resources in the remote GPU process command buffer context
  std::unique_ptr<gpu::MailboxHolder> mailbox_holder;
  uint32_t remote_texture = 0;
  uint32_t remote_image = 0;

  // Resources in the local GL context
  uint32_t local_texture = 0;
  // This refptr keeps the image alive while processing a frame. That's
  // required because it owns underlying resources, and must still be
  // alive when the mailbox texture backed by this image is used.
  scoped_refptr<gl::GLImageEGL> local_glimage;
};

struct WebXrFrame {
  WebXrFrame();
  ~WebXrFrame();

  bool IsValid();
  void Recycle();

  // If true, this frame cannot change state until unlocked. Used to mark
  // processing frames for the critical stage from drawing to Surface until
  // they arrive in OnWebVRFrameAvailable. See also recycle_once_unlocked.
  bool state_locked = false;

  // Start of elements that need to be reset on Recycle

  int16_t index = -1;

  // Set on an animating frame if it is waiting for being able to transition
  // to processing state.
  base::OnceClosure deferred_start_processing;

  // Set if a frame recycle failed due to being locked. The client should check
  // this after unlocking it and retry recycling it at that time.
  bool recycle_once_unlocked = false;

  std::unique_ptr<gl::GLFence> gvr_handoff_fence;

  // End of elements that need to be reset on Recycle

  base::TimeTicks time_pose;
  base::TimeTicks time_js_submit;
  base::TimeTicks time_copied;
  gfx::Transform head_pose;

  // In SharedBuffer mode, keep a swap chain.
  std::unique_ptr<WebXrSharedBuffer> shared_buffer;

  DISALLOW_COPY_AND_ASSIGN(WebXrFrame);
};

struct Viewport {
  gvr::BufferViewport left;
  gvr::BufferViewport right;

  void SetSourceBufferIndex(int index) {
    left.SetSourceBufferIndex(index);
    right.SetSourceBufferIndex(index);
  }

  void SetSourceUv(const gvr::Rectf& uv) {
    left.SetSourceUv(uv);
    right.SetSourceUv(uv);
  }
};

class WebXrPresentationState {
 public:
  // WebXR frames use an arbitrary sequential ID to help catch logic errors
  // involving out-of-order frames. We use an 8-bit unsigned counter, wrapping
  // from 255 back to 0. Elsewhere we use -1 to indicate a non-WebXR frame, so
  // most internal APIs use int16_t to ensure that they can store a full
  // -1..255 value range.
  using FrameIndexType = uint8_t;

  WebXrPresentationState();
  ~WebXrPresentationState();

  // State transitions for normal flow
  FrameIndexType StartFrameAnimating();
  void TransitionFrameAnimatingToProcessing();
  void TransitionFrameProcessingToRendering();
  void EndFrameRendering();

  // Shuts down a presentation session. This will recycle any
  // animating or rendering frame. A processing frame cannot be
  // recycled if its state is locked, it will be recycled later
  // once the state unlocks.
  void EndPresentation();

  // Variant transitions, if Renderer didn't call SubmitFrame,
  // or if we want to discard an unwanted incoming frame.
  void RecycleUnusedAnimatingFrame();
  bool RecycleProcessingFrameIfPossible();

  bool HaveAnimatingFrame() { return animating_frame_; }
  WebXrFrame* GetAnimatingFrame();
  bool HaveProcessingFrame() { return processing_frame_; }
  WebXrFrame* GetProcessingFrame();
  bool HaveRenderingFrame() { return rendering_frame_; }
  WebXrFrame* GetRenderingFrame();

  // Used by WebVrCanAnimateFrame() to detect when ui_->CanSendWebVrVSync()
  // transitions from false to true, as part of starting the incoming frame
  // timeout.
  bool last_ui_allows_sending_vsync = false;

  // GpuMemoryBuffer creation needs a buffer ID. We don't really care about
  // this, but try to keep it unique to avoid confusion.
  int next_memory_buffer_id = 0;

  base::OnceClosure end_presentation_callback;

 private:
  std::vector<std::unique_ptr<WebXrFrame>> frames_storage_;

  // Index of the next animating WebXR frame.
  FrameIndexType next_frame_index_ = 0;

  WebXrFrame* animating_frame_ = nullptr;
  WebXrFrame* processing_frame_ = nullptr;
  WebXrFrame* rendering_frame_ = nullptr;
  base::queue<WebXrFrame*> idle_frames_;

  DISALLOW_COPY_AND_ASSIGN(WebXrPresentationState);
};

// This class manages all GLThread owned objects and GL rendering for VrShell.
// It is not threadsafe and must only be used on the GL thread.
class VrShellGl : public device::mojom::VRPresentationProvider {
 public:
  VrShellGl(GlBrowserInterface* browser_interface,
            std::unique_ptr<Ui> ui,
            gvr_context* gvr_api,
            bool reprojected_rendering,
            bool daydream_support,
            bool start_in_web_vr_mode,
            bool pause_content,
            bool low_density);
  ~VrShellGl() override;

  void Initialize();
  void InitializeGl(gfx::AcceleratedWidget window);

  void OnTriggerEvent(bool pressed);
  void OnPause();
  void OnResume();
  void OnExitPresent();

  base::WeakPtr<BrowserUiInterface> GetBrowserUiWeakPtr();

  scoped_refptr<base::SingleThreadTaskRunner> GetTaskRunner() {
    return task_runner_;
  }

  void SetWebVrMode(bool enabled);
  void CreateOrResizeWebVRSurface(const gfx::Size& size);
  void WebVrCreateOrResizeSharedBufferImage(WebXrSharedBuffer* buffer,
                                            const gfx::Size& size);
  void WebVrPrepareSharedBuffer(const gfx::Size& size);
  void ContentBoundsChanged(int width, int height);
  void BufferBoundsChanged(const gfx::Size& content_buffer_size,
                           const gfx::Size& overlay_buffer_size);
  void UIBoundsChanged(int width, int height);
  void ResumeContentRendering();

  base::WeakPtr<VrShellGl> GetWeakPtr();

  void ConnectPresentingService(
      device::mojom::VRSubmitFrameClientPtrInfo submit_client_info,
      device::mojom::VRPresentationProviderRequest request,
      device::mojom::VRDisplayInfoPtr display_info,
      device::mojom::VRRequestPresentOptionsPtr present_options);

  void set_is_exiting(bool exiting) { is_exiting_ = exiting; }

  void OnSwapContents(int new_content_id);

  void EnableAlertDialog(PlatformInputHandler* input_handler,
                         float width,
                         float height);
  void DisableAlertDialog();

  void SetAlertDialogSize(float width, float height);
  void SetDialogLocation(float x, float y);
  void SetDialogFloating(bool floating);

  void ShowToast(const base::string16& text);
  void CancelToast();

  void AcceptDoffPromptForTesting();
  void PerformUiActionForTesting(UiTestInput test_input);
  void SetUiExpectingActivityForTesting(
      UiTestActivityExpectation ui_expectation);

 private:
  void GvrInit(gvr_context* gvr_api);
  device::mojom::VRDisplayFrameTransportOptionsPtr
      GetWebVrFrameTransportOptions(device::mojom::VRRequestPresentOptionsPtr);
  void InitializeRenderer();
  void UpdateViewports();
  void OnGpuProcessConnectionReady();
  // Returns true if successfully resized.
  bool ResizeForWebVR(int16_t frame_index);
  void UpdateSamples();
  void UpdateEyeInfos(const gfx::Transform& head_pose,
                      Viewport& viewport,
                      const gfx::Size& render_size,
                      RenderInfo* out_render_info);
  void UpdateContentViewportTransforms(const gfx::Transform& head_pose);
  void DrawFrame(int16_t frame_index, base::TimeTicks current_time);
  void DrawIntoAcquiredFrame(int16_t frame_index, base::TimeTicks current_time);
  void DrawFrameSubmitWhenReady(int16_t frame_index,
                                const gfx::Transform& head_pose,
                                std::unique_ptr<gl::GLFenceEGL> fence);
  void DrawFrameSubmitNow(int16_t frame_index, const gfx::Transform& head_pose);
  bool ShouldDrawWebVr();
  void DrawWebVr();
  void DrawContentQuad(bool draw_overlay_texture);
  bool ShouldSendGesturesToWebVr();
  bool WebVrPoseByteIsValid(int pose_index_byte);

  void UpdateController(const RenderInfo& render_info,
                        base::TimeTicks current_time);

  void SendImmediateExitRequestIfNecessary();
  void HandleControllerInput(const gfx::Point3F& laser_origin,
                             const RenderInfo& render_info,
                             base::TimeTicks current_time);
  void HandleControllerAppButtonActivity(
      const gfx::Vector3dF& controller_direction);

  void OnContentFrameAvailable();
  void OnContentOverlayFrameAvailable();
  void OnUiFrameAvailable();
  void OnWebVRFrameAvailable();
  void OnNewWebVRFrame();
  void ScheduleOrCancelWebVrFrameTimeout();
  void OnWebVrTimeoutImminent();
  void OnWebVrFrameTimedOut();

  base::TimeDelta GetPredictedFrameTime();
  void AddWebVrRenderTimeEstimate(int16_t frame_index,
                                  const base::TimeTicks& fence_complete_time);

  void OnVSync(base::TimeTicks frame_time);

  bool IsSubmitFrameExpected(int16_t frame_index);

  // VRPresentationProvider
  void GetVSync(GetVSyncCallback callback) override;
  void SubmitFrameMissing(int16_t frame_index, const gpu::SyncToken&) override;
  void SubmitFrame(int16_t frame_index,
                   const gpu::MailboxHolder& mailbox,
                   base::TimeDelta time_waited) override;
  void SubmitFrameWithTextureHandle(int16_t frame_index,
                                    mojo::ScopedHandle texture_handle) override;
  void SubmitFrameDrawnIntoTexture(int16_t frame_index,
                                   const gpu::SyncToken&,
                                   base::TimeDelta time_waited) override;
  void UpdateLayerBounds(int16_t frame_index,
                         const gfx::RectF& left_bounds,
                         const gfx::RectF& right_bounds,
                         const gfx::Size& source_size) override;

  // Shared logic for SubmitFrame variants, including sanity checks.
  // Returns true if OK to proceed.
  bool SubmitFrameCommon(int16_t frame_index, base::TimeDelta time_waited);

  void ForceExitVr();

  // Sends a GetVSync response to the presentation client.
  void SendVSync();

  // Heuristics to avoid excessive backlogged frames.
  bool WebVrHasSlowRenderingFrame();
  bool WebVrHasOverstuffedBuffers();

  // Checks if we're in a valid state for starting animation of a new frame.
  // Invalid states include a previous animating frame that's not complete
  // yet (including deferred processing not having started yet), or timing
  // heuristics indicating that it should be retried later.
  bool WebVrCanAnimateFrame(bool is_from_onvsync);
  // Call this after state changes that could result in WebVrCanAnimateFrame
  // becoming true.
  void WebVrTryStartAnimatingFrame(bool is_from_onvsync);

  // Checks if we're in a valid state for processing the current animating
  // frame. Invalid states include mailbox_bridge_ready_ being false, or an
  // already existing processing frame that's not done yet.
  bool WebVrCanProcessFrame();
  // Call this after state changes that could result in WebVrCanProcessFrame
  // becoming true.
  void WebVrTryDeferredProcessing();
  // Transition a frame from animating to processing.
  void ProcessWebVrFrameFromGMB(int16_t frame_index,
                                const gpu::SyncToken& sync_token);
  void ProcessWebVrFrameFromMailbox(int16_t frame_index,
                                    const gpu::MailboxHolder& mailbox);

  // Used for discarding unwanted WebVR frames while UI is showing. We can't
  // safely cancel frames from processing start until they show up in
  // OnWebVRFrameAvailable, so only support cancelling them before or after
  // that lifecycle segment.
  void WebVrCancelAnimatingFrame();
  void WebVrCancelProcessingFrameAfterTransfer();

  void WebVrSendRenderNotification(bool was_rendered);

  void ClosePresentationBindings();

  device::mojom::XRInputSourceStatePtr GetGazeInputSourceState();

  void ReportUiStatusForTesting(const base::TimeTicks& current_time,
                                bool ui_updated);
  void ReportUiActivityResultForTesting(VrUiTestActivityResult result);

  // samplerExternalOES texture data for WebVR content image.
  int webvr_texture_id_ = 0;
  int content_texture_id_ = 0;
  int content_overlay_texture_id_ = 0;

  // Set from feature flags.
  bool webvr_vsync_align_;

  scoped_refptr<gl::GLSurface> surface_;
  scoped_refptr<gl::GLContext> context_;
  scoped_refptr<gl::SurfaceTexture> content_surface_texture_;
  scoped_refptr<gl::SurfaceTexture> content_overlay_surface_texture_;
  scoped_refptr<gl::SurfaceTexture> ui_surface_texture_;
  scoped_refptr<gl::SurfaceTexture> webvr_surface_texture_;
  float webvr_surface_texture_uv_transform_[16];
  std::unique_ptr<gl::ScopedJavaSurface> content_surface_;
  std::unique_ptr<gl::ScopedJavaSurface> ui_surface_;
  std::unique_ptr<gl::ScopedJavaSurface> content_overlay_surface_;

  std::unique_ptr<gvr::GvrApi> gvr_api_;
  gvr::BufferViewportList viewport_list_;
  Viewport main_viewport_;
  Viewport webvr_viewport_;
  Viewport webvr_overlay_viewport_;
  Viewport content_underlay_viewport_;
  bool viewports_need_updating_ = true;
  gvr::SwapChain swap_chain_;
  gvr::Frame acquired_frame_;
  base::queue<std::pair<WebXrPresentationState::FrameIndexType, WebVrBounds>>
      pending_bounds_;
  WebVrBounds current_webvr_frame_bounds_ =
      WebVrBounds(gfx::RectF(), gfx::RectF(), gfx::Size());
  base::queue<uint16_t> pending_frames_;
  std::unique_ptr<MailboxToSurfaceBridge> mailbox_bridge_;
  bool mailbox_bridge_ready_ = false;

  // A fence used to avoid overstuffed GVR buffers in WebVR mode.
  std::unique_ptr<gl::GLFenceAndroidNativeFenceSync>
      webvr_prev_frame_completion_fence_;
  std::unique_ptr<ScopedGpuTrace> gpu_trace_;

  // The default size for the render buffers.
  gfx::Size render_size_default_;
  gfx::Size render_size_webvr_ui_;
  const bool low_density_;

  // WebVR currently supports multiple render path choices, with runtime
  // selection based on underlying support being available and feature flags.
  // The webvr_use_* booleans choose among the implementations. Please don't
  // check WebXrRenderPath or other feature flags in individual code paths
  // directly to avoid inconsistent logic.
  bool webvr_use_gpu_fence_ = false;
  bool webvr_use_shared_buffer_draw_ = false;

  void WebVrWaitForServerFence();
  void OnWebVRTokenSignaled(int16_t frame_index,
                            std::unique_ptr<gfx::GpuFence>);

  int webvr_unstuff_ratelimit_frames_ = 0;

  bool cardboard_ = false;
  gfx::Quaternion controller_quat_;

  gfx::Size content_tex_buffer_size_ = {0, 0};
  gfx::Size webvr_surface_size_ = {0, 0};

  std::unique_ptr<WebXrPresentationState> webxr_ = nullptr;

  std::unique_ptr<Ui> ui_;

  bool web_vr_mode_ = false;
  bool ready_to_draw_ = false;
  bool paused_ = true;
  const bool surfaceless_rendering_;
  bool daydream_support_;
  bool is_exiting_ = false;
  bool content_paused_;
  bool report_webxr_input_ = false;
  bool cardboard_trigger_pressed_ = false;
  bool cardboard_trigger_clicked_ = false;

  std::unique_ptr<VrController> controller_;
  std::vector<device::mojom::XRInputSourceStatePtr> input_states_;

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  // Attributes tracking WebVR rAF/VSync animation loop state. Blink schedules
  // a callback using the GetVSync mojo call which is stored in
  // get_vsync_callback_. The callback is executed by SendVSync once
  // WebVrCanAnimateFrame returns true.
  //
  // pending_vsync_ is set to true in OnVSync and false in SendVSync. It
  // throttles animation to no faster than the VSync rate. The pending_time_ is
  // updated in OnVSync and used as the rAF animation timer in SendVSync.
  base::TimeTicks pending_time_;
  bool pending_vsync_ = false;
  GetVSyncCallback get_vsync_callback_;

  mojo::Binding<device::mojom::VRPresentationProvider> binding_;
  device::mojom::VRSubmitFrameClientPtr submit_client_;

  GlBrowserInterface* browser_;

  uint64_t webvr_frames_received_ = 0;

  // Attributes for gesture detection while holding app button.
  gfx::Vector3dF controller_start_direction_;
  base::TimeTicks app_button_down_time_;
  bool app_button_long_pressed_ = false;

  FPSMeter vr_ui_fps_meter_;
  FPSMeter webvr_fps_meter_;

  // JS time is from SendVSync (pose time) to incoming JS submitFrame.
  SlidingTimeDeltaAverage webvr_js_time_;

  // Render time is from JS submitFrame to estimated render completion.
  // This is an estimate when submitting incomplete frames to GVR.
  // If submitFrame blocks, that means the previous frame wasn't done
  // rendering yet.
  SlidingTimeDeltaAverage webvr_render_time_;

  // JS wait time is spent waiting for the previous frame to complete
  // rendering, as reported from the Renderer via mojo.
  SlidingTimeDeltaAverage webvr_js_wait_time_;

  // GVR acquire/submit times for scheduling heuristics.
  SlidingTimeDeltaAverage webvr_acquire_time_;
  SlidingTimeDeltaAverage webvr_submit_time_;

  SlidingTimeDeltaAverage ui_processing_time_;
  SlidingTimeDeltaAverage ui_controller_update_time_;

  gfx::Point3F pointer_start_;

  RenderInfo render_info_;

  AndroidVSyncHelper vsync_helper_;

  base::CancelableOnceCallback<void()> webvr_frame_timeout_;
  base::CancelableOnceCallback<void()> webvr_spinner_timeout_;

  // WebVR defers submitting a frame to GVR by scheduling a closure
  // for later. If we exit WebVR before it is executed, we need to
  // cancel it to avoid inconsistent state.
  base::CancelableOnceCallback<
      void(int16_t, const gfx::Transform&, std::unique_ptr<gl::GLFenceEGL>)>
      webvr_delayed_gvr_submit_;

  std::vector<gvr::BufferSpec> specs_;

  bool content_frame_available_ = false;
  gfx::Transform last_used_head_pose_;

  ControllerModel controller_model_;

  std::unique_ptr<PlatformUiInputDelegate> vr_dialog_input_delegate_;
  bool showing_vr_dialog_ = false;
  std::unique_ptr<UiTestState> ui_test_state_;

  base::WeakPtrFactory<VrShellGl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(VrShellGl);
};

}  // namespace vr

#endif  // CHROME_BROWSER_ANDROID_VR_VR_SHELL_GL_H_
