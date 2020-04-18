// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/immersive/immersive_fullscreen_controller.h"

#include <set>

#include "ash/public/cpp/immersive/immersive_context.h"
#include "ash/public/cpp/immersive/immersive_focus_watcher.h"
#include "ash/public/cpp/immersive/immersive_fullscreen_controller_delegate.h"
#include "ash/public/cpp/immersive/immersive_gesture_handler.h"
#include "ash/public/cpp/immersive/immersive_handler_factory.h"
#include "base/metrics/histogram_macros.h"
#include "ui/aura/window.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/events/base_event_utils.h"
#include "ui/gfx/animation/slide_animation.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace ash {

namespace {

// Duration for the reveal show/hide slide animation. The slower duration is
// used for the initial slide out to give the user more change to see what
// happened.
const int kRevealSlowAnimationDurationMs = 400;
const int kRevealFastAnimationDurationMs = 200;

// The delay in milliseconds between the mouse stopping at the top edge of the
// screen and the top-of-window views revealing.
const int kMouseRevealDelayMs = 200;

// The maximum amount of pixels that the cursor can move for the cursor to be
// considered "stopped". This allows the user to reveal the top-of-window views
// without holding the cursor completely still.
const int kMouseRevealXThresholdPixels = 3;

// Used to multiply x value of an update in check to determine if gesture is
// vertical. This is used to make sure that gesture is close to vertical instead
// of just more vertical then horizontal.
const int kSwipeVerticalThresholdMultiplier = 3;

// The height in pixels of the region above the top edge of the display which
// hosts the immersive fullscreen window in which mouse events are ignored
// (cannot reveal or unreveal the top-of-window views).
// See ShouldIgnoreMouseEventAtLocation() for more details.
const int kHeightOfDeadRegionAboveTopContainer = 10;

}  // namespace

// static
const int ImmersiveFullscreenController::kImmersiveFullscreenTopEdgeInset = 8;

// static
const int ImmersiveFullscreenController::kMouseRevealBoundsHeight = 3;

////////////////////////////////////////////////////////////////////////////////

ImmersiveFullscreenController::ImmersiveFullscreenController()
    : delegate_(NULL),
      top_container_(NULL),
      widget_(NULL),
      observers_enabled_(false),
      enabled_(false),
      reveal_state_(CLOSED),
      revealed_lock_count_(0),
      mouse_x_when_hit_top_in_screen_(-1),
      gesture_begun_(false),
      animation_(new gfx::SlideAnimation(this)),
      animations_disabled_for_test_(false),
      weak_ptr_factory_(this) {}

ImmersiveFullscreenController::~ImmersiveFullscreenController() {
  EnableWindowObservers(false);
}

void ImmersiveFullscreenController::Init(
    ImmersiveFullscreenControllerDelegate* delegate,
    views::Widget* widget,
    views::View* top_container) {
  delegate_ = delegate;
  top_container_ = top_container;
  widget_ = widget;
  ImmersiveContext::Get()->InstallResizeHandleWindowTargeter(this);
}

void ImmersiveFullscreenController::SetEnabled(WindowType window_type,
                                               bool enabled) {
  if (enabled_ == enabled)
    return;
  enabled_ = enabled;

  EnableWindowObservers(enabled_);

  ImmersiveContext::Get()->OnEnteringOrExitingImmersive(this, enabled);

  if (enabled_) {
    // Animate enabling immersive mode by sliding out the top-of-window views.
    // No animation occurs if a lock is holding the top-of-window views open.

    // Do a reveal to set the initial state for the animation. (And any
    // required state in case the animation cannot run because of a lock holding
    // the top-of-window views open.)
    MaybeStartReveal(ANIMATE_NO);

    // Reset the located event so that it does not affect whether the
    // top-of-window views are hidden.
    located_event_revealed_lock_.reset();

    // Try doing the animation.
    MaybeEndReveal(ANIMATE_SLOW);

    if (reveal_state_ == REVEALED) {
      // Reveal was unsuccessful. Reacquire the revealed locks if appropriate.
      UpdateLocatedEventRevealedLock();
      if (immersive_focus_watcher_)
        immersive_focus_watcher_->UpdateFocusRevealedLock();
    }

    delegate_->OnImmersiveFullscreenEntered();
  } else {
    // Stop cursor-at-top tracking.
    top_edge_hover_timer_.Stop();
    reveal_state_ = CLOSED;

    delegate_->OnImmersiveFullscreenExited();
  }

  if (enabled_) {
    UMA_HISTOGRAM_ENUMERATION("Ash.ImmersiveFullscreen.WindowType", window_type,
                              WINDOW_TYPE_COUNT);
  }
}

bool ImmersiveFullscreenController::IsEnabled() const {
  return enabled_;
}

bool ImmersiveFullscreenController::IsRevealed() const {
  return enabled_ && reveal_state_ != CLOSED;
}

ImmersiveRevealedLock* ImmersiveFullscreenController::GetRevealedLock(
    AnimateReveal animate_reveal) {
  return new ImmersiveRevealedLock(weak_ptr_factory_.GetWeakPtr(),
                                   animate_reveal);
}

////////////////////////////////////////////////////////////////////////////////

void ImmersiveFullscreenController::OnMouseEvent(
    const ui::MouseEvent& event,
    const gfx::Point& location_in_screen,
    views::Widget* target) {
  if (!enabled_)
    return;

  if (event.type() != ui::ET_MOUSE_MOVED &&
      event.type() != ui::ET_MOUSE_PRESSED &&
      event.type() != ui::ET_MOUSE_RELEASED &&
      event.type() != ui::ET_MOUSE_CAPTURE_CHANGED) {
    return;
  }

  // Mouse hover can initiate revealing the top-of-window views while |widget_|
  // is inactive.

  if (reveal_state_ == SLIDING_OPEN || reveal_state_ == REVEALED) {
    top_edge_hover_timer_.Stop();
    UpdateLocatedEventRevealedLock(&event, location_in_screen);
  } else if (event.type() != ui::ET_MOUSE_CAPTURE_CHANGED) {
    // Trigger a reveal if the cursor pauses at the top of the screen for a
    // while.
    UpdateTopEdgeHoverTimer(event, location_in_screen, target);
  }
}

void ImmersiveFullscreenController::OnTouchEvent(
    const ui::TouchEvent& event,
    const gfx::Point& location_in_screen) {
  if (!enabled_ || event.type() != ui::ET_TOUCH_PRESSED)
    return;

  // Touch should not initiate revealing the top-of-window views while |widget_|
  // is inactive.
  if (!widget_->IsActive())
    return;

  UpdateLocatedEventRevealedLock(&event, location_in_screen);
}

void ImmersiveFullscreenController::OnGestureEvent(
    ui::GestureEvent* event,
    const gfx::Point& location_in_screen) {
  if (!enabled_)
    return;

  // Touch gestures should not initiate revealing the top-of-window views while
  // |widget_| is inactive.
  if (!widget_->IsActive())
    return;

  switch (event->type()) {
    case ui::ET_GESTURE_SCROLL_BEGIN:
      if (ShouldHandleGestureEvent(location_in_screen)) {
        gesture_begun_ = true;
        // Do not consume the event. Otherwise, we end up consuming all
        // ui::ET_GESTURE_SCROLL_BEGIN events in the top-of-window views
        // when the top-of-window views are revealed.
      }
      break;
    case ui::ET_GESTURE_SCROLL_UPDATE:
      if (gesture_begun_) {
        if (UpdateRevealedLocksForSwipe(GetSwipeType(*event)))
          event->SetHandled();
        gesture_begun_ = false;
      }
      break;
    case ui::ET_GESTURE_SCROLL_END:
    case ui::ET_SCROLL_FLING_START:
      gesture_begun_ = false;
      break;
    default:
      break;
  }
}

void ImmersiveFullscreenController::OnPointerEventObserved(
    const ui::PointerEvent& event,
    const gfx::Point& location_in_screen,
    gfx::NativeView target) {
  if (event.IsMousePointerEvent()) {
    if (event.type() == ui::ET_POINTER_WHEEL_CHANGED) {
      const ui::MouseWheelEvent mouse_wheel_event(event);
      OnMouseEvent(mouse_wheel_event, location_in_screen,
                   views::Widget::GetTopLevelWidgetForNativeView(target));
    } else {
      const ui::MouseEvent mouse_event(event);
      OnMouseEvent(mouse_event, location_in_screen,
                   views::Widget::GetTopLevelWidgetForNativeView(target));
    }
  } else {
    DCHECK(event.IsTouchPointerEvent());
    const ui::TouchEvent touch_event(event);
    OnTouchEvent(touch_event, location_in_screen);
  }
}

////////////////////////////////////////////////////////////////////////////////
// views::WidgetObserver overrides:

void ImmersiveFullscreenController::OnWidgetDestroying(views::Widget* widget) {
  EnableWindowObservers(false);

  // Set |enabled_| to false such that any calls to MaybeStartReveal() and
  // MaybeEndReveal() have no effect.
  enabled_ = false;
}

////////////////////////////////////////////////////////////////////////////////
// gfx::AnimationDelegate overrides:

void ImmersiveFullscreenController::AnimationEnded(
    const gfx::Animation* animation) {
  if (reveal_state_ == SLIDING_OPEN) {
    OnSlideOpenAnimationCompleted();
  } else if (reveal_state_ == SLIDING_CLOSED) {
    OnSlideClosedAnimationCompleted();
  }
}

void ImmersiveFullscreenController::AnimationProgressed(
    const gfx::Animation* animation) {
  delegate_->SetVisibleFraction(animation->GetCurrentValue());
}

////////////////////////////////////////////////////////////////////////////////
// ImmersiveRevealedLock::Delegate overrides:

void ImmersiveFullscreenController::LockRevealedState(
    AnimateReveal animate_reveal) {
  ++revealed_lock_count_;
  Animate animate =
      (animate_reveal == ANIMATE_REVEAL_YES) ? ANIMATE_FAST : ANIMATE_NO;
  MaybeStartReveal(animate);
}

void ImmersiveFullscreenController::UnlockRevealedState() {
  --revealed_lock_count_;
  DCHECK_GE(revealed_lock_count_, 0);
  if (revealed_lock_count_ == 0) {
    // Always animate ending the reveal fast.
    MaybeEndReveal(ANIMATE_FAST);
  }
}

////////////////////////////////////////////////////////////////////////////////
// private:

void ImmersiveFullscreenController::EnableWindowObservers(bool enable) {
  if (observers_enabled_ == enable)
    return;
  observers_enabled_ = enable;

  if (enable) {
    immersive_focus_watcher_ =
        ImmersiveHandlerFactory::Get()->CreateFocusWatcher(this);
    immersive_gesture_handler_ =
        ImmersiveHandlerFactory::Get()->CreateGestureHandler(this);
    widget_->AddObserver(this);
    ImmersiveContext::Get()->AddPointerWatcher(
        this, views::PointerWatcherEventTypes::MOVES);
  } else {
    ImmersiveContext::Get()->RemovePointerWatcher(this);
    widget_->RemoveObserver(this);
    immersive_gesture_handler_.reset();
    immersive_focus_watcher_.reset();

    animation_->Stop();
  }
}

bool ImmersiveFullscreenController::IsTargetForWidget(
    views::Widget* target) const {
  return target == widget_ || target == top_container_->GetWidget();
}

void ImmersiveFullscreenController::UpdateTopEdgeHoverTimer(
    const ui::MouseEvent& event,
    const gfx::Point& location_in_screen,
    views::Widget* target) {
  DCHECK(enabled_);
  DCHECK(reveal_state_ == SLIDING_CLOSED || reveal_state_ == CLOSED);

  // Check whether |widget_| is the event target instead of checking for
  // activation. This allows the timer to be started when |widget_| is inactive
  // but prevents starting the timer if the mouse is over a portion of the top
  // edge obscured by an unrelated widget.
  if (!top_edge_hover_timer_.IsRunning() && !IsTargetForWidget(target))
    return;

  // Mouse hover should not initiate revealing the top-of-window views while a
  // window has mouse capture.
  if (ImmersiveContext::Get()->DoesAnyWindowHaveCapture())
    return;

  if (ShouldIgnoreMouseEventAtLocation(location_in_screen))
    return;

  // Stop the timer if the cursor left the top edge or is on a different
  // display.
  gfx::Rect hit_bounds_in_screen = GetDisplayBoundsInScreen();
  hit_bounds_in_screen.set_height(kMouseRevealBoundsHeight);
  if (!hit_bounds_in_screen.Contains(location_in_screen)) {
    top_edge_hover_timer_.Stop();
    return;
  }

  // The cursor is now at the top of the screen. Consider the cursor "not
  // moving" even if it moves a little bit because users don't have perfect
  // pointing precision. (The y position is not tested because
  // |hit_bounds_in_screen| is short.)
  if (top_edge_hover_timer_.IsRunning() &&
      abs(location_in_screen.x() - mouse_x_when_hit_top_in_screen_) <=
          kMouseRevealXThresholdPixels)
    return;

  // Start the reveal if the cursor doesn't move for some amount of time.
  mouse_x_when_hit_top_in_screen_ = location_in_screen.x();
  top_edge_hover_timer_.Stop();
  // Timer is stopped when |this| is destroyed, hence Unretained() is safe.
  top_edge_hover_timer_.Start(
      FROM_HERE, base::TimeDelta::FromMilliseconds(kMouseRevealDelayMs),
      base::Bind(
          &ImmersiveFullscreenController::AcquireLocatedEventRevealedLock,
          base::Unretained(this)));
}

void ImmersiveFullscreenController::UpdateLocatedEventRevealedLock(
    const ui::LocatedEvent* event,
    const gfx::Point& location_in_screen) {
  if (!enabled_)
    return;
  DCHECK(!event || event->IsMouseEvent() || event->IsTouchEvent());

  // Neither the mouse nor touch can initiate a reveal when the top-of-window
  // views are sliding closed or are closed with the following exceptions:
  // - Hovering at y = 0 which is handled in OnMouseEvent().
  // - Doing a SWIPE_OPEN edge gesture which is handled in OnGestureEvent().
  if (reveal_state_ == CLOSED || reveal_state_ == SLIDING_CLOSED)
    return;

  // For the sake of simplicity, ignore |widget_|'s activation in computing
  // whether the top-of-window views should stay revealed. Ideally, the
  // top-of-window views would stay revealed only when the mouse cursor is
  // hovered above a non-obscured portion of the top-of-window views. The
  // top-of-window views may be partially obscured when |widget_| is inactive.

  // Ignore all events while a window has capture. This keeps the top-of-window
  // views revealed during a drag.
  if (ImmersiveContext::Get()->DoesAnyWindowHaveCapture())
    return;

  if ((!event || event->IsMouseEvent()) &&
      ShouldIgnoreMouseEventAtLocation(location_in_screen)) {
    return;
  }

  // The visible bounds of |top_container_| should be contained in
  // |hit_bounds_in_screen|.
  std::vector<gfx::Rect> hit_bounds_in_screen =
      delegate_->GetVisibleBoundsInScreen();
  bool keep_revealed = false;
  for (size_t i = 0; i < hit_bounds_in_screen.size(); ++i) {
    // Allow the cursor to move slightly off the top-of-window views before
    // sliding closed. In the case of ImmersiveModeControllerAsh, this helps
    // when the user is attempting to click on the bookmark bar and overshoots
    // slightly.
    if (event && event->type() == ui::ET_MOUSE_MOVED) {
      const int kBoundsOffsetY = 8;
      hit_bounds_in_screen[i].Inset(0, 0, 0, -kBoundsOffsetY);
    }

    if (hit_bounds_in_screen[i].Contains(location_in_screen)) {
      keep_revealed = true;
      break;
    }
  }

  if (keep_revealed)
    AcquireLocatedEventRevealedLock();
  else
    located_event_revealed_lock_.reset();
}

void ImmersiveFullscreenController::UpdateLocatedEventRevealedLock() {
  if (!ImmersiveContext::Get()->IsMouseEventsEnabled()) {
    // If mouse events are disabled, the user's last interaction was probably
    // via touch. Do no do further processing in this case as there is no easy
    // way of retrieving the position of the user's last touch.
    return;
  }
  UpdateLocatedEventRevealedLock(
      nullptr, display::Screen::GetScreen()->GetCursorScreenPoint());
}

void ImmersiveFullscreenController::AcquireLocatedEventRevealedLock() {
  // CAUTION: Acquiring the lock results in a reentrant call to
  // AcquireLocatedEventRevealedLock() when
  // |ImmersiveFullscreenController::animations_disabled_for_test_| is true.
  if (!located_event_revealed_lock_.get())
    located_event_revealed_lock_.reset(GetRevealedLock(ANIMATE_REVEAL_YES));
}

bool ImmersiveFullscreenController::UpdateRevealedLocksForSwipe(
    SwipeType swipe_type) {
  if (!enabled_ || swipe_type == SWIPE_NONE)
    return false;

  // Swipes while |widget_| is inactive should have been filtered out in
  // OnGestureEvent().
  DCHECK(widget_->IsActive());

  if (reveal_state_ == SLIDING_CLOSED || reveal_state_ == CLOSED) {
    if (swipe_type == SWIPE_OPEN && !located_event_revealed_lock_.get()) {
      located_event_revealed_lock_.reset(GetRevealedLock(ANIMATE_REVEAL_YES));
      return true;
    }
  } else {
    if (swipe_type == SWIPE_CLOSE) {
      // Attempt to end the reveal. If other code is holding onto a lock, the
      // attempt will be unsuccessful.
      located_event_revealed_lock_.reset();
      if (immersive_focus_watcher_)
        immersive_focus_watcher_->ReleaseLock();

      if (reveal_state_ == SLIDING_CLOSED || reveal_state_ == CLOSED) {
        widget_->GetFocusManager()->ClearFocus();
        return true;
      }

      // Ending the reveal was unsuccessful. Reaquire the locks if appropriate.
      UpdateLocatedEventRevealedLock();
      if (immersive_focus_watcher_)
        immersive_focus_watcher_->UpdateFocusRevealedLock();
    }
  }
  return false;
}

int ImmersiveFullscreenController::GetAnimationDuration(Animate animate) const {
  switch (animate) {
    case ANIMATE_NO:
      return 0;
    case ANIMATE_SLOW:
      return kRevealSlowAnimationDurationMs;
    case ANIMATE_FAST:
      return kRevealFastAnimationDurationMs;
  }
  NOTREACHED();
  return 0;
}

void ImmersiveFullscreenController::MaybeStartReveal(Animate animate) {
  if (!enabled_)
    return;

  if (animations_disabled_for_test_)
    animate = ANIMATE_NO;

  // Callers with ANIMATE_NO expect this function to synchronously reveal the
  // top-of-window views.
  if (reveal_state_ == REVEALED ||
      (reveal_state_ == SLIDING_OPEN && animate != ANIMATE_NO)) {
    return;
  }

  RevealState previous_reveal_state = reveal_state_;
  reveal_state_ = SLIDING_OPEN;
  if (previous_reveal_state == CLOSED) {
    delegate_->OnImmersiveRevealStarted();

    // Do not do any more processing if OnImmersiveRevealStarted() changed
    // |reveal_state_|.
    if (reveal_state_ != SLIDING_OPEN)
      return;
  }
  // Slide in the reveal view.
  if (animate == ANIMATE_NO) {
    animation_->Reset(1);
    OnSlideOpenAnimationCompleted();
  } else {
    animation_->SetSlideDuration(GetAnimationDuration(animate));
    animation_->Show();
  }
}

void ImmersiveFullscreenController::OnSlideOpenAnimationCompleted() {
  DCHECK_EQ(SLIDING_OPEN, reveal_state_);
  reveal_state_ = REVEALED;
  delegate_->SetVisibleFraction(1);

  // The user may not have moved the mouse since the reveal was initiated.
  // Update the revealed lock to reflect the mouse's current state.
  UpdateLocatedEventRevealedLock();
}

void ImmersiveFullscreenController::MaybeEndReveal(Animate animate) {
  if (!enabled_ || revealed_lock_count_ != 0)
    return;

  if (animations_disabled_for_test_)
    animate = ANIMATE_NO;

  // Callers with ANIMATE_NO expect this function to synchronously close the
  // top-of-window views.
  if (reveal_state_ == CLOSED ||
      (reveal_state_ == SLIDING_CLOSED && animate != ANIMATE_NO)) {
    return;
  }

  reveal_state_ = SLIDING_CLOSED;
  int duration_ms = GetAnimationDuration(animate);
  if (duration_ms > 0) {
    animation_->SetSlideDuration(duration_ms);
    animation_->Hide();
  } else {
    animation_->Reset(0);
    OnSlideClosedAnimationCompleted();
  }
}

void ImmersiveFullscreenController::OnSlideClosedAnimationCompleted() {
  DCHECK_EQ(SLIDING_CLOSED, reveal_state_);
  reveal_state_ = CLOSED;
  delegate_->OnImmersiveRevealEnded();
}

ImmersiveFullscreenController::SwipeType
ImmersiveFullscreenController::GetSwipeType(
    const ui::GestureEvent& event) const {
  if (event.type() != ui::ET_GESTURE_SCROLL_UPDATE)
    return SWIPE_NONE;
  // Make sure that it is a clear vertical gesture.
  if (std::abs(event.details().scroll_y()) <=
      kSwipeVerticalThresholdMultiplier * std::abs(event.details().scroll_x()))
    return SWIPE_NONE;
  if (event.details().scroll_y() < 0)
    return SWIPE_CLOSE;
  if (event.details().scroll_y() > 0)
    return SWIPE_OPEN;
  return SWIPE_NONE;
}

bool ImmersiveFullscreenController::ShouldIgnoreMouseEventAtLocation(
    const gfx::Point& location) const {
  // Ignore mouse events in the region immediately above the top edge of the
  // display. This is to handle the case of a user with a vertical display
  // layout (primary display above/below secondary display) and the immersive
  // fullscreen window on the bottom display. It is really hard to trigger a
  // reveal in this case because:
  // - It is hard to stop the cursor in the top |kMouseRevealBoundsHeight|
  //   pixels of the bottom display.
  // - The cursor is warped to the top display if the cursor gets to the top
  //   edge of the bottom display.
  // Mouse events are ignored in the bottom few pixels of the top display
  // (Mouse events in this region cannot start or end a reveal). This allows a
  // user to overshoot the top of the bottom display and still reveal the
  // top-of-window views.
  gfx::Rect dead_region = GetDisplayBoundsInScreen();
  dead_region.set_y(dead_region.y() - kHeightOfDeadRegionAboveTopContainer);
  dead_region.set_height(kHeightOfDeadRegionAboveTopContainer);
  return dead_region.Contains(location);
}

bool ImmersiveFullscreenController::ShouldHandleGestureEvent(
    const gfx::Point& location) const {
  DCHECK(widget_->IsActive());
  if (reveal_state_ == REVEALED) {
    std::vector<gfx::Rect> hit_bounds_in_screen(
        delegate_->GetVisibleBoundsInScreen());
    for (size_t i = 0; i < hit_bounds_in_screen.size(); ++i) {
      if (hit_bounds_in_screen[i].Contains(location))
        return true;
    }
    return false;
  }

  // When the top-of-window views are not fully revealed, handle gestures which
  // start in the top few pixels of the screen.
  gfx::Rect hit_bounds_in_screen(GetDisplayBoundsInScreen());
  hit_bounds_in_screen.set_height(kImmersiveFullscreenTopEdgeInset);
  if (hit_bounds_in_screen.Contains(location))
    return true;

  // There may be a bezel sensor off screen logically above
  // |hit_bounds_in_screen|. The check for the event not contained by the
  // closest screen ensures that the event is from a valid bezel (as opposed to
  // another screen in an extended desktop).
  gfx::Rect screen_bounds =
      display::Screen::GetScreen()->GetDisplayNearestPoint(location).bounds();
  return (!screen_bounds.Contains(location) &&
          location.y() < hit_bounds_in_screen.y() &&
          location.x() >= hit_bounds_in_screen.x() &&
          location.x() < hit_bounds_in_screen.right());
}

gfx::Rect ImmersiveFullscreenController::GetDisplayBoundsInScreen() const {
  return ImmersiveContext::Get()->GetDisplayBoundsInScreen(widget_);
}

}  // namespace ash
