// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/system_logs/single_debug_daemon_log_source.h"

#include <memory>
#include <string>

#include "base/bind.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/fake_debug_daemon_client.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace system_logs {

using SupportedSource = SingleDebugDaemonLogSource::SupportedSource;

class SingleDebugDaemonLogSourceTest : public ::testing::Test {
 public:
  SingleDebugDaemonLogSourceTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::UI),
        num_callback_calls_(0) {}

  void SetUp() override {
    // Since no debug daemon will be available during a unit test, use
    // FakeDebugDaemonClient to provide dummy DebugDaemonClient functionality.
    chromeos::DBusThreadManager::GetSetterForTesting()->SetDebugDaemonClient(
        std::make_unique<chromeos::FakeDebugDaemonClient>());
  }

  void TearDown() override {
    chromeos::DBusThreadManager::GetSetterForTesting()->SetDebugDaemonClient(
        nullptr);
  }

 protected:
  SysLogsSourceCallback fetch_callback() {
    return base::BindOnce(&SingleDebugDaemonLogSourceTest::OnFetchComplete,
                          base::Unretained(this));
  }

  int num_callback_calls() const { return num_callback_calls_; }

  const SystemLogsResponse& response() const { return response_; }

  void ClearResponse() { response_.clear(); }

 private:
  void OnFetchComplete(std::unique_ptr<SystemLogsResponse> response) {
    ++num_callback_calls_;
    response_ = *response;
  }

  // For running scheduled tasks.
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  // Creates the necessary browser threads. Defined after
  // |scoped_task_environment_| in order to use the MessageLoop it created.
  content::TestBrowserThreadBundle browser_thread_bundle_;

  // Used to verify that OnFetchComplete was called the correct number of times.
  int num_callback_calls_;

  // Stores results from the log source.
  SystemLogsResponse response_;

  DISALLOW_COPY_AND_ASSIGN(SingleDebugDaemonLogSourceTest);
};

TEST_F(SingleDebugDaemonLogSourceTest, SingleCall) {
  SingleDebugDaemonLogSource source(SupportedSource::kModetest);

  source.Fetch(fetch_callback());
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(1, num_callback_calls());
  ASSERT_EQ(1U, response().size());

  EXPECT_EQ("modetest", response().begin()->first);
  EXPECT_EQ("modetest: response from GetLog", response().begin()->second);
}

TEST_F(SingleDebugDaemonLogSourceTest, MultipleCalls) {
  SingleDebugDaemonLogSource source(SupportedSource::kLsusb);

  source.Fetch(fetch_callback());
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(1, num_callback_calls());
  ASSERT_EQ(1U, response().size());

  EXPECT_EQ("lsusb", response().begin()->first);
  EXPECT_EQ("lsusb: response from GetLog", response().begin()->second);

  ClearResponse();

  source.Fetch(fetch_callback());
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(2, num_callback_calls());
  ASSERT_EQ(1U, response().size());

  EXPECT_EQ("lsusb", response().begin()->first);
  EXPECT_EQ("lsusb: response from GetLog", response().begin()->second);

  ClearResponse();

  source.Fetch(fetch_callback());
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(3, num_callback_calls());
  ASSERT_EQ(1U, response().size());

  EXPECT_EQ("lsusb", response().begin()->first);
  EXPECT_EQ("lsusb: response from GetLog", response().begin()->second);
}

TEST_F(SingleDebugDaemonLogSourceTest, MultipleSources) {
  SingleDebugDaemonLogSource source1(SupportedSource::kModetest);
  source1.Fetch(fetch_callback());
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(1, num_callback_calls());
  ASSERT_EQ(1U, response().size());

  EXPECT_EQ("modetest", response().begin()->first);
  EXPECT_EQ("modetest: response from GetLog", response().begin()->second);

  ClearResponse();

  SingleDebugDaemonLogSource source2(SupportedSource::kLsusb);
  source2.Fetch(fetch_callback());
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(2, num_callback_calls());
  ASSERT_EQ(1U, response().size());

  EXPECT_EQ("lsusb", response().begin()->first);
  EXPECT_EQ("lsusb: response from GetLog", response().begin()->second);
}

}  // namespace system_logs
