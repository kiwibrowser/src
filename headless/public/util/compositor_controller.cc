// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/public/util/compositor_controller.h"

#include <memory>

#include "base/base64.h"
#include "base/bind.h"
#include "base/cancelable_callback.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "headless/public/util/virtual_time_controller.h"

namespace headless {

// Sends BeginFrames to advance animations while virtual time advances in
// intervals.
class CompositorController::AnimationBeginFrameTask
    : public VirtualTimeController::RepeatingTask,
      public VirtualTimeController::ResumeDeferrer {
 public:
  explicit AnimationBeginFrameTask(CompositorController* compositor_controller)
      : RepeatingTask(StartPolicy::START_IMMEDIATELY, -1),
        compositor_controller_(compositor_controller),
        weak_ptr_factory_(this) {}

  // VirtualTimeController::RepeatingTask implementation:
  void IntervalElapsed(
      base::TimeDelta virtual_time_offset,
      base::OnceCallback<void(ContinuePolicy)> continue_callback) override {
    needs_begin_frame_on_virtual_time_resume_ = true;
    std::move(continue_callback).Run(ContinuePolicy::NOT_REQUIRED);
  }

  // VirtualTimeController::ResumeDeferrer implementation:
  void DeferResume(base::OnceClosure continue_callback) override {
    // Run a BeginFrame if we scheduled one in the last interval and no other
    // BeginFrame was sent while virtual time was paused.
    if (needs_begin_frame_on_virtual_time_resume_) {
      continue_callback_ = std::move(continue_callback);
      IssueAnimationBeginFrame();
      return;
    }
    std::move(continue_callback).Run();
  }

  void CompositorControllerIssuingScreenshotBeginFrame() {
    TRACE_EVENT0("headless",
                 "CompositorController::AnimationBeginFrameTask::"
                 "CompositorControllerIssuingScreenshotBeginFrame");
    // The screenshotting BeginFrame will replace our animation-only BeginFrame.
    // We cancel any pending animation BeginFrame to avoid sending two
    // BeginFrames within the same virtual time pause.
    needs_begin_frame_on_virtual_time_resume_ = false;
  }

 private:
  void IssueAnimationBeginFrame() {
    TRACE_EVENT0("headless",
                 "CompositorController::AnimationBeginFrameTask::"
                 "IssueAnimationBeginFrame");
    needs_begin_frame_on_virtual_time_resume_ = false;

    bool update_display =
        compositor_controller_->update_display_for_animations_;
    // Display needs to be updated for first BeginFrame. Otherwise, the
    // RenderWidget's surface may not be created and the root surface may block
    // waiting for it forever.
    update_display |=
        compositor_controller_->last_begin_frame_time_ == base::TimeTicks();

    compositor_controller_->PostBeginFrame(
        base::BindOnce(&AnimationBeginFrameTask::BeginFrameComplete,
                       weak_ptr_factory_.GetWeakPtr()),
        !update_display);
  }

  void BeginFrameComplete(std::unique_ptr<BeginFrameResult>) {
    TRACE_EVENT0(
        "headless",
        "CompositorController::AnimationBeginFrameTask::BeginFrameComplete");
    DCHECK(continue_callback_);
    std::move(continue_callback_).Run();
  }

  CompositorController* compositor_controller_;  // NOT OWNED
  bool needs_begin_frame_on_virtual_time_resume_ = true;
  base::CancelableClosure begin_frame_task_;

  base::OnceClosure continue_callback_;
  base::WeakPtrFactory<AnimationBeginFrameTask> weak_ptr_factory_;
};

CompositorController::CompositorController(
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    HeadlessDevToolsClient* devtools_client,
    VirtualTimeController* virtual_time_controller,
    base::TimeDelta animation_begin_frame_interval,
    bool update_display_for_animations)
    : task_runner_(std::move(task_runner)),
      devtools_client_(devtools_client),
      virtual_time_controller_(virtual_time_controller),
      animation_task_(std::make_unique<AnimationBeginFrameTask>(this)),
      animation_begin_frame_interval_(animation_begin_frame_interval),
      update_display_for_animations_(update_display_for_animations),
      weak_ptr_factory_(this) {
  devtools_client_->GetHeadlessExperimental()->GetExperimental()->AddObserver(
      this);
  // No need to wait for completion of this, since we are waiting for the
  // setNeedsBeginFramesChanged event instead, which will be sent at some point
  // after enabling the domain.
  devtools_client_->GetHeadlessExperimental()->GetExperimental()->Enable(
      headless_experimental::EnableParams::Builder().Build());
  virtual_time_controller_->ScheduleRepeatingTask(
      animation_task_.get(), animation_begin_frame_interval_);
  virtual_time_controller_->SetResumeDeferrer(animation_task_.get());
}

CompositorController::~CompositorController() {
  virtual_time_controller_->CancelRepeatingTask(animation_task_.get());
  virtual_time_controller_->SetResumeDeferrer(nullptr);
  devtools_client_->GetHeadlessExperimental()
      ->GetExperimental()
      ->RemoveObserver(this);
}

void CompositorController::PostBeginFrame(
    base::OnceCallback<void(std::unique_ptr<BeginFrameResult>)>
        begin_frame_complete_callback,
    bool no_display_updates,
    std::unique_ptr<ScreenshotParams> screenshot) {
  // In certain nesting situations, we should not issue a BeginFrame immediately
  // - for example, issuing a new BeginFrame within a BeginFrameCompleted or
  // NeedsBeginFramesChanged event can upset the compositor. We avoid these
  // situations by issuing our BeginFrames from a separately posted task.
  task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&CompositorController::BeginFrame,
                                weak_ptr_factory_.GetWeakPtr(),
                                std::move(begin_frame_complete_callback),
                                no_display_updates, std::move(screenshot)));
}

void CompositorController::BeginFrame(
    base::OnceCallback<void(std::unique_ptr<BeginFrameResult>)>
        begin_frame_complete_callback,
    bool no_display_updates,
    std::unique_ptr<ScreenshotParams> screenshot) {
  DCHECK(!begin_frame_complete_callback_);
  begin_frame_complete_callback_ = std::move(begin_frame_complete_callback);
  if (needs_begin_frames_ || screenshot) {
    auto params_builder = headless_experimental::BeginFrameParams::Builder();

    // Use virtual time for frame time, so that rendering of animations etc. is
    // aligned with virtual time progression.
    base::TimeTicks frame_time =
        virtual_time_controller_->GetCurrentVirtualTime();
    if (frame_time <= last_begin_frame_time_) {
      // Frame time cannot go backwards or stop, so we issue another BeginFrame
      // with a small time offset from the last BeginFrame's time instead.
      frame_time =
          last_begin_frame_time_ + base::TimeDelta::FromMicroseconds(1);
    }
    params_builder.SetFrameTimeTicks(
        (frame_time - base::TimeTicks()).InMillisecondsF());
    DCHECK_GT(frame_time, last_begin_frame_time_);
    last_begin_frame_time_ = frame_time;

    params_builder.SetInterval(
        animation_begin_frame_interval_.InMillisecondsF());

    params_builder.SetNoDisplayUpdates(no_display_updates);

    if (screenshot)
      params_builder.SetScreenshot(std::move(screenshot));

    devtools_client_->GetHeadlessExperimental()->GetExperimental()->BeginFrame(
        params_builder.Build(),
        base::BindOnce(&CompositorController::BeginFrameComplete,
                       weak_ptr_factory_.GetWeakPtr()));
  } else {
    BeginFrameComplete(nullptr);
  }
}

void CompositorController::BeginFrameComplete(
    std::unique_ptr<BeginFrameResult> result) {
  std::move(begin_frame_complete_callback_).Run(std::move(result));
  if (idle_callback_)
    std::move(idle_callback_).Run();
}

void CompositorController::OnNeedsBeginFramesChanged(
    const NeedsBeginFramesChangedParams& params) {
  needs_begin_frames_ = params.GetNeedsBeginFrames();
}

void CompositorController::WaitUntilIdle(base::OnceClosure idle_callback) {
  TRACE_EVENT_INSTANT1("headless", "CompositorController::WaitUntilIdle",
                       TRACE_EVENT_SCOPE_THREAD, "begin_frame_in_flight",
                       !!begin_frame_complete_callback_);
  DCHECK(!idle_callback_);

  if (!begin_frame_complete_callback_) {
    std::move(idle_callback).Run();
    return;
  }

  idle_callback_ = std::move(idle_callback);
}

void CompositorController::CaptureScreenshot(
    ScreenshotParamsFormat format,
    int quality,
    base::OnceCallback<void(const std::string&)> screenshot_captured_callback) {
  TRACE_EVENT0("headless", "CompositorController::CaptureScreenshot");
  DCHECK(!begin_frame_complete_callback_);
  DCHECK(!screenshot_captured_callback_);

  screenshot_captured_callback_ = std::move(screenshot_captured_callback);

  // Let AnimationBeginFrameTask know that it doesn't need to issue an
  // animation BeginFrame for the current virtual time pause.
  animation_task_->CompositorControllerIssuingScreenshotBeginFrame();

  const bool no_display_updates = false;
  PostBeginFrame(
      base::BindOnce(&CompositorController::CaptureScreenshotBeginFrameComplete,
                     weak_ptr_factory_.GetWeakPtr()),
      no_display_updates,
      ScreenshotParams::Builder()
          .SetFormat(format)
          .SetQuality(quality)
          .Build());
}

void CompositorController::CaptureScreenshotBeginFrameComplete(
    std::unique_ptr<BeginFrameResult> result) {
  TRACE_EVENT1(
      "headless", "CompositorController::CaptureScreenshotBeginFrameComplete",
      "hasScreenshotData",
      result ? std::to_string(result->HasScreenshotData()) : "invalid");
  DCHECK(screenshot_captured_callback_);
  if (result && result->HasScreenshotData()) {
    // TODO(eseckler): Look into returning binary screenshot data via DevTools.
    std::string decoded_data;
    base::Base64Decode(result->GetScreenshotData(), &decoded_data);
    std::move(screenshot_captured_callback_).Run(decoded_data);
  } else {
    LOG(ERROR) << "Screenshotting failed, BeginFrameResult has no data and "
                  "hasDamage is "
               << (result ? std::to_string(result->HasScreenshotData())
                          : "invalid");
    std::move(screenshot_captured_callback_).Run(std::string());
  }
}

}  // namespace headless
