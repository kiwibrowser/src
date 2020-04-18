// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/password_manager/password_store_signin_notifier_impl.h"

#include "chrome/browser/signin/fake_signin_manager_builder.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "components/password_manager/core/browser/mock_password_store.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;

namespace password_manager {
namespace {

class PasswordStoreSigninNotifierImplTest : public testing::Test {
 public:
  PasswordStoreSigninNotifierImplTest() {
    TestingProfile::Builder builder;
    builder.AddTestingFactory(SigninManagerFactory::GetInstance(),
                              BuildFakeSigninManagerBase);
    testing_profile_.reset(builder.Build().release());
    fake_signin_manager_ = static_cast<FakeSigninManagerForTesting*>(
        SigninManagerFactory::GetForProfile(testing_profile_.get()));
    store_ = new MockPasswordStore();
  }

  ~PasswordStoreSigninNotifierImplTest() override {
    store_->ShutdownOnUIThread();
  }

 protected:
  content::TestBrowserThreadBundle thread_bundle;
  std::unique_ptr<TestingProfile> testing_profile_;
  FakeSigninManagerForTesting* fake_signin_manager_;  // Weak
  scoped_refptr<MockPasswordStore> store_;
};

// Checks that if a notifier is subscribed on sign-in events, then
// a password store receives sign-in notifications.
TEST_F(PasswordStoreSigninNotifierImplTest, Subscribed) {
  PasswordStoreSigninNotifierImpl notifier(testing_profile_.get());
  notifier.SubscribeToSigninEvents(store_.get());
  EXPECT_CALL(
      *store_,
      SaveSyncPasswordHash(
          "username", base::ASCIIToUTF16("password"),
          metrics_util::SyncPasswordHashChange::SAVED_ON_CHROME_SIGNIN));
  fake_signin_manager_->SignIn("accountid", "username", "password");
  testing::Mock::VerifyAndClearExpectations(store_.get());
  EXPECT_CALL(*store_, ClearPasswordHash(_));
  fake_signin_manager_->ForceSignOut();
  notifier.UnsubscribeFromSigninEvents();
}

// Checks that if a notifier is unsubscribed on sign-in events, then
// a password store receives no sign-in notifications.
TEST_F(PasswordStoreSigninNotifierImplTest, Unsubscribed) {
  PasswordStoreSigninNotifierImpl notifier(testing_profile_.get());
  notifier.SubscribeToSigninEvents(store_.get());

  notifier.UnsubscribeFromSigninEvents();
  EXPECT_CALL(*store_, SaveSyncPasswordHash(_, _, _)).Times(0);
  EXPECT_CALL(*store_, ClearPasswordHash(_)).Times(0);
  fake_signin_manager_->SignIn("accountid", "username", "secret");
  fake_signin_manager_->ForceSignOut();
}

}  // namespace
}  // namespace password_manager
