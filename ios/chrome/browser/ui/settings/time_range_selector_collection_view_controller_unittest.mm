// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/time_range_selector_collection_view_controller.h"

#include "base/files/file_path.h"
#include "base/test/scoped_task_environment.h"
#include "components/browsing_data/core/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/pref_service_mock_factory.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_text_item.h"
#import "ios/chrome/browser/ui/collection_view/collection_view_controller_test.h"
#include "ios/chrome/grit/ios_strings.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/ocmock/OCMock/OCMock.h"
#import "third_party/ocmock/gtest_support.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface TimeRangeSelectorCollectionViewController (ExposedForTesting)
- (void)updatePrefValue:(int)prefValue;
@end

namespace {

const NSInteger kNumberOfItems = 5;

class TimeRangeSelectorCollectionViewControllerTest
    : public CollectionViewControllerTest {
 protected:
  TimeRangeSelectorCollectionViewControllerTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::UI) {}

  void SetUp() override {
    CollectionViewControllerTest::SetUp();
    pref_service_ = CreateLocalState();
    delegate_ = [OCMockObject
        mockForProtocol:@protocol(
                            TimeRangeSelectorCollectionViewControllerDelegate)];
    CreateController();
  }

  CollectionViewController* InstantiateController() override {
    time_range_selector_controller_ =
        [[TimeRangeSelectorCollectionViewController alloc]
            initWithPrefs:pref_service_.get()
                 delegate:delegate_];
    return time_range_selector_controller_;
  }

  std::unique_ptr<PrefService> CreateLocalState() {
    scoped_refptr<PrefRegistrySimple> registry(new PrefRegistrySimple());
    registry->RegisterIntegerPref(browsing_data::prefs::kDeleteTimePeriod, 0);

    sync_preferences::PrefServiceMockFactory factory;
    return factory.Create(registry.get());
  }

  // Verifies that the cell at |item| in |section| has the given |accessory|
  // type.
  void CheckTextItemAccessoryType(
      MDCCollectionViewCellAccessoryType accessory_type,
      int section,
      int item) {
    CollectionViewTextItem* cell = GetCollectionViewItem(section, item);
    EXPECT_EQ(accessory_type, cell.accessoryType);
  }

  base::test::ScopedTaskEnvironment scoped_task_environment_;
  std::unique_ptr<PrefService> pref_service_;
  id delegate_;
  TimeRangeSelectorCollectionViewController* time_range_selector_controller_;
};

TEST_F(TimeRangeSelectorCollectionViewControllerTest, TestModel) {
  CheckController();
  EXPECT_EQ(1, NumberOfSections());

  // No section header + 5 rows
  EXPECT_EQ(kNumberOfItems, NumberOfItemsInSection(0));

  CheckTextItemAccessoryType(MDCCollectionViewCellAccessoryCheckmark, 0, 0);
  CheckTextItemAccessoryType(MDCCollectionViewCellAccessoryNone, 0, 1);
  CheckTextItemAccessoryType(MDCCollectionViewCellAccessoryNone, 0, 2);
  CheckTextItemAccessoryType(MDCCollectionViewCellAccessoryNone, 0, 3);
  CheckTextItemAccessoryType(MDCCollectionViewCellAccessoryNone, 0, 4);

  CheckTextCellTitleWithId(
      IDS_IOS_CLEAR_BROWSING_DATA_TIME_RANGE_OPTION_PAST_HOUR, 0, 0);
  CheckTextCellTitleWithId(
      IDS_IOS_CLEAR_BROWSING_DATA_TIME_RANGE_OPTION_PAST_DAY, 0, 1);
  CheckTextCellTitleWithId(
      IDS_IOS_CLEAR_BROWSING_DATA_TIME_RANGE_OPTION_PAST_WEEK, 0, 2);
  CheckTextCellTitleWithId(
      IDS_IOS_CLEAR_BROWSING_DATA_TIME_RANGE_OPTION_LAST_FOUR_WEEKS, 0, 3);
  CheckTextCellTitleWithId(
      IDS_IOS_CLEAR_BROWSING_DATA_TIME_RANGE_OPTION_BEGINNING_OF_TIME, 0, 4);
}

TEST_F(TimeRangeSelectorCollectionViewControllerTest, TestUpdateCheckedState) {
  CheckController();
  ASSERT_EQ(1, NumberOfSections());
  ASSERT_EQ(kNumberOfItems, NumberOfItemsInSection(0));

  for (NSInteger checkedItem = 0; checkedItem < kNumberOfItems; ++checkedItem) {
    [time_range_selector_controller_ updatePrefValue:checkedItem];
    for (NSInteger item = 0; item < kNumberOfItems; ++item) {
      if (item == checkedItem) {
        CheckTextItemAccessoryType(MDCCollectionViewCellAccessoryCheckmark, 0,
                                   item);
      } else {
        CheckTextItemAccessoryType(MDCCollectionViewCellAccessoryNone, 0, item);
      }
    }
  }
}

TEST_F(TimeRangeSelectorCollectionViewControllerTest, TestUpdatePrefValue) {
  CheckController();
  UICollectionView* collectionView =
      time_range_selector_controller_.collectionView;
  for (NSInteger checkedItem = 0; checkedItem < kNumberOfItems; ++checkedItem) {
    NSIndexPath* indexPath =
        [NSIndexPath indexPathForItem:checkedItem inSection:0];
    [[delegate_ expect]
        timeRangeSelectorViewController:time_range_selector_controller_
                    didSelectTimePeriod:static_cast<browsing_data::TimePeriod>(
                                            checkedItem)];
    [time_range_selector_controller_ collectionView:collectionView
                           didSelectItemAtIndexPath:indexPath];
    EXPECT_EQ(
        pref_service_->GetInteger(browsing_data::prefs::kDeleteTimePeriod),
        checkedItem);
    EXPECT_OCMOCK_VERIFY(delegate_);
  }
}

}  // namespace
