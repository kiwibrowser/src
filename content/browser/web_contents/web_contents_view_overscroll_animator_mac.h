// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEB_CONTENTS_WEB_CONTENTS_VIEW_OVERSCROLL_ANIMATOR_MAC_H_
#define CONTENT_BROWSER_WEB_CONTENTS_WEB_CONTENTS_VIEW_OVERSCROLL_ANIMATOR_MAC_H_

#import <Cocoa/Cocoa.h>

namespace content {
class WebContentsImpl;

// The direction of the overscroll animations. Backwards means that the user
// wants to navigate backwards in the navigation history. The opposite applies
// to forwards.
enum OverscrollAnimatorDirection {
  OVERSCROLL_ANIMATOR_DIRECTION_BACKWARDS,
  OVERSCROLL_ANIMATOR_DIRECTION_FORWARDS,
};
}  // namespace content

// NSViews that intend to manage the animation associated with an overscroll
// must implement this protocol.
@protocol WebContentsOverscrollAnimator
// Some implementations require the WebContentsView to supply a snapshot of a
// previous navigation state. This method determines whether the snapshot passed
// to the overscroll animator is expected to be non-nil.
- (BOOL)needsNavigationSnapshot;

// Begin an overscroll animation. The method -needsNavigationSnapshot determines
// whether |snapshot| can be nil.
- (void)beginOverscrollInDirection:
            (content::OverscrollAnimatorDirection)direction
                navigationSnapshot:(NSImage*)snapshot;

// Due to the nature of some of the overscroll animations, implementators of
// this protocol must have control over the layout of the RenderWidgetHost's
// NativeView. When there is no overscroll animation in progress, the
// implementor must guarantee that the frame of the RenderWidgetHost's
// NativeView in screen coordinates is the same as its own frame in screen
// coordinates.
// Due to the odd ownership cycles of the RenderWidgetHost's NativeView, it is
// important that its presence in the NSView hierarchy is the only strong
// reference, and that when it gets removed from the NSView hierarchy, it will
// be dealloc'ed shortly thereafter.
- (void)addRenderWidgetHostNativeView:(NSView*)view;

// During an overscroll animation, |progress| ranges from 0 to 2, and indicates
// how close the overscroll is to completing. If the overscroll ends with
// |progress| >= 1, then the overscroll is considered completed.
- (void)updateOverscrollProgress:(CGFloat)progress;

// Animate the finish of the overscroll and perform a navigation. The navigation
// may not happen synchronously, but is guaranteed to eventually occur.
- (void)completeOverscroll:(content::WebContentsImpl*)webContents;

// Animate the cancellation of the overscroll.
- (void)cancelOverscroll;
@end

#endif  // CONTENT_BROWSER_WEB_CONTENTS_WEB_CONTENTS_VIEW_OVERSCROLL_ANIMATOR_MAC_H_
