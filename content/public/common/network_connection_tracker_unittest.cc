// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/network_connection_tracker.h"

#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread.h"
#include "base/threading/thread_checker.h"
#include "net/base/mock_network_change_notifier.h"
#include "services/network/network_service.h"
#include "services/network/public/mojom/network_change_manager.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

class TestNetworkConnectionObserver
    : public NetworkConnectionTracker::NetworkConnectionObserver {
 public:
  explicit TestNetworkConnectionObserver(NetworkConnectionTracker* tracker)
      : num_notifications_(0),
        tracker_(tracker),
        run_loop_(std::make_unique<base::RunLoop>()),
        connection_type_(network::mojom::ConnectionType::CONNECTION_UNKNOWN) {
    tracker_->AddNetworkConnectionObserver(this);
  }

  ~TestNetworkConnectionObserver() override {
    tracker_->RemoveNetworkConnectionObserver(this);
  }

  // Helper to synchronously get connection type from NetworkConnectionTracker.
  network::mojom::ConnectionType GetConnectionTypeSync() {
    network::mojom::ConnectionType type;
    base::RunLoop run_loop;
    bool sync = tracker_->GetConnectionType(
        &type, base::BindOnce(
                   &TestNetworkConnectionObserver::GetConnectionTypeCallback,
                   &run_loop, &type));
    if (!sync)
      run_loop.Run();
    return type;
  }

  // NetworkConnectionObserver implementation:
  void OnConnectionChanged(network::mojom::ConnectionType type) override {
    EXPECT_EQ(type, GetConnectionTypeSync());

    num_notifications_++;
    connection_type_ = type;
    run_loop_->Quit();
  }

  size_t num_notifications() const { return num_notifications_; }
  void WaitForNotification() {
    run_loop_->Run();
    run_loop_.reset(new base::RunLoop());
  }

  network::mojom::ConnectionType connection_type() const {
    return connection_type_;
  }

 private:
  static void GetConnectionTypeCallback(base::RunLoop* run_loop,
                                        network::mojom::ConnectionType* out,
                                        network::mojom::ConnectionType type) {
    *out = type;
    run_loop->Quit();
  }

  size_t num_notifications_;
  NetworkConnectionTracker* tracker_;
  std::unique_ptr<base::RunLoop> run_loop_;
  network::mojom::ConnectionType connection_type_;

  DISALLOW_COPY_AND_ASSIGN(TestNetworkConnectionObserver);
};

// A helper class to call NetworkConnectionTracker::GetConnectionType().
class ConnectionTypeGetter {
 public:
  explicit ConnectionTypeGetter(NetworkConnectionTracker* tracker)
      : tracker_(tracker),
        connection_type_(network::mojom::ConnectionType::CONNECTION_UNKNOWN) {}
  ~ConnectionTypeGetter() {}

  bool GetConnectionType() {
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
    return tracker_->GetConnectionType(
        &connection_type_,
        base::BindOnce(&ConnectionTypeGetter::OnGetConnectionType,
                       base::Unretained(this)));
  }

  void WaitForConnectionType(
      network::mojom::ConnectionType expected_connection_type) {
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
    run_loop_.Run();
    EXPECT_EQ(expected_connection_type, connection_type_);
  }

  network::mojom::ConnectionType connection_type() const {
    return connection_type_;
  }

 private:
  void OnGetConnectionType(network::mojom::ConnectionType type) {
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
    connection_type_ = type;
    run_loop_.Quit();
  }

  base::RunLoop run_loop_;
  NetworkConnectionTracker* tracker_;
  network::mojom::ConnectionType connection_type_;
  THREAD_CHECKER(thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(ConnectionTypeGetter);
};

}  // namespace

class NetworkConnectionTrackerTest : public testing::Test {
 public:
  NetworkConnectionTrackerTest() {
    network::mojom::NetworkServicePtr network_service_ptr;
    network::mojom::NetworkServiceRequest network_service_request =
        mojo::MakeRequest(&network_service_ptr);
    network_service_ =
        network::NetworkService::Create(std::move(network_service_request),
                                        /*netlog=*/nullptr);
    tracker_ = std::make_unique<NetworkConnectionTracker>();
    tracker_->Initialize(network_service_.get());
    observer_ = std::make_unique<TestNetworkConnectionObserver>(tracker_.get());
  }

  ~NetworkConnectionTrackerTest() override {}

  network::NetworkService* network_service() { return network_service_.get(); }

  NetworkConnectionTracker* network_connection_tracker() {
    return tracker_.get();
  }

  TestNetworkConnectionObserver* network_connection_observer() {
    return observer_.get();
  }

  // Simulates a connection type change and broadcast it to observers.
  void SimulateConnectionTypeChange(
      net::NetworkChangeNotifier::ConnectionType type) {
    mock_network_change_notifier_.NotifyObserversOfNetworkChangeForTests(type);
  }

  // Sets the current connection type of the mock network change notifier.
  void SetConnectionType(net::NetworkChangeNotifier::ConnectionType type) {
    mock_network_change_notifier_.SetConnectionType(type);
  }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  net::test::MockNetworkChangeNotifier mock_network_change_notifier_;
  std::unique_ptr<network::NetworkService> network_service_;
  std::unique_ptr<NetworkConnectionTracker> tracker_;
  std::unique_ptr<TestNetworkConnectionObserver> observer_;

  DISALLOW_COPY_AND_ASSIGN(NetworkConnectionTrackerTest);
};

TEST_F(NetworkConnectionTrackerTest, ObserverNotified) {
  EXPECT_EQ(network::mojom::ConnectionType::CONNECTION_UNKNOWN,
            network_connection_observer()->connection_type());

  // Simulate a network change.
  SimulateConnectionTypeChange(
      net::NetworkChangeNotifier::ConnectionType::CONNECTION_3G);

  network_connection_observer()->WaitForNotification();
  EXPECT_EQ(network::mojom::ConnectionType::CONNECTION_3G,
            network_connection_observer()->connection_type());
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1u, network_connection_observer()->num_notifications());
}

TEST_F(NetworkConnectionTrackerTest, UnregisteredObserverNotNotified) {
  auto network_connection_observer2 =
      std::make_unique<TestNetworkConnectionObserver>(
          network_connection_tracker());

  // Simulate a network change.
  SimulateConnectionTypeChange(
      net::NetworkChangeNotifier::ConnectionType::CONNECTION_WIFI);

  network_connection_observer2->WaitForNotification();
  EXPECT_EQ(network::mojom::ConnectionType::CONNECTION_WIFI,
            network_connection_observer2->connection_type());
  network_connection_observer()->WaitForNotification();
  EXPECT_EQ(network::mojom::ConnectionType::CONNECTION_WIFI,
            network_connection_observer()->connection_type());
  base::RunLoop().RunUntilIdle();

  network_connection_observer2.reset();

  // Simulate an another network change.
  SimulateConnectionTypeChange(
      net::NetworkChangeNotifier::ConnectionType::CONNECTION_2G);
  network_connection_observer()->WaitForNotification();
  EXPECT_EQ(network::mojom::ConnectionType::CONNECTION_2G,
            network_connection_observer()->connection_type());
  EXPECT_EQ(2u, network_connection_observer()->num_notifications());
}

TEST_F(NetworkConnectionTrackerTest, GetConnectionType) {
  SetConnectionType(net::NetworkChangeNotifier::ConnectionType::CONNECTION_3G);
  // Creates a new NetworkService so it initializes a NetworkChangeManager
  // with initial connection type as CONNECTION_3G.
  network::mojom::NetworkServicePtr network_service_ptr;
  network::mojom::NetworkServiceRequest network_service_request =
      mojo::MakeRequest(&network_service_ptr);
  std::unique_ptr<network::NetworkService> network_service =
      network::NetworkService::Create(std::move(network_service_request),
                                      nullptr);
  NetworkConnectionTracker tracker;
  tracker.Initialize(network_service_ptr.get());

  ConnectionTypeGetter getter1(&tracker), getter2(&tracker);
  // These two GetConnectionType() will finish asynchonously because network
  // service is not yet set up.
  EXPECT_FALSE(getter1.GetConnectionType());
  EXPECT_FALSE(getter2.GetConnectionType());

  getter1.WaitForConnectionType(
      /*expected_connection_type=*/network::mojom::ConnectionType::
          CONNECTION_3G);
  getter2.WaitForConnectionType(
      /*expected_connection_type=*/network::mojom::ConnectionType::
          CONNECTION_3G);

  ConnectionTypeGetter getter3(&tracker);
  // This GetConnectionType() should finish synchronously.
  EXPECT_TRUE(getter3.GetConnectionType());
  EXPECT_EQ(network::mojom::ConnectionType::CONNECTION_3G,
            getter3.connection_type());
}

// Tests GetConnectionType() on a different thread.
class NetworkGetConnectionTest : public NetworkConnectionTrackerTest {
 public:
  NetworkGetConnectionTest()
      : getter_thread_("NetworkGetConnectionTestThread") {
    getter_thread_.Start();
  }

  ~NetworkGetConnectionTest() override {}

  void GetConnectionType() {
    DCHECK(getter_thread_.task_runner()->RunsTasksInCurrentSequence());
    getter_ =
        std::make_unique<ConnectionTypeGetter>(network_connection_tracker());
    EXPECT_FALSE(getter_->GetConnectionType());
  }

  void WaitForConnectionType(
      network::mojom::ConnectionType expected_connection_type) {
    DCHECK(getter_thread_.task_runner()->RunsTasksInCurrentSequence());
    getter_->WaitForConnectionType(expected_connection_type);
  }

  base::Thread* getter_thread() { return &getter_thread_; }

 private:
  base::Thread getter_thread_;

  // Accessed on |getter_thread_|.
  std::unique_ptr<ConnectionTypeGetter> getter_;

  DISALLOW_COPY_AND_ASSIGN(NetworkGetConnectionTest);
};

TEST_F(NetworkGetConnectionTest, GetConnectionTypeOnDifferentThread) {
  // Flush pending OnInitialConnectionType() notification and force |tracker| to
  // use async for GetConnectionType() calls.
  base::RunLoop().RunUntilIdle();
  base::subtle::NoBarrier_Store(&network_connection_tracker()->connection_type_,
                                -1);
  {
    base::RunLoop run_loop;
    getter_thread()->task_runner()->PostTaskAndReply(
        FROM_HERE,
        base::BindOnce(&NetworkGetConnectionTest::GetConnectionType,
                       base::Unretained(this)),
        base::BindOnce([](base::RunLoop* run_loop) { run_loop->Quit(); },
                       base::Unretained(&run_loop)));
    run_loop.Run();
  }

  network_connection_tracker()->OnInitialConnectionType(
      network::mojom::ConnectionType::CONNECTION_3G);
  {
    base::RunLoop run_loop;
    getter_thread()->task_runner()->PostTaskAndReply(
        FROM_HERE,
        base::BindOnce(&NetworkGetConnectionTest::WaitForConnectionType,
                       base::Unretained(this),
                       /*expected_connection_type=*/
                       network::mojom::ConnectionType::CONNECTION_3G),
        base::BindOnce([](base::RunLoop* run_loop) { run_loop->Quit(); },
                       base::Unretained(&run_loop)));
    run_loop.Run();
  }
}

}  // namespace content
