// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_TRAY_SYSTEM_TRAY_NOTIFIER_H_
#define ASH_SYSTEM_TRAY_SYSTEM_TRAY_NOTIFIER_H_

#include <string>

#include "ash/ash_export.h"
#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "base/strings/string16.h"
#include "base/time/time.h"

namespace ash {

class BluetoothObserver;
class IMEObserver;
class NetworkObserver;
class ScreenCaptureObserver;
class ScreenShareObserver;
class SystemTrayFocusObserver;
class VirtualKeyboardObserver;

namespace mojom {
enum class UpdateSeverity;
}

// Observer and notification manager for the ash system tray.
class ASH_EXPORT SystemTrayNotifier {
 public:
  SystemTrayNotifier();
  ~SystemTrayNotifier();

  // Bluetooth.
  void AddBluetoothObserver(BluetoothObserver* observer);
  void RemoveBluetoothObserver(BluetoothObserver* observer);
  void NotifyRefreshBluetooth();
  void NotifyBluetoothDiscoveringChanged();

  // Input methods.
  void AddIMEObserver(IMEObserver* observer);
  void RemoveIMEObserver(IMEObserver* observer);
  void NotifyRefreshIME();
  void NotifyRefreshIMEMenu(bool is_active);

  // Network.
  void AddNetworkObserver(NetworkObserver* observer);
  void RemoveNetworkObserver(NetworkObserver* observer);
  void NotifyRequestToggleWifi();

  // Screen capture.
  void AddScreenCaptureObserver(ScreenCaptureObserver* observer);
  void RemoveScreenCaptureObserver(ScreenCaptureObserver* observer);
  void NotifyScreenCaptureStart(const base::Closure& stop_callback,
                                const base::string16& sharing_app_name);
  void NotifyScreenCaptureStop();

  // Screen share.
  void AddScreenShareObserver(ScreenShareObserver* observer);
  void RemoveScreenShareObserver(ScreenShareObserver* observer);
  void NotifyScreenShareStart(const base::Closure& stop_callback,
                              const base::string16& helper_name);
  void NotifyScreenShareStop();

  // System tray focus.
  void AddSystemTrayFocusObserver(SystemTrayFocusObserver* observer);
  void RemoveSystemTrayFocusObserver(SystemTrayFocusObserver* observer);
  void NotifyFocusOut(bool reverse);

  // Virtual keyboard.
  void AddVirtualKeyboardObserver(VirtualKeyboardObserver* observer);
  void RemoveVirtualKeyboardObserver(VirtualKeyboardObserver* observer);
  void NotifyVirtualKeyboardSuppressionChanged(bool suppressed);

 private:
  base::ObserverList<BluetoothObserver> bluetooth_observers_;
  base::ObserverList<IMEObserver> ime_observers_;
  base::ObserverList<NetworkObserver> network_observers_;
  base::ObserverList<ScreenCaptureObserver> screen_capture_observers_;
  base::ObserverList<ScreenShareObserver> screen_share_observers_;
  base::ObserverList<SystemTrayFocusObserver> system_tray_focus_observers_;
  base::ObserverList<VirtualKeyboardObserver> virtual_keyboard_observers_;

  DISALLOW_COPY_AND_ASSIGN(SystemTrayNotifier);
};

}  // namespace ash

#endif  // ASH_SYSTEM_TRAY_SYSTEM_TRAY_NOTIFIER_H_
