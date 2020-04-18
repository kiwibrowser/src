// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/status_bubble_mac.h"

#include <limits>

#include "base/bind.h"
#include "base/compiler_specific.h"
#include "base/debug/stack_trace.h"
#include "base/mac/scoped_block.h"
#include "base/mac/sdk_forward_declarations.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#import "chrome/browser/ui/cocoa/bubble_view.h"
#include "components/url_formatter/elide_url.h"
#include "components/url_formatter/url_formatter.h"
#import "third_party/google_toolbox_for_mac/src/AppKit/GTMNSAnimation+Duration.h"
#import "third_party/google_toolbox_for_mac/src/AppKit/GTMNSBezierPath+RoundRect.h"
#import "third_party/google_toolbox_for_mac/src/AppKit/GTMNSColor+Luminance.h"
#include "ui/base/cocoa/cocoa_base_utils.h"
#include "ui/base/cocoa/window_size_constants.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/platform_font.h"
#include "ui/gfx/text_elider.h"
#include "ui/gfx/text_utils.h"

namespace {

const int kWindowHeight = 18;

// The width of the bubble in relation to the width of the parent window.
const CGFloat kWindowWidthPercent = 1.0 / 3.0;

// How close the mouse can get to the infobubble before it starts sliding
// off-screen.
const int kMousePadding = 20;

const int kTextPadding = 3;

// The status bubble's maximum opacity, when fully faded in.
const CGFloat kBubbleOpacity = 1.0;

// Delay before showing or hiding the bubble after a SetStatus or SetURL call.
const int64_t kShowDelayMS = 80;
const int64_t kHideDelayMS = 250;

// How long each fade should last.
const NSTimeInterval kShowFadeInDurationSeconds = 0.120;
const NSTimeInterval kHideFadeOutDurationSeconds = 0.200;

// The minimum representable time interval.  This can be used as the value
// passed to +[NSAnimationContext setDuration:] to stop an in-progress
// animation as quickly as possible.
const NSTimeInterval kMinimumTimeInterval =
    std::numeric_limits<NSTimeInterval>::min();

// How quickly the status bubble should expand.
const CGFloat kExpansionDurationSeconds = 0.125;

}  // namespace

// StatusBubbleWindow becomes a child of |statusBubbleParentWindow|, but waits
// until |statusBubbleParentWindow| is visible. This works around macOS
// bugs/features which make unexpected things happen when adding a child window
// to a window that's in another space, miniaturized, or hidden
// (https://crbug.com/783521, https://crbug.com/798792).
@interface StatusBubbleWindow : NSWindow

// The window which this window should become a child of. May be changed or
// nilled out at any time.
@property(assign, nonatomic) NSWindow* statusBubbleParentWindow;

@end

@implementation StatusBubbleWindow {
  BOOL observingParentWindowVisibility_;
}
@synthesize statusBubbleParentWindow = statusBubbleParentWindow_;

- (void)dealloc {
  // StatusBubbleMac is expected to always clear statusBubbleParentWindow
  // before releasing StatusBubbleWindow. If that changes, it's OK to remove
  // this DCHECK as long as StatusBubbleWindow will never outlive its parent.
  DCHECK(!statusBubbleParentWindow_);

  [self stopObserving];
  [super dealloc];
}

- (void)setStatusBubbleParentWindow:(NSWindow*)statusBubbleParentWindow {
  if (statusBubbleParentWindow_ == statusBubbleParentWindow)
    return;

  // First, detach from the current parent window, if any.
  if (statusBubbleParentWindow_) {
    [self stopObserving];
    [self orderOut:nil];  // Also removes |self| from its parent window.
  }

  // Assign the new parent window.
  statusBubbleParentWindow_ = statusBubbleParentWindow;

  if (statusBubbleParentWindow_) {
    // Attach to the new parent window if it's visible and on the active space.
    [self maybeAttach];

    if (!self.parentWindow) {
      // If maybeAttach bailed, start observing the window's visibility and the
      // active space, and try again when they change.
      observingParentWindowVisibility_ = YES;
      [statusBubbleParentWindow_ addObserver:self
                                  forKeyPath:@"visible"
                                     options:0
                                     context:nil];
      [[NSWorkspace sharedWorkspace].notificationCenter
          addObserver:self
             selector:@selector(maybeAttach)
                 name:NSWorkspaceActiveSpaceDidChangeNotification
               object:[NSWorkspace sharedWorkspace]];
    }
  }
}

- (void)stopObserving {
  if (!observingParentWindowVisibility_)
    return;
  observingParentWindowVisibility_ = NO;
  [statusBubbleParentWindow_ removeObserver:self
                                 forKeyPath:@"visible"
                                    context:nil];
  [[NSWorkspace sharedWorkspace].notificationCenter removeObserver:self];
}

- (void)maybeAttach {
  if (![statusBubbleParentWindow_ isVisible])
    return;
  if (![statusBubbleParentWindow_ isOnActiveSpace])
    return;
  [self stopObserving];
  // Adding |self| as a child window also orders it in.
  [statusBubbleParentWindow_ addChildWindow:self ordered:NSWindowAbove];
}

- (void)observeValueForKeyPath:(NSString*)keyPath
                      ofObject:(id)object
                        change:(NSDictionary<NSKeyValueChangeKey, id>*)change
                       context:(void*)context {
  [self maybeAttach];
}

@end

StatusBubbleMac::StatusBubbleMac(NSWindow* parent, id delegate)
    : parent_(parent),
      delegate_(delegate),
      status_text_(nil),
      url_text_(nil),
      state_(kBubbleHidden),
      immediate_(false),
      is_expanded_(false),
      timer_factory_(this),
      expand_timer_factory_(this),
      completion_handler_factory_(this) {
  Create();
}

StatusBubbleMac::~StatusBubbleMac() {
  DCHECK(window_);

  Hide();

  completion_handler_factory_.InvalidateWeakPtrs();
  Detach();
}

void StatusBubbleMac::SetStatus(const base::string16& status) {
  SetText(status, false);
}

void StatusBubbleMac::SetURL(const GURL& url) {
  url_ = url;

  CGFloat bubble_width = NSWidth([window_ frame]);
  if (state_ == kBubbleHidden)
    bubble_width = NSWidth(CalculateWindowFrame(/*expand=*/false));
  int text_width = static_cast<int>(bubble_width -
                                    kBubbleViewTextPositionX -
                                    kTextPadding);

  // Scale from view to window coordinates before eliding URL string.
  NSSize scaled_width = NSMakeSize(text_width, 0);
  scaled_width = [[parent_ contentView] convertSize:scaled_width fromView:nil];
  text_width = static_cast<int>(scaled_width.width);
  NSFont* font = [[window_ contentView] font];
  gfx::FontList font_list_chr(
      gfx::Font(gfx::PlatformFont::CreateFromNativeFont(font)));

  base::string16 original_url_text = url_formatter::FormatUrl(url);
  base::string16 status = url_formatter::ElideUrl(
      url, font_list_chr, text_width, gfx::Typesetter::BROWSER);

  SetText(status, true);

  // In testing, don't use animation. When ExpandBubble is tested, it is
  // called explicitly.
  if (immediate_)
    return;
  else
    CancelExpandTimer();

  // If the bubble has been expanded, the user has already hovered over a link
  // to trigger the expanded state.  Don't wait to change the bubble in this
  // case -- immediately expand or contract to fit the URL.
  if (is_expanded_ && !url.is_empty()) {
    ExpandBubble();
  } else if (original_url_text.length() > status.length()) {
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, base::Bind(&StatusBubbleMac::ExpandBubble,
                              expand_timer_factory_.GetWeakPtr()),
        base::TimeDelta::FromMilliseconds(kExpandHoverDelayMS));
  }
}

void StatusBubbleMac::SetText(const base::string16& text, bool is_url) {
  // The status bubble allows the status and URL strings to be set
  // independently.  Whichever was set non-empty most recently will be the
  // value displayed.  When both are empty, the status bubble hides.

  NSString* text_ns = base::SysUTF16ToNSString(text);

  NSString** main;
  NSString** backup;

  if (is_url) {
    main = &url_text_;
    backup = &status_text_;
  } else {
    main = &status_text_;
    backup = &url_text_;
  }

  // Don't return from this function early.  It's important to make sure that
  // all calls to StartShowing and StartHiding are made, so that all delays
  // are observed properly.  Specifically, if the state is currently
  // kBubbleShowingTimer, the timer will need to be restarted even if
  // [text_ns isEqualToString:*main] is true.

  [*main autorelease];
  *main = [text_ns retain];

  bool show = true;
  if ([*main length] > 0)
    [[window_ contentView] setContent:*main];
  else if ([*backup length] > 0)
    [[window_ contentView] setContent:*backup];
  else
    show = false;

  if (show) {
    // Call StartShowing() first to update the current bubble state before
    // calculating a new size.
    StartShowing();
    UpdateSizeAndPosition();
  } else {
    StartHiding();
  }
}

void StatusBubbleMac::Hide() {
  CancelTimer();
  CancelExpandTimer();
  is_expanded_ = false;

  bool fade_out = false;
  if (state_ == kBubbleHidingFadeOut || state_ == kBubbleShowingFadeIn) {
    SetState(kBubbleHidingFadeOut);

    if (!immediate_) {
      // An animation is in progress.  Cancel it by starting a new animation.
      // Use kMinimumTimeInterval to set the opacity as rapidly as possible.
      fade_out = true;
      AnimateWindowAlpha(0.0, kMinimumTimeInterval);
    }
  }

  NSRect frame = CalculateWindowFrame(/*expand=*/false);
  if (!fade_out) {
    // No animation is in progress, so the opacity can be set directly.
    [window_ setAlphaValue:0.0];
    SetState(kBubbleHidden);
    frame.size = ui::kWindowSizeDeterminedLater.size;
  }

  // Stop any width animation and reset the bubble size.
  if (!immediate_) {
    [NSAnimationContext beginGrouping];
    [[NSAnimationContext currentContext] setDuration:kMinimumTimeInterval];
    [[window_ animator] setFrame:frame display:NO];
    [NSAnimationContext endGrouping];
  } else {
    [window_ setFrame:frame display:NO];
  }

  [status_text_ release];
  status_text_ = nil;
  [url_text_ release];
  url_text_ = nil;
}

void StatusBubbleMac::SetFrameAvoidingMouse(
    NSRect window_frame, const gfx::Point& mouse_pos) {
  if (!window_)
    return;

  // Bubble's base rect in |parent_| (window base) coordinates.
  NSRect base_rect;
  if ([delegate_ respondsToSelector:@selector(statusBubbleBaseFrame)]) {
    base_rect = [delegate_ statusBubbleBaseFrame];
  } else {
    base_rect = [[parent_ contentView] bounds];
    base_rect = [[parent_ contentView] convertRect:base_rect toView:nil];
  }

  // To start, assume default positioning in the lower left corner.
  // The window_frame position is in global (screen) coordinates.
  window_frame.origin =
      ui::ConvertPointFromWindowToScreen(parent_, base_rect.origin);

  // Get the cursor position relative to the top right corner of the bubble.
  gfx::Point relative_pos(mouse_pos.x() - NSMaxX(window_frame),
                          mouse_pos.y() - NSMaxY(window_frame));

  // If the mouse is in a position where we think it would move the
  // status bubble, figure out where and how the bubble should be moved, and
  // what sorts of corners it should have.
  unsigned long corner_flags;
  if (relative_pos.y() < kMousePadding &&
      relative_pos.x() < kMousePadding) {
    int offset = kMousePadding - relative_pos.y();

    // Make the movement non-linear.
    offset = offset * offset / kMousePadding;

    // When the mouse is entering from the right, we want the offset to be
    // scaled by how horizontally far away the cursor is from the bubble.
    if (relative_pos.x() > 0) {
      offset *= (kMousePadding - relative_pos.x()) / kMousePadding;
    }

    bool is_on_screen = true;
    NSScreen* screen = [window_ screen];
    if (screen &&
        NSMinY([screen visibleFrame]) > NSMinY(window_frame) - offset) {
      is_on_screen = false;
    }

    // If something is shown below tab contents (devtools, download shelf etc.),
    // adjust the position to sit on top of it.
    bool is_any_shelf_visible = NSMinY(base_rect) > 0;

    if (is_on_screen && !is_any_shelf_visible) {
      // Cap the offset and change the visual presentation of the bubble
      // depending on where it ends up (so that rounded corners square off
      // and mate to the edges of the tab content).
      if (offset >= NSHeight(window_frame)) {
        offset = NSHeight(window_frame);
        corner_flags = kRoundedBottomLeftCorner | kRoundedBottomRightCorner;
      } else if (offset > 0) {
        corner_flags = kRoundedTopRightCorner |
                       kRoundedBottomLeftCorner |
                       kRoundedBottomRightCorner;
      } else {
        corner_flags = kRoundedTopRightCorner;
      }

      // Place the bubble on the left, but slightly lower.
      window_frame.origin.y -= offset;
    } else {
      // Cannot move the bubble down without obscuring other content.
      // Move it to the far right instead.
      corner_flags = kRoundedTopLeftCorner;
      window_frame.origin.x += NSWidth(base_rect) - NSWidth(window_frame);
    }
  } else {
    // Use the default position in the lower left corner of the content area.
    corner_flags = kRoundedTopRightCorner;
  }

  corner_flags |= OSDependentCornerFlags(window_frame);

  [[window_ contentView] setCornerFlags:corner_flags];
  [window_ setFrame:window_frame display:YES];
}

void StatusBubbleMac::MouseMoved(bool left_content) {
  MouseMovedAt(gfx::Point([NSEvent mouseLocation]), left_content);
}

void StatusBubbleMac::MouseMovedAt(
    const gfx::Point& location, bool left_content) {
  if (!left_content)
    SetFrameAvoidingMouse([window_ frame], location);
}

void StatusBubbleMac::UpdateDownloadShelfVisibility(bool visible) {
  UpdateSizeAndPosition();
}

void StatusBubbleMac::Create() {
  DCHECK(!window_);

  window_.reset([[StatusBubbleWindow alloc]
      initWithContentRect:ui::kWindowSizeDeterminedLater
                styleMask:NSBorderlessWindowMask
                  backing:NSBackingStoreBuffered
                    defer:NO]);
  [window_ setCollectionBehavior:[window_ collectionBehavior] |
                                 NSWindowCollectionBehaviorTransient];
  [window_ setMovableByWindowBackground:NO];
  [window_ setBackgroundColor:[NSColor clearColor]];
  [window_ setOpaque:NO];
  [window_ setHasShadow:NO];

  base::scoped_nsobject<BubbleView> view(
      [[BubbleView alloc] initWithFrame:NSZeroRect]);
  [window_ setContentView:view];

  [window_ setAlphaValue:0.0];

  // TODO(dtseng): Ignore until we provide NSAccessibility support.
  [window_ accessibilitySetOverrideValue:NSAccessibilityUnknownRole
                            forAttribute:NSAccessibilityRoleAttribute];

  [view setCornerFlags:kRoundedTopRightCorner];
  MouseMovedAt(gfx::Point(), false);
}

void StatusBubbleMac::Attach() {
  if (is_attached())
    return;
  [window_ setStatusBubbleParentWindow:parent_];
  [[window_ contentView] setThemeProvider:parent_];
}

void StatusBubbleMac::Detach() {
  if (!is_attached())
    return;

  // Magic setFrame: See http://crbug.com/58506 and http://crrev.com/3564021 .
  // TODO(rohitrao): Does the frame size actually matter here?  Can we always
  // set it to kWindowSizeDeterminedLater?
  NSRect frame = [window_ frame];
  frame.size = ui::kWindowSizeDeterminedLater.size;
  if (state_ != kBubbleHidden) {
    frame = CalculateWindowFrame(/*expand=*/false);
  }
  // See https://crbug.com/28107 and https://crbug.com/29054.
  [window_ setFrame:frame display:NO];
  [window_ setStatusBubbleParentWindow:nil];

  [[window_ contentView] setThemeProvider:nil];
}

bool StatusBubbleMac::is_attached() {
  return [window_ statusBubbleParentWindow] != nil;
}

void StatusBubbleMac::AnimationDidStop() {
  DCHECK([NSThread isMainThread]);
  DCHECK(state_ == kBubbleShowingFadeIn || state_ == kBubbleHidingFadeOut);
  DCHECK(is_attached());

  if (state_ == kBubbleShowingFadeIn) {
    DCHECK_EQ([[window_ animator] alphaValue], kBubbleOpacity);
    SetState(kBubbleShown);
  } else {
    DCHECK_EQ([[window_ animator] alphaValue], 0.0);
    SetState(kBubbleHidden);
  }
}

void StatusBubbleMac::SetState(StatusBubbleState state) {
  if (state == state_)
    return;

  if (state == kBubbleHidden) {
    is_expanded_ = false;
    Detach();
  } else {
    Attach();
  }

  if ([delegate_ respondsToSelector:@selector(statusBubbleWillEnterState:)])
    [delegate_ statusBubbleWillEnterState:state];

  state_ = state;
}

void StatusBubbleMac::Fade(bool show) {
  DCHECK([NSThread isMainThread]);

  StatusBubbleState fade_state = kBubbleShowingFadeIn;
  StatusBubbleState target_state = kBubbleShown;
  NSTimeInterval full_duration = kShowFadeInDurationSeconds;
  CGFloat opacity = kBubbleOpacity;

  if (!show) {
    fade_state = kBubbleHidingFadeOut;
    target_state = kBubbleHidden;
    full_duration = kHideFadeOutDurationSeconds;
    opacity = 0.0;
  }

  DCHECK(state_ == fade_state || state_ == target_state);

  if (state_ == target_state)
    return;

  if (immediate_) {
    [window_ setAlphaValue:opacity];
    SetState(target_state);
    return;
  }

  // If an incomplete transition has left the opacity somewhere between 0 and
  // kBubbleOpacity, the fade rate is kept constant by shortening the duration.
  NSTimeInterval duration =
      full_duration *
      fabs(opacity - [[window_ animator] alphaValue]) / kBubbleOpacity;

  // 0.0 will not cancel an in-progress animation.
  if (duration == 0.0)
    duration = kMinimumTimeInterval;

  // Cancel an in-progress transition and replace it with this fade.
  AnimateWindowAlpha(opacity, duration);
}

void StatusBubbleMac::AnimateWindowAlpha(CGFloat alpha,
                                         NSTimeInterval duration) {
  completion_handler_factory_.InvalidateWeakPtrs();
  base::WeakPtr<StatusBubbleMac> weak_ptr(
      completion_handler_factory_.GetWeakPtr());
  [NSAnimationContext runAnimationGroup:^(NSAnimationContext* context) {
    [context setDuration:duration];
    [[window_ animator] setAlphaValue:alpha];
  }
      completionHandler:^{
        if (weak_ptr)
          weak_ptr->AnimationDidStop();
      }];
}

void StatusBubbleMac::StartTimer(int64_t delay_ms) {
  DCHECK([NSThread isMainThread]);
  DCHECK(state_ == kBubbleShowingTimer || state_ == kBubbleHidingTimer);

  if (immediate_) {
    TimerFired();
    return;
  }

  // There can only be one running timer.
  CancelTimer();

  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::Bind(&StatusBubbleMac::TimerFired, timer_factory_.GetWeakPtr()),
      base::TimeDelta::FromMilliseconds(delay_ms));
}

void StatusBubbleMac::CancelTimer() {
  DCHECK([NSThread isMainThread]);

  if (timer_factory_.HasWeakPtrs())
    timer_factory_.InvalidateWeakPtrs();
}

void StatusBubbleMac::TimerFired() {
  DCHECK(state_ == kBubbleShowingTimer || state_ == kBubbleHidingTimer);
  DCHECK([NSThread isMainThread]);

  if (state_ == kBubbleShowingTimer) {
    SetState(kBubbleShowingFadeIn);
    Fade(true);
  } else {
    SetState(kBubbleHidingFadeOut);
    Fade(false);
  }
}

void StatusBubbleMac::StartShowing() {
  if (state_ == kBubbleHidden) {
    // Arrange to begin fading in after a delay.
    SetState(kBubbleShowingTimer);
    StartTimer(kShowDelayMS);
  } else if (state_ == kBubbleHidingFadeOut) {
    // Cancel the fade-out in progress and replace it with a fade in.
    SetState(kBubbleShowingFadeIn);
    Fade(true);
  } else if (state_ == kBubbleHidingTimer) {
    // The bubble was already shown but was waiting to begin fading out.  It's
    // given a stay of execution.
    SetState(kBubbleShown);
    CancelTimer();
  } else if (state_ == kBubbleShowingTimer) {
    // The timer was already running but nothing was showing yet.  Reaching
    // this point means that there is a new request to show something.  Start
    // over again by resetting the timer, effectively invalidating the earlier
    // request.
    StartTimer(kShowDelayMS);
  }

  // If the state is kBubbleShown or kBubbleShowingFadeIn, leave everything
  // alone.
}

void StatusBubbleMac::StartHiding() {
  if (state_ == kBubbleShown) {
    // Arrange to begin fading out after a delay.
    SetState(kBubbleHidingTimer);
    StartTimer(kHideDelayMS);
  } else if (state_ == kBubbleShowingFadeIn) {
    // Cancel the fade-in in progress and replace it with a fade out.
    SetState(kBubbleHidingFadeOut);
    Fade(false);
  } else if (state_ == kBubbleShowingTimer) {
    // The bubble was already hidden but was waiting to begin fading in.  Too
    // bad, it won't get the opportunity now.
    SetState(kBubbleHidden);
    CancelTimer();
  }

  // If the state is kBubbleHidden, kBubbleHidingFadeOut, or
  // kBubbleHidingTimer, leave everything alone.  The timer is not reset as
  // with kBubbleShowingTimer in StartShowing() because a subsequent request
  // to hide something while one is already in flight does not invalidate the
  // earlier request.
}

void StatusBubbleMac::CancelExpandTimer() {
  DCHECK([NSThread isMainThread]);
  expand_timer_factory_.InvalidateWeakPtrs();
}

// Get the current location of the mouse in screen coordinates. To make this
// class testable, all code should use this method rather than using
// NSEvent mouseLocation directly.
gfx::Point StatusBubbleMac::GetMouseLocation() {
  NSPoint p = [NSEvent mouseLocation];
  --p.y;  // The docs say the y coord starts at 1 not 0; don't ask why.
  return gfx::Point(p.x, p.y);
}

void StatusBubbleMac::ExpandBubble() {
  // Calculate the width available for expanded and standard bubbles.
  NSRect window_frame = CalculateWindowFrame(/*expand=*/true);
  CGFloat max_bubble_width = NSWidth(window_frame);
  CGFloat standard_bubble_width =
      NSWidth(CalculateWindowFrame(/*expand=*/false));

  // Generate the URL string that fits in the expanded bubble.
  NSFont* font = [[window_ contentView] font];
  gfx::FontList font_list_chr(
      gfx::Font(gfx::PlatformFont::CreateFromNativeFont(font)));
  base::string16 expanded_url = url_formatter::ElideUrl(
      url_, font_list_chr, max_bubble_width, gfx::Typesetter::BROWSER);

  // Scale width from gfx::Font in view coordinates to window coordinates.
  int required_width_for_string =
      gfx::GetStringWidth(expanded_url, font_list_chr,
                          gfx::Typesetter::NATIVE) +
      kTextPadding * 2 + kBubbleViewTextPositionX;
  NSSize scaled_width = NSMakeSize(required_width_for_string, 0);
  scaled_width = [[parent_ contentView] convertSize:scaled_width toView:nil];
  required_width_for_string = scaled_width.width;

  // The expanded width must be at least as wide as the standard width, but no
  // wider than the maximum width for its parent frame.
  int expanded_bubble_width =
      std::max(standard_bubble_width,
               std::min(max_bubble_width,
                        static_cast<CGFloat>(required_width_for_string)));

  SetText(expanded_url, true);
  is_expanded_ = true;
  window_frame.size.width = expanded_bubble_width;

  // In testing, don't do any animation.
  if (immediate_) {
    [window_ setFrame:window_frame display:YES];
    return;
  }

  NSRect actual_window_frame = [window_ frame];
  // Adjust status bubble origin if bubble was moved to the right.
  // TODO(alekseys): fix for RTL.
  if (NSMinX(actual_window_frame) > NSMinX(window_frame)) {
    actual_window_frame.origin.x =
        NSMaxX(actual_window_frame) - NSWidth(window_frame);
  }
  actual_window_frame.size.width = NSWidth(window_frame);

  // Do not expand if it's going to cover mouse location.
  gfx::Point p = GetMouseLocation();
  if (NSPointInRect(NSMakePoint(p.x(), p.y()), actual_window_frame))
    return;

  // Get the current corner flags and see what needs to change based on the
  // expansion.
  unsigned long corner_flags = [[window_ contentView] cornerFlags];
  corner_flags |= OSDependentCornerFlags(actual_window_frame);
  [[window_ contentView] setCornerFlags:corner_flags];


  [NSAnimationContext beginGrouping];
  [[NSAnimationContext currentContext] setDuration:kExpansionDurationSeconds];
  [[window_ animator] setFrame:actual_window_frame display:YES];
  [NSAnimationContext endGrouping];
}

void StatusBubbleMac::UpdateSizeAndPosition() {
  if (!window_)
    return;

  // There is no need to update the size if the bubble is hidden.
  if (state_ == kBubbleHidden)
    return;

  SetFrameAvoidingMouse(CalculateWindowFrame(/*expand=*/false),
                        GetMouseLocation());
}

void StatusBubbleMac::SwitchParentWindow(NSWindow* parent) {
  DCHECK(parent);
  Detach();
  parent_ = parent;
  Attach();
  UpdateSizeAndPosition();
}

NSRect StatusBubbleMac::CalculateWindowFrame(bool expanded_width) {
  DCHECK(parent_);

  NSRect screenRect;
  if ([delegate_ respondsToSelector:@selector(statusBubbleBaseFrame)]) {
    screenRect = [delegate_ statusBubbleBaseFrame];
    screenRect = [parent_ convertRectToScreen:screenRect];
  } else {
    screenRect = [parent_ frame];
  }

  NSSize size = NSMakeSize(0, kWindowHeight);
  size = [[parent_ contentView] convertSize:size toView:nil];

  if (expanded_width) {
    size.width = screenRect.size.width;
  } else {
    size.width = kWindowWidthPercent * screenRect.size.width;
  }

  screenRect.size = size;
  return screenRect;
}

unsigned long StatusBubbleMac::OSDependentCornerFlags(NSRect window_frame) {
  unsigned long corner_flags = 0;

  NSRect parent_frame = [parent_ frame];

  // Round the bottom corners when they're right up against the
  // corresponding edge of the parent window, or when below the parent
  // window.
  if (NSMinY(window_frame) <= NSMinY(parent_frame)) {
    if (NSMinX(window_frame) == NSMinX(parent_frame)) {
      corner_flags |= kRoundedBottomLeftCorner;
    }

    if (NSMaxX(window_frame) == NSMaxX(parent_frame)) {
      corner_flags |= kRoundedBottomRightCorner;
    }
  }

  // Round the top corners when the bubble is below the parent window.
  if (NSMinY(window_frame) < NSMinY(parent_frame)) {
    corner_flags |= kRoundedTopLeftCorner | kRoundedTopRightCorner;
  }

  return corner_flags;
}

NSWindow* StatusBubbleMac::GetWindow() {
  return window_;
}
