// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/showcase/payments/sc_payments_selector_coordinator.h"

#include "base/logging.h"
#import "ios/chrome/browser/ui/collection_view/collection_view_model.h"
#import "ios/chrome/browser/ui/payments/cells/payments_is_selectable.h"
#import "ios/chrome/browser/ui/payments/cells/payments_text_item.h"
#import "ios/chrome/browser/ui/payments/payment_request_selector_view_controller.h"
#import "ios/chrome/browser/ui/payments/payment_request_selector_view_controller_data_source.h"
#import "ios/showcase/common/protocol_alerter.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface SCPaymentsSelectorCoordinator ()<
    PaymentRequestSelectorViewControllerDataSource>

// Index for the currently selected item or NSUIntegerMax if there is none.
@property(nonatomic, readwrite) NSUInteger selectedItemIndex;

// The view controller to present.
@property(nonatomic, strong)
    PaymentRequestSelectorViewController* selectorViewController;

// The selectable items created and managed by the
// PaymentRequestSelectorViewControllerDataSource.
@property(nonatomic, strong)
    NSArray<CollectionViewItem<PaymentsIsSelectable>*>* items;

// The alerter that conforms to PaymentRequestSelectorViewControllerDelegate
// and displays a UIAlert every time delegate methods are called.
@property(nonatomic, strong) ProtocolAlerter* alerter;

@end

@implementation SCPaymentsSelectorCoordinator

@synthesize state = _state;
@synthesize selectedItemIndex = _selectedItemIndex;
@synthesize baseViewController = _baseViewController;
@synthesize selectorViewController = _selectorViewController;
@synthesize items = _items;
@synthesize alerter = _alerter;

- (void)start {
  self.items = [self createItems];
  self.selectedItemIndex = NSUIntegerMax;

  self.alerter = [[ProtocolAlerter alloc] initWithProtocols:@[
    @protocol(PaymentRequestSelectorViewControllerDelegate)
  ]];
  self.alerter.baseViewController = self.baseViewController;

  self.selectorViewController = [[PaymentRequestSelectorViewController alloc]
      initWithLayout:[[MDCCollectionViewFlowLayout alloc] init]
               style:CollectionViewControllerStyleAppBar];
  [self.selectorViewController setTitle:@"Select an item"];
  [self.selectorViewController setDataSource:self];
  [self.selectorViewController loadModel];
  [self.selectorViewController
      setDelegate:static_cast<id<PaymentRequestSelectorViewControllerDelegate>>(
                      self.alerter)];
  [self.baseViewController pushViewController:self.selectorViewController
                                     animated:YES];
}

#pragma mark - PaymentRequestSelectorViewControllerDataSource

- (BOOL)allowsEditMode {
  return NO;
}

- (NSString*)title {
  return @"Title";
}

- (CollectionViewItem*)headerItem {
  return [self createItemWithText:@"Header item"];
}

- (NSArray<CollectionViewItem<PaymentsIsSelectable>*>*)selectableItems {
  return self.items;
}

- (CollectionViewItem*)addButtonItem {
  return [self createItemWithText:@"Add an item"];
}

#pragma mark - Helper methods

- (NSArray<CollectionViewItem<PaymentsIsSelectable>*>*)createItems {
  return @[
    [self createItemWithText:@"First selectable item"],
    [self createItemWithText:@"Second selectable item"],
    [self createItemWithText:@"Third selectable item"],
    [self createItemWithText:@"Fourth selectable item"]
  ];
}

- (CollectionViewItem<PaymentsIsSelectable>*)createItemWithText:
    (NSString*)text {
  PaymentsTextItem* item = [[PaymentsTextItem alloc] init];
  item.text = text;
  return item;
}

@end
