// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_DEVICES_INPUT_DEVICE_OBSERVER_WIN_H_
#define UI_EVENTS_DEVICES_INPUT_DEVICE_OBSERVER_WIN_H_

#include "base/observer_list.h"
#include "base/win/registry.h"
#include "ui/events/devices/input_device_event_observer.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}

namespace ui {
class EVENTS_DEVICES_EXPORT InputDeviceObserverWin {
 public:
  static InputDeviceObserverWin* GetInstance();
  ~InputDeviceObserverWin();

  void AddObserver(InputDeviceEventObserver* observer);
  void RemoveObserver(InputDeviceEventObserver* observer);

 protected:
  InputDeviceObserverWin();

 private:
  void OnRegistryKeyChanged(base::win::RegKey* key);
  bool IsSlateModeEnabled(base::win::RegKey* key);
  void NotifyObserversKeyboardDeviceConfigurationChanged();
  void NotifyObserversTouchpadDeviceConfigurationChanged();

  std::unique_ptr<base::win::RegKey> registry_key_;
  base::ObserverList<InputDeviceEventObserver> observers_;
  bool slate_mode_enabled_;

  friend struct base::DefaultSingletonTraits<InputDeviceObserverWin>;

  base::WeakPtrFactory<InputDeviceObserverWin> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(InputDeviceObserverWin);
};

}  // namespace ui

#endif  // UI_EVENTS_DEVICES_INPUT_DEVICE_OBSERVER_WIN_H_