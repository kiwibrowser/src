// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/content/screen_capture_device_core.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_checker.h"

namespace media {

namespace {

void DeleteCaptureMachine(
    std::unique_ptr<VideoCaptureMachine> capture_machine) {
  capture_machine.reset();
}

}  // namespace

VideoCaptureMachine::VideoCaptureMachine() = default;

VideoCaptureMachine::~VideoCaptureMachine() = default;

bool VideoCaptureMachine::IsAutoThrottlingEnabled() const {
  return false;
}

void ScreenCaptureDeviceCore::AllocateAndStart(
    const VideoCaptureParams& params,
    std::unique_ptr<VideoCaptureDevice::Client> client) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (state_ != kIdle) {
    DVLOG(1) << "Allocate() invoked when not in state Idle.";
    return;
  }

  if (params.requested_format.pixel_format != PIXEL_FORMAT_I420) {
    client->OnError(
        FROM_HERE,
        base::StringPrintf(
            "unsupported format: %s",
            VideoCaptureFormat::ToString(params.requested_format).c_str()));
    return;
  }

  oracle_proxy_ = new ThreadSafeCaptureOracle(
      std::move(client), params, capture_machine_->IsAutoThrottlingEnabled());

  capture_machine_->Start(
      oracle_proxy_, params,
      base::Bind(&ScreenCaptureDeviceCore::CaptureStarted, AsWeakPtr()));

  TransitionStateTo(kCapturing);
}

void ScreenCaptureDeviceCore::RequestRefreshFrame() {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (state_ != kCapturing)
    return;

  capture_machine_->MaybeCaptureForRefresh();
}

void ScreenCaptureDeviceCore::Suspend() {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (state_ != kCapturing)
    return;

  TransitionStateTo(kSuspended);

  capture_machine_->Suspend();
}

void ScreenCaptureDeviceCore::Resume() {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (state_ != kSuspended)
    return;

  TransitionStateTo(kCapturing);

  capture_machine_->Resume();
}

void ScreenCaptureDeviceCore::StopAndDeAllocate() {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (state_ != kCapturing && state_ != kSuspended)
    return;

  oracle_proxy_->Stop();
  oracle_proxy_ = NULL;

  TransitionStateTo(kIdle);

  capture_machine_->Stop(base::DoNothing());
}

void ScreenCaptureDeviceCore::OnConsumerReportingUtilization(
    int frame_feedback_id,
    double utilization) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(oracle_proxy_);
  oracle_proxy_->OnConsumerReportingUtilization(frame_feedback_id, utilization);
}

void ScreenCaptureDeviceCore::CaptureStarted(bool success) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!success)
    Error(FROM_HERE, "Failed to start capture machine.");
  else if (oracle_proxy_)
    oracle_proxy_->ReportStarted();
}

ScreenCaptureDeviceCore::ScreenCaptureDeviceCore(
    std::unique_ptr<VideoCaptureMachine> capture_machine)
    : state_(kIdle), capture_machine_(std::move(capture_machine)) {
  DCHECK(capture_machine_.get());
}

ScreenCaptureDeviceCore::~ScreenCaptureDeviceCore() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(state_ != kCapturing && state_ != kSuspended);
  if (capture_machine_) {
    capture_machine_->Stop(
        base::Bind(&DeleteCaptureMachine, base::Passed(&capture_machine_)));
  }
  DVLOG(1) << "ScreenCaptureDeviceCore@" << this << " destroying.";
}

void ScreenCaptureDeviceCore::TransitionStateTo(State next_state) {
  DCHECK(thread_checker_.CalledOnValidThread());

#ifndef NDEBUG
  static const char* kStateNames[] = {"Idle", "Capturing", "Suspended",
                                      "Error"};
  static_assert(arraysize(kStateNames) == kLastCaptureState,
                "Different number of states and textual descriptions");
  DVLOG(1) << "State change: " << kStateNames[state_] << " --> "
           << kStateNames[next_state];
#endif

  state_ = next_state;
}

void ScreenCaptureDeviceCore::Error(const base::Location& from_here,
                                    const std::string& reason) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (state_ == kIdle)
    return;

  if (oracle_proxy_)
    oracle_proxy_->ReportError(from_here, reason);

  StopAndDeAllocate();
  TransitionStateTo(kError);
}

}  // namespace media
