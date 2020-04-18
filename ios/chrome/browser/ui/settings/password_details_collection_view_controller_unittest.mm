// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/password_details_collection_view_controller.h"

#include "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "components/autofill/core/common/password_form.h"
#import "ios/chrome/browser/ui/collection_view/collection_view_controller_test.h"
#import "ios/chrome/browser/ui/settings/cells/password_details_item.h"
#import "ios/chrome/browser/ui/settings/reauthentication_module.h"
#import "ios/chrome/browser/web/chrome_web_test.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ios/chrome/test/app/password_test_util.h"
#include "ios/web/public/test/test_web_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface MockSavePasswordsCollectionViewController
    : NSObject<PasswordDetailsCollectionViewControllerDelegate>

- (void)deletePassword:(const autofill::PasswordForm&)passwordForm;

@end

@implementation MockSavePasswordsCollectionViewController

- (void)deletePassword:(const autofill::PasswordForm&)passwordForm {
}

@end

namespace {

NSString* const kSite = @"https://testorigin.com/";
NSString* const kUsername = @"testusername";
NSString* const kPassword = @"testpassword";

// Indices related to the layout for a non-blacklisted, non-federated password.
const int kSiteSection = 0;
const int kSiteItem = 0;
const int kCopySiteButtonItem = 1;

const int kUsernameSection = 1;
const int kUsernameItem = 0;
const int kCopyUsernameButtonItem = 1;

const int kPasswordSection = 2;
const int kPasswordItem = 0;
const int kCopyPasswordButtonItem = 1;
const int kShowHideButtonItem = 2;

const int kDeleteSection = 3;
const int kDeleteButtonItem = 0;

class PasswordDetailsCollectionViewControllerTest
    : public CollectionViewControllerTest {
 protected:
  PasswordDetailsCollectionViewControllerTest() {
    origin_ = kSite;
    form_.username_value = base::SysNSStringToUTF16(kUsername);
    form_.password_value = base::SysNSStringToUTF16(kPassword);
    form_.signon_realm = base::SysNSStringToUTF8(origin_);
    form_.origin = GURL(form_.signon_realm);
  }

  void SetUp() override {
    CollectionViewControllerTest::SetUp();
    delegate_ = [[MockSavePasswordsCollectionViewController alloc] init];
    reauthentication_module_ = [[MockReauthenticationModule alloc] init];
    reauthentication_module_.shouldSucceed = YES;
  }

  CollectionViewController* InstantiateController() override {
    return [[PasswordDetailsCollectionViewController alloc]
          initWithPasswordForm:form_
                      delegate:delegate_
        reauthenticationModule:reauthentication_module_];
  }

  web::TestWebThreadBundle thread_bundle_;
  MockSavePasswordsCollectionViewController* delegate_;
  MockReauthenticationModule* reauthentication_module_;
  NSString* origin_;
  autofill::PasswordForm form_;
};

TEST_F(PasswordDetailsCollectionViewControllerTest,
       TestInitialization_NormalPassword) {
  CreateController();
  CheckController();
  EXPECT_EQ(4, NumberOfSections());
  // Site section
  EXPECT_EQ(2, NumberOfItemsInSection(kSiteSection));
  CheckSectionHeaderWithId(IDS_IOS_SHOW_PASSWORD_VIEW_SITE, kSiteSection);
  PasswordDetailsItem* siteItem =
      GetCollectionViewItem(kSiteSection, kSiteItem);
  EXPECT_NSEQ(origin_, siteItem.text);
  EXPECT_TRUE(siteItem.showingText);
  CheckTextCellTitleWithId(IDS_IOS_SETTINGS_SITE_COPY_BUTTON, kSiteSection,
                           kCopySiteButtonItem);
  // Username section
  EXPECT_EQ(2, NumberOfItemsInSection(kUsernameSection));
  CheckSectionHeaderWithId(IDS_IOS_SHOW_PASSWORD_VIEW_USERNAME,
                           kUsernameSection);
  PasswordDetailsItem* usernameItem =
      GetCollectionViewItem(kUsernameSection, kUsernameItem);
  EXPECT_NSEQ(kUsername, usernameItem.text);
  EXPECT_TRUE(usernameItem.showingText);
  CheckTextCellTitleWithId(IDS_IOS_SETTINGS_USERNAME_COPY_BUTTON,
                           kUsernameSection, kCopyUsernameButtonItem);
  // Password section
  EXPECT_EQ(3, NumberOfItemsInSection(kPasswordSection));
  CheckSectionHeaderWithId(IDS_IOS_SHOW_PASSWORD_VIEW_PASSWORD,
                           kPasswordSection);
  PasswordDetailsItem* passwordItem =
      GetCollectionViewItem(kPasswordSection, kPasswordItem);
  EXPECT_NSEQ(kPassword, passwordItem.text);
  EXPECT_FALSE(passwordItem.showingText);
  CheckTextCellTitleWithId(IDS_IOS_SETTINGS_PASSWORD_COPY_BUTTON,
                           kPasswordSection, kCopyPasswordButtonItem);
  CheckTextCellTitleWithId(IDS_IOS_SETTINGS_PASSWORD_SHOW_BUTTON,
                           kPasswordSection, kShowHideButtonItem);
  // Delete section
  EXPECT_EQ(1, NumberOfItemsInSection(kDeleteSection));
  CheckTextCellTitleWithId(IDS_IOS_SETTINGS_PASSWORD_DELETE_BUTTON,
                           kDeleteSection, kDeleteButtonItem);
}

TEST_F(PasswordDetailsCollectionViewControllerTest,
       TestInitialization_Blacklisted) {
  constexpr int kBlacklistedSiteSection = 0;
  constexpr int kBlacklistedSiteItem = 0;
  constexpr int kBlacklistedCopySiteButtonItem = 1;

  constexpr int kBlacklistedDeleteSection = 1;
  constexpr int kBlacklistedDeleteButtonItem = 0;

  form_.username_value.clear();
  form_.password_value.clear();
  form_.blacklisted_by_user = true;
  CreateController();
  CheckController();
  EXPECT_EQ(2, NumberOfSections());
  // Site section
  EXPECT_EQ(2, NumberOfItemsInSection(kBlacklistedSiteSection));
  CheckSectionHeaderWithId(IDS_IOS_SHOW_PASSWORD_VIEW_SITE,
                           kBlacklistedSiteSection);
  PasswordDetailsItem* siteItem =
      GetCollectionViewItem(kBlacklistedSiteSection, kBlacklistedSiteItem);
  EXPECT_NSEQ(origin_, siteItem.text);
  EXPECT_TRUE(siteItem.showingText);
  CheckTextCellTitleWithId(IDS_IOS_SETTINGS_SITE_COPY_BUTTON,
                           kBlacklistedSiteSection,
                           kBlacklistedCopySiteButtonItem);
  // Delete section
  EXPECT_EQ(1, NumberOfItemsInSection(kBlacklistedDeleteSection));
  CheckTextCellTitleWithId(IDS_IOS_SETTINGS_PASSWORD_DELETE_BUTTON,
                           kBlacklistedDeleteSection,
                           kBlacklistedDeleteButtonItem);
}

TEST_F(PasswordDetailsCollectionViewControllerTest,
       TestInitialization_Federated) {
  constexpr int kFederatedSiteSection = 0;
  constexpr int kFederatedSiteItem = 0;
  constexpr int kFederatedCopySiteButtonItem = 1;

  constexpr int kFederatedUsernameSection = 1;
  constexpr int kFederatedUsernameItem = 0;
  constexpr int kFederatedCopyUsernameButtonItem = 1;

  constexpr int kFederatedFederationSection = 2;
  constexpr int kFederatedFederationItem = 0;

  constexpr int kFederatedDeleteSection = 3;
  constexpr int kFederatedDeleteButtonItem = 0;

  form_.password_value.clear();
  form_.federation_origin =
      url::Origin::Create(GURL("https://famous.provider.net"));
  CreateController();
  CheckController();
  EXPECT_EQ(4, NumberOfSections());
  // Site section
  EXPECT_EQ(2, NumberOfItemsInSection(kFederatedSiteSection));
  CheckSectionHeaderWithId(IDS_IOS_SHOW_PASSWORD_VIEW_SITE,
                           kFederatedSiteSection);
  PasswordDetailsItem* siteItem =
      GetCollectionViewItem(kFederatedSiteSection, kFederatedSiteItem);
  EXPECT_NSEQ(origin_, siteItem.text);
  EXPECT_TRUE(siteItem.showingText);
  CheckTextCellTitleWithId(IDS_IOS_SETTINGS_SITE_COPY_BUTTON,
                           kFederatedSiteSection, kFederatedCopySiteButtonItem);
  // Username section
  EXPECT_EQ(2, NumberOfItemsInSection(kFederatedUsernameSection));
  CheckSectionHeaderWithId(IDS_IOS_SHOW_PASSWORD_VIEW_USERNAME,
                           kFederatedUsernameSection);
  PasswordDetailsItem* usernameItem =
      GetCollectionViewItem(kFederatedUsernameSection, kFederatedUsernameItem);
  EXPECT_NSEQ(kUsername, usernameItem.text);
  EXPECT_TRUE(usernameItem.showingText);
  CheckTextCellTitleWithId(IDS_IOS_SETTINGS_USERNAME_COPY_BUTTON,
                           kFederatedUsernameSection,
                           kFederatedCopyUsernameButtonItem);
  // Federated section
  EXPECT_EQ(1, NumberOfItemsInSection(kFederatedFederationSection));
  CheckSectionHeaderWithId(IDS_IOS_SHOW_PASSWORD_VIEW_FEDERATION,
                           kFederatedFederationSection);
  PasswordDetailsItem* federationItem = GetCollectionViewItem(
      kFederatedFederationSection, kFederatedFederationItem);
  EXPECT_NSEQ(@"famous.provider.net", federationItem.text);
  EXPECT_TRUE(federationItem.showingText);
  // Delete section
  EXPECT_EQ(1, NumberOfItemsInSection(kFederatedDeleteSection));
  CheckTextCellTitleWithId(IDS_IOS_SETTINGS_PASSWORD_DELETE_BUTTON,
                           kFederatedDeleteSection, kFederatedDeleteButtonItem);
}

struct SimplifyOriginTestData {
  GURL origin;
  NSString* expectedSimplifiedOrigin;
};

TEST_F(PasswordDetailsCollectionViewControllerTest, SimplifyOrigin) {
  SimplifyOriginTestData test_data[] = {
      {GURL("http://test.com/index.php"), @"test.com"},
      {GURL("https://example.com/index.php"), @"example.com"},
      {GURL("android://"
            "Qllt1FacrB0NYCeSFvmudHvssWBPFfC54EbtHTpFxukvw2wClI1rafcVB3kQOMxfJg"
            "xbVAkGXvC_A52kbPL1EQ==@com.parkingpanda.mobile/"),
       @"mobile.parkingpanda.com"}};

  for (const auto& data : test_data) {
    origin_ = base::SysUTF8ToNSString(data.origin.spec());
    form_.signon_realm = base::SysNSStringToUTF8(origin_);
    form_.origin = GURL(form_.signon_realm);
    CreateController();
    EXPECT_NSEQ(data.expectedSimplifiedOrigin, controller().title)
        << " for origin " << data.origin;
    ResetController();
  }
}

TEST_F(PasswordDetailsCollectionViewControllerTest, CopySite) {
  CreateController();
  [controller() collectionView:[controller() collectionView]
      didSelectItemAtIndexPath:[NSIndexPath indexPathForRow:kCopySiteButtonItem
                                                  inSection:kSiteSection]];
  UIPasteboard* generalPasteboard = [UIPasteboard generalPasteboard];
  EXPECT_NSEQ(origin_, generalPasteboard.string);
}

TEST_F(PasswordDetailsCollectionViewControllerTest, CopyUsername) {
  CreateController();
  [controller() collectionView:[controller() collectionView]
      didSelectItemAtIndexPath:[NSIndexPath
                                   indexPathForRow:kCopyUsernameButtonItem
                                         inSection:kUsernameSection]];
  UIPasteboard* generalPasteboard = [UIPasteboard generalPasteboard];
  EXPECT_NSEQ(kUsername, generalPasteboard.string);
}

TEST_F(PasswordDetailsCollectionViewControllerTest, ShowPassword) {
  CreateController();
  [controller() collectionView:[controller() collectionView]
      didSelectItemAtIndexPath:[NSIndexPath indexPathForRow:kShowHideButtonItem
                                                  inSection:kPasswordSection]];
  PasswordDetailsItem* passwordItem =
      GetCollectionViewItem(kPasswordSection, kPasswordItem);
  EXPECT_NSEQ(kPassword, passwordItem.text);
  EXPECT_TRUE(passwordItem.showingText);
  EXPECT_NSEQ(
      l10n_util::GetNSString(IDS_IOS_SETTINGS_PASSWORD_REAUTH_REASON_SHOW),
      reauthentication_module_.localizedReasonForAuthentication);
  CheckTextCellTitleWithId(IDS_IOS_SETTINGS_PASSWORD_HIDE_BUTTON,
                           kPasswordSection, kShowHideButtonItem);
}

TEST_F(PasswordDetailsCollectionViewControllerTest, HidePassword) {
  CreateController();
  // First show the password.
  [controller() collectionView:[controller() collectionView]
      didSelectItemAtIndexPath:[NSIndexPath indexPathForRow:kShowHideButtonItem
                                                  inSection:kPasswordSection]];
  // Then hide it.
  [controller() collectionView:[controller() collectionView]
      didSelectItemAtIndexPath:[NSIndexPath indexPathForRow:kShowHideButtonItem
                                                  inSection:kPasswordSection]];
  PasswordDetailsItem* passwordItem =
      GetCollectionViewItem(kPasswordSection, kPasswordItem);
  EXPECT_NSEQ(kPassword, passwordItem.text);
  EXPECT_FALSE(passwordItem.showingText);
  CheckTextCellTitleWithId(IDS_IOS_SETTINGS_PASSWORD_SHOW_BUTTON,
                           kPasswordSection, kShowHideButtonItem);
}

TEST_F(PasswordDetailsCollectionViewControllerTest, CopyPassword) {
  CreateController();
  [controller() collectionView:[controller() collectionView]
      didSelectItemAtIndexPath:[NSIndexPath
                                   indexPathForRow:kCopyPasswordButtonItem
                                         inSection:kPasswordSection]];
  UIPasteboard* generalPasteboard = [UIPasteboard generalPasteboard];
  EXPECT_NSEQ(kPassword, generalPasteboard.string);
  EXPECT_NSEQ(
      l10n_util::GetNSString(IDS_IOS_SETTINGS_PASSWORD_REAUTH_REASON_COPY),
      reauthentication_module_.localizedReasonForAuthentication);
}

}  // namespace
