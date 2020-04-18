// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/message_loop/message_loop.h"
#include "chromeos/binder/command_broker.h"
#include "chromeos/binder/constants.h"
#include "chromeos/binder/driver.h"
#include "chromeos/binder/transaction_data.h"
#include "chromeos/binder/writable_transaction_data.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace binder {

class BinderCommandBrokerTest : public ::testing::Test {
 public:
  BinderCommandBrokerTest() : command_broker_(&driver_) {}
  ~BinderCommandBrokerTest() override {}

  void SetUp() override { ASSERT_TRUE(driver_.Initialize()); }

 protected:
  base::MessageLoop message_loop_;
  Driver driver_;
  CommandBroker command_broker_;
};

TEST_F(BinderCommandBrokerTest, Transact) {
  WritableTransactionData data;
  data.SetCode(kPingTransactionCode);
  std::unique_ptr<TransactionData> reply;
  EXPECT_TRUE(command_broker_.Transact(kContextManagerHandle, data, &reply));
  ASSERT_TRUE(reply);
  EXPECT_FALSE(reply->HasStatus());  // No error.
}

}  // namespace binder
