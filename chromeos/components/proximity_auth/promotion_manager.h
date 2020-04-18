// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_PROXIMITY_AUTH_PROMOTION_MANAGER_H_
#define CHROMEOS_COMPONENTS_PROXIMITY_AUTH_PROMOTION_MANAGER_H_

#include <memory>

#include "chromeos/components/proximity_auth/screenlock_bridge.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"

namespace base {
class Clock;
class TaskRunner;
}  // namespace base

namespace cryptauth {
class CryptAuthClient;
class CryptAuthClientFactory;
class LocalDeviceDataProvider;
}  // namespace cryptauth

namespace proximity_auth {

class NotificationController;
class ProximityAuthPrefManager;

// The PromotionManager periodically checks with CryptAuth to determine if the
// user is eligible to be shown a promotion notification (eg. the user must have
// a phone). Once Smart Lock is set up, this promotion will no longer be shown.
class PromotionManager : public ScreenlockBridge::Observer {
 public:
  PromotionManager(
      cryptauth::LocalDeviceDataProvider* local_device_data_provider,
      NotificationController* notification_controller,
      ProximityAuthPrefManager* pref_manager,
      std::unique_ptr<cryptauth::CryptAuthClientFactory> client_factory,
      base::Clock* clock,
      scoped_refptr<base::TaskRunner> task_runner);
  ~PromotionManager() override;

  void Start();

  void set_check_eligibility_probability(double probability) {
    check_eligibility_probability_ = probability;
  }

 private:
  // ScreenlockBridge::Observer
  void OnFocusedUserChanged(const AccountId& account_id) override;
  void OnScreenDidLock(
      ScreenlockBridge::LockHandler::ScreenType screen_type) override;
  void OnScreenDidUnlock(
      ScreenlockBridge::LockHandler::ScreenType screen_type) override;

  // True if the freshness period since the last check has elapsed. Updates the
  // last check timestamp if necessary.
  bool HasFreshnessPeriodElapsed();

  // True if it should continue with the promotion check.
  bool RollForPromotionCheck();

  // FindEligibleForPromotion callbacks.
  void OnFindEligibleForPromotionSuccess(
      const cryptauth::FindEligibleForPromotionResponse& response);
  void OnFindEligibleForPromotionFailure(const std::string& error);

  // FindEligibleUnlockDevices callbacks.
  void OnFindEligibleUnlockDevicesSuccess(
      const cryptauth::FindEligibleUnlockDevicesResponse& response);
  void OnFindEligibleUnlockDevicesFailure(const std::string& error);

  // Send the FindEligigleUnlockDevices query to CryptAuth.
  void FindEligibleUnlockDevices();

  // Show the EasyUnlock promotion.
  void ShowPromotion();

  // The probability for each operation to actually check with CryptAuth.
  double check_eligibility_probability_;

  // Provides the local device information, e.g. public key.
  cryptauth::LocalDeviceDataProvider* local_device_data_provider_;

  // Displays the notification to the user.
  NotificationController* notification_controller_;

  // Used to store the last a promotion check was done.
  ProximityAuthPrefManager* pref_manager_;

  // The factory for |client_| instances.
  std::unique_ptr<cryptauth::CryptAuthClientFactory> client_factory_;

  // The client used to make request to CryptAuth.
  std::unique_ptr<cryptauth::CryptAuthClient> client_;

  // Used to determine the time.
  base::Clock* clock_;

  // Used to schedule delayed tasks.
  scoped_refptr<base::TaskRunner> task_runner_;

  base::WeakPtrFactory<PromotionManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(PromotionManager);
};

}  // namespace proximity_auth

#endif  // CHROMEOS_COMPONENTS_PROXIMITY_AUTH_PROMOTION_MANAGER_H_
