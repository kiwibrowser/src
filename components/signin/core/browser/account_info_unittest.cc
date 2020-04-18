// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/core/browser/account_info.h"
#include "testing/gtest/include/gtest/gtest.h"

class AccountInfoTest : public testing::Test {};

TEST_F(AccountInfoTest, IsEmpty) {
  {
    AccountInfo info_empty;
    EXPECT_TRUE(info_empty.IsEmpty());
  }
  {
    AccountInfo info_with_account_id;
    info_with_account_id.account_id = "test_id";
    EXPECT_FALSE(info_with_account_id.IsEmpty());
  }
  {
    AccountInfo info_with_email;
    info_with_email.email = "test_email@email.com";
    EXPECT_FALSE(info_with_email.IsEmpty());
  }
  {
    AccountInfo info_with_gaia;
    info_with_gaia.gaia = "test_gaia";
    EXPECT_FALSE(info_with_gaia.IsEmpty());
  }
}

// Tests that IsValid() returns true only when all the fields are non-empty.
TEST_F(AccountInfoTest, IsValid) {
  AccountInfo info;
  EXPECT_FALSE(info.IsValid());

  info.account_id = info.gaia = info.email = "test_id";
  EXPECT_FALSE(info.IsValid());

  info.full_name = info.given_name = "test_name";
  info.hosted_domain = "test_domain";
  info.locale = "test_locale";
  info.picture_url = "test_picture_url";
  EXPECT_TRUE(info.IsValid());

  info.is_child_account = true;
  EXPECT_TRUE(info.IsValid());
}

// Tests that UpdateWith() correctly ignores parameters with a different account
// id.
TEST_F(AccountInfoTest, UpdateWithDifferentAccountId) {
  AccountInfo info;
  info.account_id = "test_id";

  AccountInfo other;
  other.account_id = other.gaia = other.email = "test_other_id";

  EXPECT_FALSE(info.UpdateWith(other));
  EXPECT_TRUE(info.gaia.empty());
  EXPECT_TRUE(info.email.empty());
}

// Tests that UpdateWith() doesn't update the fields that were already set.
TEST_F(AccountInfoTest, UpdateWithNoModification) {
  AccountInfo info;
  info.account_id = info.gaia = info.email = "test_id";
  info.is_child_account = true;

  AccountInfo other;
  other.account_id = "test_id";
  other.gaia = other.email = "test_other_id";
  other.is_child_account = false;

  EXPECT_FALSE(info.UpdateWith(other));
  EXPECT_EQ("test_id", info.gaia);
  EXPECT_EQ("test_id", info.email);
  EXPECT_TRUE(info.is_child_account);
}

// Tests that UpdateWith() correctly updates its fields that were not set.
TEST_F(AccountInfoTest, UpdateWithSuccessfulUpdate) {
  AccountInfo info;
  info.account_id = info.gaia = info.email = "test_id";

  AccountInfo other;
  other.account_id = "test_id";
  other.full_name = other.given_name = "test_name";
  other.is_child_account = true;

  EXPECT_TRUE(info.UpdateWith(other));
  EXPECT_EQ("test_id", info.gaia);
  EXPECT_EQ("test_id", info.email);
  EXPECT_EQ("test_name", info.full_name);
  EXPECT_EQ("test_name", info.given_name);
  EXPECT_TRUE(info.is_child_account);
}
