// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/contextual_search/contextual_search_highlighter_view.h"

#include "base/logging.h"
#import "ios/chrome/browser/ui/contextual_search/contextual_search_controller.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation ContextualSearchHighlighterView {
  // Store size of the container view. If size change, text layout will likely
  // change and should be updated.
  CGSize _previousSize;

  // Store values that were used last time the rects were drawn.
  CGPoint _scroll;
  CGFloat _zoom;
  CGFloat _offset;

  __weak id<ContextualSearchHighlighterDelegate> _delegate;
}

- (instancetype)initWithFrame:(CGRect)frame
                     delegate:
                         (id<ContextualSearchHighlighterDelegate>)delegate {
  self = [super initWithFrame:frame];
  if (self) {
    [self setUserInteractionEnabled:NO];
    [self setAutoresizingMask:UIViewAutoresizingFlexibleWidth |
                              UIViewAutoresizingFlexibleHeight];
    _previousSize = frame.size;
    _delegate = delegate;
  }
  return self;
}

- (instancetype)initWithFrame:(CGRect)frame {
  NOTREACHED();
  return nil;
}

- (instancetype)initWithCoder:(NSCoder*)aDecoder {
  NOTREACHED();
  return nil;
}

- (void)layoutSubviews {
  [super layoutSubviews];
  if (!CGSizeEqualToSize(_previousSize, self.frame.size)) {
    _previousSize = self.frame.size;
    dispatch_async(dispatch_get_main_queue(), ^{
      [_delegate updateHighlight];
    });
  }
}

- (void)highlightRects:(NSArray*)rects
            withOffset:(CGFloat)offset
                  zoom:(CGFloat)zoom
                scroll:(CGPoint)scroll {
  for (UIView* view : [self subviews]) {
    [view removeFromSuperview];
  }
  for (NSValue* value in rects) {
    CGRect rect = [value CGRectValue];
    rect.origin.x *= zoom;
    rect.origin.y *= zoom;
    rect.size.width *= zoom;
    rect.size.height *= zoom;
    rect.origin.x -= scroll.x;
    rect.origin.y += offset - scroll.y;
    UIView* view = [[UIView alloc] initWithFrame:rect];
    [self addSubview:view];
    view.backgroundColor =
        [UIColor colorWithRed:0.67 green:0.88 blue:0.96 alpha:0.6];
  }
  [[self superview] bringSubviewToFront:self];
  _zoom = zoom;
  _scroll = scroll;
  _offset = offset;
}

- (void)setScroll:(CGPoint)newScroll
             zoom:(CGFloat)newZoom
           offset:(CGFloat)newOffset {
  for (UIView* view : [self subviews]) {
    CGRect oldFrame = [view frame];
    CGRect newFrame;
    CGFloat scaleAdjust = newZoom / _zoom;
    newFrame.origin.x =
        (oldFrame.origin.x + _scroll.x) * scaleAdjust - newScroll.x;
    newFrame.origin.y =
        (oldFrame.origin.y + _scroll.y - _offset) * scaleAdjust + newOffset -
        newScroll.y;
    newFrame.size.width = oldFrame.size.width * scaleAdjust;
    newFrame.size.height = oldFrame.size.height * scaleAdjust;
    [view setFrame:newFrame];
  }
  _scroll = newScroll;
}

- (CGRect)boundingRect {
  CGRect bounding = CGRectNull;
  for (UIView* view : [self subviews]) {
    bounding = CGRectUnion(bounding, view.frame);
  }
  bounding.origin.x += _scroll.x;
  bounding.origin.y += _scroll.y;
  return bounding;
}

@end
