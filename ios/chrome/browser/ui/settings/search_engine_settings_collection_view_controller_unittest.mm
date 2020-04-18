// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/search_engine_settings_collection_view_controller.h"

#include <memory>

#include "base/compiler_specific.h"
#include "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "components/search_engines/template_url_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#include "ios/chrome/browser/search_engines/template_url_service_factory.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_text_item.h"
#import "ios/chrome/browser/ui/collection_view/collection_view_controller_test.h"
#import "ios/third_party/material_components_ios/src/components/CollectionCells/src/MaterialCollectionCells.h"
#include "ios/web/public/test/test_web_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

class SearchEngineSettingsCollectionViewControllerTest
    : public CollectionViewControllerTest {
 protected:
  void SetUp() override {
    CollectionViewControllerTest::SetUp();
    TestChromeBrowserState::Builder test_cbs_builder;
    test_cbs_builder.AddTestingFactory(
        ios::TemplateURLServiceFactory::GetInstance(),
        ios::TemplateURLServiceFactory::GetDefaultFactory());
    chrome_browser_state_ = test_cbs_builder.Build();
    DefaultSearchManager::SetFallbackSearchEnginesDisabledForTesting(true);
    template_url_service_ = ios::TemplateURLServiceFactory::GetForBrowserState(
        chrome_browser_state_.get());
    template_url_service_->Load();
  }

  CollectionViewController* InstantiateController() override {
    return [[SearchEngineSettingsCollectionViewController alloc]
        initWithBrowserState:chrome_browser_state_.get()];
  }

  std::unique_ptr<TemplateURL> NewTemplateUrl(const std::string& shortName) {
    TemplateURLData data;
    data.SetShortName(base::ASCIIToUTF16(shortName));
    return std::unique_ptr<TemplateURL>(new TemplateURL(data));
  }

  void FillTemplateUrlService() {
    TemplateURL* defaultProvider =
        template_url_service_->Add(NewTemplateUrl("first_url"));
    template_url_service_->SetUserSelectedDefaultSearchProvider(
        defaultProvider);
    template_url_service_->Add(NewTemplateUrl("second_url"));
    template_url_service_->Add(NewTemplateUrl("third_url"));
  }

  void CheckModelMatchesTemplateURLs() {
    TemplateURLService::TemplateURLVector urls =
        template_url_service_->GetTemplateURLs();
    EXPECT_EQ(1, NumberOfSections());
    ASSERT_EQ(urls.size(),
              static_cast<unsigned int>(NumberOfItemsInSection(0)));
    for (unsigned int i = 0; i < urls.size(); ++i) {
      BOOL isDefault =
          template_url_service_->GetDefaultSearchProvider() == urls[i];

      CheckTextCellTitle(base::SysUTF16ToNSString(urls[i]->short_name()), 0, i);
      CollectionViewTextItem* textItem =
          base::mac::ObjCCastStrict<CollectionViewTextItem>(
              GetCollectionViewItem(0, i));
      EXPECT_EQ(isDefault ? MDCCollectionViewCellAccessoryCheckmark
                          : MDCCollectionViewCellAccessoryNone,
                textItem.accessoryType);
    }
  }

  web::TestWebThreadBundle thread_bundle_;
  std::unique_ptr<TestChromeBrowserState> chrome_browser_state_;
  TemplateURLService* template_url_service_;  // weak
};

TEST_F(SearchEngineSettingsCollectionViewControllerTest, TestNoUrl) {
  CreateController();
  CheckController();
  EXPECT_EQ(0, NumberOfSections());
}

TEST_F(SearchEngineSettingsCollectionViewControllerTest, TestWithUrlsLoaded) {
  FillTemplateUrlService();
  CreateController();
  CheckModelMatchesTemplateURLs();
}

TEST_F(SearchEngineSettingsCollectionViewControllerTest, TestWithAddedUrl) {
  FillTemplateUrlService();
  CreateController();

  TemplateURL* newUrl = template_url_service_->Add(NewTemplateUrl("new_url"));
  CheckModelMatchesTemplateURLs();

  template_url_service_->SetUserSelectedDefaultSearchProvider(newUrl);
  CheckModelMatchesTemplateURLs();

  DCHECK(newUrl != template_url_service_->GetTemplateURLs()[0]);
  template_url_service_->SetUserSelectedDefaultSearchProvider(
      template_url_service_->GetTemplateURLs()[0]);
  template_url_service_->Remove(newUrl);
  CheckModelMatchesTemplateURLs();
}

TEST_F(SearchEngineSettingsCollectionViewControllerTest, TestChangeProvider) {
  FillTemplateUrlService();
  CreateController();

  [controller() collectionView:[controller() collectionView]
      didSelectItemAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]];
  EXPECT_EQ(template_url_service_->GetTemplateURLs()[0],
            template_url_service_->GetDefaultSearchProvider());
  CheckModelMatchesTemplateURLs();

  [controller() collectionView:[controller() collectionView]
      didSelectItemAtIndexPath:[NSIndexPath indexPathForRow:1 inSection:0]];
  TemplateURL* url = template_url_service_->GetTemplateURLs()[1];
  EXPECT_EQ(url, template_url_service_->GetDefaultSearchProvider());

  // Check that the selection was written back to the prefs.
  const base::DictionaryValue* searchProviderDict =
      chrome_browser_state_->GetTestingPrefService()->GetDictionary(
          DefaultSearchManager::kDefaultSearchProviderDataPrefName);
  EXPECT_TRUE(searchProviderDict);
  base::string16 shortName;
  EXPECT_TRUE(searchProviderDict->GetString(DefaultSearchManager::kShortName,
                                            &shortName));
  EXPECT_EQ(url->short_name(), shortName);
  CheckModelMatchesTemplateURLs();
}

}  // namespace
