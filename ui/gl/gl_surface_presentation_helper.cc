// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gl/gl_surface_presentation_helper.h"

#include "base/threading/thread_task_runner_handle.h"
#include "ui/gfx/vsync_provider.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gpu_timing.h"

namespace gl {

GLSurfacePresentationHelper::ScopedSwapBuffers::ScopedSwapBuffers(
    GLSurfacePresentationHelper* helper,
    const GLSurface::PresentationCallback& callback)
    : helper_(helper) {
  if (helper_)
    helper_->PreSwapBuffers(callback);
}

GLSurfacePresentationHelper::ScopedSwapBuffers::~ScopedSwapBuffers() {
  if (helper_)
    helper_->PostSwapBuffers(result_);
}

GLSurfacePresentationHelper::Frame::Frame(Frame&& other) = default;

GLSurfacePresentationHelper::Frame::Frame(
    std::unique_ptr<GPUTimer>&& timer,
    const GLSurface::PresentationCallback& callback)
    : timer(std::move(timer)), callback(callback) {}

GLSurfacePresentationHelper::Frame::~Frame() = default;

GLSurfacePresentationHelper::Frame& GLSurfacePresentationHelper::Frame::
operator=(Frame&& other) = default;

GLSurfacePresentationHelper::GLSurfacePresentationHelper(
    gfx::VSyncProvider* vsync_provider)
    : vsync_provider_(vsync_provider), weak_ptr_factory_(this) {}

GLSurfacePresentationHelper::GLSurfacePresentationHelper(
    base::TimeTicks timebase,
    base::TimeDelta interval)
    : vsync_provider_(nullptr),
      vsync_timebase_(timebase),
      vsync_interval_(interval),
      weak_ptr_factory_(this) {}

GLSurfacePresentationHelper::~GLSurfacePresentationHelper() {
  // Discard pending frames and run presentation callback with empty
  // PresentationFeedback.
  bool has_context = gl_context_ && gl_context_->IsCurrent(surface_);
  for (auto& frame : pending_frames_) {
    if (frame.timer)
      frame.timer->Destroy(has_context);
    frame.callback.Run(gfx::PresentationFeedback());
  }
  pending_frames_.clear();
}

void GLSurfacePresentationHelper::OnMakeCurrent(GLContext* context,
                                                GLSurface* surface) {
  DCHECK(context);
  DCHECK(surface);
  DCHECK(surface == surface_ || !surface_);
  if (context == gl_context_)
    return;

  surface_ = surface;
  // If context is changed, we assume SwapBuffers issued for previous context
  // will be discarded.
  if (gpu_timing_client_) {
    gpu_timing_client_ = nullptr;
    for (auto& frame : pending_frames_) {
      frame.timer->Destroy(false /* has_context */);
      frame.callback.Run(gfx::PresentationFeedback());
    }
    pending_frames_.clear();
  }

  gl_context_ = context;
  gpu_timing_client_ = context->CreateGPUTimingClient();
  if (!gpu_timing_client_->IsAvailable())
    gpu_timing_client_ = nullptr;
}

void GLSurfacePresentationHelper::PreSwapBuffers(
    const GLSurface::PresentationCallback& callback) {
  std::unique_ptr<GPUTimer> timer;
  if (gpu_timing_client_) {
    timer = gpu_timing_client_->CreateGPUTimer(false /* prefer_elapsed_time */);
    timer->QueryTimeStamp();
  }
  pending_frames_.push_back(Frame(std::move(timer), callback));
}

void GLSurfacePresentationHelper::PostSwapBuffers(gfx::SwapResult result) {
  DCHECK(!pending_frames_.empty());
  auto& frame = pending_frames_.back();
  frame.result = result;
  ScheduleCheckPendingFrames(false /* align_with_next_vsync */);
}

void GLSurfacePresentationHelper::CheckPendingFrames() {
  DCHECK(gl_context_ || pending_frames_.empty());

  if (vsync_provider_ &&
      vsync_provider_->SupportGetVSyncParametersIfAvailable()) {
    if (!vsync_provider_->GetVSyncParametersIfAvailable(&vsync_timebase_,
                                                        &vsync_interval_)) {
      vsync_timebase_ = base::TimeTicks();
      vsync_interval_ = base::TimeDelta();
      LOG(ERROR) << "GetVSyncParametersIfAvailable() failed!";
    }
  }

  if (pending_frames_.empty())
    return;

  // If the |gl_context_| is not current anymore, we assume SwapBuffers issued
  // for previous context will be discarded.
  if (gl_context_ && !gl_context_->IsCurrent(surface_)) {
    gpu_timing_client_ = nullptr;
    for (auto& frame : pending_frames_) {
      if (frame.timer)
        frame.timer->Destroy(false /* has_context */);
      frame.callback.Run(gfx::PresentationFeedback());
    }
    pending_frames_.clear();
    return;
  }

  bool need_update_vsync = false;
  bool disjoint_occurred =
      gpu_timing_client_ && gpu_timing_client_->CheckAndResetTimerErrors();
  if (disjoint_occurred || !gpu_timing_client_) {
    // If GPUTimer is not avaliable or disjoint occurred, we will compute the
    // next VSync's timestamp and use it to run presentation callback.
    uint32_t flags = 0;
    auto timestamp = base::TimeTicks::Now();
    if (!vsync_interval_.is_zero()) {
      timestamp = timestamp.SnappedToNextTick(vsync_timebase_, vsync_interval_);
      flags = gfx::PresentationFeedback::kVSync;
    }
    gfx::PresentationFeedback feedback(timestamp, vsync_interval_, flags);
    for (auto& frame : pending_frames_) {
      if (frame.timer)
        frame.timer->Destroy(true /* has_context */);
      if (frame.result == gfx::SwapResult::SWAP_ACK)
        frame.callback.Run(feedback);
      else
        frame.callback.Run(gfx::PresentationFeedback());
    }
    pending_frames_.clear();
    // We want to update VSync, if we can not get VSync parameters
    // synchronously. Otherwise we will update the VSync parameters with the
    // next SwapBuffers.
    if (vsync_provider_ &&
        !vsync_provider_->SupportGetVSyncParametersIfAvailable()) {
      need_update_vsync = true;
    }
  }

  const bool fixed_vsync = !vsync_provider_;
  const bool hw_clock = vsync_provider_ && vsync_provider_->IsHWClock();

  while (!pending_frames_.empty()) {
    auto& frame = pending_frames_.front();
    // Helper lambda for running the presentation callback and releasing the
    // frame.
    auto frame_presentation_callback =
        [this, &frame](const gfx::PresentationFeedback& feedback) {
          frame.timer->Destroy(true /* has_context */);
          frame.callback.Run(feedback);
          pending_frames_.pop_front();
        };

    if (frame.result != gfx::SwapResult::SWAP_ACK) {
      frame_presentation_callback(gfx::PresentationFeedback());
      continue;
    }

    if (!frame.timer->IsAvailable())
      break;

    int64_t start = 0;
    int64_t end = 0;
    frame.timer->GetStartEndTimestamps(&start, &end);
    auto timestamp =
        base::TimeTicks() + base::TimeDelta::FromMicroseconds(start);


    if (vsync_interval_.is_zero() || fixed_vsync) {
      // If VSync parameters are fixed or not avaliable, we just run
      // presentation callbacks with timestamp from GPUTimers.
      frame_presentation_callback(
          gfx::PresentationFeedback(timestamp, vsync_interval_, 0 /* flags */));
    } else if (timestamp < vsync_timebase_) {
      // We got a VSync whose timestamp is after GPU finished renderering this
      // back buffer.
      uint32_t flags = gfx::PresentationFeedback::kVSync |
                       gfx::PresentationFeedback::kHWCompletion;
      auto delta = vsync_timebase_ - timestamp;
      if (delta < vsync_interval_) {
        // The |vsync_timebase_| is the closest VSync's timestamp after the GPU
        // finished renderering.
        timestamp = vsync_timebase_;
        if (hw_clock)
          flags |= gfx::PresentationFeedback::kHWClock;
      } else {
        // The |vsync_timebase_| isn't the closest VSync's timestamp after the
        // GPU finished renderering. We have to compute the closest VSync's
        // timestmp.
        timestamp =
            timestamp.SnappedToNextTick(vsync_timebase_, vsync_interval_);
      }
      frame_presentation_callback(
          gfx::PresentationFeedback(timestamp, vsync_interval_, flags));
    } else {
      // The |vsync_timebase_| is earlier than |timestamp|, we will compute the
      // next vSync's timestamp and use it to run callback.
      uint32_t flags = 0;
      if (!vsync_interval_.is_zero()) {
        timestamp =
            timestamp.SnappedToNextTick(vsync_timebase_, vsync_interval_);
        flags = gfx::PresentationFeedback::kVSync;
      }
      frame_presentation_callback(
          gfx::PresentationFeedback(timestamp, vsync_interval_, flags));
    }
  }

  if (pending_frames_.empty() && !need_update_vsync)
    return;
  ScheduleCheckPendingFrames(true /* align_with_next_vsync */);
}

void GLSurfacePresentationHelper::CheckPendingFramesCallback() {
  DCHECK(check_pending_frame_scheduled_);
  check_pending_frame_scheduled_ = false;
  CheckPendingFrames();
}

void GLSurfacePresentationHelper::UpdateVSyncCallback(
    const base::TimeTicks timebase,
    const base::TimeDelta interval) {
  DCHECK(check_pending_frame_scheduled_);
  check_pending_frame_scheduled_ = false;
  vsync_timebase_ = timebase;
  vsync_interval_ = interval;
  CheckPendingFrames();
}

void GLSurfacePresentationHelper::ScheduleCheckPendingFrames(
    bool align_with_next_vsync) {
  if (check_pending_frame_scheduled_)
    return;
  check_pending_frame_scheduled_ = true;

  if (!align_with_next_vsync) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(&GLSurfacePresentationHelper::CheckPendingFramesCallback,
                       weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  if (vsync_provider_ &&
      !vsync_provider_->SupportGetVSyncParametersIfAvailable()) {
    // In this case, the |vsync_provider_| will call the callback when the next
    // VSync is received.
    vsync_provider_->GetVSyncParameters(
        base::BindRepeating(&GLSurfacePresentationHelper::UpdateVSyncCallback,
                            weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  // If the |vsync_provider_| can not notify us for the next VSync
  // asynchronically, we have to compute the next VSync time and post a delayed
  // task so we can check the VSync later.
  base::TimeDelta interval = vsync_interval_.is_zero()
                                 ? base::TimeDelta::FromSeconds(1) / 60
                                 : vsync_interval_;
  auto now = base::TimeTicks::Now();
  auto next_vsync = now.SnappedToNextTick(vsync_timebase_, interval);
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&GLSurfacePresentationHelper::CheckPendingFramesCallback,
                     weak_ptr_factory_.GetWeakPtr()),
      next_vsync - now);
}

}  // namespace gl
