// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/showcase/manual_fill/keyboard_accessory_view.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface KeyboardAccessoryView ()

@property(nonatomic, readonly, weak) id<KeyboardAccessoryViewDelegate> delegate;

@end

@implementation KeyboardAccessoryView

@synthesize delegate = _delegate;

- (instancetype)initWithDelegate:(id<KeyboardAccessoryViewDelegate>)delegate {
  self = [super initWithFrame:CGRectZero];
  if (self) {
    _delegate = delegate;
    // TODO:(javierrobles) abstract this colors to a constant.
    self.backgroundColor = [UIColor colorWithRed:245.0 / 255.0
                                           green:245.0 / 255.0
                                            blue:245.0 / 255.0
                                           alpha:1.0];

    UIColor* tintColor = [UIColor colorWithRed:115.0 / 255.0
                                         green:115.0 / 255.0
                                          blue:115.0 / 255.0
                                         alpha:1.0];

    UIButton* passwordButton = [UIButton buttonWithType:UIButtonTypeCustom];
    UIImage* keyImage = [UIImage imageNamed:@"ic_vpn_key"];
    [passwordButton setImage:keyImage forState:UIControlStateNormal];
    passwordButton.tintColor = tintColor;
    passwordButton.translatesAutoresizingMaskIntoConstraints = NO;
    [passwordButton addTarget:_delegate
                       action:@selector(passwordButtonPressed)
             forControlEvents:UIControlEventTouchUpInside];
    [self addSubview:passwordButton];

    UIButton* cardsButton = [UIButton buttonWithType:UIButtonTypeCustom];
    UIImage* cardImage = [UIImage imageNamed:@"ic_credit_card"];
    [cardsButton setImage:cardImage forState:UIControlStateNormal];
    cardsButton.tintColor = tintColor;
    cardsButton.translatesAutoresizingMaskIntoConstraints = NO;
    [cardsButton addTarget:_delegate
                    action:@selector(cardsButtonPressed)
          forControlEvents:UIControlEventTouchUpInside];
    [self addSubview:cardsButton];

    UIButton* accountButton = [UIButton buttonWithType:UIButtonTypeCustom];
    UIImage* accountImage = [UIImage imageNamed:@"ic_account_circle"];
    [accountButton setImage:accountImage forState:UIControlStateNormal];
    accountButton.tintColor = tintColor;
    accountButton.translatesAutoresizingMaskIntoConstraints = NO;
    [accountButton addTarget:_delegate
                      action:@selector(accountButtonPressed)
            forControlEvents:UIControlEventTouchUpInside];
    [self addSubview:accountButton];

    UIButton* arrrowUp = [UIButton buttonWithType:UIButtonTypeCustom];
    UIImage* arrrowUpImage = [UIImage imageNamed:@"ic_keyboard_arrow_up"];
    [arrrowUp setImage:arrrowUpImage forState:UIControlStateNormal];
    arrrowUp.tintColor = tintColor;
    arrrowUp.translatesAutoresizingMaskIntoConstraints = NO;
    [arrrowUp addTarget:_delegate
                  action:@selector(arrowUpPressed)
        forControlEvents:UIControlEventTouchUpInside];
    [self addSubview:arrrowUp];

    UIButton* arrrowDown = [UIButton buttonWithType:UIButtonTypeCustom];
    UIImage* arrrowDownImage = [UIImage imageNamed:@"ic_keyboard_arrow_down"];
    [arrrowDown setImage:arrrowDownImage forState:UIControlStateNormal];
    arrrowDown.tintColor = tintColor;
    arrrowDown.translatesAutoresizingMaskIntoConstraints = NO;
    [arrrowDown addTarget:_delegate
                   action:@selector(arrowDownPressed)
         forControlEvents:UIControlEventTouchUpInside];
    [self addSubview:arrrowDown];

    UIButton* doneButton = [UIButton buttonWithType:UIButtonTypeSystem];
    [doneButton setTitle:@"Cancel" forState:UIControlStateNormal];
    doneButton.tintColor = tintColor;
    doneButton.translatesAutoresizingMaskIntoConstraints = NO;
    [doneButton addTarget:_delegate
                   action:@selector(cancelButtonPressed)
         forControlEvents:UIControlEventTouchUpInside];
    [self addSubview:doneButton];

    NSLayoutXAxisAnchor* menuLeadingAnchor = self.leadingAnchor;
    if (@available(iOS 11, *)) {
      menuLeadingAnchor = self.safeAreaLayoutGuide.leadingAnchor;
    }

    NSLayoutXAxisAnchor* menuTrailingAnchor = self.trailingAnchor;
    if (@available(iOS 11, *)) {
      menuTrailingAnchor = self.safeAreaLayoutGuide.trailingAnchor;
    }

    [NSLayoutConstraint activateConstraints:@[
      // Vertical constraints.
      [passwordButton.heightAnchor constraintEqualToAnchor:self.heightAnchor],
      [passwordButton.topAnchor constraintEqualToAnchor:self.topAnchor],

      [cardsButton.heightAnchor constraintEqualToAnchor:self.heightAnchor],
      [cardsButton.topAnchor constraintEqualToAnchor:self.topAnchor],

      [accountButton.heightAnchor constraintEqualToAnchor:self.heightAnchor],
      [accountButton.topAnchor constraintEqualToAnchor:self.topAnchor],

      [arrrowUp.heightAnchor constraintEqualToAnchor:self.heightAnchor],
      [arrrowUp.topAnchor constraintEqualToAnchor:self.topAnchor],

      [arrrowDown.heightAnchor constraintEqualToAnchor:self.heightAnchor],
      [arrrowDown.topAnchor constraintEqualToAnchor:self.topAnchor],

      [doneButton.heightAnchor constraintEqualToAnchor:self.heightAnchor],
      [doneButton.topAnchor constraintEqualToAnchor:self.topAnchor],

      // Horizontal constraints.
      [passwordButton.leadingAnchor constraintEqualToAnchor:menuLeadingAnchor
                                                   constant:12],

      [cardsButton.leadingAnchor
          constraintEqualToAnchor:passwordButton.trailingAnchor
                         constant:8],

      [accountButton.leadingAnchor
          constraintEqualToAnchor:cardsButton.trailingAnchor
                         constant:8],

      [doneButton.trailingAnchor constraintEqualToAnchor:menuTrailingAnchor
                                                constant:-12],
      [arrrowDown.trailingAnchor
          constraintEqualToAnchor:doneButton.leadingAnchor
                         constant:-8],

      [arrrowUp.trailingAnchor constraintEqualToAnchor:arrrowDown.leadingAnchor
                                              constant:-8],
    ]];
  }

  return self;
}

@end
