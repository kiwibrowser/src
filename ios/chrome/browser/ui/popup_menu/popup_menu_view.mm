// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/popup_menu/popup_menu_view.h"

#import <QuartzCore/QuartzCore.h>

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// The image edge insets for popup_background.png.
NS_INLINE UIEdgeInsets PopupBackgroundInsets() {
  return UIEdgeInsetsMake(28, 28, 28, 28);
}
};

@implementation PopupMenuView {
  UIImageView* imageView_;
}

@synthesize delegate = delegate_;

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self)
    [self commonInitialization];

  return self;
}

- (instancetype)initWithCoder:(NSCoder*)aDecoder {
  self = [super initWithCoder:aDecoder];
  if (self)
    [self commonInitialization];

  return self;
}

- (void)commonInitialization {
  UIImage* image = [UIImage imageNamed:@"popup_background"];
  image = [image resizableImageWithCapInsets:PopupBackgroundInsets()];

  imageView_ = [[UIImageView alloc] initWithImage:image];
  [self addSubview:imageView_];
}

- (void)layoutSubviews {
  [super layoutSubviews];
  [imageView_ setFrame:[self bounds]];
}

- (BOOL)accessibilityPerformEscape {
  if (delegate_) {
    [delegate_ dismissPopupMenu];
    UIAccessibilityPostNotification(UIAccessibilityScreenChangedNotification,
                                    nil);
    return YES;
  }
  return NO;
}

@end
