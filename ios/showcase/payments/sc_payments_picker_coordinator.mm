// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/showcase/payments/sc_payments_picker_coordinator.h"

#import "ios/chrome/browser/ui/payments/payment_request_picker_row.h"
#import "ios/chrome/browser/ui/payments/payment_request_picker_view_controller.h"
#import "ios/showcase/common/protocol_alerter.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface SCPaymentsPickerCoordinator ()

@property(nonatomic, strong)
    PaymentRequestPickerViewController* pickerViewController;
@property(nonatomic, strong) ProtocolAlerter* alerter;

@end

@implementation SCPaymentsPickerCoordinator

@synthesize baseViewController = _baseViewController;
@synthesize pickerViewController = _pickerViewController;
@synthesize alerter = _alerter;

- (void)start {
  self.alerter = [[ProtocolAlerter alloc] initWithProtocols:@[
    @protocol(PaymentRequestPickerViewControllerDelegate)
  ]];
  self.alerter.baseViewController = self.baseViewController;

  NSArray<PickerRow*>* rows = [self rows];
  _pickerViewController = [[PaymentRequestPickerViewController alloc]
      initWithRows:rows
          selected:rows[rows.count - 1]];
  [_pickerViewController setTitle:@"Select a country"];
  [_pickerViewController
      setDelegate:static_cast<id<PaymentRequestPickerViewControllerDelegate>>(
                      self.alerter)];

  [self.baseViewController pushViewController:_pickerViewController
                                     animated:YES];
}

#pragma mark - Helper methods

- (NSArray<PickerRow*>*)rows {
  return @[
    [[PickerRow alloc] initWithLabel:@"Chile" value:@"CHL"],
    [[PickerRow alloc] initWithLabel:@"Canada" value:@"CAN"],
    [[PickerRow alloc] initWithLabel:@"Belgium" value:@"BEL"],
    [[PickerRow alloc] initWithLabel:@"España" value:@"ESP"],
    [[PickerRow alloc] initWithLabel:@"México" value:@"MEX"],
    [[PickerRow alloc] initWithLabel:@"Brazil" value:@"BRA"],
    [[PickerRow alloc] initWithLabel:@"China" value:@"CHN"]
  ];
}

@end
