// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/core/browser/signin_error_controller.h"

#include <stddef.h>

#include <functional>
#include <memory>

#include "base/macros.h"
#include "components/signin/core/browser/fake_auth_status_provider.h"
#include "testing/gtest/include/gtest/gtest.h"

static const char kTestAccountId[] = "testuser@test.com";
static const char kOtherTestAccountId[] = "otheruser@test.com";

TEST(SigninErrorControllerTest, NoErrorAuthStatusProviders) {
  SigninErrorController error_controller(
      SigninErrorController::AccountMode::ANY_ACCOUNT);
  std::unique_ptr<FakeAuthStatusProvider> provider;

  // No providers.
  ASSERT_FALSE(error_controller.HasError());

  // Add a provider.
  provider.reset(new FakeAuthStatusProvider(&error_controller));
  ASSERT_FALSE(error_controller.HasError());

  // Remove the provider.
  provider.reset();
  ASSERT_FALSE(error_controller.HasError());
}

TEST(SigninErrorControllerTest, ErrorAuthStatusProvider) {
  SigninErrorController error_controller(
      SigninErrorController::AccountMode::ANY_ACCOUNT);
  std::unique_ptr<FakeAuthStatusProvider> provider;
  std::unique_ptr<FakeAuthStatusProvider> error_provider;

  provider.reset(new FakeAuthStatusProvider(&error_controller));
  ASSERT_FALSE(error_controller.HasError());

  error_provider.reset(new FakeAuthStatusProvider(&error_controller));
  error_provider->SetAuthError(
      kTestAccountId,
      GoogleServiceAuthError(
          GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS));
  ASSERT_TRUE(error_controller.HasError());

  error_provider.reset();
  ASSERT_FALSE(error_controller.HasError());

  provider.reset();
  // All providers should be removed now.
  ASSERT_FALSE(error_controller.HasError());
}

TEST(SigninErrorControllerTest, AuthStatusProviderErrorTransition) {
  SigninErrorController error_controller(
      SigninErrorController::AccountMode::ANY_ACCOUNT);
  std::unique_ptr<FakeAuthStatusProvider> provider0(
      new FakeAuthStatusProvider(&error_controller));
  std::unique_ptr<FakeAuthStatusProvider> provider1(
      new FakeAuthStatusProvider(&error_controller));

  ASSERT_FALSE(error_controller.HasError());
  provider0->SetAuthError(
      kTestAccountId,
      GoogleServiceAuthError(
          GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS));
  ASSERT_TRUE(error_controller.HasError());
  provider1->SetAuthError(
      kTestAccountId,
      GoogleServiceAuthError(GoogleServiceAuthError::ACCOUNT_DISABLED));
  ASSERT_TRUE(error_controller.HasError());

  // Now resolve the auth errors - the menu item should go away.
  provider0->SetAuthError(kTestAccountId,
                         GoogleServiceAuthError::AuthErrorNone());
  ASSERT_TRUE(error_controller.HasError());
  provider1->SetAuthError(kTestAccountId,
                          GoogleServiceAuthError::AuthErrorNone());
  ASSERT_FALSE(error_controller.HasError());

  provider0.reset();
  provider1.reset();
  ASSERT_FALSE(error_controller.HasError());
}

TEST(SigninErrorControllerTest, AuthStatusProviderAccountTransitionAnyAccount) {
  SigninErrorController error_controller(
      SigninErrorController::AccountMode::ANY_ACCOUNT);
  std::unique_ptr<FakeAuthStatusProvider> provider0(
      new FakeAuthStatusProvider(&error_controller));
  std::unique_ptr<FakeAuthStatusProvider> provider1(
      new FakeAuthStatusProvider(&error_controller));

  ASSERT_FALSE(error_controller.HasError());

  provider0->SetAuthError(
      kTestAccountId,
      GoogleServiceAuthError(
          GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS));
  provider1->SetAuthError(
      kOtherTestAccountId,
      GoogleServiceAuthError(GoogleServiceAuthError::NONE));
  ASSERT_TRUE(error_controller.HasError());
  ASSERT_STREQ(kTestAccountId, error_controller.error_account_id().c_str());

  // Swap providers reporting errors.
  provider1->set_error_without_status_change(
      GoogleServiceAuthError(
          GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS));
  provider0->set_error_without_status_change(
      GoogleServiceAuthError(GoogleServiceAuthError::NONE));
  error_controller.AuthStatusChanged();
  ASSERT_TRUE(error_controller.HasError());
  ASSERT_STREQ(kOtherTestAccountId,
               error_controller.error_account_id().c_str());

  // Now resolve the auth errors - the menu item should go away.
  provider0->set_error_without_status_change(
      GoogleServiceAuthError::AuthErrorNone());
  provider1->set_error_without_status_change(
      GoogleServiceAuthError::AuthErrorNone());
  error_controller.AuthStatusChanged();
  ASSERT_FALSE(error_controller.HasError());

  provider0.reset();
  provider1.reset();
  ASSERT_FALSE(error_controller.HasError());
}

TEST(SigninErrorControllerTest,
     AuthStatusProviderAccountTransitionPrimaryAccount) {
  SigninErrorController error_controller(
      SigninErrorController::AccountMode::PRIMARY_ACCOUNT);
  std::unique_ptr<FakeAuthStatusProvider> provider0(
      new FakeAuthStatusProvider(&error_controller));
  std::unique_ptr<FakeAuthStatusProvider> provider1(
      new FakeAuthStatusProvider(&error_controller));

  ASSERT_FALSE(error_controller.HasError());

  provider0->SetAuthError(
      kTestAccountId,
      GoogleServiceAuthError(GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS));
  provider1->SetAuthError(kOtherTestAccountId,
                          GoogleServiceAuthError(GoogleServiceAuthError::NONE));
  ASSERT_FALSE(error_controller.HasError());  // No primary account.
  error_controller.SetPrimaryAccountID(kOtherTestAccountId);
  ASSERT_FALSE(error_controller.HasError());  // Error on secondary.
  error_controller.SetPrimaryAccountID(kTestAccountId);
  ASSERT_TRUE(error_controller.HasError());
  ASSERT_STREQ(kTestAccountId, error_controller.error_account_id().c_str());

  // Change the primary account.
  provider1->SetAuthError(
      kOtherTestAccountId,
      GoogleServiceAuthError(GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS));
  ASSERT_TRUE(error_controller.HasError());
  ASSERT_STREQ(kTestAccountId, error_controller.error_account_id().c_str());
  error_controller.SetPrimaryAccountID(kOtherTestAccountId);
  ASSERT_TRUE(error_controller.HasError());
  ASSERT_STREQ(kOtherTestAccountId,
               error_controller.error_account_id().c_str());

  // Signout.
  error_controller.SetPrimaryAccountID("");
  ASSERT_FALSE(error_controller.HasError());

  provider0.reset();
  provider1.reset();
}

// Verify that SigninErrorController handles errors properly.
TEST(SigninErrorControllerTest, AuthStatusEnumerateAllErrors) {
  SigninErrorController error_controller(
      SigninErrorController::AccountMode::ANY_ACCOUNT);
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
      "table array does not match the number of auth error types");

  for (size_t i = 0; i < arraysize(table); ++i) {
    if (GoogleServiceAuthError::IsDeprecated(table[i].error_state))
      continue;
    FakeAuthStatusProvider provider(&error_controller);
    provider.SetAuthError(kTestAccountId,
                          GoogleServiceAuthError(table[i].error_state));

    EXPECT_EQ(error_controller.HasError(), table[i].is_error);

    if (table[i].is_error) {
      EXPECT_EQ(table[i].error_state, error_controller.auth_error().state());
      EXPECT_STREQ(kTestAccountId, error_controller.error_account_id().c_str());
    } else {
      EXPECT_EQ(GoogleServiceAuthError::NONE,
                error_controller.auth_error().state());
      EXPECT_STREQ("", error_controller.error_account_id().c_str());
    }
  }
}

// Verify that existing error is not replaced by new error.
TEST(SigninErrorControllerTest, AuthStatusChange) {
  SigninErrorController error_controller(
      SigninErrorController::AccountMode::ANY_ACCOUNT);
  std::unique_ptr<FakeAuthStatusProvider> fake_provider0(
      new FakeAuthStatusProvider(&error_controller));
  std::unique_ptr<FakeAuthStatusProvider> fake_provider1(
      new FakeAuthStatusProvider(&error_controller));

  // If there are multiple providers in the provider set...
  //
  // | provider0 |       provider1          | ...
  // |   NONE    | INVALID_GAIA_CREDENTIALS | ...
  //
  // SigninErrorController picks the first error found when iterating through
  // the set. But if another error crops up...
  //
  // |     provider0       |       provider1          | ...
  // |   SERVICE_ERROR     | INVALID_GAIA_CREDENTIALS | ...
  //
  // we want the controller to still use the original error.

  // The provider pointers are stored in a set, which is sorted by std::less.
  std::less<SigninErrorController::AuthStatusProvider*> compare;
  FakeAuthStatusProvider* provider0 =
      compare(fake_provider0.get(), fake_provider1.get()) ?
          fake_provider0.get() : fake_provider1.get();
  FakeAuthStatusProvider* provider1 =
      provider0 == fake_provider0.get() ?
          fake_provider1.get() : fake_provider0.get();

  provider0->SetAuthError(
      kTestAccountId,
      GoogleServiceAuthError(
          GoogleServiceAuthError::NONE));
  provider1->SetAuthError(
      kOtherTestAccountId,
      GoogleServiceAuthError(
          GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS));
  ASSERT_EQ(GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS,
            error_controller.auth_error().state());
  ASSERT_STREQ(kOtherTestAccountId,
               error_controller.error_account_id().c_str());

  // Change the 1st provider's error.
  provider1->SetAuthError(
      kOtherTestAccountId,
      GoogleServiceAuthError(GoogleServiceAuthError::SERVICE_ERROR));
  ASSERT_EQ(GoogleServiceAuthError::SERVICE_ERROR,
            error_controller.auth_error().state());
  ASSERT_STREQ(kOtherTestAccountId,
               error_controller.error_account_id().c_str());

  // Set the 0th provider's error -- nothing should change.
  provider0->SetAuthError(
      kTestAccountId,
      GoogleServiceAuthError(
          GoogleServiceAuthError::UNEXPECTED_SERVICE_RESPONSE));
  ASSERT_EQ(GoogleServiceAuthError::SERVICE_ERROR,
            error_controller.auth_error().state());
  ASSERT_STREQ(kOtherTestAccountId,
               error_controller.error_account_id().c_str());

  // Clear the 1st provider's error, so the 0th provider's error is used.
  provider1->SetAuthError(
      kOtherTestAccountId,
      GoogleServiceAuthError(
          GoogleServiceAuthError::NONE));
  ASSERT_EQ(GoogleServiceAuthError::UNEXPECTED_SERVICE_RESPONSE,
            error_controller.auth_error().state());
  ASSERT_STREQ(kTestAccountId, error_controller.error_account_id().c_str());

  fake_provider0.reset();
  fake_provider1.reset();
  ASSERT_FALSE(error_controller.HasError());
}
