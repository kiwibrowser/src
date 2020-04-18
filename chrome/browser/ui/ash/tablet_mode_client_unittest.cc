// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/tablet_mode_client.h"

#include "ash/public/interfaces/tablet_mode.mojom.h"
#include "base/macros.h"
#include "base/test/scoped_task_environment.h"
#include "chrome/browser/ui/ash/fake_tablet_mode_controller.h"
#include "chrome/browser/ui/ash/tablet_mode_client_observer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class TestTabletModeClientObserver : public TabletModeClientObserver {
 public:
  TestTabletModeClientObserver() = default;
  ~TestTabletModeClientObserver() override = default;

  void OnTabletModeToggled(bool enabled) override {
    ++toggle_count_;
    last_toggle_ = enabled;
  }

  int toggle_count_ = 0;
  bool last_toggle_ = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(TestTabletModeClientObserver);
};

class TabletModeClientTest : public testing::Test {
 public:
  TabletModeClientTest() = default;
  ~TabletModeClientTest() override = default;

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  DISALLOW_COPY_AND_ASSIGN(TabletModeClientTest);
};

TEST_F(TabletModeClientTest, Construction) {
  TabletModeClient client;
  FakeTabletModeController controller;
  client.InitForTesting(controller.CreateInterfacePtr());
  client.FlushForTesting();

  // Singleton was initialized.
  EXPECT_EQ(&client, TabletModeClient::Get());

  // Object was set as client.
  EXPECT_TRUE(controller.was_client_set());
}

TEST_F(TabletModeClientTest, Observers) {
  TabletModeClient client;
  TestTabletModeClientObserver observer;
  client.AddObserver(&observer);

  // Observer is not notified with state when added.
  EXPECT_EQ(0, observer.toggle_count_);

  // Setting state notifies observer.
  client.OnTabletModeToggled(true);
  EXPECT_EQ(1, observer.toggle_count_);
  EXPECT_TRUE(observer.last_toggle_);

  client.OnTabletModeToggled(false);
  EXPECT_EQ(2, observer.toggle_count_);
  EXPECT_FALSE(observer.last_toggle_);

  client.RemoveObserver(&observer);
}

}  // namespace
