// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/import_data_collection_view_controller.h"

#include "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_detail_item.h"
#import "ios/chrome/browser/ui/collection_view/collection_view_controller_test.h"
#import "ios/chrome/browser/ui/collection_view/collection_view_model.h"
#import "ios/chrome/browser/ui/settings/cells/card_multiline_item.h"
#include "ios/chrome/grit/ios_strings.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface ImportDataControllerTestDelegate
    : NSObject<ImportDataControllerDelegate>
@property(nonatomic, readonly) BOOL didChooseClearDataPolicyCalled;
@property(nonatomic, readonly) ShouldClearData shouldClearData;
@end

@implementation ImportDataControllerTestDelegate

@synthesize didChooseClearDataPolicyCalled = _didChooseClearDataPolicyCalled;
@synthesize shouldClearData = _shouldClearData;

- (void)didChooseClearDataPolicy:(ImportDataCollectionViewController*)controller
                 shouldClearData:(ShouldClearData)shouldClearData {
  _didChooseClearDataPolicyCalled = YES;
  _shouldClearData = shouldClearData;
}

@end

namespace {

class ImportDataCollectionViewControllerTest
    : public CollectionViewControllerTest {
 public:
  ImportDataControllerTestDelegate* delegate() { return delegate_; }

 protected:
  void SetUp() override {
    CollectionViewControllerTest::SetUp();
    is_signed_in_ = true;
  }

  CollectionViewController* InstantiateController() override {
    delegate_ = [[ImportDataControllerTestDelegate alloc] init];
    return [[ImportDataCollectionViewController alloc]
        initWithDelegate:delegate_
               fromEmail:@"fromEmail@gmail.com"
                 toEmail:@"toEmail@gmail.com"
              isSignedIn:is_signed_in_];
  }

  void set_is_signed_in(bool is_signed_in) { is_signed_in_ = is_signed_in; }

  bool is_signed_in_;
  ImportDataControllerTestDelegate* delegate_;
};

TEST_F(ImportDataCollectionViewControllerTest, TestModelSignedIn) {
  CreateController();
  CheckController();
  ASSERT_EQ(2, NumberOfSections());
  EXPECT_EQ(1, NumberOfItemsInSection(0));
  CardMultilineItem* item = GetCollectionViewItem(0, 0);
  EXPECT_NSEQ(
      l10n_util::GetNSStringF(IDS_IOS_OPTIONS_IMPORT_DATA_HEADER,
                              base::SysNSStringToUTF16(@"fromEmail@gmail.com")),
      item.text);
  EXPECT_EQ(2, NumberOfItemsInSection(1));
  CheckTextCellTitleAndSubtitle(
      l10n_util::GetNSString(IDS_IOS_OPTIONS_IMPORT_DATA_KEEP_TITLE),
      l10n_util::GetNSStringF(IDS_IOS_OPTIONS_IMPORT_DATA_KEEP_SUBTITLE_SWITCH,
                              base::SysNSStringToUTF16(@"fromEmail@gmail.com")),
      1, 0);
  CheckAccessoryType(MDCCollectionViewCellAccessoryCheckmark, 1, 0);
  CheckTextCellTitleAndSubtitle(
      l10n_util::GetNSString(IDS_IOS_OPTIONS_IMPORT_DATA_IMPORT_TITLE),
      l10n_util::GetNSStringF(IDS_IOS_OPTIONS_IMPORT_DATA_IMPORT_SUBTITLE,
                              base::SysNSStringToUTF16(@"toEmail@gmail.com")),
      1, 1);
  CheckAccessoryType(MDCCollectionViewCellAccessoryNone, 1, 1);
}

TEST_F(ImportDataCollectionViewControllerTest, TestModelSignedOut) {
  set_is_signed_in(false);
  CreateController();
  CheckController();
  ASSERT_EQ(2, NumberOfSections());
  EXPECT_EQ(1, NumberOfItemsInSection(0));
  CardMultilineItem* item = GetCollectionViewItem(0, 0);
  EXPECT_NSEQ(
      l10n_util::GetNSStringF(IDS_IOS_OPTIONS_IMPORT_DATA_HEADER,
                              base::SysNSStringToUTF16(@"fromEmail@gmail.com")),
      item.text);
  EXPECT_EQ(2, NumberOfItemsInSection(1));
  CheckTextCellTitleAndSubtitle(
      l10n_util::GetNSString(IDS_IOS_OPTIONS_IMPORT_DATA_IMPORT_TITLE),
      l10n_util::GetNSStringF(IDS_IOS_OPTIONS_IMPORT_DATA_IMPORT_SUBTITLE,
                              base::SysNSStringToUTF16(@"toEmail@gmail.com")),
      1, 0);
  CheckAccessoryType(MDCCollectionViewCellAccessoryCheckmark, 1, 0);
  CheckTextCellTitleAndSubtitle(
      l10n_util::GetNSString(IDS_IOS_OPTIONS_IMPORT_DATA_KEEP_TITLE),
      l10n_util::GetNSString(IDS_IOS_OPTIONS_IMPORT_DATA_KEEP_SUBTITLE_SIGNIN),
      1, 1);
  CheckAccessoryType(MDCCollectionViewCellAccessoryNone, 1, 1);
}

// Tests that checking a checkbox correctly uncheck the other one.
TEST_F(ImportDataCollectionViewControllerTest, TestUniqueBoxChecked) {
  CreateController();

  ImportDataCollectionViewController* import_data_controller =
      base::mac::ObjCCastStrict<ImportDataCollectionViewController>(
          controller());
  NSIndexPath* importIndexPath = [NSIndexPath indexPathForItem:0 inSection:1];
  NSIndexPath* keepSeparateIndexPath =
      [NSIndexPath indexPathForItem:1 inSection:1];
  CollectionViewDetailItem* importItem =
      base::mac::ObjCCastStrict<CollectionViewDetailItem>(
          [import_data_controller.collectionViewModel
              itemAtIndexPath:importIndexPath]);
  CollectionViewDetailItem* keepSeparateItem =
      base::mac::ObjCCastStrict<CollectionViewDetailItem>(
          [import_data_controller.collectionViewModel
              itemAtIndexPath:keepSeparateIndexPath]);

  [import_data_controller collectionView:[import_data_controller collectionView]
                didSelectItemAtIndexPath:importIndexPath];
  EXPECT_EQ(MDCCollectionViewCellAccessoryCheckmark, importItem.accessoryType);
  EXPECT_EQ(MDCCollectionViewCellAccessoryNone, keepSeparateItem.accessoryType);

  [import_data_controller collectionView:[import_data_controller collectionView]
                didSelectItemAtIndexPath:keepSeparateIndexPath];
  EXPECT_EQ(MDCCollectionViewCellAccessoryNone, importItem.accessoryType);
  EXPECT_EQ(MDCCollectionViewCellAccessoryCheckmark,
            keepSeparateItem.accessoryType);
}

// Tests that the default choice when the user is signed in is Clear Data and
// that tapping continue will correctly select it. Regression test for
// crbug.com/649533
TEST_F(ImportDataCollectionViewControllerTest, TestDefaultChoiceSignedIn) {
  CreateController();

  EXPECT_FALSE(delegate().didChooseClearDataPolicyCalled);
  UIBarButtonItem* item = controller().navigationItem.rightBarButtonItem;
  ASSERT_TRUE(item);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  [item.target performSelector:item.action];
#pragma clang diagnostic pop

  EXPECT_TRUE(delegate().didChooseClearDataPolicyCalled);
  EXPECT_EQ(SHOULD_CLEAR_DATA_CLEAR_DATA, delegate().shouldClearData);
}

// Tests that the default choice when the user is signed out is Merge Data and
// that tapping continue will correctly select it. Regression test for
// crbug.com/649533
TEST_F(ImportDataCollectionViewControllerTest, TestDefaultChoiceSignedOut) {
  set_is_signed_in(false);
  CreateController();

  EXPECT_FALSE(delegate().didChooseClearDataPolicyCalled);
  UIBarButtonItem* item = controller().navigationItem.rightBarButtonItem;
  ASSERT_TRUE(item);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  [item.target performSelector:item.action];
#pragma clang diagnostic pop

  EXPECT_TRUE(delegate().didChooseClearDataPolicyCalled);
  EXPECT_EQ(SHOULD_CLEAR_DATA_MERGE_DATA, delegate().shouldClearData);
}

}  // namespace
