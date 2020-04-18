// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/login/easy_unlock/easy_unlock_service.h"
#include "chrome/browser/extensions/api/screenlock_private/screenlock_private_api.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/signin/identity_manager_factory.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chromeos/components/proximity_auth/screenlock_bridge.h"
#include "content/public/browser/notification_service.h"
#include "extensions/browser/api/test/test_api.h"
#include "extensions/browser/notification_types.h"
#include "extensions/common/switches.h"
#include "services/identity/public/cpp/identity_manager.h"
#include "services/identity/public/cpp/identity_test_utils.h"

namespace extensions {

namespace {

const char kAttemptClickAuthMessage[] = "attemptClickAuth";
const char kTestExtensionId[] = "lkegkdgachcnekllcdfkijonogckdnjo";
const char kTestUser[] = "testuser@gmail.com";

}  // namespace

class ScreenlockPrivateApiTest : public ExtensionApiTest,
                                 public content::NotificationObserver {
 public:
  ScreenlockPrivateApiTest() {}

  ~ScreenlockPrivateApiTest() override {}

  // ExtensionApiTest
  void SetUpCommandLine(base::CommandLine* command_line) override {
    ExtensionApiTest::SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII(
        extensions::switches::kWhitelistedExtensionID, kTestExtensionId);

  }

  void SetUpOnMainThread() override {
    identity::IdentityManager* identity_manager =
        IdentityManagerFactory::GetForProfile(profile());
    identity::MakePrimaryAccountAvailable(
        SigninManagerFactory::GetForProfile(profile()),
        ProfileOAuth2TokenServiceFactory::GetForProfile(profile()),
        identity_manager, kTestUser);
    test_account_id_ = AccountId::FromUserEmailGaiaId(
        kTestUser, identity_manager->GetPrimaryAccountInfo().gaia);
    registrar_.Add(this,
                   extensions::NOTIFICATION_EXTENSION_TEST_MESSAGE,
                   content::NotificationService::AllSources());
    ExtensionApiTest::SetUpOnMainThread();
  }

  void TearDownOnMainThread() override {
    ExtensionApiTest::TearDownOnMainThread();
    registrar_.RemoveAll();
  }

 protected:
  // content::NotificationObserver override:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override {
    const std::string& message =
        content::Details<std::pair<std::string, bool*>>(details).ptr()->first;
    if (message == kAttemptClickAuthMessage) {
      proximity_auth::ScreenlockBridge::Get()->lock_handler()->SetAuthType(
          test_account_id_, proximity_auth::mojom::AuthType::USER_CLICK,
          base::string16());
      chromeos::EasyUnlockService::Get(profile())->AttemptAuth(
          test_account_id_);
    }
  }

  // Loads |extension_name| and waits for a pass / fail notification.
  void RunTest(const std::string& extension_name) {
    ASSERT_TRUE(RunComponentExtensionTest(extension_name)) << message_;
  }

 private:
  AccountId test_account_id_;
  content::NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(ScreenlockPrivateApiTest);
};

// Locking is currently implemented only on ChromeOS.
#if defined(OS_CHROMEOS)

// Flaky under MSan. http://crbug.com/478091
#if defined(MEMORY_SANITIZER)
#define MAYBE_LockUnlock DISABLED_LockUnlock
#else
#define MAYBE_LockUnlock LockUnlock
#endif

IN_PROC_BROWSER_TEST_F(ScreenlockPrivateApiTest, MAYBE_LockUnlock) {
  RunTest("screenlock_private/lock_unlock");
}

IN_PROC_BROWSER_TEST_F(ScreenlockPrivateApiTest, AuthType) {
  RunTest("screenlock_private/auth_type");
}

#endif  // defined(OS_CHROMEOS)

}  // namespace extensions
