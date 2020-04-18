// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/power/power_status_view.h"

#include "ash/strings/grit/ash_strings.h"
#include "ash/system/power/power_status.h"
#include "ash/test/ash_test_base.h"
#include "chromeos/dbus/power_manager/power_supply_properties.pb.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/time_format.h"
#include "ui/views/controls/label.h"

using power_manager::PowerSupplyProperties;

namespace ash {

class PowerStatusViewTest : public AshTestBase {
 public:
  PowerStatusViewTest() = default;
  ~PowerStatusViewTest() override = default;

  // Overridden from testing::Test:
  void SetUp() override {
    AshTestBase::SetUp();
    view_.reset(new PowerStatusView());
  }

  void TearDown() override {
    view_.reset();
    AshTestBase::TearDown();
  }

 protected:
  void UpdatePowerStatus(const power_manager::PowerSupplyProperties& proto) {
    PowerStatus::Get()->SetProtoForTesting(proto);
    view_->OnPowerStatusChanged();
  }

  bool IsPercentageVisible() const {
    return view_->percentage_label_->visible();
  }

  bool IsTimeStatusVisible() const {
    return view_->time_status_label_->visible();
  }

  base::string16 RemainingTimeInView() const {
    return view_->time_status_label_->text();
  }

 private:
  std::unique_ptr<PowerStatusView> view_;

  DISALLOW_COPY_AND_ASSIGN(PowerStatusViewTest);
};

TEST_F(PowerStatusViewTest, Basic) {
  EXPECT_FALSE(IsPercentageVisible());
  EXPECT_TRUE(IsTimeStatusVisible());

  // Disconnect the power.
  PowerSupplyProperties prop;
  prop.set_external_power(PowerSupplyProperties::DISCONNECTED);
  prop.set_battery_state(PowerSupplyProperties::DISCHARGING);
  prop.set_battery_percent(99.0);
  prop.set_battery_time_to_empty_sec(120);
  prop.set_is_calculating_battery_time(true);
  UpdatePowerStatus(prop);

  EXPECT_TRUE(IsPercentageVisible());
  EXPECT_TRUE(IsTimeStatusVisible());
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_BATTERY_CALCULATING),
            RemainingTimeInView());

  prop.set_is_calculating_battery_time(false);
  UpdatePowerStatus(prop);
  EXPECT_NE(l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_BATTERY_CALCULATING),
            RemainingTimeInView());
  EXPECT_NE(l10n_util::GetStringUTF16(
                IDS_ASH_STATUS_TRAY_BATTERY_CHARGING_UNRELIABLE),
            RemainingTimeInView());

  prop.set_external_power(PowerSupplyProperties::AC);
  prop.set_battery_state(PowerSupplyProperties::CHARGING);
  prop.set_battery_time_to_full_sec(120);
  UpdatePowerStatus(prop);
  EXPECT_TRUE(IsPercentageVisible());
  EXPECT_TRUE(IsTimeStatusVisible());
  EXPECT_NE(l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_BATTERY_CALCULATING),
            RemainingTimeInView());
  EXPECT_NE(l10n_util::GetStringUTF16(
                IDS_ASH_STATUS_TRAY_BATTERY_CHARGING_UNRELIABLE),
            RemainingTimeInView());

  prop.set_external_power(PowerSupplyProperties::USB);
  UpdatePowerStatus(prop);
  EXPECT_TRUE(IsPercentageVisible());
  EXPECT_TRUE(IsTimeStatusVisible());
  EXPECT_EQ(l10n_util::GetStringUTF16(
                IDS_ASH_STATUS_TRAY_BATTERY_CHARGING_UNRELIABLE),
            RemainingTimeInView());

  // Tricky -- connected to non-USB but still discharging. Not likely happening
  // on production though.
  prop.set_external_power(PowerSupplyProperties::AC);
  prop.set_battery_state(PowerSupplyProperties::DISCHARGING);
  prop.set_battery_time_to_full_sec(120);
  UpdatePowerStatus(prop);
  EXPECT_TRUE(IsPercentageVisible());
  EXPECT_FALSE(IsTimeStatusVisible());
}

}  // namespace ash
