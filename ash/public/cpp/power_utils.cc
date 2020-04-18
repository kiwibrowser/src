// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/power_utils.h"

#include "base/time/time.h"
#include "chromeos/dbus/power_manager/power_supply_properties.pb.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/chromeos/strings/grit/ui_chromeos_strings.h"

namespace ash {

namespace power_utils {

bool ShouldDisplayBatteryTime(const base::TimeDelta& time) {
  // Put limits on the maximum and minimum battery time-to-full or time-to-empty
  // that should be displayed in the UI. If the current is close to zero,
  // battery time estimates can get very large; avoid displaying these large
  // numbers.
  return time >= base::TimeDelta::FromMinutes(1) &&
         time <= base::TimeDelta::FromDays(1);
}

int GetRoundedBatteryPercent(double battery_percent) {
  // Minimum battery percentage rendered in UI.
  constexpr int kMinBatteryPercent = 1;
  return std::max(kMinBatteryPercent, static_cast<int>(battery_percent + 0.5));
}

void SplitTimeIntoHoursAndMinutes(const base::TimeDelta& time,
                                  int* hours,
                                  int* minutes) {
  DCHECK(hours);
  DCHECK(minutes);
  const int total_minutes = static_cast<int>(time.InSecondsF() / 60 + 0.5);
  *hours = total_minutes / 60;
  *minutes = total_minutes % 60;
}

base::string16 PowerSourceToDisplayString(
    const power_manager::PowerSupplyProperties_PowerSource& source) {
  auto source_to_id = [](const power_manager::PowerSupplyProperties_PowerSource&
                             source) {
    switch (source.port()) {
      case power_manager::PowerSupplyProperties_PowerSource_Port_UNKNOWN:
        return IDS_POWER_SOURCE_PORT_UNKNOWN;
      case power_manager::PowerSupplyProperties_PowerSource_Port_LEFT:
        return IDS_POWER_SOURCE_PORT_LEFT;
      case power_manager::PowerSupplyProperties_PowerSource_Port_RIGHT:
        return IDS_POWER_SOURCE_PORT_RIGHT;
      case power_manager::PowerSupplyProperties_PowerSource_Port_BACK:
        return IDS_POWER_SOURCE_PORT_BACK;
      case power_manager::PowerSupplyProperties_PowerSource_Port_FRONT:
        return IDS_POWER_SOURCE_PORT_FRONT;
      case power_manager::PowerSupplyProperties_PowerSource_Port_LEFT_FRONT:
        return IDS_POWER_SOURCE_PORT_LEFT_FRONT;
      case power_manager::PowerSupplyProperties_PowerSource_Port_LEFT_BACK:
        return IDS_POWER_SOURCE_PORT_LEFT_BACK;
      case power_manager::PowerSupplyProperties_PowerSource_Port_RIGHT_FRONT:
        return IDS_POWER_SOURCE_PORT_RIGHT_FRONT;
      case power_manager::PowerSupplyProperties_PowerSource_Port_RIGHT_BACK:
        return IDS_POWER_SOURCE_PORT_RIGHT_BACK;
      case power_manager::PowerSupplyProperties_PowerSource_Port_BACK_LEFT:
        return IDS_POWER_SOURCE_PORT_BACK_LEFT;
      case power_manager::PowerSupplyProperties_PowerSource_Port_BACK_RIGHT:
        return IDS_POWER_SOURCE_PORT_BACK_RIGHT;
    }
    NOTREACHED();
    return 0;
  };
  return l10n_util::GetStringUTF16(source_to_id(source));
}

}  // namespace power_utils

}  // namespace ash
