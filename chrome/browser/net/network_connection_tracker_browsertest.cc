// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/callback_forward.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process_impl.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/common/network_connection_tracker.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/common/service_names.mojom.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/base/network_change_notifier.h"
#include "services/network/public/cpp/features.h"
#include "services/network/public/mojom/network_service_test.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace content {

namespace {

class TestNetworkConnectionObserver
    : public NetworkConnectionTracker::NetworkConnectionObserver {
 public:
  explicit TestNetworkConnectionObserver(NetworkConnectionTracker* tracker)
      : num_notifications_(0),
        tracker_(tracker),
        connection_type_(network::mojom::ConnectionType::CONNECTION_UNKNOWN) {
    tracker_->AddNetworkConnectionObserver(this);
  }

  ~TestNetworkConnectionObserver() override {
    tracker_->RemoveNetworkConnectionObserver(this);
  }

  // NetworkConnectionObserver implementation:
  void OnConnectionChanged(network::mojom::ConnectionType type) override {
    network::mojom::ConnectionType queried_type;
    bool sync = tracker_->GetConnectionType(
        &queried_type,
        base::BindOnce([](network::mojom::ConnectionType type) {}));
    EXPECT_TRUE(sync);
    EXPECT_EQ(type, queried_type);

    num_notifications_++;
    connection_type_ = type;
    run_loop_.Quit();
  }

  void WaitForNotification() { run_loop_.Run(); }

  size_t num_notifications() const { return num_notifications_; }
  network::mojom::ConnectionType connection_type() const {
    return connection_type_;
  }

 private:
  size_t num_notifications_;
  NetworkConnectionTracker* tracker_;
  base::RunLoop run_loop_;
  network::mojom::ConnectionType connection_type_;

  DISALLOW_COPY_AND_ASSIGN(TestNetworkConnectionObserver);
};

}  // namespace

class NetworkConnectionTrackerBrowserTest
    : public InProcessBrowserTest,
      public testing::WithParamInterface<bool> {
 public:
  NetworkConnectionTrackerBrowserTest() : network_service_enabled_(GetParam()) {
    if (network_service_enabled_) {
      scoped_feature_list_.InitAndEnableFeature(
          network::features::kNetworkService);
    } else {
      scoped_feature_list_.InitAndDisableFeature(
          network::features::kNetworkService);
    }
  }
  ~NetworkConnectionTrackerBrowserTest() override {}

  // Simulates a network connection change.
  void SimulateNetworkChange(network::mojom::ConnectionType type) {
    if (network_service_enabled_ &&
        !content::IsNetworkServiceRunningInProcess()) {
      network::mojom::NetworkServiceTestPtr network_service_test;
      ServiceManagerConnection::GetForProcess()->GetConnector()->BindInterface(
          mojom::kNetworkServiceName, &network_service_test);
      base::RunLoop run_loop;
      network_service_test->SimulateNetworkChange(
          type, base::Bind([](base::RunLoop* run_loop) { run_loop->Quit(); },
                           base::Unretained(&run_loop)));
      run_loop.Run();
      return;
    }
    net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
        net::NetworkChangeNotifier::ConnectionType(type));
  }

  bool network_service_enabled() const { return network_service_enabled_; }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
  const bool network_service_enabled_;
};

// Basic test to make sure NetworkConnectionTracker is set up.
IN_PROC_BROWSER_TEST_P(NetworkConnectionTrackerBrowserTest,
                       NetworkConnectionTracker) {
#if defined(OS_CHROMEOS) || defined(OS_MACOSX)
  // NetworkService on ChromeOS doesn't yet have a NetworkChangeManager
  // implementation. OSX uses a separate binary for service processes and
  // browser test fixture doesn't have NetworkServiceTest mojo code.
  if (network_service_enabled())
    return;
#endif
  NetworkConnectionTracker* tracker =
      g_browser_process->network_connection_tracker();
  EXPECT_NE(nullptr, tracker);
  // Issue a GetConnectionType() request to make sure NetworkService has been
  // started up. This way, NetworkService will receive the broadcast when
  // SimulateNetworkChange() is called.
  base::RunLoop run_loop;
  network::mojom::ConnectionType ignored_type;
  bool sync = tracker->GetConnectionType(
      &ignored_type,
      base::BindOnce(
          [](base::RunLoop* run_loop, network::mojom::ConnectionType type) {
            run_loop->Quit();
          },
          base::Unretained(&run_loop)));
  if (!sync)
    run_loop.Run();
  TestNetworkConnectionObserver network_connection_observer(tracker);
  SimulateNetworkChange(network::mojom::ConnectionType::CONNECTION_3G);
  network_connection_observer.WaitForNotification();
  EXPECT_EQ(network::mojom::ConnectionType::CONNECTION_3G,
            network_connection_observer.connection_type());
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1u, network_connection_observer.num_notifications());
}

INSTANTIATE_TEST_CASE_P(/* no prefix */,
                        NetworkConnectionTrackerBrowserTest,
                        testing::Bool());

}  // namespace content
