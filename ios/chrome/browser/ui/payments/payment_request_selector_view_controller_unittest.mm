// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/payments/payment_request_selector_view_controller.h"

#include "base/mac/foundation_util.h"
#import "ios/chrome/browser/ui/autofill/cells/status_item.h"
#import "ios/chrome/browser/ui/collection_view/collection_view_controller_test.h"
#import "ios/chrome/browser/ui/payments/cells/payments_text_item.h"
#import "ios/chrome/browser/ui/payments/payment_request_selector_view_controller_data_source.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
NSString* const kTestTitle = @"title";
}  // namespace

@interface TestPaymentRequestSelectorMediator
    : NSObject<PaymentRequestSelectorViewControllerDataSource>

@property(nonatomic, copy) NSArray<CollectionViewItem*>* items;

@property(nonatomic, readwrite, assign) PaymentRequestSelectorState state;

@end

@implementation TestPaymentRequestSelectorMediator

@synthesize state = _state;
@synthesize selectedItemIndex = _selectedItemIndex;
@synthesize items = _items;

- (instancetype)init {
  self = [super init];
  if (self) {
    _items =
        @[ [[PaymentsTextItem alloc] init], [[PaymentsTextItem alloc] init] ];
    _selectedItemIndex = 0;
  }
  return self;
}

#pragma mark - PaymentRequestSelectorViewControllerDataSource

- (BOOL)allowsEditMode {
  return NO;
}

- (NSString*)title {
  return kTestTitle;
}

- (CollectionViewItem*)headerItem {
  return [[CollectionViewItem alloc] init];
}

- (NSArray<CollectionViewItem*>*)selectableItems {
  return _items;
}

- (CollectionViewItem*)addButtonItem {
  return [[CollectionViewItem alloc] init];
}

@end

class PaymentRequestSelectorViewControllerTest
    : public CollectionViewControllerTest {
 protected:
  PaymentRequestSelectorViewControllerTest() {}

  CollectionViewController* InstantiateController() override {
    PaymentRequestSelectorViewController* viewController =
        [[PaymentRequestSelectorViewController alloc] init];
    mediator_ = [[TestPaymentRequestSelectorMediator alloc] init];
    [viewController setDataSource:mediator_];
    return viewController;
  }

  PaymentRequestSelectorViewController*
  GetPaymentRequestSelectorViewController() {
    return base::mac::ObjCCastStrict<PaymentRequestSelectorViewController>(
        controller());
  }

  TestPaymentRequestSelectorMediator* mediator_;
};

// Tests that the correct number of items are displayed after loading the model
// and that the correct item appears to be selected.
TEST_F(PaymentRequestSelectorViewControllerTest, TestModel) {
  CreateController();
  CheckController();

  [GetPaymentRequestSelectorViewController() loadModel];
  CheckTitle(kTestTitle);

  ASSERT_EQ(2, NumberOfSections());
  // One header item and two selectable items.
  ASSERT_EQ(3, NumberOfItemsInSection(0));

  // The first item should be of type CollectionViewItem.
  id item = GetCollectionViewItem(0, 0);
  ASSERT_TRUE([item isMemberOfClass:[CollectionViewItem class]]);

  // The next two selectable items should be of type PaymentsTextItem. The first
  // one should appear to be selected.
  item = GetCollectionViewItem(0, 1);
  ASSERT_TRUE([item isMemberOfClass:[PaymentsTextItem class]]);
  PaymentsTextItem* selectable_item = item;
  EXPECT_EQ(MDCCollectionViewCellAccessoryCheckmark,
            selectable_item.accessoryType);

  item = GetCollectionViewItem(0, 2);
  ASSERT_TRUE([item isMemberOfClass:[PaymentsTextItem class]]);
  selectable_item = item;
  EXPECT_EQ(MDCCollectionViewCellAccessoryNone, selectable_item.accessoryType);

  // One add button.
  ASSERT_EQ(1, NumberOfItemsInSection(1));

  // The item should be of type CollectionViewItem.
  item = GetCollectionViewItem(1, 0);
  EXPECT_TRUE([item isMemberOfClass:[CollectionViewItem class]]);
}

// Tests that the correct number of items are displayed after loading the model
// in the pending state.
TEST_F(PaymentRequestSelectorViewControllerTest, TestModelPendingState) {
  CreateController();
  CheckController();

  [mediator_ setState:PaymentRequestSelectorStatePending];
  [GetPaymentRequestSelectorViewController() loadModel];

  ASSERT_EQ(1, NumberOfSections());
  // There should be only one item.
  ASSERT_EQ(1, NumberOfItemsInSection(0));

  // The item should be of type StatusItem.
  id item = GetCollectionViewItem(0, 0);
  EXPECT_TRUE([item isMemberOfClass:[StatusItem class]]);
}
