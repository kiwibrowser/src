// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/autofill_profile_edit_collection_view_controller.h"

#include <memory>

#include "base/guid.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/browser/autofill_profile.h"
#include "components/autofill/core/browser/personal_data_manager.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/autofill/personal_data_manager_factory.h"
#include "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#import "ios/chrome/browser/ui/collection_view/collection_view_model.h"
#include "ios/chrome/browser/ui/settings/personal_data_manager_data_changed_observer.h"
#include "ios/web/public/test/test_web_thread_bundle.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

const char kTestFullName[] = "That Guy John";
const char kTestAddressLine1[] = "Some person's garage";

static NSArray* FindTextFieldDescendants(UIView* root) {
  NSMutableArray* textFields = [NSMutableArray array];
  NSMutableArray* descendants = [NSMutableArray array];

  [descendants addObject:root];

  while ([descendants count]) {
    UIView* view = [descendants objectAtIndex:0];
    if ([view isKindOfClass:[UITextField class]])
      [textFields addObject:view];

    [descendants addObjectsFromArray:[view subviews]];
    [descendants removeObjectAtIndex:0];
  }

  return textFields;
}

class AutofillProfileEditCollectionViewControllerTest : public PlatformTest {
 protected:
  AutofillProfileEditCollectionViewControllerTest() {
    TestChromeBrowserState::Builder test_cbs_builder;
    chrome_browser_state_ = test_cbs_builder.Build();
    chrome_browser_state_->CreateWebDataService();
    personal_data_manager_ =
        autofill::PersonalDataManagerFactory::GetForBrowserState(
            chrome_browser_state_.get());
    PersonalDataManagerDataChangedObserver observer(personal_data_manager_);

    std::string guid = base::GenerateGUID();

    autofill::AutofillProfile autofill_profile;
    autofill_profile =
        autofill::AutofillProfile(guid, "https://www.example.com/");
    autofill_profile.SetRawInfo(autofill::NAME_FULL,
                                base::UTF8ToUTF16(kTestFullName));
    autofill_profile.SetRawInfo(autofill::ADDRESS_HOME_LINE1,
                                base::UTF8ToUTF16(kTestAddressLine1));

    personal_data_manager_->SaveImportedProfile(autofill_profile);
    observer.Wait();  // Wait for the completion of the asynchronous operation.

    autofill_profile_edit_controller_ =
        [AutofillProfileEditCollectionViewController
            controllerWithProfile:autofill_profile
              personalDataManager:personal_data_manager_];
  }

  web::TestWebThreadBundle thread_bundle_;
  std::unique_ptr<TestChromeBrowserState> chrome_browser_state_;
  autofill::PersonalDataManager* personal_data_manager_;
  AutofillProfileEditCollectionViewController*
      autofill_profile_edit_controller_;
};

// Default test case of no addresses or credit cards.
TEST_F(AutofillProfileEditCollectionViewControllerTest, TestInitialization) {
  CollectionViewModel* model =
      [autofill_profile_edit_controller_ collectionViewModel];

  EXPECT_EQ(1, [model numberOfSections]);
  EXPECT_EQ(10, [model numberOfItemsInSection:0]);
}

// Adding a single address results in an address section.
TEST_F(AutofillProfileEditCollectionViewControllerTest, TestOneProfile) {
  CollectionViewModel* model =
      [autofill_profile_edit_controller_ collectionViewModel];
  UICollectionView* collectionView =
      [autofill_profile_edit_controller_ collectionView];

  EXPECT_EQ(1, [model numberOfSections]);
  EXPECT_EQ(10, [model numberOfItemsInSection:0]);

  NSIndexPath* path = [NSIndexPath indexPathForRow:0 inSection:0];

  UIView* cell =
      [autofill_profile_edit_controller_ collectionView:collectionView
                                 cellForItemAtIndexPath:path];
  EXPECT_TRUE([cell isKindOfClass:[MDCCollectionViewCell class]]);

  NSArray* textFields = FindTextFieldDescendants(cell);
  EXPECT_TRUE([textFields count] > 0);
  UITextField* field = [textFields objectAtIndex:0];
  EXPECT_TRUE([field isKindOfClass:[UITextField class]]);
  EXPECT_TRUE(
      [[field text] isEqualToString:base::SysUTF8ToNSString(kTestFullName)]);

  path = [NSIndexPath indexPathForRow:2 inSection:0];
  cell = [autofill_profile_edit_controller_ collectionView:collectionView
                                    cellForItemAtIndexPath:path];
  EXPECT_TRUE([cell isKindOfClass:[MDCCollectionViewCell class]]);
  textFields = FindTextFieldDescendants(cell);
  EXPECT_TRUE([textFields count] > 0);
  field = [textFields objectAtIndex:0];
  EXPECT_TRUE([field isKindOfClass:[UITextField class]]);
  EXPECT_TRUE([[field text]
      isEqualToString:base::SysUTF8ToNSString(kTestAddressLine1)]);
}

}  // namespace
