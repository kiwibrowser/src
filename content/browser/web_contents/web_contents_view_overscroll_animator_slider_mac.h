// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEB_CONTENTS_WEB_CONTENTS_VIEW_OVERSCROLL_ANIMATOR_SLIDER_MAC_H_
#define CONTENT_BROWSER_WEB_CONTENTS_WEB_CONTENTS_VIEW_OVERSCROLL_ANIMATOR_SLIDER_MAC_H_

#import <Cocoa/Cocoa.h>

#include <memory>

#include "base/mac/scoped_nsobject.h"
#include "content/browser/web_contents/web_contents_view_overscroll_animator_mac.h"

namespace overscroll_animator {
class WebContentsPaintObserver;
}  // namespace overscroll_animator

@interface OverscrollAnimatorSliderView
    : NSView<WebContentsOverscrollAnimator> {
  // This container view holds the RenderWidgetHost's NativeViews. Most of the
  // time, its frame in screen coordinates is the same as SliderView's frame in
  // screen coordinates. During an overscroll animation, it may temporarily be
  // relocated, but it will return to its original position after the overscroll
  // animation is finished.
  base::scoped_nsobject<NSView> middleView_;

  // This view is a sibling of middleView_, and is guaranteed to live below it.
  // Most of the time, it is hidden. During a backwards overscroll animation,
  // middleView_ is slid to the right, and bottomView_ peeks out from the
  // original position of middleView_.
  base::scoped_nsobject<NSImageView> bottomView_;

  // This view is a sibling of middleView_, and is guaranteed to live above it.
  // Most of the time, it is hidden. During a forwards overscroll animation,
  // topView_ is slid to the left from off screen, its final position exactly
  // covering middleView_.
  base::scoped_nsobject<NSImageView> topView_;

  // The direction of the current overscroll animation. This property has no
  // meaning when inOverscroll_ is false.
  content::OverscrollAnimatorDirection direction_;

  // Indicates that this view is completing or cancelling the overscroll. This
  // animation cannot be cancelled.
  BOOL animating_;

  // Reflects whether this view is in the process of handling an overscroll.
  BOOL inOverscroll_;

  // The most recent value passed to -updateOverscrollProgress:.
  CGFloat progress_;

  // An observer that reports the first non-empty paint of a WebContents. This
  // is used when completing an overscroll animation.
  std::unique_ptr<overscroll_animator::WebContentsPaintObserver> observer_;
}
@end

#endif  // CONTENT_BROWSER_WEB_CONTENTS_WEB_CONTENTS_VIEW_OVERSCROLL_ANIMATOR_SLIDER_MAC_H_
