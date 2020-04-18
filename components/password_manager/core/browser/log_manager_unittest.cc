// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/log_manager.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "components/password_manager/core/browser/log_receiver.h"
#include "components/password_manager/core/browser/log_router.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;

namespace password_manager {

namespace {

const char kTestText[] = "abcd1234";

class MockLogReceiver : public password_manager::LogReceiver {
 public:
  MockLogReceiver() = default;
  MOCK_METHOD1(LogSavePasswordProgress, void(const std::string&));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockLogReceiver);
};

class MockNotifiedObject {
 public:
  MockNotifiedObject() = default;
  MOCK_METHOD0(NotifyAboutLoggingActivity, void());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockNotifiedObject);
};

}  // namespace

class LogManagerTest : public testing::Test {
 protected:
  void SetUp() override {
    manager_ = LogManager::Create(
        &router_, base::Bind(&MockNotifiedObject::NotifyAboutLoggingActivity,
                             base::Unretained(&notified_object_)));
  }

  void TearDown() override {
    manager_.reset();  // Destruct before LogRouter.
  }

 protected:
  testing::StrictMock<MockLogReceiver> receiver_;
  LogRouter router_;
  testing::StrictMock<MockNotifiedObject> notified_object_;
  std::unique_ptr<LogManager> manager_;
};

TEST_F(LogManagerTest, LogSavePasswordProgressNoReceiver) {
  EXPECT_CALL(receiver_, LogSavePasswordProgress(_)).Times(0);
  // Before attaching the receiver, no text should be passed.
  manager_->LogSavePasswordProgress(kTestText);
  EXPECT_FALSE(manager_->IsLoggingActive());
}

TEST_F(LogManagerTest, LogSavePasswordProgressAttachReceiver) {
  EXPECT_FALSE(manager_->IsLoggingActive());

  EXPECT_CALL(notified_object_, NotifyAboutLoggingActivity());
  EXPECT_EQ(std::string(), router_.RegisterReceiver(&receiver_));
  EXPECT_TRUE(manager_->IsLoggingActive());
  // After attaching the logger, text should be passed.
  EXPECT_CALL(receiver_, LogSavePasswordProgress(kTestText));
  manager_->LogSavePasswordProgress(kTestText);
  EXPECT_CALL(notified_object_, NotifyAboutLoggingActivity());
  router_.UnregisterReceiver(&receiver_);
  EXPECT_FALSE(manager_->IsLoggingActive());
}

TEST_F(LogManagerTest, LogSavePasswordProgressDetachReceiver) {
  EXPECT_CALL(notified_object_, NotifyAboutLoggingActivity());
  EXPECT_EQ(std::string(), router_.RegisterReceiver(&receiver_));
  EXPECT_TRUE(manager_->IsLoggingActive());
  EXPECT_CALL(notified_object_, NotifyAboutLoggingActivity());
  router_.UnregisterReceiver(&receiver_);
  EXPECT_FALSE(manager_->IsLoggingActive());

  // After detaching the logger, no text should be passed.
  EXPECT_CALL(receiver_, LogSavePasswordProgress(_)).Times(0);
  manager_->LogSavePasswordProgress(kTestText);
}

TEST_F(LogManagerTest, NullCallbackWillNotCrash) {
  manager_ = LogManager::Create(&router_, base::Closure());
  EXPECT_EQ(std::string(), router_.RegisterReceiver(&receiver_));
  router_.UnregisterReceiver(&receiver_);
}

TEST_F(LogManagerTest, SetSuspended_WithActiveLogging) {
  EXPECT_CALL(notified_object_, NotifyAboutLoggingActivity());
  EXPECT_EQ(std::string(), router_.RegisterReceiver(&receiver_));
  EXPECT_TRUE(manager_->IsLoggingActive());

  EXPECT_CALL(notified_object_, NotifyAboutLoggingActivity());
  manager_->SetSuspended(true);
  EXPECT_FALSE(manager_->IsLoggingActive());

  EXPECT_CALL(notified_object_, NotifyAboutLoggingActivity());
  manager_->SetSuspended(false);
  EXPECT_TRUE(manager_->IsLoggingActive());

  EXPECT_CALL(notified_object_, NotifyAboutLoggingActivity());
  router_.UnregisterReceiver(&receiver_);
  EXPECT_FALSE(manager_->IsLoggingActive());
}

TEST_F(LogManagerTest, SetSuspended_WithInactiveLogging) {
  EXPECT_FALSE(manager_->IsLoggingActive());

  manager_->SetSuspended(true);
  EXPECT_FALSE(manager_->IsLoggingActive());

  manager_->SetSuspended(false);
  EXPECT_FALSE(manager_->IsLoggingActive());
}

TEST_F(LogManagerTest, InterleaveSuspendAndLoggingActivation_SuspendFirst) {
  manager_->SetSuspended(true);
  EXPECT_FALSE(manager_->IsLoggingActive());

  EXPECT_EQ(std::string(), router_.RegisterReceiver(&receiver_));
  EXPECT_FALSE(manager_->IsLoggingActive());

  EXPECT_CALL(notified_object_, NotifyAboutLoggingActivity());
  manager_->SetSuspended(false);
  EXPECT_TRUE(manager_->IsLoggingActive());

  EXPECT_CALL(notified_object_, NotifyAboutLoggingActivity());
  router_.UnregisterReceiver(&receiver_);
  EXPECT_FALSE(manager_->IsLoggingActive());
}

TEST_F(LogManagerTest, InterleaveSuspendAndLoggingActivation_ActiveFirst) {
  EXPECT_FALSE(manager_->IsLoggingActive());

  EXPECT_CALL(notified_object_, NotifyAboutLoggingActivity());
  EXPECT_EQ(std::string(), router_.RegisterReceiver(&receiver_));
  EXPECT_TRUE(manager_->IsLoggingActive());

  EXPECT_CALL(notified_object_, NotifyAboutLoggingActivity());
  manager_->SetSuspended(true);
  EXPECT_FALSE(manager_->IsLoggingActive());

  router_.UnregisterReceiver(&receiver_);
  EXPECT_FALSE(manager_->IsLoggingActive());

  manager_->SetSuspended(false);
  EXPECT_FALSE(manager_->IsLoggingActive());
}

}  // namespace password_manager
