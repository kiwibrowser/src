// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_PUBLIC_CPP_INPUT_DEVICES_INPUT_DEVICE_CLIENT_H_
#define SERVICES_UI_PUBLIC_CPP_INPUT_DEVICES_INPUT_DEVICE_CLIENT_H_

#include <vector>

#include "base/macros.h"
#include "base/observer_list.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/ui/public/interfaces/input_devices/input_device_server.mojom.h"
#include "ui/events/devices/input_device.h"
#include "ui/events/devices/input_device_event_observer.h"
#include "ui/events/devices/input_device_manager.h"
#include "ui/events/devices/touchscreen_device.h"

namespace ui {

// Allows in-process client code to register as a InputDeviceEventObserver and
// get information about input-devices. InputDeviceClient itself acts as an
// InputDeviceObserverMojo and registers to get updates from InputDeviceServer.
// Essentially, InputDeviceClient forwards input-device events and caches
// input-device state.
class InputDeviceClient : public mojom::InputDeviceObserverMojo,
                          public ui::InputDeviceManager {
 public:
  InputDeviceClient();
  ~InputDeviceClient() override;

  // Connects to mojo:ui as an observer on InputDeviceServer to receive input
  // device updates.
  void Connect(mojom::InputDeviceServerPtr server);

  // ui::InputDeviceManager:
  const std::vector<ui::InputDevice>& GetKeyboardDevices() const override;
  const std::vector<ui::TouchscreenDevice>& GetTouchscreenDevices()
      const override;
  const std::vector<ui::InputDevice>& GetMouseDevices() const override;
  const std::vector<ui::InputDevice>& GetTouchpadDevices() const override;
  bool AreDeviceListsComplete() const override;
  bool AreTouchscreensEnabled() const override;
  bool AreTouchscreenTargetDisplaysValid() const override;
  void AddObserver(ui::InputDeviceEventObserver* observer) override;
  void RemoveObserver(ui::InputDeviceEventObserver* observer) override;

 protected:
  // Default constructor registers as InputDeviceManager. Can be subclassed in
  // tests to avoid this.
  explicit InputDeviceClient(bool is_input_device_manager);
  mojom::InputDeviceObserverMojoPtr GetIntefacePtr();

  // mojom::InputDeviceObserverMojo:
  void OnKeyboardDeviceConfigurationChanged(
      const std::vector<ui::InputDevice>& devices) override;
  void OnTouchscreenDeviceConfigurationChanged(
      const std::vector<ui::TouchscreenDevice>& devices,
      bool touchscreen_target_display_ids_changed) override;
  void OnMouseDeviceConfigurationChanged(
      const std::vector<ui::InputDevice>& devices) override;
  void OnTouchpadDeviceConfigurationChanged(
      const std::vector<ui::InputDevice>& devices) override;
  void OnDeviceListsComplete(
      const std::vector<ui::InputDevice>& keyboard_devices,
      const std::vector<ui::TouchscreenDevice>& touchscreen_devices,
      const std::vector<ui::InputDevice>& mouse_devices,
      const std::vector<ui::InputDevice>& touchpad_devices,
      bool are_touchscreen_target_displays_valid) override;
  void OnStylusStateChanged(StylusState state) override;

 private:
  friend class InputDeviceClientTestApi;

  void NotifyObserversDeviceListsComplete();
  void NotifyObserversKeyboardDeviceConfigurationChanged();
  void NotifyObserversTouchscreenDeviceConfigurationChanged();

  mojo::Binding<mojom::InputDeviceObserverMojo> binding_;

  bool is_input_device_manager_;

  // Holds the list of input devices and signal that we have received the lists
  // after initialization.
  std::vector<ui::InputDevice> keyboard_devices_;
  std::vector<ui::TouchscreenDevice> touchscreen_devices_;
  std::vector<ui::InputDevice> mouse_devices_;
  std::vector<ui::InputDevice> touchpad_devices_;
  bool device_lists_complete_ = false;
  bool are_touchscreen_target_displays_valid_ = false;

  // List of in-process observers.
  base::ObserverList<ui::InputDeviceEventObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(InputDeviceClient);
};

}  // namespace ui

#endif  // SERVICES_UI_PUBLIC_CPP_INPUT_DEVICES_INPUT_DEVICE_CLIENT_H_
