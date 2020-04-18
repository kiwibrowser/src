// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/message_loop/message_loop.h"
#include "base/test/test_timeouts.h"
#include "chromeos/binder/command_broker.h"
#include "chromeos/binder/driver.h"
#include "chromeos/binder/object.h"
#include "chromeos/binder/service_manager_proxy.h"
#include "chromeos/binder/test_service.h"
#include "chromeos/binder/transaction_data.h"
#include "chromeos/binder/transaction_data_reader.h"
#include "chromeos/binder/writable_transaction_data.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace binder {

class BinderEndToEndTest : public ::testing::Test {
 public:
  BinderEndToEndTest() : command_broker_(&driver_) {}

  void SetUp() override {
    ASSERT_TRUE(driver_.Initialize());

    // Start the test service and get a remote object from it.
    ASSERT_TRUE(test_service_.StartAndWait());
    remote_object_ = ServiceManagerProxy::CheckService(
        &command_broker_, test_service_.service_name());
    ASSERT_EQ(Object::TYPE_REMOTE, remote_object_->GetType());
    ASSERT_TRUE(remote_object_);
  }

  // Performs SIGNAL_TRANSACTION with a new CommandBroker instance.
  // Can be called from any threads.
  void PerformSignalTransaction() {
    CommandBroker command_broker(&driver_);
    WritableTransactionData data;
    data.SetCode(TestService::SIGNAL_TRANSACTION);
    std::unique_ptr<TransactionData> reply;
    ASSERT_TRUE(remote_object_->Transact(&command_broker, data, &reply));
    ASSERT_TRUE(reply);

    TransactionDataReader reader(*reply);
    uint32_t code = 0;
    EXPECT_TRUE(reader.ReadUint32(&code));
    EXPECT_EQ(TestService::SIGNAL_TRANSACTION, code);
  }

 protected:
  base::MessageLoopForIO message_loop_;
  Driver driver_;
  CommandBroker command_broker_;
  TestService test_service_;
  scoped_refptr<Object> remote_object_;
};

TEST_F(BinderEndToEndTest, IncrementInt) {
  const int32_t kInput = 42;
  WritableTransactionData data;
  data.SetCode(TestService::INCREMENT_INT_TRANSACTION);
  data.WriteInt32(kInput);
  std::unique_ptr<TransactionData> reply;
  ASSERT_TRUE(remote_object_->Transact(&command_broker_, data, &reply));
  ASSERT_TRUE(reply);

  TransactionDataReader reader(*reply);
  int32_t result = 0;
  EXPECT_TRUE(reader.ReadInt32(&result));
  EXPECT_EQ(kInput + 1, result);
}

TEST_F(BinderEndToEndTest, GetFD) {
  WritableTransactionData data;
  data.SetCode(TestService::GET_FD_TRANSACTION);
  std::unique_ptr<TransactionData> reply;
  ASSERT_TRUE(remote_object_->Transact(&command_broker_, data, &reply));
  ASSERT_TRUE(reply);

  TransactionDataReader reader(*reply);
  int fd = -1;
  EXPECT_TRUE(reader.ReadFileDescriptor(&fd));

  const std::string kExpected = TestService::GetFileContents();
  std::vector<char> buf(kExpected.size());
  EXPECT_TRUE(base::ReadFromFD(fd, buf.data(), buf.size()));
  EXPECT_EQ(kExpected, std::string(buf.data(), buf.size()));
}

// Tests if the multithreading is correctly supported by ensuring that the
// test service can handle two transactions in parallel (i.e. handling
// SIGNAL_TRANSACTION while one thread is blocked by WAIT_TRANSACTION).
TEST_F(BinderEndToEndTest, MultiThread) {
  // Signal the object on a separate thread.
  base::Thread signal_thread("SignalThread");
  ASSERT_TRUE(signal_thread.Start());
  ASSERT_TRUE(signal_thread.WaitUntilThreadStarted());
  // The use of delayed task here can result in a race if it takes long to
  // perform the wait transaction, but in practice it doesn't matter as it takes
  // only about 1 ms to perform a transaction.
  signal_thread.task_runner()->PostDelayedTask(
      FROM_HERE, base::Bind(&BinderEndToEndTest::PerformSignalTransaction,
                            base::Unretained(this)),
      TestTimeouts::tiny_timeout());

  // Wait for the signal.
  WritableTransactionData data;
  data.SetCode(TestService::WAIT_TRANSACTION);
  std::unique_ptr<TransactionData> reply;
  ASSERT_TRUE(remote_object_->Transact(&command_broker_, data, &reply));
  ASSERT_TRUE(reply);

  TransactionDataReader reader(*reply);
  uint32_t code = 0;
  EXPECT_TRUE(reader.ReadUint32(&code));
  EXPECT_EQ(TestService::WAIT_TRANSACTION, code);
}

}  // namespace binder
