// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_DEVICES_INPUT_DEVICE_MANAGER_H_
#define UI_EVENTS_DEVICES_INPUT_DEVICE_MANAGER_H_

#include <vector>

#include "base/macros.h"
#include "ui/events/devices/events_devices_export.h"
#include "ui/events/devices/input_device.h"
#include "ui/events/devices/input_device_event_observer.h"
#include "ui/events/devices/touchscreen_device.h"

namespace ui {

// Interface to query available input devices. Holds a thread-local pointer to
// an implementation that provides this service. The implementation could be
// DeviceDataManager or something that mirrors the necessary state if
// DeviceDataManager is in a different process.
class EVENTS_DEVICES_EXPORT InputDeviceManager {
 public:
  InputDeviceManager() {}

  static InputDeviceManager* GetInstance();
  static bool HasInstance();

  virtual const std::vector<InputDevice>& GetKeyboardDevices() const = 0;
  virtual const std::vector<TouchscreenDevice>& GetTouchscreenDevices()
      const = 0;
  virtual const std::vector<InputDevice>& GetMouseDevices() const = 0;
  virtual const std::vector<InputDevice>& GetTouchpadDevices() const = 0;

  virtual bool AreDeviceListsComplete() const = 0;
  virtual bool AreTouchscreensEnabled() const = 0;

  // Returns true if the |target_display_id| of the TouchscreenDevices returned
  // from GetTouchscreenDevices() is valid.
  virtual bool AreTouchscreenTargetDisplaysValid() const = 0;

  virtual void AddObserver(InputDeviceEventObserver* observer) = 0;
  virtual void RemoveObserver(InputDeviceEventObserver* observer) = 0;

 protected:
  // Sets the instance. This should only be set once per thread.
  static void SetInstance(InputDeviceManager* instance);

  // Clears the instance. InputDeviceManager doesn't own the instance and won't
  // destroy it, so it should be cleared before it is destroyed elsewhere.
  static void ClearInstance();

 private:
  static InputDeviceManager* instance_;  // Not owned.

  DISALLOW_COPY_AND_ASSIGN(InputDeviceManager);
};

}  // namespace ui

#endif  // UI_EVENTS_DEVICES_INPUT_DEVICE_MANAGER_H_
