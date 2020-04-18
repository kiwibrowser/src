// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/bubble_anchor_helper_views.h"

#import <Cocoa/Cocoa.h>

#include "base/bind.h"
#import "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#include "chrome/browser/ui/cocoa/l10n_util.h"
#import "chrome/browser/ui/cocoa/location_bar/location_bar_decoration.h"
#import "chrome/browser/ui/cocoa/location_bar/location_bar_view_mac.h"
#import "chrome/browser/ui/cocoa/location_bar/manage_passwords_decoration.h"
#import "chrome/browser/ui/cocoa/location_bar/page_info_bubble_decoration.h"
#import "chrome/browser/ui/cocoa/location_bar/save_credit_card_decoration.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"
#include "ui/views/widget/widget_observer.h"

namespace {

// Whether frame changes on the parent window are observed or ignored.
enum AnchorType { OBSERVE_PARENT, IGNORE_PARENT };

// Self-deleting object that hosts Objective-C observers watching for parent
// window resizes to reposition a bubble Widget. Deletes itself when the bubble
// Widget closes.
class BubbleAnchorHelper final : public views::WidgetObserver {
 public:
  BubbleAnchorHelper(views::BubbleDialogDelegateView* bubble,
                     LocationBarDecoration* decoration,
                     AnchorType type);

 private:
  // Observe |notification| on the bubble parent window with a block to call
  // RepositionFromAnchor().
  void ObserveParentWindow(NSString* notification);

  // Observe |notification| on the bubble window with a block to call
  // RecalculateAnchor();
  void ObserveBubbleWindow(NSString* notification);

  // Use |callback| to observe |notification| on |window|.
  void ObserveWindow(NSWindow* window,
                     NSString* notification,
                     base::RepeatingClosure callback);

  // Whether offset from the left of the parent window is fixed.
  bool IsMinXFixed() const {
    return cocoa_l10n_util::ShouldDoExperimentalRTLLayout() !=
           views::BubbleBorder::is_arrow_on_left(bubble_->arrow());
  }

  // Re-positions |bubble_| so that the offset to the parent window at
  // construction time is preserved.
  void RepositionFromAnchor();

  // Recalculates the offsets for |bubble_|.
  void RecalculateAnchor();

  // WidgetObserver:
  void OnWidgetDestroying(views::Widget* widget) override;

  base::scoped_nsobject<NSMutableArray> observer_tokens_;
  views::BubbleDialogDelegateView* bubble_;
  CGFloat horizontal_offset_;  // Offset from the left or right.
  CGFloat vertical_offset_;    // Offset from the top.

  // The omnibox decoration that |bubble_| is anchored to.
  // Weak. The lifetime is tied to the controller of the bubble's parent
  // window, which owns a LocationBarViewMac that directly owns |decoration_|.
  LocationBarDecoration* decoration_;

  DISALLOW_COPY_AND_ASSIGN(BubbleAnchorHelper);
};

}  // namespace

LocationBarDecoration* GetManagePasswordDecoration(gfx::NativeWindow window) {
  BrowserWindowController* window_controller =
      [BrowserWindowController browserWindowControllerForWindow:window];
  LocationBarViewMac* location_bar = [window_controller locationBarBridge];
  return location_bar ? location_bar->manage_passwords_decoration() : nullptr;
}

LocationBarDecoration* GetPageInfoDecoration(gfx::NativeWindow window) {
  BrowserWindowController* window_controller =
      [BrowserWindowController browserWindowControllerForWindow:window];
  LocationBarViewMac* location_bar = [window_controller locationBarBridge];
  return location_bar ? location_bar->page_info_decoration() : nullptr;
}

void KeepBubbleAnchored(views::BubbleDialogDelegateView* bubble,
                        LocationBarDecoration* decoration) {
  new BubbleAnchorHelper(bubble, decoration, OBSERVE_PARENT);
}

void TrackBubbleState(views::BubbleDialogDelegateView* bubble,
                      LocationBarDecoration* decoration) {
  new BubbleAnchorHelper(bubble, decoration, IGNORE_PARENT);
}

BubbleAnchorHelper::BubbleAnchorHelper(views::BubbleDialogDelegateView* bubble,
                                       LocationBarDecoration* decoration,
                                       AnchorType type)
    : observer_tokens_([[NSMutableArray alloc] init]),
      bubble_(bubble),
      decoration_(decoration) {
  DCHECK(bubble->GetWidget());
  DCHECK(bubble->parent_window());
  bubble->GetWidget()->AddObserver(this);

  if (decoration_)
    decoration_->SetActive(true);

  if (type == IGNORE_PARENT)
    return;

  RecalculateAnchor();

  ObserveParentWindow(NSWindowDidEnterFullScreenNotification);
  ObserveParentWindow(NSWindowDidExitFullScreenNotification);
  ObserveParentWindow(NSWindowDidResizeNotification);

  // Also monitor move. Note that for user-initiated window moves this is not
  // necessary: the bubble's child window status keeps the position pinned to
  // the parent during the move (which is handy since AppKit doesn't send out
  // notifications until the move completes). Programmatic -[NSWindow
  // setFrame:..] calls, however, do not update child window positions for us.
  ObserveParentWindow(NSWindowDidMoveNotification);

  ObserveBubbleWindow(NSWindowDidResizeNotification);
  // Don't ObserveBubbleWindow(NSWindowDidMoveNotification) unless it is
  // possible to distinguish between when BubbleAnchorHelper sets the position
  // of the bubble and when the existing NSWindow machinery sets the position of
  // the bubble. The NSWindow adjustments only occur if |bubble| is an actual
  // child of bubble->parent_window(). As of High Sierra, the BubbleAnchorHelper
  // and NSWindow disagree on the final positioning.
}

void BubbleAnchorHelper::ObserveParentWindow(NSString* notification) {
  ObserveWindow([bubble_->parent_window() window], notification,
                base::BindRepeating(&BubbleAnchorHelper::RepositionFromAnchor,
                                    base::Unretained(this)));
}

void BubbleAnchorHelper::ObserveBubbleWindow(NSString* notification) {
  ObserveWindow(bubble_->GetWidget()->GetNativeWindow(), notification,
                base::BindRepeating(&BubbleAnchorHelper::RecalculateAnchor,
                                    base::Unretained(this)));
}

void BubbleAnchorHelper::ObserveWindow(NSWindow* window,
                                       NSString* notification,
                                       base::RepeatingClosure callback) {
  NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
  id token = [center addObserverForName:notification
                                 object:window
                                  queue:nil
                             usingBlock:^(NSNotification* notification) {
                               callback.Run();
                             }];
  [observer_tokens_ addObject:token];
}

void BubbleAnchorHelper::RepositionFromAnchor() {
  NSRect bubble_frame = [bubble_->GetWidget()->GetNativeWindow() frame];
  NSRect parent_frame = [[bubble_->parent_window() window] frame];
  if (IsMinXFixed())
    bubble_frame.origin.x = NSMinX(parent_frame) - horizontal_offset_;
  else
    bubble_frame.origin.x = NSMaxX(parent_frame) - horizontal_offset_;
  bubble_frame.origin.y = NSMaxY(parent_frame) - vertical_offset_;
  [bubble_->GetWidget()->GetNativeWindow() setFrame:bubble_frame
                                            display:YES
                                            animate:NO];
}

void BubbleAnchorHelper::RecalculateAnchor() {
  NSRect parent_frame = [[bubble_->parent_window() window] frame];
  NSRect bubble_frame = [bubble_->GetWidget()->GetNativeWindow() frame];
  horizontal_offset_ =
      (IsMinXFixed() ? NSMinX(parent_frame) : NSMaxX(parent_frame)) -
      NSMinX(bubble_frame);
  vertical_offset_ = NSMaxY(parent_frame) - NSMinY(bubble_frame);
}

void BubbleAnchorHelper::OnWidgetDestroying(views::Widget* widget) {
  // NativeWidgetMac guarantees that a child's OnWidgetDestroying() is invoked
  // before the parent NSWindow has closed.
  if (decoration_)
    decoration_->SetActive(false);
  widget->RemoveObserver(this);
  for (id token in observer_tokens_.get())
    [[NSNotificationCenter defaultCenter] removeObserver:token];
  delete this;
}
