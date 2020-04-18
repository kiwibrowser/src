// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/autofill_collection_view_controller.h"

#include "base/guid.h"
#include "base/mac/foundation_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/browser/autofill_profile.h"
#include "components/autofill/core/browser/credit_card.h"
#include "components/autofill/core/browser/personal_data_manager.h"
#include "ios/chrome/browser/autofill/personal_data_manager_factory.h"
#include "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#import "ios/chrome/browser/ui/collection_view/collection_view_controller_test.h"
#include "ios/chrome/browser/ui/settings/personal_data_manager_data_changed_observer.h"
#import "ios/testing/wait_util.h"
#include "ios/web/public/test/test_web_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface SettingsRootCollectionViewController (ExposedForTesting)
- (void)editButtonPressed;
@end

namespace {

class AutofillCollectionViewControllerTest
    : public CollectionViewControllerTest {
 protected:
  AutofillCollectionViewControllerTest() {
    TestChromeBrowserState::Builder test_cbs_builder;
    chrome_browser_state_ = test_cbs_builder.Build();
    // Profile import requires a PersonalDataManager which itself needs the
    // WebDataService; this is not initialized on a TestChromeBrowserState by
    // default.
    chrome_browser_state_->CreateWebDataService();
  }

  CollectionViewController* InstantiateController() override {
    return [[AutofillCollectionViewController alloc]
        initWithBrowserState:chrome_browser_state_.get()];
  }

  void AddProfile(const std::string& origin,
                  const std::string& name,
                  const std::string& address) {
    autofill::PersonalDataManager* personal_data_manager =
        autofill::PersonalDataManagerFactory::GetForBrowserState(
            chrome_browser_state_.get());
    PersonalDataManagerDataChangedObserver observer(personal_data_manager);

    autofill::AutofillProfile autofill_profile(base::GenerateGUID(), origin);
    autofill_profile.SetRawInfo(autofill::NAME_FULL, base::ASCIIToUTF16(name));
    autofill_profile.SetRawInfo(autofill::ADDRESS_HOME_LINE1,
                                base::ASCIIToUTF16(address));
    personal_data_manager->SaveImportedProfile(autofill_profile);
    observer.Wait();  // Wait for completion of the asynchronous operation.
  }

  web::TestWebThreadBundle thread_bundle_;
  std::unique_ptr<TestChromeBrowserState> chrome_browser_state_;
};

// Default test case of no addresses or credit cards.
TEST_F(AutofillCollectionViewControllerTest, TestInitialization) {
  CreateController();
  CheckController();

  // Expect one header section.
  EXPECT_EQ(1, NumberOfSections());
  // Expect header section to contain two rows.
  EXPECT_EQ(2, NumberOfItemsInSection(0));
}

// Adding a single address results in an address section.
TEST_F(AutofillCollectionViewControllerTest, TestOneProfile) {
  AddProfile("https://www.example.com/", "John Doe", "1 Main Street");
  CreateController();
  // Expect two sections (header and addresses section).
  EXPECT_EQ(2, NumberOfSections());
  // Expect header section to contain two rows.
  EXPECT_EQ(2, NumberOfItemsInSection(0));
  // Expect address section to contain 1 row (the address itself).
  EXPECT_EQ(1, NumberOfItemsInSection(1));
}

// Adding a single credit card results in a credit card section.
TEST_F(AutofillCollectionViewControllerTest, TestOneCreditCard) {
  autofill::PersonalDataManager* personal_data_manager =
      autofill::PersonalDataManagerFactory::GetForBrowserState(
          chrome_browser_state_.get());
  PersonalDataManagerDataChangedObserver observer(personal_data_manager);

  autofill::CreditCard credit_card(base::GenerateGUID(),
                                   "https://www.example.com/");
  credit_card.SetRawInfo(autofill::CREDIT_CARD_NAME_FULL,
                         base::ASCIIToUTF16("Alan Smithee"));
  credit_card.SetRawInfo(autofill::CREDIT_CARD_NUMBER,
                         base::ASCIIToUTF16("378282246310005"));
  personal_data_manager->SaveImportedCreditCard(credit_card);
  observer.Wait();  // Wait for completion of the asynchronous operation.

  CreateController();
  // Expect two sections (header and credit card section).
  EXPECT_EQ(2, NumberOfSections());
  // Expect header section to contain two rows.
  EXPECT_EQ(2, NumberOfItemsInSection(0));
  // Expect credit card section to contain 1 row (the credit card itself).
  EXPECT_EQ(1, NumberOfItemsInSection(1));
}

// Deleting the only profile results in item deletion and section deletion.
TEST_F(AutofillCollectionViewControllerTest, TestOneProfileItemDeleted) {
  AddProfile("https://www.example.com/", "John Doe", "1 Main Street");
  CreateController();
  // Expect two sections (header and addresses section).
  EXPECT_EQ(2, NumberOfSections());
  // Expect header section to contain two rows.
  EXPECT_EQ(2, NumberOfItemsInSection(0));
  // Expect address section to contain 1 row (the address itself).
  EXPECT_EQ(1, NumberOfItemsInSection(1));

  AutofillCollectionViewController* view_controller =
      base::mac::ObjCCastStrict<AutofillCollectionViewController>(controller());
  // Put the collectionView in 'edit' mode.
  [view_controller editButtonPressed];

  // This is a bit of a shortcut, since actually clicking on the 'delete'
  // button would be tough.
  void (^delete_item_with_wait)(int, int) = ^(int i, int j) {
    __block BOOL completion_called = NO;
    this->DeleteItem(i, j, ^{
      completion_called = YES;
    });
    EXPECT_TRUE(testing::WaitUntilConditionOrTimeout(
        testing::kWaitForUIElementTimeout, ^bool() {
          return completion_called;
        }));
  };

  autofill::PersonalDataManager* personal_data_manager =
      autofill::PersonalDataManagerFactory::GetForBrowserState(
          chrome_browser_state_.get());
  PersonalDataManagerDataChangedObserver observer(personal_data_manager);

  // This call cause a modification of the PersonalDataManager, so wait until
  // the asynchronous task complete in addition to waiting for the UI update.
  delete_item_with_wait(1, 0);
  observer.Wait();  // Wait for completion of the asynchronous operation.

  // Exit 'edit' mode.
  [view_controller editButtonPressed];

  // Verify the resulting UI.
  EXPECT_EQ(1, NumberOfSections());
  EXPECT_EQ(2, NumberOfItemsInSection(0));
}

}  // namespace
