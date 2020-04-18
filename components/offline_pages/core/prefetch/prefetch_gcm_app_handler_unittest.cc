// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/prefetch_gcm_app_handler.h"

#include <utility>

#include "base/bind.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/gcm_driver/instance_id/instance_id.h"
#include "components/offline_pages/core/prefetch/prefetch_service_test_taco.h"
#include "components/offline_pages/core/prefetch/test_prefetch_dispatcher.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {

class TestTokenFactory : public PrefetchGCMAppHandler::TokenFactory {
 public:
  TestTokenFactory() = default;
  ~TestTokenFactory() override = default;

  void GetGCMToken(
      instance_id::InstanceID::GetTokenCallback callback) override {
    callback.Run(token, result);
  }

  instance_id::InstanceID::Result result = instance_id::InstanceID::SUCCESS;
  std::string token = "default_token";

 private:
  DISALLOW_COPY_AND_ASSIGN(TestTokenFactory);
};

class PrefetchGCMAppHandlerTest : public testing::Test {
 public:
  PrefetchGCMAppHandlerTest()
      : task_runner_(new base::TestSimpleTaskRunner),
        task_runner_handle_(task_runner_) {}

  void SetUp() override {
    auto dispatcher = std::make_unique<TestPrefetchDispatcher>();
    test_dispatcher_ = dispatcher.get();

    auto token_factory = std::make_unique<TestTokenFactory>();
    token_factory_ = token_factory.get();

    auto gcm_app_handler =
        std::make_unique<PrefetchGCMAppHandler>(std::move(token_factory));
    handler_ = gcm_app_handler.get();

    prefetch_service_taco_.reset(new PrefetchServiceTestTaco);
    prefetch_service_taco_->SetPrefetchGCMHandler(std::move(gcm_app_handler));
    prefetch_service_taco_->SetPrefetchDispatcher(std::move(dispatcher));
    prefetch_service_taco_->CreatePrefetchService();
  }

  void TearDown() override {
    // Ensures that the store is properly disposed off.
    prefetch_service_taco_.reset();
    task_runner_->RunUntilIdle();
  }

  TestPrefetchDispatcher* dispatcher() { return test_dispatcher_; }
  PrefetchGCMAppHandler* handler() { return handler_; }
  TestTokenFactory* token_factory() { return token_factory_; }

 private:
  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle task_runner_handle_;
  std::unique_ptr<PrefetchServiceTestTaco> prefetch_service_taco_;

  // Owned by the taco.
  TestPrefetchDispatcher* test_dispatcher_;
  // Owned by the taco.
  PrefetchGCMAppHandler* handler_;
  // Owned by the PrefetchGCMAppHandler.
  TestTokenFactory* token_factory_;

  DISALLOW_COPY_AND_ASSIGN(PrefetchGCMAppHandlerTest);
};

TEST_F(PrefetchGCMAppHandlerTest, TestOnMessage) {
  gcm::IncomingMessage message;
  const char kMessage[] = "123";
  message.data["pageBundle"] = kMessage;
  handler()->OnMessage("An App ID", message);

  EXPECT_EQ(1U, dispatcher()->operation_list.size());
  EXPECT_EQ(kMessage, dispatcher()->operation_list[0]);
}

TEST_F(PrefetchGCMAppHandlerTest, TestInvalidMessage) {
  gcm::IncomingMessage message;
  const char kMessage[] = "123";
  message.data["whatAMess"] = kMessage;
  handler()->OnMessage("An App ID", message);

  EXPECT_EQ(0U, dispatcher()->operation_list.size());
}

TEST_F(PrefetchGCMAppHandlerTest, TestGetToken) {
  std::string result_token;

  handler()->GetGCMToken(base::Bind(
      [](std::string* result_token, const std::string& token,
         instance_id::InstanceID::Result result) { *result_token = token; },
      &result_token));
  EXPECT_EQ(token_factory()->token, result_token);
}

}  // namespace offline_pages
