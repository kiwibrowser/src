// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display_embedder/external_begin_frame_controller_impl.h"

namespace viz {

ExternalBeginFrameControllerImpl::ExternalBeginFrameControllerImpl(
    mojom::ExternalBeginFrameControllerAssociatedRequest controller_request,
    mojom::ExternalBeginFrameControllerClientPtr client)
    : binding_(this, std::move(controller_request)),
      client_(std::move(client)),
      begin_frame_source_(this) {}

ExternalBeginFrameControllerImpl::~ExternalBeginFrameControllerImpl() = default;

void ExternalBeginFrameControllerImpl::IssueExternalBeginFrame(
    const BeginFrameArgs& args) {
  begin_frame_source_.OnBeginFrame(args);

  // Ensure that Display will receive the BeginFrame (as a missed one), even
  // if it doesn't currently need it. This way, we ensure that
  // OnDisplayDidFinishFrame will be called for this BeginFrame.
  DCHECK(display_);
  display_->SetNeedsOneBeginFrame();
}

void ExternalBeginFrameControllerImpl::OnNeedsBeginFrames(
    bool needs_begin_frames) {
  needs_begin_frames_ = needs_begin_frames;
  client_->OnNeedsBeginFrames(needs_begin_frames_);
}

void ExternalBeginFrameControllerImpl::OnDisplayDidFinishFrame(
    const BeginFrameAck& ack) {
  client_->OnDisplayDidFinishFrame(ack);
}

void ExternalBeginFrameControllerImpl::SetDisplay(Display* display) {
  if (display_)
    display_->RemoveObserver(this);
  display_ = display;
  if (display_)
    display_->AddObserver(this);
}

}  // namespace viz
