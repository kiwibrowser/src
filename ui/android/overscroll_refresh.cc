// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/android/overscroll_refresh.h"

#include "base/logging.h"
#include "ui/android/overscroll_refresh_handler.h"

namespace ui {
namespace {

// Experimentally determined constant used to allow activation even if touch
// release results in a small upward fling (quite common during a slow scroll).
const float kMinFlingVelocityForActivation = -500.f;

}  // namespace

OverscrollRefresh::OverscrollRefresh(OverscrollRefreshHandler* handler)
    : scrolled_to_top_(true),
      overflow_y_hidden_(false),
      scroll_consumption_state_(DISABLED),
      handler_(handler) {
  DCHECK(handler);
}

OverscrollRefresh::OverscrollRefresh()
    : scrolled_to_top_(true),
      overflow_y_hidden_(false),
      scroll_consumption_state_(DISABLED),
      handler_(nullptr) {}

OverscrollRefresh::~OverscrollRefresh() {
}

void OverscrollRefresh::Reset() {
  scroll_consumption_state_ = DISABLED;
  handler_->PullReset();
}

void OverscrollRefresh::OnScrollBegin() {
  ReleaseWithoutActivation();
  if (scrolled_to_top_ && !overflow_y_hidden_)
    scroll_consumption_state_ = AWAITING_SCROLL_UPDATE_ACK;
}

void OverscrollRefresh::OnScrollEnd(const gfx::Vector2dF& scroll_velocity) {
  bool allow_activation = scroll_velocity.y() > kMinFlingVelocityForActivation;
  Release(allow_activation);
}

void OverscrollRefresh::OnOverscrolled() {
  if (scroll_consumption_state_ != AWAITING_SCROLL_UPDATE_ACK)
    return;

  scroll_consumption_state_ = handler_->PullStart() ? ENABLED : DISABLED;
}

bool OverscrollRefresh::WillHandleScrollUpdate(
    const gfx::Vector2dF& scroll_delta) {
  switch (scroll_consumption_state_) {
    case DISABLED:
      return false;

    case AWAITING_SCROLL_UPDATE_ACK:
      // If the initial scroll motion is downward, never allow activation.
      if (scroll_delta.y() <= 0)
        scroll_consumption_state_ = DISABLED;
      return false;

    case ENABLED:
      handler_->PullUpdate(scroll_delta.y());
      return true;
  }

  NOTREACHED() << "Invalid overscroll state: " << scroll_consumption_state_;
  return false;
}

void OverscrollRefresh::ReleaseWithoutActivation() {
  bool allow_activation = false;
  Release(allow_activation);
}

bool OverscrollRefresh::IsActive() const {
  return scroll_consumption_state_ == ENABLED;
}

bool OverscrollRefresh::IsAwaitingScrollUpdateAck() const {
  return scroll_consumption_state_ == AWAITING_SCROLL_UPDATE_ACK;
}

void OverscrollRefresh::OnFrameUpdated(
    const gfx::Vector2dF& content_scroll_offset,
    bool root_overflow_y_hidden) {
  scrolled_to_top_ = content_scroll_offset.y() == 0;
  overflow_y_hidden_ = root_overflow_y_hidden;
}

void OverscrollRefresh::Release(bool allow_refresh) {
  if (scroll_consumption_state_ == ENABLED)
    handler_->PullRelease(allow_refresh);
  scroll_consumption_state_ = DISABLED;
}

}  // namespace ui
