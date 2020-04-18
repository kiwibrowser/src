// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/signin_error_notifier_ash.h"

#include <stddef.h>

#include <memory>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/login/users/mock_user_manager.h"
#include "chrome/browser/notifications/notification_display_service_tester.h"
#include "chrome/browser/signin/fake_signin_manager_builder.h"
#include "chrome/browser/signin/signin_error_controller_factory.h"
#include "chrome/browser/signin/signin_error_notifier_factory_ash.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/test/base/browser_with_test_window_test.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/signin/core/browser/fake_auth_status_provider.h"
#include "components/signin/core/browser/signin_error_controller.h"
#include "components/signin/core/browser/signin_manager.h"
#include "components/user_manager/scoped_user_manager.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/message_center/public/cpp/notification.h"

namespace {

static const char kTestAccountId[] = "testing_profile";

// Notification ID corresponding to kProfileSigninNotificationId +
// kTestAccountId.
static const char kNotificationId[] =
    "chrome://settings/signin/testing_profile";

class SigninErrorNotifierTest : public BrowserWithTestWindowTest {
 public:
  void SetUp() override {
    BrowserWithTestWindowTest::SetUp();

    mock_user_manager_ = new chromeos::MockUserManager();
    user_manager_enabler_ = std::make_unique<user_manager::ScopedUserManager>(
        base::WrapUnique(mock_user_manager_));

    error_controller_ =
        SigninErrorControllerFactory::GetForProfile(GetProfile());
    SigninErrorNotifierFactory::GetForProfile(GetProfile());
    display_service_ =
        std::make_unique<NotificationDisplayServiceTester>(profile());
  }

  TestingProfile::TestingFactories GetTestingFactories() override {
    return {{SigninManagerFactory::GetInstance(), BuildFakeSigninManagerBase}};
  }

 protected:
  SigninErrorController* error_controller_;
  std::unique_ptr<NotificationDisplayServiceTester> display_service_;
  chromeos::MockUserManager* mock_user_manager_;  // Not owned.
  std::unique_ptr<user_manager::ScopedUserManager> user_manager_enabler_;
};

TEST_F(SigninErrorNotifierTest, NoErrorAuthStatusProviders) {
  EXPECT_FALSE(display_service_->GetNotification(kNotificationId));
  {
    // Add a provider (removes itself on exiting this scope).
    FakeAuthStatusProvider provider(error_controller_);
    EXPECT_FALSE(display_service_->GetNotification(kNotificationId));
  }
  EXPECT_FALSE(display_service_->GetNotification(kNotificationId));
}

TEST_F(SigninErrorNotifierTest, ErrorAuthStatusProvider) {
  {
    FakeAuthStatusProvider provider(error_controller_);
    EXPECT_FALSE(display_service_->GetNotification(kNotificationId));
    {
      FakeAuthStatusProvider error_provider(error_controller_);
      error_provider.SetAuthError(
          kTestAccountId,
          GoogleServiceAuthError(
              GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS));
      EXPECT_TRUE(display_service_->GetNotification(kNotificationId));
    }
    // error_provider is removed now that we've left that scope.
    EXPECT_FALSE(display_service_->GetNotification(kNotificationId));
  }
  // All providers should be removed now.
  EXPECT_FALSE(display_service_->GetNotification(kNotificationId));
}

TEST_F(SigninErrorNotifierTest, AuthStatusProviderErrorTransition) {
  {
    FakeAuthStatusProvider provider0(error_controller_);
    FakeAuthStatusProvider provider1(error_controller_);
    provider0.SetAuthError(
        kTestAccountId,
        GoogleServiceAuthError(
            GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS));

    base::Optional<message_center::Notification> notification =
        display_service_->GetNotification(kNotificationId);
    ASSERT_TRUE(notification);
    base::string16 message = notification->message();
    EXPECT_FALSE(message.empty());

    // Now set another auth error and clear the original.
    provider1.SetAuthError(
        kTestAccountId,
        GoogleServiceAuthError(
            GoogleServiceAuthError::UNEXPECTED_SERVICE_RESPONSE));
    provider0.SetAuthError(
        kTestAccountId,
        GoogleServiceAuthError::AuthErrorNone());

    notification = display_service_->GetNotification(kNotificationId);
    ASSERT_TRUE(notification);
    base::string16 new_message = notification->message();
    EXPECT_FALSE(new_message.empty());

    ASSERT_NE(new_message, message);

    provider1.SetAuthError(
        kTestAccountId, GoogleServiceAuthError::AuthErrorNone());
    EXPECT_FALSE(display_service_->GetNotification(kNotificationId));
  }
}

// Verify that SigninErrorNotifier ignores certain errors.
TEST_F(SigninErrorNotifierTest, AuthStatusEnumerateAllErrors) {
  typedef struct {
    GoogleServiceAuthError::State error_state;
    bool is_error;
  } ErrorTableEntry;

  ErrorTableEntry table[] = {
    { GoogleServiceAuthError::NONE, false },
    { GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS, true },
    { GoogleServiceAuthError::USER_NOT_SIGNED_UP, true },
    { GoogleServiceAuthError::CONNECTION_FAILED, false },
    { GoogleServiceAuthError::CAPTCHA_REQUIRED, true },
    { GoogleServiceAuthError::ACCOUNT_DELETED, true },
    { GoogleServiceAuthError::ACCOUNT_DISABLED, true },
    { GoogleServiceAuthError::SERVICE_UNAVAILABLE, false },
    { GoogleServiceAuthError::TWO_FACTOR, true },
    { GoogleServiceAuthError::REQUEST_CANCELED, false },
    { GoogleServiceAuthError::HOSTED_NOT_ALLOWED_DEPRECATED, false },
    { GoogleServiceAuthError::UNEXPECTED_SERVICE_RESPONSE, true },
    { GoogleServiceAuthError::SERVICE_ERROR, true },
    { GoogleServiceAuthError::WEB_LOGIN_REQUIRED, true },
  };
  static_assert(arraysize(table) == GoogleServiceAuthError::NUM_STATES,
      "table size should match number of auth error types");

  for (size_t i = 0; i < arraysize(table); ++i) {
    if (GoogleServiceAuthError::IsDeprecated(table[i].error_state))
      continue;
    FakeAuthStatusProvider provider(error_controller_);
    provider.SetAuthError(kTestAccountId,
                          GoogleServiceAuthError(table[i].error_state));
    base::Optional<message_center::Notification> notification =
        display_service_->GetNotification(kNotificationId);
    ASSERT_EQ(table[i].is_error, !!notification);
    if (table[i].is_error) {
      EXPECT_FALSE(notification->title().empty());
      EXPECT_FALSE(notification->message().empty());
      EXPECT_EQ((size_t)1, notification->buttons().size());
    }
  }
}

}  // namespace
