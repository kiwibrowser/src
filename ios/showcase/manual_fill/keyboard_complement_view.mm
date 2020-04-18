// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/showcase/manual_fill/keyboard_complement_view.h"

#import "ios/showcase/manual_fill/keyboard_accessory_view.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation KeyboardComplementView

@synthesize containerView = _containerView;

- (instancetype)initWithDelegate:(id<KeyboardAccessoryViewDelegate>)delegate {
  self = [super initWithFrame:CGRectZero];
  if (self) {
    // TODO:(javierrobles) abstract this color to a constant.
    self.backgroundColor = [UIColor colorWithRed:245.0 / 255.0
                                           green:245.0 / 255.0
                                            blue:245.0 / 255.0
                                           alpha:1.0];

    KeyboardAccessoryView* toolbar =
        [[KeyboardAccessoryView alloc] initWithDelegate:delegate];
    toolbar.translatesAutoresizingMaskIntoConstraints = NO;
    [self addSubview:toolbar];

    UIView* containerView = [[UIView alloc] init];
    containerView.translatesAutoresizingMaskIntoConstraints = NO;
    [self addSubview:containerView];
    _containerView = containerView;

    [NSLayoutConstraint activateConstraints:@[
      [toolbar.leadingAnchor constraintEqualToAnchor:self.leadingAnchor
                                            constant:0.0],
      [toolbar.trailingAnchor constraintEqualToAnchor:self.trailingAnchor],
      [toolbar.topAnchor constraintEqualToAnchor:self.topAnchor],
      [toolbar.heightAnchor constraintEqualToConstant:44],

      [containerView.topAnchor constraintEqualToAnchor:toolbar.bottomAnchor],
      [containerView.bottomAnchor constraintEqualToAnchor:self.bottomAnchor],
      [containerView.leadingAnchor constraintEqualToAnchor:self.leadingAnchor],
      [containerView.trailingAnchor
          constraintEqualToAnchor:self.trailingAnchor],
    ]];
  }
  return self;
}

@end
