// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/sync/ios_chrome_profile_sync_service_factory.h"

#include <stddef.h>

#include <vector>

#include "base/command_line.h"
#include "base/task_scheduler/task_scheduler.h"
#include "components/browser_sync/browser_sync_switches.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/sync/base/model_type.h"
#include "components/sync/driver/data_type_controller.h"
#include "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#include "ios/web/public/test/test_web_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

using syncer::DataTypeController;

class IOSChromeProfileSyncServiceFactoryTest : public PlatformTest {
 public:
  IOSChromeProfileSyncServiceFactoryTest() {
    TestChromeBrowserState::Builder browser_state_builder;
    chrome_browser_state_ = browser_state_builder.Build();
  }

  void SetUp() override {
    // Some services will only be created if there is a WebDataService.
    chrome_browser_state_->CreateWebDataService();
  }

  void TearDown() override {
    base::TaskScheduler::GetInstance()->FlushForTesting();
  }

 protected:
  // Returns the collection of default datatypes.
  std::vector<syncer::ModelType> DefaultDatatypes() {
    std::vector<syncer::ModelType> datatypes;

    // Common types.
    datatypes.push_back(syncer::AUTOFILL);
    datatypes.push_back(syncer::AUTOFILL_PROFILE);
    datatypes.push_back(syncer::AUTOFILL_WALLET_DATA);
    datatypes.push_back(syncer::AUTOFILL_WALLET_METADATA);
    datatypes.push_back(syncer::BOOKMARKS);
    datatypes.push_back(syncer::DEVICE_INFO);
    datatypes.push_back(syncer::FAVICON_TRACKING);
    datatypes.push_back(syncer::FAVICON_IMAGES);
    datatypes.push_back(syncer::HISTORY_DELETE_DIRECTIVES);
    datatypes.push_back(syncer::PASSWORDS);
    datatypes.push_back(syncer::PREFERENCES);
    datatypes.push_back(syncer::PRIORITY_PREFERENCES);
    datatypes.push_back(syncer::READING_LIST);
    datatypes.push_back(syncer::SESSIONS);
    datatypes.push_back(syncer::PROXY_TABS);
    datatypes.push_back(syncer::TYPED_URLS);
    datatypes.push_back(syncer::USER_EVENTS);

    return datatypes;
  }

  // Returns the number of default datatypes.
  size_t DefaultDatatypesCount() { return DefaultDatatypes().size(); }

  // Asserts that all the default datatypes are in |types|, except
  // for |exception_types|, which are asserted to not be in |types|.
  void CheckDefaultDatatypesInSetExcept(syncer::ModelTypeSet types,
                                        syncer::ModelTypeSet exception_types) {
    std::vector<syncer::ModelType> defaults = DefaultDatatypes();
    std::vector<syncer::ModelType>::iterator iter;
    for (iter = defaults.begin(); iter != defaults.end(); ++iter) {
      if (exception_types.Has(*iter))
        EXPECT_FALSE(types.Has(*iter))
            << *iter << " found in dataypes map, shouldn't be there.";
      else
        EXPECT_TRUE(types.Has(*iter)) << *iter << " not found in datatypes map";
    }
  }

  void SetDisabledTypes(syncer::ModelTypeSet disabled_types) {
    base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
        switches::kDisableSyncTypes,
        syncer::ModelTypeSetToString(disabled_types));
  }

  ios::ChromeBrowserState* chrome_browser_state() {
    return chrome_browser_state_.get();
  }

 private:
  web::TestWebThreadBundle thread_bundle_;
  std::unique_ptr<TestChromeBrowserState> chrome_browser_state_;
};

// Verify that the disable sync flag disables creation of the sync service.
TEST_F(IOSChromeProfileSyncServiceFactoryTest, DisableSyncFlag) {
  base::CommandLine::ForCurrentProcess()->AppendSwitch(switches::kDisableSync);
  EXPECT_FALSE(IOSChromeProfileSyncServiceFactory::GetForBrowserState(
      chrome_browser_state()));
}

// Verify that a normal (no command line flags) PSS can be created and
// properly initialized.
TEST_F(IOSChromeProfileSyncServiceFactoryTest, CreatePSSDefault) {
  browser_sync::ProfileSyncService* pss =
      IOSChromeProfileSyncServiceFactory::GetForBrowserState(
          chrome_browser_state());
  syncer::ModelTypeSet types = pss->GetRegisteredDataTypes();
  EXPECT_EQ(DefaultDatatypesCount(), types.Size());
  CheckDefaultDatatypesInSetExcept(types, syncer::ModelTypeSet());
}

// Verify that a PSS with a disabled datatype can be created and properly
// initialized.
TEST_F(IOSChromeProfileSyncServiceFactoryTest, CreatePSSDisableOne) {
  syncer::ModelTypeSet disabled_types(syncer::AUTOFILL);
  SetDisabledTypes(disabled_types);
  browser_sync::ProfileSyncService* pss =
      IOSChromeProfileSyncServiceFactory::GetForBrowserState(
          chrome_browser_state());
  syncer::ModelTypeSet types = pss->GetRegisteredDataTypes();
  EXPECT_EQ(DefaultDatatypesCount() - disabled_types.Size(), types.Size());
  CheckDefaultDatatypesInSetExcept(types, disabled_types);
}

// Verify that a PSS with multiple disabled datatypes can be created and
// properly initialized.
TEST_F(IOSChromeProfileSyncServiceFactoryTest, CreatePSSDisableMultiple) {
  syncer::ModelTypeSet disabled_types(syncer::AUTOFILL_PROFILE,
                                      syncer::BOOKMARKS);
  SetDisabledTypes(disabled_types);
  browser_sync::ProfileSyncService* pss =
      IOSChromeProfileSyncServiceFactory::GetForBrowserState(
          chrome_browser_state());
  syncer::ModelTypeSet types = pss->GetRegisteredDataTypes();
  EXPECT_EQ(DefaultDatatypesCount() - disabled_types.Size(), types.Size());
  CheckDefaultDatatypesInSetExcept(types, disabled_types);
}
