// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/network/network_icon.h"

#include <memory>

#include "ash/strings/grit/ash_strings.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/shill_device_client.h"
#include "chromeos/network/network_handler.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/network_state_handler.h"
#include "chromeos/network/network_state_test.h"
#include "chromeos/network/tether_constants.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/cros_system_api/dbus/shill/dbus-constants.h"
#include "ui/gfx/image/image_unittest_util.h"

namespace ash {

namespace network_icon {

class NetworkIconTest : public chromeos::NetworkStateTest {
 public:
  NetworkIconTest() = default;
  ~NetworkIconTest() override = default;

  void SetUp() override {
    chromeos::DBusThreadManager::Initialize();
    chromeos::NetworkStateTest::SetUp();

    chromeos::NetworkHandler::Initialize();
    handler_ = chromeos::NetworkHandler::Get()->network_state_handler();

    tether_network_ =
        std::make_unique<chromeos::NetworkState>("tetherNetworkPath");
    tether_network_->set_type(chromeos::kTypeTether);

    wifi_network_ = std::make_unique<chromeos::NetworkState>("wifiServicePath");
    wifi_network_->set_type(shill::kTypeWifi);

    cellular_network_ =
        std::make_unique<chromeos::NetworkState>("cellularServicePath");
    cellular_network_->set_type(shill::kTypeCellular);

    wifi_tether_network_ =
        std::make_unique<chromeos::NetworkState>("wifiTetherServicePath");
    wifi_tether_network_->set_type(shill::kTypeWifi);
    wifi_tether_network_.get()->set_tether_guid("tetherNetworkGuid");

    ethernet_network_ =
        std::make_unique<chromeos::NetworkState>("ethernetNetworkPath");
    ethernet_network_->set_type(shill::kTypeEthernet);
  }

  void TearDown() override {
    PurgeNetworkIconCache();

    chromeos::NetworkHandler::Shutdown();

    ShutdownNetworkState();
    chromeos::NetworkStateTest::TearDown();
    chromeos::DBusThreadManager::Shutdown();
  }

  gfx::Image ImageForNetwork(chromeos::NetworkState* network) {
    gfx::ImageSkia image_skia = GetImageForNetwork(network, icon_type_);
    return gfx::Image(image_skia);
  }

  // The icon for a Tether network should be the same as one for a cellular
  // network. The icon for a Tether network should be different from one for a
  // Wi-Fi network. The icon for a cellular network should be different from one
  // for a Wi-Fi network. The icon for a Tether network should be the same as
  // one for a Wi-Fi network with an associated Tether guid.
  void GetAndCompareImagesByNetworkType() {
    gfx::Image tether_image = ImageForNetwork(tether_network_.get());
    gfx::Image wifi_image = ImageForNetwork(wifi_network_.get());
    gfx::Image cellular_image = ImageForNetwork(cellular_network_.get());
    gfx::Image wifi_tether_image = ImageForNetwork(wifi_tether_network_.get());

    EXPECT_FALSE(gfx::test::AreImagesEqual(tether_image, wifi_image));
    EXPECT_FALSE(gfx::test::AreImagesEqual(cellular_image, wifi_image));
    EXPECT_TRUE(gfx::test::AreImagesEqual(tether_image, cellular_image));

    EXPECT_TRUE(gfx::test::AreImagesEqual(tether_image, wifi_tether_image));
  }

  void SetCellularUnavailable() {
    test_manager_client()->RemoveTechnology(shill::kTypeCellular);

    base::RunLoop().RunUntilIdle();

    ASSERT_EQ(
        chromeos::NetworkStateHandler::TechnologyState::TECHNOLOGY_UNAVAILABLE,
        handler_->GetTechnologyState(chromeos::NetworkTypePattern::Cellular()));
  }

  void SetCellularUninitialized() {
    test_manager_client()->RemoveTechnology(shill::kTypeCellular);
    test_manager_client()->AddTechnology(shill::kTypeCellular, false);
    test_manager_client()->SetTechnologyInitializing(shill::kTypeCellular,
                                                     true);

    base::RunLoop().RunUntilIdle();

    ASSERT_EQ(
        chromeos::NetworkStateHandler::TechnologyState::
            TECHNOLOGY_UNINITIALIZED,
        handler_->GetTechnologyState(chromeos::NetworkTypePattern::Cellular()));
  }

  const base::MessageLoop message_loop_;

  IconType icon_type_ = ICON_TYPE_TRAY;

  chromeos::NetworkStateHandler* handler_;

  std::unique_ptr<chromeos::NetworkState> tether_network_;
  std::unique_ptr<chromeos::NetworkState> wifi_network_;
  std::unique_ptr<chromeos::NetworkState> cellular_network_;
  // A network whose type is shill::kTypeWifi, but which is associated with
  // a Tether network via its Tether network ID.
  std::unique_ptr<chromeos::NetworkState> wifi_tether_network_;
  std::unique_ptr<chromeos::NetworkState> ethernet_network_;

 private:
  DISALLOW_COPY_AND_ASSIGN(NetworkIconTest);
};

// This tests that the correct icons are being generated for the correct
// networks by pairwise comparison of three different network types, verifying
// that the Tether and cellular icon are the same, Tether and Wi-Fi icons are
// different, and cellular and Wi-Fi icons are different. Additionally, it
// verifies that the Tether network and Wi-Fi network with associated Tether
// guid are treated the same for purposes of icon display
TEST_F(NetworkIconTest, CompareImagesByNetworkType_NotVisible) {
  GetAndCompareImagesByNetworkType();
}

TEST_F(NetworkIconTest, CompareImagesByNetworkType_Connecting) {
  tether_network_->set_visible(true);
  tether_network_->set_connection_state(shill::kStateAssociation);

  wifi_network_->set_visible(true);
  wifi_network_->set_connection_state(shill::kStateAssociation);

  cellular_network_->set_visible(true);
  cellular_network_->set_connection_state(shill::kStateAssociation);

  wifi_tether_network_->set_visible(true);
  wifi_tether_network_->set_connection_state(shill::kStateAssociation);

  GetAndCompareImagesByNetworkType();
}

TEST_F(NetworkIconTest, CompareImagesByNetworkType_Connected) {
  tether_network_->set_visible(true);
  tether_network_->set_connection_state(shill::kStateOnline);

  wifi_network_->set_visible(true);
  wifi_network_->set_connection_state(shill::kStateOnline);

  cellular_network_->set_visible(true);
  cellular_network_->set_connection_state(shill::kStateOnline);

  wifi_tether_network_->set_visible(true);
  wifi_tether_network_->set_connection_state(shill::kStateOnline);

  GetAndCompareImagesByNetworkType();
}

TEST_F(NetworkIconTest,
       GetCellularUninitializedMsg_NoUninitializedMessageExpected) {
  EXPECT_EQ(0, GetCellularUninitializedMsg());
}

TEST_F(NetworkIconTest, GetCellularUninitializedMsg_CellularUninitialized) {
  SetCellularUninitialized();

  EXPECT_EQ(IDS_ASH_STATUS_TRAY_INITIALIZING_CELLULAR,
            GetCellularUninitializedMsg());
}

TEST_F(NetworkIconTest, GetCellularUninitializedMsg_CellularScanning) {
  SetCellularUninitialized();

  test_manager_client()->AddTechnology(shill::kTypeCellular, true);

  chromeos::DBusThreadManager* dbus_manager =
      chromeos::DBusThreadManager::Get();
  chromeos::ShillDeviceClient::TestInterface* device_test =
      dbus_manager->GetShillDeviceClient()->GetTestInterface();

  device_test->SetDeviceProperty("/device/cellular1", shill::kScanningProperty,
                                 base::Value(true), /*notify_changed=*/true);
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(
      handler_->GetScanningByType(chromeos::NetworkTypePattern::Cellular()));

  EXPECT_EQ(IDS_ASH_STATUS_TRAY_MOBILE_SCANNING, GetCellularUninitializedMsg());
}

TEST_F(NetworkIconTest, NetworkSignalStrength) {
  using ss = SignalStrength;

  // Verify non-wirless network types return SignalStrength::NOT_WIRELESS, and
  // wireless network types return something other than
  // SignalStrength::NOT_WIRELESS.
  EXPECT_EQ(ss::NOT_WIRELESS,
            GetSignalStrengthForNetwork(ethernet_network_.get()));
  EXPECT_NE(ss::NOT_WIRELESS, GetSignalStrengthForNetwork(wifi_network_.get()));

  // Signal strength is divided into four categories: none, weak, medium and
  // strong. They are meant to match the number of sections in the wifi icon.
  // The wifi icon currently has four levels; signals [0, 100] are mapped to [1,
  // 4]. There are only three signal strengths so icons that were mapped to 2
  // are also considered weak.
  wifi_network_->set_signal_strength(0);
  EXPECT_EQ(ss::NONE, GetSignalStrengthForNetwork(wifi_network_.get()));
  wifi_network_->set_signal_strength(50);
  EXPECT_EQ(ss::WEAK, GetSignalStrengthForNetwork(wifi_network_.get()));
  wifi_network_->set_signal_strength(51);
  EXPECT_EQ(ss::MEDIUM, GetSignalStrengthForNetwork(wifi_network_.get()));
  wifi_network_->set_signal_strength(75);
  EXPECT_EQ(ss::MEDIUM, GetSignalStrengthForNetwork(wifi_network_.get()));
  wifi_network_->set_signal_strength(76);
  EXPECT_EQ(ss::STRONG, GetSignalStrengthForNetwork(wifi_network_.get()));
  wifi_network_->set_signal_strength(100);
  EXPECT_EQ(ss::STRONG, GetSignalStrengthForNetwork(wifi_network_.get()));
}

}  // namespace network_icon

}  // namespace ash
