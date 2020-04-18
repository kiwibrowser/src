// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/background_sync/background_sync_network_observer.h"

#include "base/run_loop.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/base/network_change_notifier.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

class BackgroundSyncNetworkObserverTest : public testing::Test {
 protected:
  BackgroundSyncNetworkObserverTest()
      : network_change_notifier(net::NetworkChangeNotifier::CreateMock()),
        network_observer_(new BackgroundSyncNetworkObserver(base::BindRepeating(
            &BackgroundSyncNetworkObserverTest::OnNetworkChanged,
            base::Unretained(this)))),
        network_changed_count_(0) {}

  void SetNetwork(net::NetworkChangeNotifier::ConnectionType connection_type) {
    net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
        connection_type);
    base::RunLoop().RunUntilIdle();
  }

  void OnNetworkChanged() { network_changed_count_++; }

  TestBrowserThreadBundle browser_thread_bundle_;

  std::unique_ptr<net::NetworkChangeNotifier> network_change_notifier;
  std::unique_ptr<BackgroundSyncNetworkObserver> network_observer_;
  int network_changed_count_;
};

TEST_F(BackgroundSyncNetworkObserverTest, NetworkChangeInvokesCallback) {
  SetNetwork(net::NetworkChangeNotifier::CONNECTION_NONE);
  network_changed_count_ = 0;

  SetNetwork(net::NetworkChangeNotifier::CONNECTION_WIFI);
  EXPECT_EQ(1, network_changed_count_);
  SetNetwork(net::NetworkChangeNotifier::CONNECTION_3G);
  EXPECT_EQ(2, network_changed_count_);
  SetNetwork(net::NetworkChangeNotifier::CONNECTION_UNKNOWN);
  EXPECT_EQ(3, network_changed_count_);
  SetNetwork(net::NetworkChangeNotifier::CONNECTION_NONE);
  EXPECT_EQ(4, network_changed_count_);
  SetNetwork(net::NetworkChangeNotifier::CONNECTION_NONE);
  EXPECT_EQ(4, network_changed_count_);
}

TEST_F(BackgroundSyncNetworkObserverTest, NetworkSufficientAnyNetwork) {
  SetNetwork(net::NetworkChangeNotifier::CONNECTION_WIFI);
  EXPECT_TRUE(network_observer_->NetworkSufficient(NETWORK_STATE_ANY));

  SetNetwork(net::NetworkChangeNotifier::CONNECTION_3G);
  EXPECT_TRUE(network_observer_->NetworkSufficient(NETWORK_STATE_ANY));

  SetNetwork(net::NetworkChangeNotifier::CONNECTION_UNKNOWN);
  EXPECT_TRUE(network_observer_->NetworkSufficient(NETWORK_STATE_ANY));

  SetNetwork(net::NetworkChangeNotifier::CONNECTION_NONE);
  EXPECT_TRUE(network_observer_->NetworkSufficient(NETWORK_STATE_ANY));
}

TEST_F(BackgroundSyncNetworkObserverTest, NetworkSufficientAvoidCellular) {
  SetNetwork(net::NetworkChangeNotifier::CONNECTION_WIFI);
  EXPECT_TRUE(
      network_observer_->NetworkSufficient(NETWORK_STATE_AVOID_CELLULAR));

  SetNetwork(net::NetworkChangeNotifier::CONNECTION_UNKNOWN);
  EXPECT_TRUE(
      network_observer_->NetworkSufficient(NETWORK_STATE_AVOID_CELLULAR));

  SetNetwork(net::NetworkChangeNotifier::CONNECTION_2G);
  EXPECT_FALSE(
      network_observer_->NetworkSufficient(NETWORK_STATE_AVOID_CELLULAR));

  SetNetwork(net::NetworkChangeNotifier::CONNECTION_3G);
  EXPECT_FALSE(
      network_observer_->NetworkSufficient(NETWORK_STATE_AVOID_CELLULAR));

  SetNetwork(net::NetworkChangeNotifier::CONNECTION_4G);
  EXPECT_FALSE(
      network_observer_->NetworkSufficient(NETWORK_STATE_AVOID_CELLULAR));

  SetNetwork(net::NetworkChangeNotifier::CONNECTION_NONE);
  EXPECT_FALSE(
      network_observer_->NetworkSufficient(NETWORK_STATE_AVOID_CELLULAR));
}

TEST_F(BackgroundSyncNetworkObserverTest, ConditionsMetOnline) {
  SetNetwork(net::NetworkChangeNotifier::CONNECTION_WIFI);
  EXPECT_TRUE(network_observer_->NetworkSufficient(NETWORK_STATE_ONLINE));

  SetNetwork(net::NetworkChangeNotifier::CONNECTION_3G);
  EXPECT_TRUE(network_observer_->NetworkSufficient(NETWORK_STATE_ONLINE));

  SetNetwork(net::NetworkChangeNotifier::CONNECTION_UNKNOWN);
  EXPECT_TRUE(network_observer_->NetworkSufficient(NETWORK_STATE_ONLINE));

  SetNetwork(net::NetworkChangeNotifier::CONNECTION_NONE);
  EXPECT_FALSE(network_observer_->NetworkSufficient(NETWORK_STATE_ONLINE));
}

}  // namespace content
