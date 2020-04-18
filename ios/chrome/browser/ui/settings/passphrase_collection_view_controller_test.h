// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_SETTINGS_PASSPHRASE_COLLECTION_VIEW_CONTROLLER_TEST_H_
#define IOS_CHROME_BROWSER_UI_SETTINGS_PASSPHRASE_COLLECTION_VIEW_CONTROLLER_TEST_H_

#include <memory>

#include "base/compiler_specific.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/sync/engine/cycle/sync_cycle_snapshot.h"
#include "google_apis/gaia/google_service_auth_error.h"
#import "ios/chrome/browser/ui/collection_view/collection_view_controller_test.h"
#include "ios/web/public/test/test_web_thread_bundle.h"
#include "testing/platform_test.h"

namespace browser_sync {
class ProfileSyncServiceMock;
}  // namespace browser_sync

namespace web {
class BrowserState;
}  // namespace web

class TestChromeBrowserState;
@class UINavigationController;
@class UIViewController;

// Base class for PassphraseCollectionViewController tests.
// Sets up a testing profile and a mock profile sync service, along with the
// supporting structure they require.
class PassphraseCollectionViewControllerTest
    : public CollectionViewControllerTest {
 public:
  static std::unique_ptr<KeyedService> CreateNiceProfileSyncServiceMock(
      web::BrowserState* context);

  PassphraseCollectionViewControllerTest();
  ~PassphraseCollectionViewControllerTest() override;

 protected:
  void SetUp() override;

  void SetUpNavigationController(UIViewController* test_controller);

  web::TestWebThreadBundle thread_bundle_;

  std::unique_ptr<TestChromeBrowserState> chrome_browser_state_;
  // Weak, owned by chrome_browser_state_.
  browser_sync::ProfileSyncServiceMock* fake_sync_service_;

  // Default return values for NiceMock<browser_sync::ProfileSyncServiceMock>.
  GoogleServiceAuthError default_auth_error_;
  syncer::SyncCycleSnapshot default_sync_cycle_snapshot_;

  // Dummy navigation stack for testing self-removal.
  // Only valid when SetUpNavigationController has been called.
  UIViewController* dummy_controller_;
  UINavigationController* nav_controller_;
};

#endif  // IOS_CHROME_BROWSER_UI_SETTINGS_PASSPHRASE_COLLECTION_VIEW_CONTROLLER_TEST_H_
