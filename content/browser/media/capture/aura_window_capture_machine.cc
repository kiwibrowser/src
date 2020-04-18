// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/capture/aura_window_capture_machine.h"

#include <algorithm>
#include <utility>

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "components/viz/common/frame_sinks/copy_output_request.h"
#include "components/viz/common/frame_sinks/copy_output_result.h"
#include "components/viz/common/gl_helper.h"
#include "content/browser/compositor/image_transport_factory.h"
#include "content/browser/media/capture/desktop_capture_device_uma_types.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/service_manager_connection.h"
#include "media/base/video_util.h"
#include "media/capture/content/thread_safe_capture_oracle.h"
#include "media/capture/content/video_capture_oracle.h"
#include "media/capture/video_capture_types.h"
#include "services/device/public/mojom/constants.mojom.h"
#include "services/device/public/mojom/wake_lock_provider.mojom.h"
#include "services/service_manager/public/cpp/connector.h"
#include "skia/ext/image_operations.h"
#include "ui/aura/client/screen_position_client.h"
#include "ui/aura/env.h"
#include "ui/aura/window.h"
#include "ui/aura/window_observer.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/cursor/cursors_aura.h"
#include "ui/compositor/compositor.h"
#include "ui/compositor/dip_util.h"
#include "ui/compositor/layer.h"

namespace content {

AuraWindowCaptureMachine::AuraWindowCaptureMachine()
    : desktop_window_(nullptr),
      screen_capture_(false),
      frame_capture_active_(true),
      weak_factory_(this) {}

AuraWindowCaptureMachine::~AuraWindowCaptureMachine() {}

void AuraWindowCaptureMachine::Start(
  const scoped_refptr<media::ThreadSafeCaptureOracle>& oracle_proxy,
  const media::VideoCaptureParams& params,
  const base::Callback<void(bool)> callback) {
  // Starts the capture machine asynchronously.
  BrowserThread::PostTaskAndReplyWithResult(
      BrowserThread::UI,
      FROM_HERE,
      base::Bind(&AuraWindowCaptureMachine::InternalStart,
                 base::Unretained(this),
                 oracle_proxy,
                 params),
      callback);
}

bool AuraWindowCaptureMachine::InternalStart(
    const scoped_refptr<media::ThreadSafeCaptureOracle>& oracle_proxy,
    const media::VideoCaptureParams& params) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // The window might be destroyed between SetWindow() and Start().
  if (!desktop_window_)
    return false;

  // If the associated layer is already destroyed then return failure.
  ui::Layer* layer = desktop_window_->layer();
  if (!layer)
    return false;

  DCHECK(oracle_proxy);
  oracle_proxy_ = oracle_proxy;
  capture_params_ = params;

  // Update capture size.
  UpdateCaptureSize();

  // Start observing compositor updates.
  aura::WindowTreeHost* const host = desktop_window_->GetHost();
  ui::Compositor* const compositor = host ? host->compositor() : nullptr;
  if (!compositor)
    return false;
  compositor->AddAnimationObserver(this);

  // Start observing for GL context losses.
  compositor->context_factory()->AddObserver(this);

  DCHECK(!wake_lock_);
  // Request Wake Lock. In some testing contexts, the service manager
  // connection isn't initialized.
  if (ServiceManagerConnection::GetForProcess()) {
    service_manager::Connector* connector =
        ServiceManagerConnection::GetForProcess()->GetConnector();
    DCHECK(connector);
    device::mojom::WakeLockProviderPtr wake_lock_provider;
    connector->BindInterface(device::mojom::kServiceName,
                             mojo::MakeRequest(&wake_lock_provider));
    wake_lock_provider->GetWakeLockWithoutContext(
        device::mojom::WakeLockType::kPreventDisplaySleep,
        device::mojom::WakeLockReason::kOther, "Aura window or desktop capture",
        mojo::MakeRequest(&wake_lock_));

    wake_lock_->RequestWakeLock();
  }

  return true;
}

void AuraWindowCaptureMachine::Suspend() {
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&AuraWindowCaptureMachine::InternalSuspend,
                     base::Unretained(this)));
}

void AuraWindowCaptureMachine::InternalSuspend() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DVLOG(1) << "Suspending frame capture and delivery.";
  frame_capture_active_ = false;
}

void AuraWindowCaptureMachine::Resume() {
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&AuraWindowCaptureMachine::InternalResume,
                     base::Unretained(this)));
}

void AuraWindowCaptureMachine::InternalResume() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DVLOG(1) << "Resuming frame capture and delivery.";
  frame_capture_active_ = true;
  // Whenever capture resumes, capture a refresh frame immediately to make sure
  // no content updates are missing from the video stream.
  MaybeCaptureForRefresh();
}

void AuraWindowCaptureMachine::Stop(const base::Closure& callback) {
  // Stops the capture machine asynchronously.
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&AuraWindowCaptureMachine::InternalStop,
                     base::Unretained(this), callback));
}

void AuraWindowCaptureMachine::InternalStop(const base::Closure& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // Cancel any and all outstanding callbacks owned by external modules.
  weak_factory_.InvalidateWeakPtrs();

  if (wake_lock_)
    wake_lock_->CancelWakeLock();
  // Stop observing compositor and window events.
  if (desktop_window_) {
    if (aura::WindowTreeHost* host = desktop_window_->GetHost()) {
      if (ui::Compositor* compositor = host->compositor()) {
        compositor->RemoveAnimationObserver(this);
        compositor->context_factory()->RemoveObserver(this);
      }
    }
    desktop_window_->RemoveObserver(this);
    desktop_window_ = nullptr;
    cursor_renderer_.reset();
  }

  OnLostResources();

  callback.Run();
}

void AuraWindowCaptureMachine::MaybeCaptureForRefresh() {
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(
          &AuraWindowCaptureMachine::Capture,
          // Use of Unretained() is safe here since this task must run
          // before InternalStop().
          base::Unretained(this), base::TimeTicks()));
}

void AuraWindowCaptureMachine::SetWindow(aura::Window* window) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  DCHECK(!desktop_window_);
  desktop_window_ = window;
  cursor_renderer_.reset(
      new CursorRendererAura(CursorRenderer::CURSOR_DISPLAYED_ALWAYS));
  cursor_renderer_->SetTargetView(window);

  // Start observing window events.
  desktop_window_->AddObserver(this);

  // We must store this for the UMA reporting in DidCopyOutput() as
  // desktop_window_ might be destroyed at that point.
  screen_capture_ = window->IsRootWindow();
  IncrementDesktopCaptureCounter(screen_capture_ ? SCREEN_CAPTURER_CREATED
                                                 : WINDOW_CAPTURER_CREATED);
}

void AuraWindowCaptureMachine::UpdateCaptureSize() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (oracle_proxy_ && desktop_window_) {
     ui::Layer* layer = desktop_window_->layer();
     oracle_proxy_->UpdateCaptureSize(ui::ConvertSizeToPixel(
         layer, layer->bounds().size()));
  }
}

void AuraWindowCaptureMachine::Capture(base::TimeTicks event_time) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // Do not capture if the desktop window is already destroyed.
  if (!desktop_window_)
    return;

  scoped_refptr<media::VideoFrame> frame;
  media::ThreadSafeCaptureOracle::CaptureFrameCallback capture_frame_cb;

  // TODO(miu): Need to fix this so the compositor is providing the presentation
  // timestamps and damage regions, to leverage the frame timestamp rewriting
  // logic.  http://crbug.com/492839
  const base::TimeTicks start_time = base::TimeTicks::Now();
  media::VideoCaptureOracle::Event event;
  if (event_time.is_null()) {
    event = media::VideoCaptureOracle::kRefreshRequest;
    event_time = start_time;
  } else {
    event = media::VideoCaptureOracle::kCompositorUpdate;
  }
  if (oracle_proxy_->ObserveEventAndDecideCapture(
          event, gfx::Rect(), event_time, &frame, &capture_frame_cb)) {
    std::unique_ptr<viz::CopyOutputRequest> request =
        std::make_unique<viz::CopyOutputRequest>(
            viz::CopyOutputRequest::ResultFormat::RGBA_TEXTURE,
            base::BindOnce(&AuraWindowCaptureMachine::DidCopyOutput,
                           weak_factory_.GetWeakPtr(), std::move(frame),
                           event_time, start_time, capture_frame_cb));
    gfx::Rect window_rect = gfx::Rect(desktop_window_->bounds().width(),
                                      desktop_window_->bounds().height());
    request->set_area(window_rect);
    desktop_window_->layer()->RequestCopyOfOutput(std::move(request));
  }
}

void AuraWindowCaptureMachine::DidCopyOutput(
    scoped_refptr<media::VideoFrame> video_frame,
    base::TimeTicks event_time,
    base::TimeTicks start_time,
    const CaptureFrameCallback& capture_frame_cb,
    std::unique_ptr<viz::CopyOutputResult> result) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  static bool first_call = true;

  const bool succeeded = ProcessCopyOutputResponse(
      video_frame, event_time, capture_frame_cb, std::move(result));

  const base::TimeDelta capture_time = base::TimeTicks::Now() - start_time;

  // The two UMA_ blocks must be put in its own scope since it creates a static
  // variable which expected constant histogram name.
  if (screen_capture_) {
    UMA_HISTOGRAM_TIMES(kUmaScreenCaptureTime, capture_time);
  } else {
    UMA_HISTOGRAM_TIMES(kUmaWindowCaptureTime, capture_time);
  }

  if (first_call) {
    first_call = false;
    if (screen_capture_) {
      IncrementDesktopCaptureCounter(succeeded ? FIRST_SCREEN_CAPTURE_SUCCEEDED
                                               : FIRST_SCREEN_CAPTURE_FAILED);
    } else {
      IncrementDesktopCaptureCounter(succeeded
                                         ? FIRST_WINDOW_CAPTURE_SUCCEEDED
                                         : FIRST_WINDOW_CAPTURE_FAILED);
    }
  }

  // If ProcessCopyOutputResponse() failed, it will not run |capture_frame_cb|,
  // so do that now.
  if (!succeeded)
    capture_frame_cb.Run(std::move(video_frame), event_time, false);
}

bool AuraWindowCaptureMachine::ProcessCopyOutputResponse(
    scoped_refptr<media::VideoFrame> video_frame,
    base::TimeTicks event_time,
    const CaptureFrameCallback& capture_frame_cb,
    std::unique_ptr<viz::CopyOutputResult> result) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK_EQ(result->format(), viz::CopyOutputResult::Format::RGBA_TEXTURE);

  if (!desktop_window_) {
    VLOG(1) << "Ignoring CopyOutputResult: Capture target has gone away.";
    return false;
  }
  if (result->IsEmpty()) {
    VLOG(1) << "CopyOutputRequest failed: Empty result.";
    return false;
  }
  DCHECK(video_frame);

  // Compute the dest size we want after the letterboxing resize. Make the
  // coordinates and sizes even because we letterbox in YUV space
  // (see CopyRGBToVideoFrame). They need to be even for the UV samples to
  // line up correctly.
  // The video frame's visible_rect() and the result's size() are both physical
  // pixels.
  gfx::Rect region_in_frame = media::ComputeLetterboxRegion(
      video_frame->visible_rect(), result->size());
  region_in_frame = gfx::Rect(region_in_frame.x() & ~1,
                              region_in_frame.y() & ~1,
                              region_in_frame.width() & ~1,
                              region_in_frame.height() & ~1);
  if (region_in_frame.IsEmpty()) {
    VLOG(1) << "Aborting capture: Computed empty letterboxed content region.";
    return false;
  }

  ImageTransportFactory* factory = ImageTransportFactory::GetInstance();
  viz::GLHelper* gl_helper = factory->GetGLHelper();
  if (!gl_helper) {
    VLOG(1) << "Aborting capture: No GLHelper available for YUV readback.";
    return false;
  }

  gpu::Mailbox mailbox = result->GetTextureResult()->mailbox;
  gpu::SyncToken sync_token = result->GetTextureResult()->sync_token;
  std::unique_ptr<viz::SingleReleaseCallback> release_callback =
      result->TakeTextureOwnership();

  if (!yuv_readback_pipeline_)
    yuv_readback_pipeline_ = gl_helper->CreateReadbackPipelineYUV(true, true);
  viz::GLHelper::ScalerInterface* const scaler =
      yuv_readback_pipeline_->scaler();
  const gfx::Vector2d scale_from(result->size().width(),
                                 result->size().height());
  const gfx::Vector2d scale_to(region_in_frame.width(),
                               region_in_frame.height());
  if (scale_from == scale_to) {
    if (scaler)
      yuv_readback_pipeline_->SetScaler(nullptr);
  } else if (!scaler || !scaler->IsSameScaleRatio(scale_from, scale_to)) {
    std::unique_ptr<viz::GLHelper::ScalerInterface> fast_scaler =
        gl_helper->CreateScaler(viz::GLHelper::SCALER_QUALITY_FAST, scale_from,
                                scale_to, false, false, false);
    DCHECK(
        fast_scaler);  // Arguments to CreateScaler() should never be invalid.
    yuv_readback_pipeline_->SetScaler(std::move(fast_scaler));
  }

  yuv_readback_pipeline_->ReadbackYUV(
      mailbox, sync_token, result->size(), gfx::Rect(region_in_frame.size()),
      video_frame->stride(media::VideoFrame::kYPlane),
      video_frame->data(media::VideoFrame::kYPlane),
      video_frame->stride(media::VideoFrame::kUPlane),
      video_frame->data(media::VideoFrame::kUPlane),
      video_frame->stride(media::VideoFrame::kVPlane),
      video_frame->data(media::VideoFrame::kVPlane), region_in_frame.origin(),
      base::Bind(&CopyOutputFinishedForVideo, weak_factory_.GetWeakPtr(),
                 event_time, capture_frame_cb, video_frame, region_in_frame,
                 base::Passed(&release_callback)));
  media::LetterboxVideoFrame(video_frame.get(), region_in_frame);
  return true;
}

using CaptureFrameCallback =
    media::ThreadSafeCaptureOracle::CaptureFrameCallback;

void AuraWindowCaptureMachine::CopyOutputFinishedForVideo(
    base::WeakPtr<AuraWindowCaptureMachine> machine,
    base::TimeTicks event_time,
    const CaptureFrameCallback& capture_frame_cb,
    scoped_refptr<media::VideoFrame> target,
    const gfx::Rect& region_in_frame,
    std::unique_ptr<viz::SingleReleaseCallback> release_callback,
    bool result) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  release_callback->Run(gpu::SyncToken(), false);

  // Render the cursor and deliver the captured frame if the
  // AuraWindowCaptureMachine has not been stopped (i.e., the WeakPtr is
  // still valid).
  if (machine) {
    if (machine->cursor_renderer_ && result)
      machine->cursor_renderer_->RenderOnVideoFrame(target.get(),
                                                    region_in_frame, nullptr);
  } else {
    VLOG(1) << "Aborting capture: AuraWindowCaptureMachine has gone away.";
    result = false;
  }

  capture_frame_cb.Run(std::move(target), event_time, result);
}

void AuraWindowCaptureMachine::OnWindowBoundsChanged(
    aura::Window* window,
    const gfx::Rect& old_bounds,
    const gfx::Rect& new_bounds,
    ui::PropertyChangeReason reason) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(desktop_window_ && window == desktop_window_);

  // Post a task to update capture size after first returning to the event loop.
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&AuraWindowCaptureMachine::UpdateCaptureSize,
                     weak_factory_.GetWeakPtr()));
}

void AuraWindowCaptureMachine::OnWindowDestroying(aura::Window* window) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  InternalStop(base::DoNothing());

  oracle_proxy_->ReportError(FROM_HERE, "OnWindowDestroying()");
}

void AuraWindowCaptureMachine::OnWindowAddedToRootWindow(
    aura::Window* window) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(window == desktop_window_);

  if (aura::WindowTreeHost* host = window->GetHost()) {
    if (ui::Compositor* compositor = host->compositor())
      compositor->AddAnimationObserver(this);
  }
}

void AuraWindowCaptureMachine::OnWindowRemovingFromRootWindow(
    aura::Window* window,
    aura::Window* new_root) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(window == desktop_window_);

  if (aura::WindowTreeHost* host = window->GetHost()) {
    if (ui::Compositor* compositor = host->compositor()) {
      compositor->RemoveAnimationObserver(this);
      compositor->context_factory()->RemoveObserver(this);
    }
  }
}

void AuraWindowCaptureMachine::OnAnimationStep(base::TimeTicks timestamp) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!timestamp.is_null());

  // HACK: The compositor invokes this observer method to step layer animation
  // forward. Scheduling frame capture was not the intention, and so invoking
  // this method does not actually indicate the content has changed. However,
  // this is the only reliable way to ensure all screen changes are captured, as
  // of this writing.
  // http://crbug.com/600031
  //
  // TODO(miu): Need a better observer callback interface from the compositor
  // for this use case. The solution here will always capture frames at the
  // maximum framerate, which means CPU/GPU is being wasted on redundant
  // captures and quality/smoothness of animating content will suffer
  // significantly.
  // http://crbug.com/492839
  if (frame_capture_active_)
    Capture(timestamp);
}

void AuraWindowCaptureMachine::OnCompositingShuttingDown(
    ui::Compositor* compositor) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  compositor->RemoveAnimationObserver(this);
  compositor->context_factory()->RemoveObserver(this);
}

void AuraWindowCaptureMachine::OnLostResources() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  yuv_readback_pipeline_.reset();
}

}  // namespace content
