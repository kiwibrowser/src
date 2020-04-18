// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/showcase/manual_fill/iphone_keyboard_proto_view_controller.h"

#import <UIKit/UIKit.h>

#import "ios/showcase/manual_fill/keyboard_complement_view.h"
#import "ios/showcase/manual_fill/password_picker_view_controller.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Finds the first responder in the shared app and returns its input accessory
// view.
//
// @return The `inputAccessoryView` of the first responder or nil if it was not
//         found.
UIView* GetKeyboardAccessoryView() {
  for (UIWindow* window in [[UIApplication sharedApplication] windows]) {
    UIView* firstResponder = manualfill::GetFirstResponderSubview(window);
    if (firstResponder) {
      return firstResponder.inputAccessoryView;
    }
  }
  return nil;
}

}  // namespace

@interface IPhoneKeyboardProtoViewController ()

// A strong reference to `inputAccessoryView` used in this class to jump
// between the web view fields.
@property(nonatomic) UIView* lastAccessoryInputView;

// This state boolean is used to determine if the keyboard input accessory view
// needs to be hidden when the keyboard animates out.
@property(nonatomic) BOOL shouldShowManualFillView;

// The manual fill view, this is what is shown behind the keyboard.
@property(nonatomic) KeyboardComplementView* manualFillView;

// TODO:(javierrobles) explore always hidding the host view instead of this.
// The keyboard accessory view, this is added above the WKWebView's.
@property(nonatomic) KeyboardAccessoryView* keyboardAccessoryView;

// This is a reference to the superview's superview of the input accessory view
// used to hide it when the keyboard dismisses to prevent input bar from
// appearing two times on screen.
@property(nonatomic, weak) UIView* accessoryHostView;

// TODO:(javierrobles) Update this constraint on rotation.
// This constraint controls the height of the manual fill view, it follows the
// size of the keyboard.
@property(nonatomic) NSLayoutConstraint* manualFillViewHeightConstraint;

// This controls the vertical position of the manual fill view. It is used to
// anchor it to the bottom, and to hide it.
@property(nonatomic) NSLayoutConstraint* manualFillViewBottomAnchorConstraint;

@end

@implementation IPhoneKeyboardProtoViewController
@synthesize lastAccessoryInputView = _lastAccessoryInputView;
@synthesize shouldShowManualFillView = _shouldShowManualFillView;
@synthesize manualFillView = _manualFillView;
@synthesize keyboardAccessoryView = _keyboardAccessoryView;
@synthesize accessoryHostView = _accessoryHostView;
@synthesize manualFillViewHeightConstraint = _manualFillViewHeightConstraint;
@synthesize manualFillViewBottomAnchorConstraint =
    _manualFillViewBottomAnchorConstraint;

#pragma mark - Life Cycle

- (void)viewDidLoad {
  [super viewDidLoad];

  self.manualFillView = [[KeyboardComplementView alloc] initWithDelegate:self];
  self.manualFillView.translatesAutoresizingMaskIntoConstraints = NO;
  [self.view addSubview:self.manualFillView];

  // Use an arbitrary height on load.
  self.manualFillViewHeightConstraint =
      [self.manualFillView.heightAnchor constraintEqualToConstant:200.0];
  self.manualFillViewBottomAnchorConstraint = [self.manualFillView.bottomAnchor
      constraintEqualToAnchor:self.view.bottomAnchor
                     constant:self.manualFillViewHeightConstraint.constant];

  [NSLayoutConstraint activateConstraints:@[
    [self.manualFillView.leadingAnchor
        constraintEqualToAnchor:self.view.leadingAnchor
                       constant:0.0],
    [self.manualFillView.trailingAnchor
        constraintEqualToAnchor:self.view.trailingAnchor],
    self.manualFillViewHeightConstraint,
    self.manualFillViewBottomAnchorConstraint
  ]];

  NSNotificationCenter* defaultCenter = [NSNotificationCenter defaultCenter];
  [defaultCenter addObserver:self
                    selector:@selector(handleKeyboardWillShowNotification:)
                        name:UIKeyboardWillShowNotification
                      object:nil];
  [defaultCenter addObserver:self
                    selector:@selector(handleKeyboardWillHideNotification:)
                        name:UIKeyboardWillHideNotification
                      object:nil];
  [defaultCenter addObserver:self
                    selector:@selector(handleKeyboardDidShowNotification:)
                        name:UIKeyboardDidShowNotification
                      object:nil];
}

- (void)dealloc {
  // Revert hidding the accessory host on dealloc.
  self.accessoryHostView.hidden = NO;
}

#pragma mark - Keyboard Notifications

- (void)handleKeyboardWillShowNotification:(NSNotification*)notification {
  self.shouldShowManualFillView = NO;
  if (!self.keyboardAccessoryView) {
    UIView* inputAccessoryView = GetKeyboardAccessoryView();
    if (inputAccessoryView) {
      self.keyboardAccessoryView =
          [[KeyboardAccessoryView alloc] initWithDelegate:self];
      self.keyboardAccessoryView.translatesAutoresizingMaskIntoConstraints = NO;
      [inputAccessoryView addSubview:self.keyboardAccessoryView];
      manualfill::AddSameConstraints(self.keyboardAccessoryView,
                                     inputAccessoryView);
    }
  }
}

- (void)handleKeyboardWillHideNotification:(NSNotification*)notification {
  if (self.shouldShowManualFillView &&
      // Only hidding the superview's superview will hide the gray background.
      // So we look for it. This may change with newer iOS versions.
      self.keyboardAccessoryView.superview.superview) {
    // This can be done more paranoic by checking and preserving the hidden
    // value.
    self.accessoryHostView = self.keyboardAccessoryView.superview.superview;
    self.accessoryHostView.hidden = YES;
  }
}

- (void)handleKeyboardDidShowNotification:(NSNotification*)notification {
  // Update the first responder and it's accessory view.
  self.lastFirstResponder = manualfill::GetFirstResponderSubview(self.view);
  // This is needed to keep a strong reference to the input accessory view.
  self.lastAccessoryInputView = self.lastFirstResponder.inputAccessoryView;

  // Update the constraints for the manual fill view.
  NSDictionary* userInfo = notification.userInfo;
  CGRect keyboardFrameEnd =
      [userInfo[UIKeyboardFrameEndUserInfoKey] CGRectValue];

  self.manualFillViewBottomAnchorConstraint.constant = 0.0;
  self.manualFillViewHeightConstraint.constant = keyboardFrameEnd.size.height;
}

#pragma mark - KeyboardAccessoryViewDelegate

- (void)accountButtonPressed {
  // TODO:(javierrobles) support an account picker.
  PasswordPickerViewController* passwordPickerViewController =
      [[PasswordPickerViewController alloc] initWithDelegate:self];
  [self prepareContainerWithViewController:passwordPickerViewController];
}

- (void)cardsButtonPressed {
  // TODO:(javierrobles) support a credit card picker.
  PasswordPickerViewController* passwordPickerViewController =
      [[PasswordPickerViewController alloc] initWithDelegate:self];
  [self prepareContainerWithViewController:passwordPickerViewController];
}

- (void)passwordButtonPressed {
  PasswordPickerViewController* passwordPickerViewController =
      [[PasswordPickerViewController alloc] initWithDelegate:self];
  [self prepareContainerWithViewController:passwordPickerViewController];
}

- (void)cancelButtonPressed {
  [self dismissComplementKeyboardAnimated:YES];
  [self.webView endEditing:YES];
}

- (void)arrowUpPressed {
  [self jumpPrevious];
}

- (void)arrowDownPressed {
  [self jumpNext];
}

#pragma mark - ManualFillViewDelegate

- (void)userDidPickContent:(NSString*)content {
  [self fillLastSelectedFieldWithString:content];
  [self jumpNext];
}

#pragma mark - Private Helpers

// Adds the view controller and its view to the respective hierarchies. The view
// is added in the container of the manual fill view.
//
// @param viewController The view controller to add.
- (void)prepareContainerWithViewController:(UIViewController*)viewController {
  [self updateActiveFieldID];
  self.shouldShowManualFillView = YES;

  // Remove any previous containment.
  NSArray* childViewControllers = [self.childViewControllers copy];
  for (UIViewController* childViewController in childViewControllers) {
    [childViewController willMoveToParentViewController:nil];
    [childViewController.view removeFromSuperview];
    [childViewController removeFromParentViewController];
  }

  [self addChildViewController:viewController];
  viewController.view.translatesAutoresizingMaskIntoConstraints = NO;
  [self.manualFillView.containerView addSubview:viewController.view];
  manualfill::AddSameConstraints(self.manualFillView.containerView,
                                 viewController.view);
  [viewController didMoveToParentViewController:self];

  [self.webView endEditing:YES];
}

// Dismisses the manual fill keyboard, can be animated.
//
// @param animated If dismissal should be animated.
- (void)dismissComplementKeyboardAnimated:(BOOL)animated {
  self.manualFillViewBottomAnchorConstraint.constant =
      self.manualFillViewHeightConstraint.constant;
  [self.view setNeedsLayout];
  // TODO:(javierrobles) This values should mimic the ones at keyboard dismiss.
  [UIView animateWithDuration:animated ? 0.25 : 0.0
      delay:0.0
      options:(7 << 16)
      animations:^{
        [self.view layoutIfNeeded];
      }
      completion:^(BOOL finished) {
        self.accessoryHostView.hidden = NO;
      }];
}

// Move the focus to the next field in the web view.
- (void)jumpNext {
// TODO:(javierrobles) use action and target instead of NSSelectorFromString
// (as Chromium currently does).
// TODO:(javierrobles) add safety.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  [self.lastAccessoryInputView
      performSelector:NSSelectorFromString(@"_nextTapped:")
           withObject:nil];
#pragma clang diagnostic pop
}

// Move the focus to the previous field in the web view.
- (void)jumpPrevious {
// TODO:(javierrobles) use action and target instead of NSSelectorFromString
// (as Chromium currently does).
// TODO:(javierrobles) add safety.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  [self.lastAccessoryInputView
      performSelector:NSSelectorFromString(@"_previousTapped:")
           withObject:nil];
#pragma clang diagnostic pop
}

@end
