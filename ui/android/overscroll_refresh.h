// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_ANDROID_OVERSCROLL_REFRESH_H_
#define UI_ANDROID_OVERSCROLL_REFRESH_H_

#include "base/macros.h"
#include "ui/android/ui_android_export.h"
#include "ui/gfx/geometry/size_f.h"
#include "ui/gfx/geometry/vector2d_f.h"

namespace ui {

class OverscrollRefreshHandler;

// Simple pull-to-refresh styled effect. Listens to scroll events, conditionally
// activating when:
//   1) The scroll begins when the page's root layer 1) has no vertical scroll
//      offset and 2) lacks the overflow-y:hidden property.
//   2) The page doesn't consume the initial scroll events.
//   3) The initial scroll direction is upward.
// The actuall pull response, animation and action are delegated to the
// provided refresh handler.
class UI_ANDROID_EXPORT OverscrollRefresh {
 public:
  // Minmum number of overscrolling pull events required to activate the effect.
  // Useful for avoiding accidental triggering when a scroll janks (is delayed),
  // capping the impulse per event.
  enum { kMinPullsToActivate = 3 };

  explicit OverscrollRefresh(OverscrollRefreshHandler* handler);

  virtual ~OverscrollRefresh();

  // Scroll event stream listening methods.
  void OnScrollBegin();
  // Returns whether the refresh was activated.
  void OnScrollEnd(const gfx::Vector2dF& velocity);

  // Scroll ack listener. The effect will only be activated if |can_navigate|
  // is true which happens when the scroll update is not consumed and the
  // overscroll_behavior on y axis is 'auto'.
  // This method is made virtual for mocking.
  virtual void OnOverscrolled();

  // Returns true if the effect has consumed the |scroll_delta|.
  bool WillHandleScrollUpdate(const gfx::Vector2dF& scroll_delta);

  // Release the effect (if active), preventing any associated refresh action.
  void ReleaseWithoutActivation();

  // Notify the effect of the latest scroll offset and overflow properties.
  // The effect will be disabled when the offset is non-zero or overflow is
  // hidden. Note: All dimensions are in device pixels.
  void OnFrameUpdated(const gfx::Vector2dF& content_scroll_offset,
                      bool root_overflow_y_hidden);

  // Reset the effect to its inactive state, immediately detaching and
  // disabling any active effects.
  // This method is made virtual for mocking.
  virtual void Reset();

  // Returns true if the refresh effect is either being manipulated or animated.
  // This method is made virtual for mocking.
  virtual bool IsActive() const;

  // Returns true if the effect is waiting for an unconsumed scroll to start.
  // This method is made virtual for mocking.
  virtual bool IsAwaitingScrollUpdateAck() const;

 protected:
  // This constructor is for mocking only.
  OverscrollRefresh();

 private:
  void Release(bool allow_refresh);

  bool scrolled_to_top_;
  bool overflow_y_hidden_;

  enum ScrollConsumptionState {
    DISABLED,
    AWAITING_SCROLL_UPDATE_ACK,
    ENABLED,
  } scroll_consumption_state_;

  OverscrollRefreshHandler* const handler_;

  DISALLOW_COPY_AND_ASSIGN(OverscrollRefresh);
};

}  // namespace ui

#endif  // UI_ANDROID_OVERSCROLL_REFRESH_H_
