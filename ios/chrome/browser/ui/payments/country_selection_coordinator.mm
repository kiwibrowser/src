// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/payments/country_selection_coordinator.h"

#include "base/strings/sys_string_conversions.h"
#import "ios/chrome/browser/ui/payments/payment_request_picker_row.h"
#include "ios/chrome/browser/ui/payments/payment_request_picker_view_controller.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// The delay in nano seconds before notifying the delegate of the selection.
// This is to let the user get a visual feedback of the selection before this
// view disappears.
const int64_t kDelegateNotificationDelayInNanoSeconds = 0.2 * NSEC_PER_SEC;
}  // namespace

@interface CountrySelectionCoordinator ()

@property(nonatomic, strong) PaymentRequestPickerViewController* viewController;

// Called when the user selects a country. The cell is checked, the UI is locked
// so that the user can't interact with it, then the delegate is notified after
// kDelegateNotificationDelayInNanoSeconds.
- (void)delayedNotifyDelegateOfSelection:(NSString*)countryCode;

@end

@implementation CountrySelectionCoordinator

@synthesize delegate = _delegate;
@synthesize countries = _countries;
@synthesize selectedCountryCode = _selectedCountryCode;
@synthesize viewController = _viewController;

- (void)start {
  // Create the rows to display in the view controller.
  NSMutableArray<PickerRow*>* rows = [[NSMutableArray alloc] init];
  __block PickerRow* selectedRow = nil;
  [self.countries
      enumerateKeysAndObjectsUsingBlock:^(NSString* countryCode,
                                          NSString* countryName, BOOL* stop) {
        PickerRow* row =
            [[PickerRow alloc] initWithLabel:countryName value:countryCode];
        [rows addObject:row];
        if (self.selectedCountryCode == countryCode)
          selectedRow = row;
      }];
  self.viewController =
      [[PaymentRequestPickerViewController alloc] initWithRows:rows
                                                      selected:selectedRow];
  self.viewController.delegate = self;

  DCHECK(self.baseViewController.navigationController);
  [self.baseViewController.navigationController
      pushViewController:self.viewController
                animated:YES];
}

- (void)stop {
  [self.viewController.navigationController popViewControllerAnimated:YES];
  self.viewController = nil;
}

#pragma mark - PaymentRequestPickerViewControllerDelegate

- (void)paymentRequestPickerViewController:
            (PaymentRequestPickerViewController*)controller
                              didSelectRow:(PickerRow*)row {
  [self delayedNotifyDelegateOfSelection:row.value];
}

- (void)paymentRequestPickerViewControllerDidFinish:
    (PaymentRequestPickerViewController*)controller {
  [self.delegate countrySelectionCoordinatorDidReturn:self];
}

#pragma mark - Helper methods

- (void)delayedNotifyDelegateOfSelection:(NSString*)countryCode {
  self.viewController.view.userInteractionEnabled = NO;
  __weak CountrySelectionCoordinator* weakSelf = self;
  dispatch_after(
      dispatch_time(DISPATCH_TIME_NOW, kDelegateNotificationDelayInNanoSeconds),
      dispatch_get_main_queue(), ^{
        weakSelf.viewController.view.userInteractionEnabled = YES;
        [weakSelf.delegate countrySelectionCoordinator:weakSelf
                              didSelectCountryWithCode:countryCode];
      });
}

@end
