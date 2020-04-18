// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_CAPTURE_AURA_WINDOW_CAPTURE_MACHINE_H_
#define CONTENT_BROWSER_MEDIA_CAPTURE_AURA_WINDOW_CAPTURE_MACHINE_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/media/capture/cursor_renderer_aura.h"
#include "media/capture/content/screen_capture_device_core.h"
#include "services/device/public/mojom/wake_lock.mojom.h"
#include "ui/aura/window.h"
#include "ui/aura/window_observer.h"
#include "ui/base/cursor/cursors_aura.h"
#include "ui/compositor/compositor.h"
#include "ui/compositor/compositor_animation_observer.h"

namespace viz {
class CopyOutputResult;
class ReadbackYUVInterface;
}

namespace content {

// AuraWindowCaptureMachine uses the compositor to capture Aura windows.
//
// It is used for browser window capture on platforms that use Aura (Windows,
// Linux, and Chrome OS) and additionally for desktop capture on Chrome OS.
class AuraWindowCaptureMachine : public media::VideoCaptureMachine,
                                 public aura::WindowObserver,
                                 public ui::ContextFactoryObserver,
                                 public ui::CompositorAnimationObserver {
 public:
  AuraWindowCaptureMachine();
  ~AuraWindowCaptureMachine() override;

  // VideoCaptureMachine overrides.
  void Start(const scoped_refptr<media::ThreadSafeCaptureOracle>& oracle_proxy,
             const media::VideoCaptureParams& params,
             const base::Callback<void(bool)> callback) override;
  void Suspend() override;
  void Resume() override;
  void Stop(const base::Closure& callback) override;
  void MaybeCaptureForRefresh() override;

  // Implements aura::WindowObserver.
  void OnWindowBoundsChanged(aura::Window* window,
                             const gfx::Rect& old_bounds,
                             const gfx::Rect& new_bounds,
                             ui::PropertyChangeReason reason) override;
  void OnWindowDestroying(aura::Window* window) override;
  void OnWindowAddedToRootWindow(aura::Window* window) override;
  void OnWindowRemovingFromRootWindow(aura::Window* window,
                                      aura::Window* new_root) override;

  // ui::CompositorAnimationObserver implementation.
  void OnAnimationStep(base::TimeTicks timestamp) override;
  void OnCompositingShuttingDown(ui::Compositor* compositor) override;

  // Sets the window to use for capture.
  void SetWindow(aura::Window* window);

 private:
  bool InternalStart(
      const scoped_refptr<media::ThreadSafeCaptureOracle>& oracle_proxy,
      const media::VideoCaptureParams& params);
  void InternalSuspend();
  void InternalResume();
  void InternalStop(const base::Closure& callback);

  // Captures a frame. |event_time| is provided by the compositor, or is null
  // for refresh requests.
  void Capture(base::TimeTicks event_time);

  // Update capture size. Must be called on the UI thread.
  void UpdateCaptureSize();

  using CaptureFrameCallback =
      media::ThreadSafeCaptureOracle::CaptureFrameCallback;

  // Response callback for cc::Layer::RequestCopyOfOutput().
  void DidCopyOutput(scoped_refptr<media::VideoFrame> video_frame,
                     base::TimeTicks event_time,
                     base::TimeTicks start_time,
                     const CaptureFrameCallback& capture_frame_cb,
                     std::unique_ptr<viz::CopyOutputResult> result);

  // A helper which does the real work for DidCopyOutput. Returns true if
  // succeeded and |capture_frame_cb| will be run at some future point. Returns
  // false on error, and |capture_frame_cb| should be run by the caller (with
  // failure status).
  bool ProcessCopyOutputResponse(scoped_refptr<media::VideoFrame> video_frame,
                                 base::TimeTicks event_time,
                                 const CaptureFrameCallback& capture_frame_cb,
                                 std::unique_ptr<viz::CopyOutputResult> result);

  // ui::ContextFactoryObserver implementation.
  void OnLostResources() override;

  // Renders the cursor if needed and then delivers the captured frame.
  static void CopyOutputFinishedForVideo(
      base::WeakPtr<AuraWindowCaptureMachine> machine,
      base::TimeTicks event_time,
      const CaptureFrameCallback& capture_frame_cb,
      scoped_refptr<media::VideoFrame> target,
      const gfx::Rect& region_in_frame,
      std::unique_ptr<viz::SingleReleaseCallback> release_callback,
      bool result);

  // The window associated with the desktop.
  aura::Window* desktop_window_;

  // Whether screen capturing or window capture.
  bool screen_capture_;

  // Makes all the decisions about which frames to copy, and how.
  scoped_refptr<media::ThreadSafeCaptureOracle> oracle_proxy_;

  // The capture parameters for this capture.
  media::VideoCaptureParams capture_params_;

  // YUV readback pipeline.
  std::unique_ptr<viz::ReadbackYUVInterface> yuv_readback_pipeline_;

  // Renders mouse cursor on frame.
  std::unique_ptr<content::CursorRendererAura> cursor_renderer_;

  // TODO(jiayl): Remove wake_lock_ when there is an API to keep the
  // screen from sleeping for the drive-by web.
  device::mojom::WakeLockPtr wake_lock_;

  // False while frame capture has been suspended. All other aspects of the
  // machine are maintained.
  bool frame_capture_active_;

  // WeakPtrs are used for the asynchronous capture callbacks passed to external
  // modules.  They are only valid on the UI thread and become invalidated
  // immediately when InternalStop() is called to ensure that no more captured
  // frames will be delivered to the client.
  base::WeakPtrFactory<AuraWindowCaptureMachine> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AuraWindowCaptureMachine);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_CAPTURE_AURA_WINDOW_CAPTURE_MACHINE_H_
