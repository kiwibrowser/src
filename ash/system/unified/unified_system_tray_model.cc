// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/unified/unified_system_tray_model.h"

#include "ash/shell.h"
#include "ash/system/brightness_control_delegate.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/power_manager/backlight.pb.h"

namespace ash {

class UnifiedSystemTrayModel::DBusObserver
    : public chromeos::PowerManagerClient::Observer {
 public:
  explicit DBusObserver(UnifiedSystemTrayModel* owner);
  ~DBusObserver() override;

 private:
  void HandleInitialBrightness(base::Optional<double> percent);

  // chromeos::PowerManagerClient::Observer:
  void ScreenBrightnessChanged(
      const power_manager::BacklightBrightnessChange& change) override;

  UnifiedSystemTrayModel* const owner_;

  base::WeakPtrFactory<DBusObserver> weak_ptr_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(DBusObserver);
};

UnifiedSystemTrayModel::DBusObserver::DBusObserver(
    UnifiedSystemTrayModel* owner)
    : owner_(owner) {
  chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->AddObserver(
      this);
  Shell::Get()->brightness_control_delegate()->GetBrightnessPercent(
      base::BindOnce(&DBusObserver::HandleInitialBrightness,
                     weak_ptr_factory_.GetWeakPtr()));
}

UnifiedSystemTrayModel::DBusObserver::~DBusObserver() {
  chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->RemoveObserver(
      this);
}

void UnifiedSystemTrayModel::DBusObserver::HandleInitialBrightness(
    base::Optional<double> percent) {
  if (percent.has_value())
    owner_->BrightnessChanged(percent.value() / 100.);
}

void UnifiedSystemTrayModel::DBusObserver::ScreenBrightnessChanged(
    const power_manager::BacklightBrightnessChange& change) {
  Shell::Get()->metrics()->RecordUserMetricsAction(
      UMA_STATUS_AREA_BRIGHTNESS_CHANGED);
  owner_->BrightnessChanged(change.percent() / 100.);
}

UnifiedSystemTrayModel::UnifiedSystemTrayModel()
    : dbus_observer_(std::make_unique<DBusObserver>(this)) {}

UnifiedSystemTrayModel::~UnifiedSystemTrayModel() = default;

void UnifiedSystemTrayModel::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void UnifiedSystemTrayModel::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void UnifiedSystemTrayModel::BrightnessChanged(float brightness) {
  brightness_ = brightness;
  for (auto& observer : observers_)
    observer.OnBrightnessChanged();
}

}  // namespace ash
