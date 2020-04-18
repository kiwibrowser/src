// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/public/web_state/ui/crw_web_view_content_view.h"

#import <WebKit/WebKit.h>
#include <cmath>
#include <limits>

#include "base/logging.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Background color RGB values for the content view which is displayed when the
// |_webView| is offset from the screen due to user interaction. Displaying this
// background color is handled by UIWebView but not WKWebView, so it needs to be
// set in CRWWebViewContentView to support both. The color value matches that
// used by UIWebView.
const CGFloat kBackgroundRGBComponents[] = {0.75f, 0.74f, 0.76f};

}  // namespace

@interface CRWWebViewContentView ()

// Changes web view frame to match |self.bounds| and optionally accommodates for
// |contentInset|.
- (void)updateWebViewFrame;

@end

@implementation CRWWebViewContentView

@synthesize contentInset = _contentInset;
@synthesize shouldUseViewContentInset = _shouldUseViewContentInset;
@synthesize scrollView = _scrollView;
@synthesize webView = _webView;

- (instancetype)initWithWebView:(UIView*)webView
                     scrollView:(UIScrollView*)scrollView {
  self = [super initWithFrame:CGRectZero];
  if (self) {
    DCHECK(webView);
    DCHECK(scrollView);
    DCHECK([scrollView isDescendantOfView:webView]);
    _webView = webView;
    _scrollView = scrollView;
  }
  return self;
}

- (instancetype)initForTesting {
  return [super initWithFrame:CGRectZero];
}

- (instancetype)initWithCoder:(NSCoder*)decoder {
  NOTREACHED();
  return nil;
}

- (instancetype)initWithFrame:(CGRect)frame {
  NOTREACHED();
  return nil;
}

- (void)didMoveToSuperview {
  [super didMoveToSuperview];
  if (self.superview) {
    self.autoresizingMask =
        UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    [self addSubview:_webView];
    self.backgroundColor = [UIColor colorWithRed:kBackgroundRGBComponents[0]
                                           green:kBackgroundRGBComponents[1]
                                            blue:kBackgroundRGBComponents[2]
                                           alpha:1.0];
  }
}

- (BOOL)becomeFirstResponder {
  return [_webView becomeFirstResponder];
}

- (void)setFrame:(CGRect)frame {
  if (CGRectEqualToRect(self.frame, frame))
    return;
  [super setFrame:frame];
  [self updateWebViewFrame];
}

- (void)setBounds:(CGRect)bounds {
  if (CGRectEqualToRect(self.bounds, bounds))
    return;
  [super setBounds:bounds];
  [self updateWebViewFrame];
}

#pragma mark Layout

- (void)layoutSubviews {
  [super layoutSubviews];
  [self updateWebViewFrame];
}

- (BOOL)isViewAlive {
  return YES;
}

- (UIEdgeInsets)contentInset {
  return self.shouldUseViewContentInset ? [_scrollView contentInset]
                                        : _contentInset;
}

- (void)setContentInset:(UIEdgeInsets)contentInset {
  CGFloat delta = std::fabs(_contentInset.top - contentInset.top) +
                  std::fabs(_contentInset.left - contentInset.left) +
                  std::fabs(_contentInset.bottom - contentInset.bottom) +
                  std::fabs(_contentInset.right - contentInset.right);
  if (delta <= std::numeric_limits<CGFloat>::epsilon())
    return;
  if (self.shouldUseViewContentInset) {
    [_scrollView setContentInset:contentInset];
  } else {
    // Update the content offset of the scroll view to match the padding
    // that will be included in the frame.
    CGFloat topPaddingChange = contentInset.top - _contentInset.top;
    CGPoint contentOffset = [_scrollView contentOffset];
    contentOffset.y += topPaddingChange;
    [_scrollView setContentOffset:contentOffset];
    _contentInset = contentInset;
    // Update web view frame immediately to make |contentInset| animatable.
    [self updateWebViewFrame];
    // Setting WKWebView frame can mistakenly reset contentOffset. Change it
    // back to the initial value if necessary.
    // TODO(crbug.com/645857): Remove this workaround once WebKit bug is
    // fixed.
    if ([_scrollView contentOffset].y != contentOffset.y) {
      [_scrollView setContentOffset:contentOffset];
    }
  }
}

- (void)setShouldUseViewContentInset:(BOOL)shouldUseViewContentInset {
  if (_shouldUseViewContentInset != shouldUseViewContentInset) {
    UIEdgeInsets oldContentInset = self.contentInset;
    self.contentInset = UIEdgeInsetsZero;
    _shouldUseViewContentInset = shouldUseViewContentInset;
    self.contentInset = oldContentInset;
  }
}

#pragma mark Private methods

- (void)updateWebViewFrame {
  CGRect webViewFrame = self.bounds;
  webViewFrame.size.height -= _contentInset.top + _contentInset.bottom;
  webViewFrame.origin.y += _contentInset.top;
  webViewFrame.size.width -= _contentInset.right + _contentInset.left;
  webViewFrame.origin.x += _contentInset.left;

  self.webView.frame = webViewFrame;
}

@end
