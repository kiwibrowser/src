// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/tabs/alert_indicator_button_cocoa.h"

#include "base/logging.h"
#include "base/mac/foundation_util.h"
#include "base/macros.h"
#include "base/metrics/user_metrics.h"
#include "base/threading/thread_task_runner_handle.h"
#import "chrome/browser/ui/cocoa/l10n_util.h"
#import "chrome/browser/ui/cocoa/tabs/tab_view.h"
#include "ui/gfx/animation/animation.h"
#include "ui/gfx/animation/animation_delegate.h"
#include "ui/gfx/image/image.h"

namespace {

// The minimum required click-to-select area of an inactive tab before allowing
// the click-to-mute functionality to be enabled.  This value is in terms of
// some percentage of the AlertIndicatorButton's width.  See comments in the
// updateEnabledForMuteToggle method.
const int kMinMouseSelectableAreaPercent = 250;

}  // namespace

@implementation AlertIndicatorButton

class FadeAnimationDelegate : public gfx::AnimationDelegate {
 public:
  explicit FadeAnimationDelegate(AlertIndicatorButton* button)
      : button_(button) {}
  ~FadeAnimationDelegate() override {}

 private:
  // gfx::AnimationDelegate implementation.
  void AnimationProgressed(const gfx::Animation* animation) override {
    [button_ setNeedsDisplay:YES];
  }

  void AnimationCanceled(const gfx::Animation* animation) override {
    AnimationEnded(animation);
  }

  void AnimationEnded(const gfx::Animation* animation) override {
    button_->showingAlertState_ = button_->alertState_;
    [button_ setNeedsDisplay:YES];
    [button_->animationDoneTarget_
        performSelector:button_->animationDoneAction_];
  }

  AlertIndicatorButton* const button_;

  DISALLOW_COPY_AND_ASSIGN(FadeAnimationDelegate);
};

@synthesize showingAlertState = showingAlertState_;

- (id)init {
  if ((self = [super initWithFrame:NSZeroRect])) {
    alertState_ = TabAlertState::NONE;
    showingAlertState_ = TabAlertState::NONE;
    isDormant_ = NO;
    [self setEnabled:NO];
    [super setTarget:self];
    [super setAction:@selector(handleClick:)];
  }
  return self;
}

- (void)removeFromSuperview {
  fadeAnimation_.reset();
  [super removeFromSuperview];
}

- (void)viewDidMoveToWindow {
  // In Material Design, the icon color depends on the theme. When the tab
  // is moved into another window, make sure that it updates the theme.
  [self updateIconForState:showingAlertState_];
}

- (void)updateIconForState:(TabAlertState)aState {
  if (aState != TabAlertState::NONE) {
    TabView* const tabView = base::mac::ObjCCast<TabView>([self superview]);
    SkColor iconColor = [tabView alertIndicatorColorForState:aState];
    NSImage* tabIndicatorImage =
        chrome::GetTabAlertIndicatorImage(aState, iconColor).ToNSImage();
    NSImage* affordanceImage =
        chrome::GetTabAlertIndicatorAffordanceImage(aState, iconColor)
            .ToNSImage();
    if (cocoa_l10n_util::ShouldDoExperimentalRTLLayout()) {
      tabIndicatorImage = cocoa_l10n_util::FlippedImage(tabIndicatorImage);
      affordanceImage = cocoa_l10n_util::FlippedImage(affordanceImage);
    }
    [self setImage:tabIndicatorImage];
    affordanceImage_.reset([affordanceImage retain]);
  }
}

- (void)transitionToAlertState:(TabAlertState)nextState {
  if (nextState == alertState_)
    return;

  [self updateIconForState:nextState];

  if ((alertState_ == TabAlertState::AUDIO_PLAYING &&
       nextState == TabAlertState::AUDIO_MUTING) ||
      (alertState_ == TabAlertState::AUDIO_MUTING &&
       nextState == TabAlertState::AUDIO_PLAYING) ||
      (alertState_ == TabAlertState::AUDIO_MUTING &&
       nextState == TabAlertState::NONE)) {
    // Instant user feedback: No fade animation.
    showingAlertState_ = nextState;
    fadeAnimation_.reset();
  } else {
    if (nextState == TabAlertState::NONE)
      showingAlertState_ = alertState_;  // Fading-out indicator.
    else
      showingAlertState_ = nextState;  // Fading-in to next indicator.
    // gfx::Animation requires a task runner is available for the current
    // thread.  Generally, only certain unit tests would not instantiate a task
    // runner.
    if (base::ThreadTaskRunnerHandle::IsSet()) {
      fadeAnimation_ = chrome::CreateTabAlertIndicatorFadeAnimation(nextState);
      if (!fadeAnimationDelegate_)
        fadeAnimationDelegate_.reset(new FadeAnimationDelegate(self));
      fadeAnimation_->set_delegate(fadeAnimationDelegate_.get());
      fadeAnimation_->Start();
    }
  }

  alertState_ = nextState;

  [self updateEnabledForMuteToggle];

  [self setNeedsDisplay:YES];
}

- (void)setTarget:(id)aTarget {
  NOTREACHED();  // See class-level comments.
}

- (void)setAction:(SEL)anAction {
  NOTREACHED();  // See class-level comments.
}

- (void)setAnimationDoneTarget:(id)target withAction:(SEL)action {
  animationDoneTarget_ = target;
  animationDoneAction_ = action;
}

- (void)setClickTarget:(id)target withAction:(SEL)action {
  clickTarget_ = target;
  clickAction_ = action;
}

- (void)mouseDown:(NSEvent*)theEvent {
  // Do not handle this left-button mouse event if any modifier keys are being
  // held down.  Instead, the Tab should react (e.g., selection or drag start).
  if ([theEvent modifierFlags] & NSDeviceIndependentModifierFlagsMask ||
      isDormant_) {
    [self setHoverState:kHoverStateNone];  // Turn off hover.
    [[self nextResponder] mouseDown:theEvent];
    return;
  }
  [super mouseDown:theEvent];
}

- (void)mouseEntered:(NSEvent*)theEvent {
  // If any modifier keys are being held down, do not turn on hover.
  if ([theEvent modifierFlags] & NSDeviceIndependentModifierFlagsMask) {
    [self setHoverState:kHoverStateNone];
    return;
  }
  [super mouseEntered:theEvent];
}

- (void)mouseExited:(NSEvent*)theEvent {
  [self exitDormantPeriod];
  [super mouseExited:theEvent];
}

- (void)mouseMoved:(NSEvent*)theEvent {
  // If any modifier keys are being held down, turn off hover.
  if ([theEvent modifierFlags] & NSDeviceIndependentModifierFlagsMask) {
    [self setHoverState:kHoverStateNone];
    return;
  }
  [super mouseMoved:theEvent];
}

- (void)rightMouseDown:(NSEvent*)theEvent {
  // All right-button mouse events should be handled by the Tab.
  [self setHoverState:kHoverStateNone];  // Turn off hover.
  [[self nextResponder] rightMouseDown:theEvent];
}

- (void)drawRect:(NSRect)dirtyRect {
  NSImage* const image = ([self hoverState] == kHoverStateNone ||
                          ![self isEnabled] || isDormant_) ?
      [self image] : affordanceImage_.get();
  if (!image)
    return;
  NSRect imageRect = NSZeroRect;
  imageRect.size = [image size];
  NSRect destRect = [self bounds];
  destRect.origin.y =
      floor((NSHeight(destRect) / 2) - (NSHeight(imageRect) / 2));
  destRect.size = imageRect.size;
  double opaqueness = 1.0;
  if (fadeAnimation_) {
    opaqueness = fadeAnimation_->GetCurrentValue();
    if (alertState_ == TabAlertState::NONE)
      opaqueness = 1.0 - opaqueness;  // Fading out, not in.
  } else if (isDormant_) {
    opaqueness = 0.5;
  }
  [image drawInRect:destRect
           fromRect:imageRect
          operation:NSCompositeSourceOver
           fraction:opaqueness
     respectFlipped:YES
              hints:nil];
}

// When disabled, the superview should receive all mouse events.
- (NSView*)hitTest:(NSPoint)aPoint {
  if ([self isEnabled] && !isDormant_ && ![self isHidden])
    return [super hitTest:aPoint];
  else
    return nil;
}

- (void)handleClick:(id)sender {
  [self enterDormantPeriod];

  // Call |-transitionToAlertState| to change the image, providing the user with
  // instant feedback.  In the very unlikely event that the mute toggle fails,
  // |-transitionToAlertState| will be called again, via another code path, to
  // set the image to be consistent with the final outcome.
  using base::UserMetricsAction;
  if (alertState_ == TabAlertState::AUDIO_PLAYING) {
    base::RecordAction(UserMetricsAction("AlertIndicatorButton_Mute"));
    [self transitionToAlertState:TabAlertState::AUDIO_MUTING];
  } else {
    DCHECK(alertState_ == TabAlertState::AUDIO_MUTING);
    base::RecordAction(UserMetricsAction("AlertIndicatorButton_Unmute"));
    [self transitionToAlertState:TabAlertState::AUDIO_PLAYING];
  }

  [clickTarget_ performSelector:clickAction_ withObject:self];
}

- (void)updateEnabledForMuteToggle {
  const BOOL wasEnabled = [self isEnabled];

  BOOL enable = chrome::AreExperimentalMuteControlsEnabled() &&
      (alertState_ == TabAlertState::AUDIO_PLAYING ||
       alertState_ == TabAlertState::AUDIO_MUTING);

  // If the tab is not the currently-active tab, make sure it is wide enough
  // before enabling click-to-mute.  This ensures that there is enough click
  // area for the user to activate a tab rather than unintentionally muting it.
  TabView* const tabView = base::mac::ObjCCast<TabView>([self superview]);
  if (enable && tabView && ([tabView state] != NSOnState)) {
    const int requiredWidth =
        NSWidth([self frame]) * kMinMouseSelectableAreaPercent / 100;
    enable = ([tabView widthOfLargestSelectableRegion] >= requiredWidth);
  }

  if (enable == wasEnabled)
    return;

  [self setEnabled:enable];

  // If the button has become enabled, check whether the mouse is currently
  // hovering.  If it is, enter a dormant period where extra user clicks are
  // prevented from having an effect (i.e., before the user has realized the
  // button has become enabled underneath their cursor).
  if (!wasEnabled && [self hoverState] == kHoverStateMouseOver)
    [self enterDormantPeriod];
  else if (![self isEnabled])
    [self exitDormantPeriod];
}

// Enters a temporary "dormant period" where this button will not trigger on
// clicks.  The user is provided a visual affordance during this period.  Sets a
// timer to call |-exitDormantPeriod|.
- (void)enterDormantPeriod {
  isDormant_ = YES;
  [self performSelector:@selector(exitDormantPeriod)
             withObject:nil
             afterDelay:[NSEvent doubleClickInterval]];
  [self setNeedsDisplay:YES];
}

// Leaves the "dormant period," allowing clicks to once again trigger an enabled
// button.
- (void)exitDormantPeriod {
  if (!isDormant_)
    return;
  isDormant_ = NO;
  [self setNeedsDisplay:YES];
}

// ThemedWindowDrawing protocol support.

- (void)windowDidChangeTheme {
  // Force the alert icon to update because the icon color may change based
  // on the current theme.
  [self updateIconForState:alertState_];
}

- (void)windowDidChangeActive {
}

@end
