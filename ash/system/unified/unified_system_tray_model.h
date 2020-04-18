// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_UNIFIED_UNIFIED_SYSTEM_TRAY_MODEL_H_
#define ASH_SYSTEM_UNIFIED_UNIFIED_SYSTEM_TRAY_MODEL_H_

#include "ash/ash_export.h"
#include "base/observer_list.h"
#include "chromeos/dbus/power_manager_client.h"

namespace ash {

// Model class that stores UnifiedSystemTray's UI specific variables. Owned by
// UnifiedSystemTray status area button. Not to be confused with UI agnostic
// SystemTrayModel.
class ASH_EXPORT UnifiedSystemTrayModel {
 public:
  class Observer {
   public:
    virtual ~Observer() {}

    virtual void OnBrightnessChanged() = 0;
  };

  UnifiedSystemTrayModel();
  ~UnifiedSystemTrayModel();

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  bool expanded_on_open() const { return expanded_on_open_; }
  float brightness() const { return brightness_; }

  void set_expanded_on_open(bool expanded_on_open) {
    expanded_on_open_ = expanded_on_open;
  }

 private:
  class DBusObserver;

  void BrightnessChanged(float brightness);

  // If UnifiedSystemTray bubble is expanded on its open. It's expanded by
  // default, and if a user collapses manually, it remembers previous state.
  bool expanded_on_open_ = true;

  // The last value of the brightness slider. Between 0.0 and 1.0.
  float brightness_ = 1.f;

  std::unique_ptr<DBusObserver> dbus_observer_;

  base::ObserverList<Observer> observers_;

  DISALLOW_COPY_AND_ASSIGN(UnifiedSystemTrayModel);
};

}  // namespace ash

#endif  // ASH_SYSTEM_UNIFIED_UNIFIED_SYSTEM_TRAY_MODEL_H_
