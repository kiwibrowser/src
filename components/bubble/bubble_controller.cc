// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/bubble/bubble_controller.h"

#include <utility>

#include "components/bubble/bubble_delegate.h"
#include "components/bubble/bubble_manager.h"
#include "components/bubble/bubble_ui.h"

BubbleController::BubbleController(BubbleManager* manager,
                                   std::unique_ptr<BubbleDelegate> delegate)
    : manager_(manager), delegate_(std::move(delegate)) {
  DCHECK(manager_);
  DCHECK(delegate_);
}

BubbleController::~BubbleController() {
  if (bubble_ui_)
    ShouldClose(BUBBLE_CLOSE_FORCED);
}

bool BubbleController::CloseBubble(BubbleCloseReason reason) {
  DCHECK(thread_checker_.CalledOnValidThread());
  return manager_->CloseBubble(this->AsWeakPtr(), reason);
}

bool BubbleController::UpdateBubbleUi() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!bubble_ui_)
    return false;
  return delegate_->UpdateBubbleUi(bubble_ui_.get());
}

std::string BubbleController::GetName() const {
  return delegate_->GetName();
}

base::TimeDelta BubbleController::GetVisibleTime() const {
  return base::TimeTicks::Now() - show_time_;
}

void BubbleController::Show() {
  DCHECK(!bubble_ui_);
  bubble_ui_ = delegate_->BuildBubbleUi();
  DCHECK(bubble_ui_);
  bubble_ui_->Show(AsWeakPtr());
  show_time_ = base::TimeTicks::Now();
}

void BubbleController::UpdateAnchorPosition() {
  DCHECK(bubble_ui_);
  bubble_ui_->UpdateAnchorPosition();
}

bool BubbleController::ShouldClose(BubbleCloseReason reason) const {
  DCHECK(bubble_ui_);
  return delegate_->ShouldClose(reason) || reason == BUBBLE_CLOSE_FORCED;
}

bool BubbleController::OwningFrameIs(
    const content::RenderFrameHost* frame) const {
  DCHECK(bubble_ui_);
  return delegate_->OwningFrame() == frame;
}

void BubbleController::DoClose(BubbleCloseReason reason) {
  DCHECK(bubble_ui_);
  bubble_ui_->Close();
  bubble_ui_.reset();
  delegate_->DidClose(reason);
}
