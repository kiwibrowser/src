// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_PROXIMITY_AUTH_PROXIMITY_AUTH_SYSTEM_H_
#define CHROMEOS_COMPONENTS_PROXIMITY_AUTH_PROXIMITY_AUTH_SYSTEM_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/time/clock.h"
#include "chromeos/components/proximity_auth/remote_device_life_cycle.h"
#include "chromeos/components/proximity_auth/screenlock_bridge.h"
#include "components/account_id/account_id.h"
#include "components/cryptauth/remote_device_ref.h"

namespace proximity_auth {

class ProximityAuthClient;
class ProximityAuthPrefManager;
class RemoteDeviceLifeCycle;
class UnlockManager;

// This is the main entry point to start Proximity Auth, the underlying system
// for the Smart Lock feature. Given a list of remote devices (i.e. a
// phone) for each registered user, the system will handle the connection,
// authentication, and messenging protocol when the screen is locked and the
// registered user is focused.
class ProximityAuthSystem : public RemoteDeviceLifeCycle::Observer,
                            public ScreenlockBridge::Observer {
 public:
  enum ScreenlockType { SESSION_LOCK, SIGN_IN };

  ProximityAuthSystem(ScreenlockType screenlock_type,
                      ProximityAuthClient* proximity_auth_client);
  ~ProximityAuthSystem() override;

  // Starts the system to connect and authenticate when a registered user is
  // focused on the lock/sign-in screen.
  void Start();

  // Stops the system.
  void Stop();

  // Registers a list of |remote_devices| for |account_id| that can be used for
  // sign-in/unlock. If devices were previously registered for the user, then
  // they will be replaced.
  void SetRemoteDevicesForUser(
      const AccountId& account_id,
      const cryptauth::RemoteDeviceRefList& remote_devices);

  // Returns the RemoteDevices registered for |account_id|. Returns an empty
  // list
  // if no devices are registered for |account_id|.
  cryptauth::RemoteDeviceRefList GetRemoteDevicesForUser(
      const AccountId& account_id) const;

  // Called when the user clicks the user pod and attempts to unlock/sign-in.
  void OnAuthAttempted(const AccountId& account_id);

  // Called when the system suspends.
  void OnSuspend();

  // Called when the system wakes up from a suspended state.
  void OnSuspendDone();

 protected:
  // Constructor which allows passing in a custom |unlock_manager_|.
  // Exposed for testing.
  ProximityAuthSystem(ScreenlockType screenlock_type,
                      ProximityAuthClient* proximity_auth_client,
                      std::unique_ptr<UnlockManager> unlock_manager,
                      base::Clock* clock,
                      ProximityAuthPrefManager* pref_manager);

  // Creates the RemoteDeviceLifeCycle for |remote_device|.
  // Exposed for testing.
  virtual std::unique_ptr<RemoteDeviceLifeCycle> CreateRemoteDeviceLifeCycle(
      cryptauth::RemoteDeviceRef remote_device);

  // RemoteDeviceLifeCycle::Observer:
  void OnLifeCycleStateChanged(RemoteDeviceLifeCycle::State old_state,
                               RemoteDeviceLifeCycle::State new_state) override;

  // ScreenlockBridge::Observer:
  void OnScreenDidLock(
      ScreenlockBridge::LockHandler::ScreenType screen_type) override;
  void OnScreenDidUnlock(
      ScreenlockBridge::LockHandler::ScreenType screen_type) override;
  void OnFocusedUserChanged(const AccountId& account_id) override;

 private:
  // Resumes |remote_device_life_cycle_| after device wakes up and waits a
  // timeout.
  void ResumeAfterWakeUpTimeout();

  // Returns true if the user should be forced to use a password to authenticate
  // rather than EasyUnlock.
  bool ShouldForcePassword();

  // The type of the screenlock (i.e. login or unlock).
  ScreenlockType screenlock_type_;

  // Lists of remote devices, keyed by user account id.
  std::map<AccountId, cryptauth::RemoteDeviceRefList> remote_devices_map_;

  // Delegate for Chrome dependent functionality.
  ProximityAuthClient* proximity_auth_client_;

  // Responsible for the life cycle of connecting and authenticating to
  // the RemoteDevice of the currently focused user.
  std::unique_ptr<RemoteDeviceLifeCycle> remote_device_life_cycle_;

  // Used to get the current timestamp.
  base::Clock* clock_;

  // Fetches EasyUnlock preferences. Must outlive this instance.
  ProximityAuthPrefManager* pref_manager_;

  // Handles the interaction with the lock screen UI.
  std::unique_ptr<UnlockManager> unlock_manager_;

  // True if the system is suspended.
  bool suspended_;

  // True if the system is started_.
  bool started_;

  base::WeakPtrFactory<ProximityAuthSystem> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ProximityAuthSystem);
};

}  // namespace proximity_auth

#endif  // CHROMEOS_COMPONENTS_PROXIMITY_AUTH_PROXIMITY_AUTH_SYSTEM_H_
