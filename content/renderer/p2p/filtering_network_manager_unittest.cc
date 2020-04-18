// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/p2p/filtering_network_manager.h"

#include <stddef.h>

#include <memory>
#include <utility>

#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/renderer/p2p/empty_network_manager.h"
#include "media/base/media_permission.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/webrtc/rtc_base/ipaddress.h"

using NetworkList = rtc::NetworkManager::NetworkList;

namespace {

enum EventType {
  MIC_DENIED,      // Receive mic permission denied.
  MIC_GRANTED,     // Receive mic permission granted.
  CAMERA_DENIED,   // Receive camera permission denied.
  CAMERA_GRANTED,  // Receive camera permission granted.
  START_UPDATING,  // Client calls StartUpdating() on FilteringNetworkManager.
  STOP_UPDATING,   // Client calls StopUpdating() on FilteringNetworkManager.
  MOCK_NETWORKS_CHANGED,  // MockNetworkManager has signaled networks changed
                          // event.
};

enum ResultType {
  NO_SIGNAL,                   // Do not expect SignalNetworksChanged fired.
  SIGNAL_ENUMERATION_BLOCKED,  // Expect SignalNetworksChanged and
                               // ENUMERATION_BLOCKED.
  SIGNAL_ENUMERATION_ALLOWED,  // Expect SignalNetworksChanged and
                               // ENUMERATION_ALLOWED.
};

struct TestEntry {
  EventType event;
  ResultType expected_result;
};

class MockNetworkManager : public rtc::NetworkManager {
 public:
  MockNetworkManager() {
    network_.reset(new rtc::Network("test_eth0", "Test Network Adapter 1",
                                    rtc::IPAddress(0x12345600U), 24,
                                    rtc::ADAPTER_TYPE_ETHERNET));
  }
  // Mimic the current behavior that once the first signal is sent, any future
  // StartUpdating() will trigger another one.
  void StartUpdating() override {
    if (sent_first_update_)
      SignalNetworksChanged();
  }
  void StopUpdating() override {}
  void GetNetworks(NetworkList* networks) const override {
    networks->push_back(network_.get());
  }

  void SendNetworksChanged() {
    sent_first_update_ = true;
    SignalNetworksChanged();
  }

 private:
  bool sent_first_update_ = false;
  std::unique_ptr<rtc::Network> network_;
};

class MockMediaPermission : public media::MediaPermission {
 public:
  MockMediaPermission() {}
  ~MockMediaPermission() override {}

  void RequestPermission(
      Type type,
      const PermissionStatusCB& permission_status_cb) override {
    NOTIMPLEMENTED();
  }

  void HasPermission(Type type,
                     const PermissionStatusCB& permission_status_cb) override {
    if (type == MediaPermission::AUDIO_CAPTURE) {
      DCHECK(mic_callback_.is_null());
      mic_callback_ = permission_status_cb;
    } else {
      DCHECK(type == MediaPermission::VIDEO_CAPTURE);
      DCHECK(camera_callback_.is_null());
      camera_callback_ = permission_status_cb;
    }
  }

  bool IsEncryptedMediaEnabled() override { return true; }

  void SetMicPermission(bool granted) {
    if (mic_callback_.is_null())
      return;

    mic_callback_.Run(granted);
    mic_callback_ = PermissionStatusCB();
  }

  void SetCameraPermission(bool granted) {
    if (camera_callback_.is_null())
      return;

    camera_callback_.Run(granted);
    camera_callback_ = PermissionStatusCB();
  }

 private:
  PermissionStatusCB mic_callback_;
  PermissionStatusCB camera_callback_;
};

}  // namespace

namespace content {

class FilteringNetworkManagerTest : public testing::Test,
                                    public sigslot::has_slots<> {
 public:
  FilteringNetworkManagerTest()
      : media_permission_(new MockMediaPermission()),
        task_runner_(new base::TestSimpleTaskRunner()),
        task_runner_handle_(task_runner_) {}
  void SetupNetworkManager(bool multiple_routes_requested) {
    mock_network_manager_.reset(new MockNetworkManager());
    if (multiple_routes_requested) {
      FilteringNetworkManager* filtering_network_manager =
          new FilteringNetworkManager(mock_network_manager_.get(), GURL(),
                                      media_permission_.get());
      filtering_network_manager->Initialize();
      network_manager_.reset(filtering_network_manager);
    } else {
      network_manager_.reset(
          new EmptyNetworkManager(mock_network_manager_.get()));
    }
    network_manager_->SignalNetworksChanged.connect(
        this, &FilteringNetworkManagerTest::OnNetworksChanged);
  }

  void RunTests(TestEntry* tests, size_t size) {
    for (size_t i = 0; i < size; ++i) {
      EXPECT_EQ(tests[i].expected_result, ProcessEvent(tests[i].event))
          << " in step: " << i;
    }
  }

  ResultType ProcessEvent(EventType event) {
    clear_callback_called();
    switch (event) {
      case MIC_DENIED:
      case MIC_GRANTED:
        media_permission_->SetMicPermission(event == MIC_GRANTED);
        break;
      case CAMERA_DENIED:
      case CAMERA_GRANTED:
        media_permission_->SetCameraPermission(event == CAMERA_GRANTED);
        break;
      case START_UPDATING:
        network_manager_->StartUpdating();
        break;
      case STOP_UPDATING:
        network_manager_->StopUpdating();
        break;
      case MOCK_NETWORKS_CHANGED:
        mock_network_manager_->SendNetworksChanged();
        break;
    }

    task_runner_->RunUntilIdle();

    if (!callback_called_)
      return NO_SIGNAL;

    if (network_manager_->enumeration_permission() ==
        rtc::NetworkManager::ENUMERATION_BLOCKED) {
      EXPECT_EQ(0u, GetP2PNetworkList().size());
      return SIGNAL_ENUMERATION_BLOCKED;
    }
    EXPECT_EQ(1u, GetP2PNetworkList().size());
    return SIGNAL_ENUMERATION_ALLOWED;
  }

 protected:
  const NetworkList& GetP2PNetworkList() {
    network_list_.clear();
    network_manager_->GetNetworks(&network_list_);
    return network_list_;
  }

  void OnNetworksChanged() { callback_called_ = true; }
  void clear_callback_called() { callback_called_ = false; }

  bool callback_called_ = false;
  std::unique_ptr<rtc::NetworkManager> network_manager_;
  std::unique_ptr<MockNetworkManager> mock_network_manager_;

  std::unique_ptr<MockMediaPermission> media_permission_;

  NetworkList network_list_;
  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle task_runner_handle_;
};

// Test that when multiple routes is not requested, SignalNetworksChanged is
// fired right after the StartUpdating().
TEST_F(FilteringNetworkManagerTest, MultipleRoutesNotRequested) {
  SetupNetworkManager(false);
  TestEntry tests[] = {
      // Underneath network manager signals, no callback as StartUpdating() is
      // not called.
      {MOCK_NETWORKS_CHANGED, NO_SIGNAL},
      // StartUpdating() is called, should receive callback as the multiple
      // routes is not requested.
      {START_UPDATING, SIGNAL_ENUMERATION_BLOCKED},
      // Further network signal should trigger callback, since the default
      // network could have changed.
      {MOCK_NETWORKS_CHANGED, SIGNAL_ENUMERATION_BLOCKED},
      // New StartUpdating() should trigger callback.
      {START_UPDATING, SIGNAL_ENUMERATION_BLOCKED},
      {STOP_UPDATING, NO_SIGNAL},
      {STOP_UPDATING, NO_SIGNAL},
      {MOCK_NETWORKS_CHANGED, NO_SIGNAL},
  };

  RunTests(tests, arraysize(tests));
}

// Test that multiple routes request is blocked and signaled right after
// StartUpdating() since mic/camera permissions are denied.
TEST_F(FilteringNetworkManagerTest, BlockMultipleRoutesByStartUpdating) {
  SetupNetworkManager(true);

  TestEntry tests[] = {
      {MOCK_NETWORKS_CHANGED, NO_SIGNAL},
      // Both mic and camera are denied.
      {MIC_DENIED, NO_SIGNAL},
      {CAMERA_DENIED, NO_SIGNAL},
      // Once StartUpdating() is called, signal network changed event with
      // ENUMERATION_BLOCKED.
      {START_UPDATING, SIGNAL_ENUMERATION_BLOCKED},
      // Further network signal should trigger callback, since the default
      // network could have changed.
      {MOCK_NETWORKS_CHANGED, SIGNAL_ENUMERATION_BLOCKED},
      // New StartUpdating() should trigger callback.
      {START_UPDATING, SIGNAL_ENUMERATION_BLOCKED},
      {STOP_UPDATING, NO_SIGNAL},
      {STOP_UPDATING, NO_SIGNAL},
      {MOCK_NETWORKS_CHANGED, NO_SIGNAL},
  };

  RunTests(tests, arraysize(tests));
}

// Test that multiple routes request is blocked and signaled right after
// last pending permission check is denied since StartUpdating() has been called
// previously.
TEST_F(FilteringNetworkManagerTest, BlockMultipleRoutesByPermissionsDenied) {
  SetupNetworkManager(true);

  TestEntry tests[] = {
      {START_UPDATING, NO_SIGNAL},
      {MOCK_NETWORKS_CHANGED, NO_SIGNAL},
      {MIC_DENIED, NO_SIGNAL},
      // The last permission check being denied should immediately trigger the
      // networks changed signal, since we already have an updated network list.
      {CAMERA_DENIED, SIGNAL_ENUMERATION_BLOCKED},
      {START_UPDATING, SIGNAL_ENUMERATION_BLOCKED},
      {STOP_UPDATING, NO_SIGNAL},
      {STOP_UPDATING, NO_SIGNAL},
      {MOCK_NETWORKS_CHANGED, NO_SIGNAL},
  };

  RunTests(tests, arraysize(tests));
}

// Test that after permissions have been denied, a network change signal from
// the internal NetworkManager is still needed before signaling a network
// change outwards. This is because even if network enumeration is blocked,
// we still want to give time to obtain the default IP addresses.
TEST_F(FilteringNetworkManagerTest, BlockMultipleRoutesByNetworksChanged) {
  SetupNetworkManager(true);

  TestEntry tests[] = {
      {START_UPDATING, NO_SIGNAL},
      {MIC_DENIED, NO_SIGNAL},
      {CAMERA_DENIED, NO_SIGNAL},
      {MOCK_NETWORKS_CHANGED, SIGNAL_ENUMERATION_BLOCKED},
      {START_UPDATING, SIGNAL_ENUMERATION_BLOCKED},
      {STOP_UPDATING, NO_SIGNAL},
      {STOP_UPDATING, NO_SIGNAL},
      {MOCK_NETWORKS_CHANGED, NO_SIGNAL},
  };

  RunTests(tests, arraysize(tests));
}

// Test that multiple routes request is granted and signaled right after
// a pending permission check is granted since StartUpdating() has been called
// previously.
TEST_F(FilteringNetworkManagerTest, AllowMultipleRoutesByPermissionsGranted) {
  SetupNetworkManager(true);

  TestEntry tests[] = {
      {START_UPDATING, NO_SIGNAL},
      {MIC_DENIED, NO_SIGNAL},
      {MOCK_NETWORKS_CHANGED, NO_SIGNAL},
      // Once one media type is granted, signal networkschanged with
      // ENUMERATION_ALLOWED.
      {CAMERA_GRANTED, SIGNAL_ENUMERATION_ALLOWED},
      {MOCK_NETWORKS_CHANGED, SIGNAL_ENUMERATION_ALLOWED},
      {START_UPDATING, SIGNAL_ENUMERATION_ALLOWED},
      {STOP_UPDATING, NO_SIGNAL},
      // If there is any outstanding StartUpdating(), new event from underneath
      // network manger should trigger SignalNetworksChanged.
      {MOCK_NETWORKS_CHANGED, SIGNAL_ENUMERATION_ALLOWED},
      {STOP_UPDATING, NO_SIGNAL},
      // No outstanding StartUpdating(), no more signal.
      {MOCK_NETWORKS_CHANGED, NO_SIGNAL},
  };

  RunTests(tests, arraysize(tests));
}

// Test that multiple routes request is granted and signaled right after
// StartUpdating() since there is at least one media permission granted.
TEST_F(FilteringNetworkManagerTest, AllowMultipleRoutesByStartUpdating) {
  SetupNetworkManager(true);

  TestEntry tests[] = {
      {MIC_DENIED, NO_SIGNAL},
      {MOCK_NETWORKS_CHANGED, NO_SIGNAL},
      {CAMERA_GRANTED, NO_SIGNAL},
      // StartUpdating() should trigger the event since at least one media is
      // granted and network information is populated.
      {START_UPDATING, SIGNAL_ENUMERATION_ALLOWED},
      {MOCK_NETWORKS_CHANGED, SIGNAL_ENUMERATION_ALLOWED},
      {START_UPDATING, SIGNAL_ENUMERATION_ALLOWED},
      {STOP_UPDATING, NO_SIGNAL},
      {MOCK_NETWORKS_CHANGED, SIGNAL_ENUMERATION_ALLOWED},
      {STOP_UPDATING, NO_SIGNAL},
      {MOCK_NETWORKS_CHANGED, NO_SIGNAL},
  };

  RunTests(tests, arraysize(tests));
}

// Test that multiple routes request is granted and signaled right after
// underneath NetworkManager's SignalNetworksChanged() as at least one
// permission is granted and StartUpdating() has been called.
TEST_F(FilteringNetworkManagerTest, AllowMultipleRoutesByNetworksChanged) {
  SetupNetworkManager(true);

  TestEntry tests[] = {
      {START_UPDATING, NO_SIGNAL},
      {CAMERA_GRANTED, NO_SIGNAL},
      // Underneath network manager's signal networks changed should trigger
      // SignalNetworksChanged with ENUMERATION_ALLOWED.
      {MOCK_NETWORKS_CHANGED, SIGNAL_ENUMERATION_ALLOWED},
      {MIC_DENIED, NO_SIGNAL},
      {MOCK_NETWORKS_CHANGED, SIGNAL_ENUMERATION_ALLOWED},
      {START_UPDATING, SIGNAL_ENUMERATION_ALLOWED},
      {STOP_UPDATING, NO_SIGNAL},
      {MOCK_NETWORKS_CHANGED, SIGNAL_ENUMERATION_ALLOWED},
      {STOP_UPDATING, NO_SIGNAL},
      {MOCK_NETWORKS_CHANGED, NO_SIGNAL},
  };

  RunTests(tests, arraysize(tests));
}

}  // namespace content
