// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <QuartzCore/QuartzCore.h>

#include "content/browser/web_contents/web_contents_view_overscroll_animator_slider_mac.h"

#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/web_contents_observer.h"

namespace {
// The minimum possible progress of an overscroll animation.
CGFloat kMinProgress = 0;
// The maximum possible progress of an overscroll animation.
CGFloat kMaxProgress = 2.0;
// The maximum duration of the completion or cancellation animations. The
// effective maximum is half of this value, since the longest animation is from
// progress = 1.0 to progress = 2.0;
CGFloat kMaxAnimationDuration = 0.2;
}  // namespace

// OverscrollAnimatorSliderView Private Category -------------------------------

@interface OverscrollAnimatorSliderView ()
// Callback from WebContentsPaintObserver.
- (void)webContentsFinishedNonEmptyPaint;

// Resets overscroll animation state.
- (void)reset;

// Given a |progress| from 0 to 2, the expected frame origin of the -movingView.
- (NSPoint)frameOriginWithProgress:(CGFloat)progress;

// The NSView that is moving during the overscroll animation.
- (NSView*)movingView;

// The expected duration of an animation from progress_ to |progress|
- (CGFloat)animationDurationForProgress:(CGFloat)progress;

// NSView override. During an overscroll animation, the cursor may no longer
// rest on the RenderWidgetHost's NativeView, which prevents wheel events from
// reaching the NativeView. The overscroll animation is driven by wheel events
// so they must be explicitly forwarded to the NativeView.
- (void)scrollWheel:(NSEvent*)event;
@end

// Helper Class (ResizingView) -------------------------------------------------

// This NSView subclass is intended to be the RenderWidgetHost's NativeView's
// parent NSView. It is possible for the RenderWidgetHost's NativeView's size to
// become out of sync with its parent NSView. The override of
// -resizeSubviewsWithOldSize: ensures that the sizes will eventually become
// consistent.
// http://crbug.com/264207
@interface ResizingView : NSView
@end

@implementation ResizingView
- (void)resizeSubviewsWithOldSize:(NSSize)oldBoundsSize {
  for (NSView* subview in self.subviews)
    [subview setFrame:self.bounds];
}
@end

// Helper Class (WebContentsPaintObserver) -------------------------------------

namespace overscroll_animator {
class WebContentsPaintObserver : public content::WebContentsObserver {
 public:
  WebContentsPaintObserver(content::WebContents* web_contents,
                           OverscrollAnimatorSliderView* slider_view)
      : WebContentsObserver(web_contents), slider_view_(slider_view) {}

  void DidFirstVisuallyNonEmptyPaint() override {
    [slider_view_ webContentsFinishedNonEmptyPaint];
  }

 private:
  OverscrollAnimatorSliderView* slider_view_;  // Weak reference.
};
}  // namespace overscroll_animator

// OverscrollAnimatorSliderView Implementation ---------------------------------

@implementation OverscrollAnimatorSliderView

- (instancetype)initWithFrame:(NSRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    bottomView_.reset([[NSImageView alloc] initWithFrame:self.bounds]);
    bottomView_.get().imageScaling = NSImageScaleNone;
    bottomView_.get().autoresizingMask =
        NSViewWidthSizable | NSViewHeightSizable;
    bottomView_.get().imageAlignment = NSImageAlignTop;
    [self addSubview:bottomView_];
    middleView_.reset([[ResizingView alloc] initWithFrame:self.bounds]);
    middleView_.get().autoresizingMask =
        NSViewWidthSizable | NSViewHeightSizable;
    [self addSubview:middleView_];
    topView_.reset([[NSImageView alloc] initWithFrame:self.bounds]);
    topView_.get().autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    topView_.get().imageScaling = NSImageScaleNone;
    topView_.get().imageAlignment = NSImageAlignTop;
    [self addSubview:topView_];

    [self reset];
  }
  return self;
}

- (void)webContentsFinishedNonEmptyPaint {
  observer_.reset();
  [self reset];
}

- (void)reset {
  DCHECK(!animating_);
  inOverscroll_ = NO;
  progress_ = kMinProgress;

  [CATransaction begin];
  [CATransaction setDisableActions:YES];
  bottomView_.get().hidden = YES;
  middleView_.get().hidden = NO;
  topView_.get().hidden = YES;

  [bottomView_ setFrameOrigin:NSMakePoint(0, 0)];
  [middleView_ setFrameOrigin:NSMakePoint(0, 0)];
  [topView_ setFrameOrigin:NSMakePoint(0, 0)];
  [CATransaction commit];
}

- (NSPoint)frameOriginWithProgress:(CGFloat)progress {
  if (direction_ == content::OVERSCROLL_ANIMATOR_DIRECTION_BACKWARDS)
    return NSMakePoint(progress / kMaxProgress * self.bounds.size.width, 0);
  return NSMakePoint((1 - progress / kMaxProgress) * self.bounds.size.width, 0);
}

- (NSView*)movingView {
  if (direction_ == content::OVERSCROLL_ANIMATOR_DIRECTION_BACKWARDS)
    return middleView_;
  return topView_;
}

- (CGFloat)animationDurationForProgress:(CGFloat)progress {
  CGFloat progressPercentage =
      fabs(progress_ - progress) / (kMaxProgress - kMinProgress);
  return progressPercentage * kMaxAnimationDuration;
}

- (void)scrollWheel:(NSEvent*)event {
  NSView* latestRenderWidgetHostView = [[middleView_ subviews] lastObject];
  [latestRenderWidgetHostView scrollWheel:event];
}

// WebContentsOverscrollAnimator Implementation --------------------------------

- (BOOL)needsNavigationSnapshot {
  return YES;
}

- (void)beginOverscrollInDirection:
            (content::OverscrollAnimatorDirection)direction
                navigationSnapshot:(NSImage*)snapshot {
  // TODO(erikchen): If snapshot is nil, need a placeholder.
  if (animating_ || inOverscroll_)
    return;

  inOverscroll_ = YES;
  direction_ = direction;
  if (direction_ == content::OVERSCROLL_ANIMATOR_DIRECTION_BACKWARDS) {
    // The middleView_ will slide to the right, revealing bottomView_.
    bottomView_.get().hidden = NO;
    [bottomView_ setImage:snapshot];
  } else {
    // The topView_ will slide in from the right, concealing middleView_.
    topView_.get().hidden = NO;
    [topView_ setFrameOrigin:NSMakePoint(self.bounds.size.width, 0)];
    [topView_ setImage:snapshot];
  }

  [self updateOverscrollProgress:kMinProgress];
}

- (void)addRenderWidgetHostNativeView:(NSView*)view {
  [middleView_ addSubview:view];
}

- (void)updateOverscrollProgress:(CGFloat)progress {
  if (animating_)
    return;
  DCHECK_LE(progress, kMaxProgress);
  DCHECK_GE(progress, kMinProgress);
  progress_ = progress;
  [[self movingView] setFrameOrigin:[self frameOriginWithProgress:progress]];
}

- (void)completeOverscroll:(content::WebContentsImpl*)webContents {
  if (animating_ || !inOverscroll_)
    return;

  animating_ = YES;

  NSView* view = [self movingView];
  [NSAnimationContext beginGrouping];
  [NSAnimationContext currentContext].duration =
      [self animationDurationForProgress:kMaxProgress];
  [[NSAnimationContext currentContext] setCompletionHandler:^{
      animating_ = NO;

      // Animation is complete. Now perform page load.
      if (direction_ == content::OVERSCROLL_ANIMATOR_DIRECTION_BACKWARDS)
        webContents->GetController().GoBack();
      else
        webContents->GetController().GoForward();

      // Reset the position of the middleView_, but wait for the page to paint
      // before showing it.
      middleView_.get().hidden = YES;
      [middleView_ setFrameOrigin:NSMakePoint(0, 0)];
      observer_.reset(
          new overscroll_animator::WebContentsPaintObserver(webContents, self));
  }];

  // Animate the moving view to its final position.
  [[view animator] setFrameOrigin:[self frameOriginWithProgress:kMaxProgress]];

  [NSAnimationContext endGrouping];
}

- (void)cancelOverscroll {
  if (animating_)
    return;

  if (!inOverscroll_) {
    [self reset];
    return;
  }

  animating_ = YES;

  NSView* view = [self movingView];
  [NSAnimationContext beginGrouping];
  [NSAnimationContext currentContext].duration =
      [self animationDurationForProgress:kMinProgress];
  [[NSAnimationContext currentContext] setCompletionHandler:^{
      // Animation is complete. Reset the state.
      animating_ = NO;
      [self reset];
  }];

  // Animate the moving view to its initial position.
  [[view animator] setFrameOrigin:[self frameOriginWithProgress:kMinProgress]];

  [NSAnimationContext endGrouping];
}

@end
