// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/user_manager/user.h"

#include "components/account_id/account_id.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace user_manager {

namespace {

const char kEmail[] = "email@example.com";
const char kGaiaId[] = "fake_gaia_id";

}  // namespace

TEST(UserTest, DeviceLocalAccountAffiliation) {
  // This implementation of RAII for User* is to prevent memory leak.
  // Smart pointers are not friends of User and can't call protected destructor.
  class ScopedUser {
   public:
    ScopedUser(const User* const user) : user_(user) {}
    ~ScopedUser() { delete user_; }

    bool IsAffiliated() const { return user_ && user_->IsAffiliated(); }

   private:
    const User* const user_;

    DISALLOW_COPY_AND_ASSIGN(ScopedUser);
  };

  const AccountId account_id = AccountId::FromUserEmailGaiaId(kEmail, kGaiaId);

  ScopedUser kiosk_user(User::CreateKioskAppUser(account_id));
  EXPECT_TRUE(kiosk_user.IsAffiliated());

  ScopedUser public_session_user(User::CreatePublicAccountUser(account_id));
  EXPECT_TRUE(public_session_user.IsAffiliated());

  ScopedUser arc_kiosk_user(User::CreateArcKioskAppUser(account_id));
  EXPECT_TRUE(arc_kiosk_user.IsAffiliated());
}

TEST(UserTest, UserSessionInitialized) {
  const AccountId account_id = AccountId::FromUserEmailGaiaId(kEmail, kGaiaId);
  std::unique_ptr<User> user(
      User::CreateRegularUser(account_id, user_manager::USER_TYPE_REGULAR));
  EXPECT_FALSE(user->profile_ever_initialized());
  user->set_profile_ever_initialized(true);
  EXPECT_TRUE(user->profile_ever_initialized());
}

}  // namespace user_manager
