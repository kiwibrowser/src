// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/showcase/manual_fill/ipad_keyboard_proto_view_controller.h"

#import <WebKit/WebKit.h>

#import "ios/showcase/manual_fill/password_picker_view_controller.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface IPadKeyboardProtoViewController ()

/// The view controller presented or nil if none.
@property(nonatomic, weak) UIViewController* presentedPickerViewController;

@end

@implementation IPadKeyboardProtoViewController

@synthesize presentedPickerViewController = _presentedPickerViewController;

#pragma mark - Life Cycle

- (void)viewDidLoad {
  [super viewDidLoad];

  NSNotificationCenter* defaultCenter = [NSNotificationCenter defaultCenter];
  [defaultCenter addObserver:self
                    selector:@selector(handleKeyboardWillChangeFrame:)
                        name:UIKeyboardWillChangeFrameNotification
                      object:nil];
}

#pragma mark - Keyboard Notifications

- (void)handleKeyboardWillChangeFrame:(NSNotification*)notification {
  // TODO:(javierrobles) state why this needs to be in this notification.
  [self overrideAccessories];
}

#pragma mark - ManualFillContentDelegate

- (void)userDidPickContent:(NSString*)content {
  UIViewController* presentingViewController =
      self.presentedPickerViewController.presentingViewController;
  [presentingViewController dismissViewControllerAnimated:YES completion:nil];
  [self fillLastSelectedFieldWithString:content];

// This code jump by to the next field by invoking a method on an Apple's owned
// bar button.
// TODO:(javierrobles) use the action instead of NSSelectorFromString.
// TODO:(javierrobles) add safety.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  [[self.lastFirstResponder inputAssistantItem]
          .trailingBarButtonGroups[1]
          .barButtonItems.firstObject.target
      performSelector:NSSelectorFromString(@"_nextTapped:")
           withObject:nil];
#pragma clang diagnostic pop
}

#pragma mark Private Helpers

// Sets the accessory items of the keyboard to the custom options we want to
// show to the user. In this case an icon for passwords, an icon for addresses
// and a last one for credit cards.
- (void)overrideAccessories {
  UIImage* keyImage = [UIImage imageNamed:@"ic_vpn_key"];
  UIBarButtonItem* itemOne =
      [[UIBarButtonItem alloc] initWithImage:keyImage
                                       style:UIBarButtonItemStylePlain
                                      target:self
                                      action:@selector(presentCredentials:)];

  UIImage* accountImage = [UIImage imageNamed:@"ic_account_circle"];
  UIBarButtonItem* itemTwo =
      [[UIBarButtonItem alloc] initWithImage:accountImage
                                       style:UIBarButtonItemStylePlain
                                      target:self
                                      action:@selector(presentAddresses:)];

  UIImage* creditCardImage = [UIImage imageNamed:@"ic_credit_card"];
  UIBarButtonItem* itemThree =
      [[UIBarButtonItem alloc] initWithImage:creditCardImage
                                       style:UIBarButtonItemStylePlain
                                      target:self
                                      action:@selector(presentCards:)];

  UIBarButtonItem* itemChoose =
      [[UIBarButtonItem alloc] initWithTitle:@"Manual Fill"
                                       style:UIBarButtonItemStylePlain
                                      target:nil
                                      action:nil];

  UIBarButtonItemGroup* group = [[UIBarButtonItemGroup alloc]
      initWithBarButtonItems:@[ itemOne, itemTwo, itemThree ]
          representativeItem:itemChoose];

  UITextInputAssistantItem* item =
      [manualfill::GetFirstResponderSubview(self.view) inputAssistantItem];
  item.leadingBarButtonGroups = @[ group ];
}

// Presents the options for the manual fill as a popover.
//
// @param sender The item requesting the pop over, used for positioning.
- (void)presentPopOverForSender:(UIBarButtonItem*)sender {
  [self updateActiveFieldID];
  self.lastFirstResponder = manualfill::GetFirstResponderSubview(self.view);

  // TODO:(javierrobles) Test this on iOS 10.
  // TODO:(javierrobles) Support / dismiss on rotation.
  UIViewController* presenter;
  UIView* buttonView;
  id view = [sender valueForKey:@"view"];
  if ([view isKindOfClass:[UIView class]]) {
    buttonView = view;
    presenter = buttonView.window.rootViewController;
    while (presenter.childViewControllers.count) {
      presenter = presenter.childViewControllers.firstObject;
    }
  }
  if (!presenter) {
    // TODO:(javierrobles) log an error.
    return;
  }

  // TODO:(javierrobles) support addresses and credit cards.
  PasswordPickerViewController* passwordPickerViewController =
      [[PasswordPickerViewController alloc] initWithDelegate:self];
  passwordPickerViewController.modalPresentationStyle =
      UIModalPresentationPopover;

  [presenter presentViewController:passwordPickerViewController
                          animated:YES
                        completion:nil];

  UIPopoverPresentationController* presentationController =
      passwordPickerViewController.popoverPresentationController;
  presentationController.permittedArrowDirections =
      UIPopoverArrowDirectionDown | UIPopoverArrowDirectionUp;
  presentationController.sourceView = buttonView;
  presentationController.sourceRect = buttonView.bounds;
  self.presentedPickerViewController = passwordPickerViewController;
}

// Called when the user presses the "key" button. Causing a list of credentials
// to be shown.
//
// @param sender The bar button item pressed.
- (void)presentCredentials:(UIBarButtonItem*)sender {
  [self presentPopOverForSender:sender];
}

// Called when the user presses the "account" button. Causing a list of
// addresses to be shown.
//
// @param sender The bar button item pressed.
- (void)presentAddresses:(UIBarButtonItem*)sender {
  [self presentPopOverForSender:sender];
}

// Called when the user presses the "card" button. Causing a list of credit
// cards to be shown.
//
// @param sender The bar button item pressed.
- (void)presentCards:(UIBarButtonItem*)sender {
  [self presentPopOverForSender:sender];
}

@end
