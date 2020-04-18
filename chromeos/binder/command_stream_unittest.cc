// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/message_loop/message_loop.h"
#include "chromeos/binder/binder_driver_api.h"
#include "chromeos/binder/command_stream.h"
#include "chromeos/binder/constants.h"
#include "chromeos/binder/driver.h"
#include "chromeos/binder/transaction_data.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace binder {

namespace {

class BinderCommandStreamTest : public ::testing::Test,
                                public CommandStream::IncomingCommandHandler {
 public:
  BinderCommandStreamTest() : command_stream_(&driver_, this) {}
  ~BinderCommandStreamTest() override {}

  void SetUp() override { ASSERT_TRUE(driver_.Initialize()); }

  // CommandStream::IncomingCommandHandler override:
  bool OnTransaction(const TransactionData& data) override { return false; }

  void OnReply(std::unique_ptr<TransactionData> data) override {
    received_response_ = RESPONSE_REPLY;
    ASSERT_TRUE(data);
    EXPECT_FALSE(data->HasStatus());  // No error.
  }

  void OnDeadReply() override { received_response_ = RESPONSE_DEAD; }

  void OnTransactionComplete() override {
    received_response_ = RESPONSE_COMPLETE;
  }

  void OnIncrementWeakReference(void* ptr, void* cookie) override {}

  void OnIncrementStrongReference(void* ptr, void* cookie) override {}

  void OnDecrementStrongReference(void* ptr, void* cookie) override {}

  void OnDecrementWeakReference(void* ptr, void* cookie) override {}

  void OnFailedReply() override { received_response_ = RESPONSE_FAILED; }

 protected:
  base::MessageLoop message_loop_;
  Driver driver_;
  CommandStream command_stream_;

  enum ResponseType {
    RESPONSE_NONE,
    RESPONSE_REPLY,
    RESPONSE_DEAD,
    RESPONSE_COMPLETE,
    RESPONSE_FAILED,
  };
  ResponseType received_response_ = RESPONSE_NONE;
};

}  // namespace

TEST_F(BinderCommandStreamTest, EnterLooper) {
  command_stream_.AppendOutgoingCommand(BC_ENTER_LOOPER, nullptr, 0);
  EXPECT_TRUE(command_stream_.Flush());
  command_stream_.AppendOutgoingCommand(BC_EXIT_LOOPER, nullptr, 0);
  EXPECT_TRUE(command_stream_.Flush());
}

TEST_F(BinderCommandStreamTest, Error) {
  // Kernel's binder.h says BC_ATTEMPT_ACQUIRE is not currently supported.
  binder_pri_desc params = {};
  command_stream_.AppendOutgoingCommand(BC_ATTEMPT_ACQUIRE, &params,
                                        sizeof(params));
  EXPECT_FALSE(command_stream_.Flush());
}

TEST_F(BinderCommandStreamTest, PingContextManager) {
  // Perform ping transaction with the context manager.
  binder_transaction_data data = {};
  data.target.handle = kContextManagerHandle;
  data.code = kPingTransactionCode;
  command_stream_.AppendOutgoingCommand(BC_TRANSACTION, &data, sizeof(data));
  ASSERT_TRUE(command_stream_.Flush());

  // Wait for transaction complete.
  while (received_response_ == RESPONSE_NONE) {
    if (command_stream_.CanProcessIncomingCommand()) {
      ASSERT_TRUE(command_stream_.ProcessIncomingCommand());
    } else {
      ASSERT_TRUE(command_stream_.Fetch());
    }
  }
  ASSERT_EQ(RESPONSE_COMPLETE, received_response_);

  // Wait for transaction reply.
  received_response_ = RESPONSE_NONE;
  while (received_response_ == RESPONSE_NONE) {
    if (command_stream_.CanProcessIncomingCommand()) {
      ASSERT_TRUE(command_stream_.ProcessIncomingCommand());
    } else {
      ASSERT_TRUE(command_stream_.Fetch());
    }
  }
  ASSERT_EQ(RESPONSE_REPLY, received_response_);
}

}  // namespace binder
