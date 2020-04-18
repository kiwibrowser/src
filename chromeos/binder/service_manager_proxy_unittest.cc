// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/binder/service_manager_proxy.h"

#include <memory>

#include "base/guid.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "chromeos/binder/command_broker.h"
#include "chromeos/binder/driver.h"
#include "chromeos/binder/local_object.h"
#include "chromeos/binder/transaction_data.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace binder {

namespace {

class DummyTransactionHandler : public LocalObject::TransactionHandler {
 public:
  DummyTransactionHandler() {}
  ~DummyTransactionHandler() override {}

  std::unique_ptr<TransactionData> OnTransact(
      CommandBroker* command_broker,
      const TransactionData& data) override {
    return std::unique_ptr<TransactionData>();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(DummyTransactionHandler);
};

}  // namespace

class BinderServiceManagerProxyTest : public ::testing::Test {
 public:
  BinderServiceManagerProxyTest() : command_broker_(&driver_) {}
  ~BinderServiceManagerProxyTest() override {}

  void SetUp() override { ASSERT_TRUE(driver_.Initialize()); }

 protected:
  base::MessageLoop message_loop_;
  Driver driver_;
  CommandBroker command_broker_;
};

TEST_F(BinderServiceManagerProxyTest, AddAndCheck) {
  const base::string16 kServiceName =
      base::ASCIIToUTF16("org.chromium.TestService-" + base::GenerateGUID());

  // Service not available yet.
  EXPECT_FALSE(
      ServiceManagerProxy::CheckService(&command_broker_, kServiceName));

  // Add service.
  scoped_refptr<Object> object(
      new LocalObject(std::make_unique<DummyTransactionHandler>()));
  EXPECT_TRUE(ServiceManagerProxy::AddService(&command_broker_, kServiceName,
                                              object, 0));

  // Service is available.
  scoped_refptr<Object> result =
      ServiceManagerProxy::CheckService(&command_broker_, kServiceName);
  ASSERT_TRUE(result);
  ASSERT_EQ(LocalObject::TYPE_LOCAL, result->GetType());
  EXPECT_EQ(object.get(), result.get());
}

}  // namespace binder
