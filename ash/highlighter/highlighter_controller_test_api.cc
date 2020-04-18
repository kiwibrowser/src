// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/highlighter/highlighter_controller_test_api.h"

#include "ash/components/fast_ink/fast_ink_points.h"
#include "ash/highlighter/highlighter_controller.h"
#include "ash/highlighter/highlighter_view.h"

namespace ash {

HighlighterControllerTestApi::HighlighterControllerTestApi(
    HighlighterController* instance)
    : binding_(this), instance_(instance) {
  AttachClient();
}

HighlighterControllerTestApi::~HighlighterControllerTestApi() {
  if (binding_.is_bound())
    DetachClient();
  if (instance_->enabled())
    instance_->SetEnabled(false);
  instance_->DestroyPointerView();
}

void HighlighterControllerTestApi::AttachClient() {
  DCHECK(!binding_.is_bound());
  DCHECK(!highlighter_controller_);
  instance_->BindRequest(mojo::MakeRequest(&highlighter_controller_));
  ash::mojom::HighlighterControllerClientPtr client;
  binding_.Bind(mojo::MakeRequest(&client));
  highlighter_controller_->SetClient(std::move(client));
  highlighter_controller_.FlushForTesting();
}

void HighlighterControllerTestApi::DetachClient() {
  DCHECK(binding_.is_bound());
  DCHECK(highlighter_controller_);
  highlighter_controller_ = nullptr;
  binding_.Close();
  instance_->FlushMojoForTesting();
}

void HighlighterControllerTestApi::SetEnabled(bool enabled) {
  instance_->SetEnabled(enabled);
}

void HighlighterControllerTestApi::DestroyPointerView() {
  instance_->DestroyPointerView();
}

void HighlighterControllerTestApi::SimulateInterruptedStrokeTimeout() {
  if (!instance_->interrupted_stroke_timer_)
    return;
  instance_->interrupted_stroke_timer_->Stop();
  instance_->RecognizeGesture();
}

bool HighlighterControllerTestApi::IsShowingHighlighter() const {
  return instance_->highlighter_view_.get();
}

bool HighlighterControllerTestApi::IsFadingAway() const {
  return IsShowingHighlighter() && instance_->highlighter_view_->animating();
}

bool HighlighterControllerTestApi::IsShowingSelectionResult() const {
  return instance_->result_view_.get();
}

bool HighlighterControllerTestApi::IsWaitingToResumeStroke() const {
  return instance_->interrupted_stroke_timer_ &&
         instance_->interrupted_stroke_timer_->IsRunning();
}

const fast_ink::FastInkPoints& HighlighterControllerTestApi::points() const {
  return instance_->highlighter_view_->points_;
}

const fast_ink::FastInkPoints& HighlighterControllerTestApi::predicted_points()
    const {
  return instance_->highlighter_view_->predicted_points_;
}

bool HighlighterControllerTestApi::HandleEnabledStateChangedCalled() {
  instance_->FlushMojoForTesting();
  return handle_enabled_state_changed_called_;
}

bool HighlighterControllerTestApi::HandleSelectionCalled() {
  instance_->FlushMojoForTesting();
  return handle_selection_called_;
}

void HighlighterControllerTestApi::HandleSelection(const gfx::Rect& rect) {
  handle_selection_called_ = true;
  selection_ = rect;
}

void HighlighterControllerTestApi::HandleEnabledStateChange(bool enabled) {
  handle_enabled_state_changed_called_ = true;
  enabled_ = enabled;
}

}  // namespace ash
