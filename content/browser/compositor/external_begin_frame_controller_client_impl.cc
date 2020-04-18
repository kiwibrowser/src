// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/compositor/external_begin_frame_controller_client_impl.h"

#include "ui/compositor/compositor.h"

namespace content {

ExternalBeginFrameControllerClientImpl::ExternalBeginFrameControllerClientImpl(
    ui::Compositor* compositor)
    : compositor_(compositor), binding_(this) {}

ExternalBeginFrameControllerClientImpl::
    ~ExternalBeginFrameControllerClientImpl() = default;

viz::mojom::ExternalBeginFrameControllerClientPtr
ExternalBeginFrameControllerClientImpl::GetBoundPtr() {
  viz::mojom::ExternalBeginFrameControllerClientPtr ptr;
  binding_.Bind(mojo::MakeRequest(&ptr));
  return ptr;
}

viz::mojom::ExternalBeginFrameControllerAssociatedRequest
ExternalBeginFrameControllerClientImpl::GetControllerRequest() {
  return mojo::MakeRequest(&controller_);
}

viz::mojom::ExternalBeginFrameControllerAssociatedPtr&
ExternalBeginFrameControllerClientImpl::GetController() {
  return controller_;
}

void ExternalBeginFrameControllerClientImpl::OnNeedsBeginFrames(
    bool needs_begin_frames) {
  compositor_->OnNeedsExternalBeginFrames(needs_begin_frames);
}

void ExternalBeginFrameControllerClientImpl::OnDisplayDidFinishFrame(
    const viz::BeginFrameAck& ack) {
  compositor_->OnDisplayDidFinishFrame(ack);
}

}  // namespace content
