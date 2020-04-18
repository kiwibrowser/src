// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/payments/payment_request_edit_view_controller.h"

#include "base/mac/foundation_util.h"
#include "base/memory/ptr_util.h"
#import "ios/chrome/browser/ui/autofill/autofill_ui_type.h"
#import "ios/chrome/browser/ui/autofill/cells/autofill_edit_item.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_footer_item.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_switch_item.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_text_item.h"
#import "ios/chrome/browser/ui/collection_view/collection_view_controller_test.h"
#import "ios/chrome/browser/ui/payments/cells/payments_selector_edit_item.h"
#import "ios/chrome/browser/ui/payments/payment_request_edit_consumer.h"
#import "ios/chrome/browser/ui/payments/payment_request_edit_view_controller_data_source.h"
#import "ios/chrome/browser/ui/payments/payment_request_editor_field.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
NSString* const kTestTitle = @"title";
}  // namespace

@interface TestPaymentRequestEditMediator
    : NSObject<PaymentRequestEditViewControllerDataSource>

@property(nonatomic, weak) id<PaymentRequestEditConsumer> consumer;

@end

@implementation TestPaymentRequestEditMediator

@synthesize state = _state;
@synthesize consumer = _consumer;

- (NSString*)title {
  return kTestTitle;
}

- (CollectionViewItem*)headerItem {
  return [[CollectionViewTextItem alloc] init];
}

- (BOOL)shouldHideBackgroundForHeaderItem {
  return NO;
}

- (BOOL)shouldFormatValueForAutofillUIType:(AutofillUIType)type {
  return NO;
}

- (NSString*)formatValue:(NSString*)value autofillUIType:(AutofillUIType)type {
  return value;
}

- (UIImage*)iconIdentifyingEditorField:(EditorField*)field {
  return nil;
}

- (void)setConsumer:(id<PaymentRequestEditConsumer>)consumer {
  _consumer = consumer;
  [self.consumer setEditorFields:@[
    [[EditorField alloc] initWithAutofillUIType:AutofillUITypeProfileFullName
                                      fieldType:EditorFieldTypeTextField
                                          label:@""
                                          value:@""
                                       required:YES],
    [[EditorField alloc] initWithAutofillUIType:AutofillUITypeCreditCardExpYear
                                      fieldType:EditorFieldTypeSelector
                                          label:@""
                                          value:@""
                                       required:YES],
    [[EditorField alloc]
        initWithAutofillUIType:AutofillUITypeCreditCardSaveToChrome
                     fieldType:EditorFieldTypeSwitch
                         label:@""
                         value:@"YES"
                      required:YES],
  ]];
}

@end

class PaymentRequestEditViewControllerTest
    : public CollectionViewControllerTest {
 protected:
  CollectionViewController* InstantiateController() override {
    PaymentRequestEditViewController* viewController =
        [[PaymentRequestEditViewController alloc]
            initWithLayout:[[MDCCollectionViewFlowLayout alloc] init]
                     style:CollectionViewControllerStyleDefault];
    mediator_ = [[TestPaymentRequestEditMediator alloc] init];
    [mediator_ setConsumer:viewController];
    [viewController setDataSource:mediator_];
    return viewController;
  }

  PaymentRequestEditViewController* GetPaymentRequestEditViewController() {
    return base::mac::ObjCCastStrict<PaymentRequestEditViewController>(
        controller());
  }

  TestPaymentRequestEditMediator* mediator_ = nil;
};

// Tests that the correct number of items are displayed after loading the model.
TEST_F(PaymentRequestEditViewControllerTest, TestModel) {
  CreateController();
  CheckController();

  [GetPaymentRequestEditViewController() loadModel];
  CheckTitle(kTestTitle);

  // There is one section containing the header item, In addition to that, there
  // is one section for every form field (there are three fields in total) and
  // one for the footer.
  ASSERT_EQ(5, NumberOfSections());

  // The header section is the first section and has one item of the type
  // CollectionViewTextItem.
  ASSERT_EQ(1U, static_cast<unsigned int>(NumberOfItemsInSection(0)));
  id item = GetCollectionViewItem(0, 0);
  EXPECT_TRUE([item isMemberOfClass:[CollectionViewTextItem class]]);

  // The next section has one item of the type AutofillEditItem.
  ASSERT_EQ(1U, static_cast<unsigned int>(NumberOfItemsInSection(1)));
  item = GetCollectionViewItem(1, 0);
  EXPECT_TRUE([item isMemberOfClass:[AutofillEditItem class]]);

  // The next section has one item of the type PaymentsSelectorEditItem.
  ASSERT_EQ(1U, static_cast<unsigned int>(NumberOfItemsInSection(2)));
  item = GetCollectionViewItem(2, 0);
  EXPECT_TRUE([item isMemberOfClass:[PaymentsSelectorEditItem class]]);

  // The next section has one item of the type CollectionViewSwitchItem.
  ASSERT_EQ(1U, static_cast<unsigned int>(NumberOfItemsInSection(3)));
  item = GetCollectionViewItem(3, 0);
  EXPECT_TRUE([item isMemberOfClass:[CollectionViewSwitchItem class]]);

  // The footer section contains one item which is of the type
  // CollectionViewFooterItem.
  ASSERT_EQ(1U, static_cast<unsigned int>(NumberOfItemsInSection(3)));
  item = GetCollectionViewItem(4, 0);
  EXPECT_TRUE([item isMemberOfClass:[CollectionViewFooterItem class]]);
}
