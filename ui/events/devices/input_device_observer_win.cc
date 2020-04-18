// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/devices/input_device_observer_win.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/memory/singleton.h"
#include "base/strings/string16.h"

#include <windows.h>

// This macro provides the implementation for the observer notification methods.
#define NOTIFY_OBSERVERS_METHOD(method_decl, observer_call) \
  void InputDeviceObserverWin::method_decl {                \
    for (InputDeviceEventObserver & observer : observers_)  \
      observer.observer_call;                               \
  }

namespace ui {

namespace {

// The registry subkey that contains information about the state of the
// detachable/convertible laptop, it tells if the device has an accessible
// keyboard.
// OEMs are expected to follow this guidelines to report docked/undocked state
// https://msdn.microsoft.com/en-us/windows/hardware/commercialize/customize/desktop/unattend/microsoft-windows-gpiobuttons-convertibleslatemode
const base::char16 kRegistryPriorityControl[] =
    L"System\\CurrentControlSet\\Control\\PriorityControl";

const base::char16 kRegistryConvertibleSlateModeKey[] = L"ConvertibleSlateMode";

}  // namespace

InputDeviceObserverWin::InputDeviceObserverWin() : weak_factory_(this) {
  registry_key_.reset(new base::win::RegKey(
      HKEY_LOCAL_MACHINE, kRegistryPriorityControl, KEY_NOTIFY | KEY_READ));

  if (registry_key_->Valid()) {
    slate_mode_enabled_ = IsSlateModeEnabled(registry_key_.get());
    // Start watching the registry for changes.
    base::win::RegKey::ChangeCallback callback =
        base::Bind(&InputDeviceObserverWin::OnRegistryKeyChanged,
                   weak_factory_.GetWeakPtr(), registry_key_.get());
    registry_key_->StartWatching(callback);
  }
}

InputDeviceObserverWin* InputDeviceObserverWin::GetInstance() {
  return base::Singleton<
      InputDeviceObserverWin,
      base::LeakySingletonTraits<InputDeviceObserverWin>>::get();
}

InputDeviceObserverWin::~InputDeviceObserverWin() {}

void InputDeviceObserverWin::OnRegistryKeyChanged(base::win::RegKey* key) {
  if (!key)
    return;

  // |OnRegistryKeyChanged| is removed as an observer when the ChangeCallback is
  // called, so we need to re-register.
  key->StartWatching(base::Bind(&InputDeviceObserverWin::OnRegistryKeyChanged,
                                weak_factory_.GetWeakPtr(),
                                base::Unretained(key)));

  bool new_slate_mode = IsSlateModeEnabled(key);
  if (slate_mode_enabled_ == new_slate_mode)
    return;

  NotifyObserversTouchpadDeviceConfigurationChanged();
  NotifyObserversKeyboardDeviceConfigurationChanged();
  slate_mode_enabled_ = new_slate_mode;
}

bool InputDeviceObserverWin::IsSlateModeEnabled(base::win::RegKey* key) {
  DWORD slate_enabled;
  if (key->ReadValueDW(kRegistryConvertibleSlateModeKey, &slate_enabled) !=
      ERROR_SUCCESS)
    return false;
  return slate_enabled == 1;
}

void InputDeviceObserverWin::AddObserver(InputDeviceEventObserver* observer) {
  observers_.AddObserver(observer);
}

void InputDeviceObserverWin::RemoveObserver(
    InputDeviceEventObserver* observer) {
  observers_.RemoveObserver(observer);
}

NOTIFY_OBSERVERS_METHOD(NotifyObserversKeyboardDeviceConfigurationChanged(),
                        OnKeyboardDeviceConfigurationChanged());

NOTIFY_OBSERVERS_METHOD(NotifyObserversTouchpadDeviceConfigurationChanged(),
                        OnTouchpadDeviceConfigurationChanged());

}  // namespace ui
