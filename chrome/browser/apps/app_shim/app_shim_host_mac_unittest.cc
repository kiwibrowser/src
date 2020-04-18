// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/apps/app_shim/app_shim_host_mac.h"

#include <memory>
#include <tuple>
#include <vector>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "chrome/common/mac/app_shim_messages.h"
#include "ipc/ipc_message.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class TestingAppShimHost : public AppShimHost {
 public:
  TestingAppShimHost() {}
  ~TestingAppShimHost() override {}

  bool ReceiveMessage(IPC::Message* message);

  const std::vector<std::unique_ptr<IPC::Message>>& sent_messages() {
    return sent_messages_;
  }

 protected:
  bool Send(IPC::Message* message) override;

 private:
  std::vector<std::unique_ptr<IPC::Message>> sent_messages_;

  DISALLOW_COPY_AND_ASSIGN(TestingAppShimHost);
};

bool TestingAppShimHost::ReceiveMessage(IPC::Message* message) {
  bool handled = OnMessageReceived(*message);
  delete message;
  return handled;
}

bool TestingAppShimHost::Send(IPC::Message* message) {
  sent_messages_.push_back(base::WrapUnique(message));
  return true;
}

const char kTestAppId[] = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
const char kTestProfileDir[] = "Profile 1";

class AppShimHostTest : public testing::Test,
                        public apps::AppShimHandler {
 public:
  AppShimHostTest() : launch_result_(apps::APP_SHIM_LAUNCH_SUCCESS),
                      launch_count_(0),
                      launch_now_count_(0),
                      close_count_(0),
                      focus_count_(0),
                      quit_count_(0) {}

  TestingAppShimHost* host() { return host_.get(); }

  void LaunchApp(apps::AppShimLaunchType launch_type) {
    EXPECT_TRUE(host()->ReceiveMessage(
        new AppShimHostMsg_LaunchApp(base::FilePath(kTestProfileDir),
                                     kTestAppId,
                                     launch_type,
                                     std::vector<base::FilePath>())));
  }

  apps::AppShimLaunchResult GetLaunchResult() {
    EXPECT_EQ(1u, host()->sent_messages().size());
    IPC::Message* message = host()->sent_messages()[0].get();
    EXPECT_EQ(AppShimMsg_LaunchApp_Done::ID, message->type());
    AppShimMsg_LaunchApp_Done::Param param;
    AppShimMsg_LaunchApp_Done::Read(message, &param);
    return std::get<0>(param);
  }

  void SimulateDisconnect() {
    static_cast<IPC::Listener*>(host_.release())->OnChannelError();
  }

 protected:
  void OnShimLaunch(Host* host,
                    apps::AppShimLaunchType launch_type,
                    const std::vector<base::FilePath>& file) override {
    ++launch_count_;
    if (launch_type == apps::APP_SHIM_LAUNCH_NORMAL)
      ++launch_now_count_;
    host->OnAppLaunchComplete(launch_result_);
  }

  void OnShimClose(Host* host) override { ++close_count_; }

  void OnShimFocus(Host* host,
                   apps::AppShimFocusType focus_type,
                   const std::vector<base::FilePath>& file) override {
    ++focus_count_;
  }

  void OnShimSetHidden(Host* host, bool hidden) override {}

  void OnShimQuit(Host* host) override { ++quit_count_; }

  apps::AppShimLaunchResult launch_result_;
  int launch_count_;
  int launch_now_count_;
  int close_count_;
  int focus_count_;
  int quit_count_;

 private:
  void SetUp() override {
    testing::Test::SetUp();
    host_.reset(new TestingAppShimHost());
  }

  std::unique_ptr<TestingAppShimHost> host_;

  DISALLOW_COPY_AND_ASSIGN(AppShimHostTest);
};


}  // namespace

TEST_F(AppShimHostTest, TestLaunchAppWithHandler) {
  apps::AppShimHandler::RegisterHandler(kTestAppId, this);
  LaunchApp(apps::APP_SHIM_LAUNCH_NORMAL);
  EXPECT_EQ(kTestAppId,
            static_cast<apps::AppShimHandler::Host*>(host())->GetAppId());
  EXPECT_EQ(apps::APP_SHIM_LAUNCH_SUCCESS, GetLaunchResult());
  EXPECT_EQ(1, launch_count_);
  EXPECT_EQ(1, launch_now_count_);
  EXPECT_EQ(0, focus_count_);
  EXPECT_EQ(0, close_count_);

  // A second OnAppLaunchComplete is ignored.
  static_cast<apps::AppShimHandler::Host*>(host())
      ->OnAppLaunchComplete(apps::APP_SHIM_LAUNCH_APP_NOT_FOUND);
  EXPECT_EQ(apps::APP_SHIM_LAUNCH_SUCCESS, GetLaunchResult());

  EXPECT_TRUE(host()->ReceiveMessage(
      new AppShimHostMsg_FocusApp(apps::APP_SHIM_FOCUS_NORMAL,
                                  std::vector<base::FilePath>())));
  EXPECT_EQ(1, focus_count_);

  EXPECT_TRUE(host()->ReceiveMessage(new AppShimHostMsg_QuitApp()));
  EXPECT_EQ(1, quit_count_);

  SimulateDisconnect();
  EXPECT_EQ(1, close_count_);
  apps::AppShimHandler::RemoveHandler(kTestAppId);
}

TEST_F(AppShimHostTest, TestNoLaunchNow) {
  apps::AppShimHandler::RegisterHandler(kTestAppId, this);
  LaunchApp(apps::APP_SHIM_LAUNCH_REGISTER_ONLY);
  EXPECT_EQ(kTestAppId,
            static_cast<apps::AppShimHandler::Host*>(host())->GetAppId());
  EXPECT_EQ(apps::APP_SHIM_LAUNCH_SUCCESS, GetLaunchResult());
  EXPECT_EQ(1, launch_count_);
  EXPECT_EQ(0, launch_now_count_);
  EXPECT_EQ(0, focus_count_);
  EXPECT_EQ(0, close_count_);
  apps::AppShimHandler::RemoveHandler(kTestAppId);
}

TEST_F(AppShimHostTest, TestFailLaunch) {
  apps::AppShimHandler::RegisterHandler(kTestAppId, this);
  launch_result_ = apps::APP_SHIM_LAUNCH_APP_NOT_FOUND;
  LaunchApp(apps::APP_SHIM_LAUNCH_NORMAL);
  EXPECT_EQ(apps::APP_SHIM_LAUNCH_APP_NOT_FOUND, GetLaunchResult());
  apps::AppShimHandler::RemoveHandler(kTestAppId);
}
