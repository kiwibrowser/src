// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "chromeos/components/proximity_auth/promotion_manager.h"

#include <stdlib.h>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/rand_util.h"
#include "base/task_runner.h"
#include "base/time/clock.h"
#include "base/time/time.h"
#include "chromeos/components/proximity_auth/logging/logging.h"
#include "chromeos/components/proximity_auth/notification_controller.h"
#include "chromeos/components/proximity_auth/proximity_auth_pref_manager.h"
#include "components/cryptauth/cryptauth_client.h"
#include "components/cryptauth/local_device_data_provider.h"

namespace proximity_auth {
namespace {

// The minimun between two consecutive promotion checks.
const int64_t kFreshnessPeriodMs = 24 * 60 * 60 * 1000;

// The number of seconds to wait for phones to contact CryptAuth
// before calling the FindEligigleUnlockDevices API.
const int64_t kWaitForPhoneOnlineDelaySec = 10;

// The probably for each operation to actually check with CryptAuth.
const double kCheckEligibilityProbability = 0.2;

// The maximum number of times the promotion will be shown to the user.
const int kMaxPromotionShownCount = 3;

}  // namespace

PromotionManager::PromotionManager(
    cryptauth::LocalDeviceDataProvider* provider,
    NotificationController* notification_controller,
    ProximityAuthPrefManager* pref_manager,
    std::unique_ptr<cryptauth::CryptAuthClientFactory> client_factory,
    base::Clock* clock,
    scoped_refptr<base::TaskRunner> task_runner)
    : check_eligibility_probability_(kCheckEligibilityProbability),
      local_device_data_provider_(provider),
      notification_controller_(notification_controller),
      pref_manager_(pref_manager),
      client_factory_(std::move(client_factory)),
      clock_(clock),
      task_runner_(task_runner),
      weak_ptr_factory_(this) {}

PromotionManager::~PromotionManager() {
  if (ScreenlockBridge::Get()) {
    ScreenlockBridge::Get()->RemoveObserver(this);
  }
}

void PromotionManager::Start() {
  if (pref_manager_->GetPromotionShownCount() >= kMaxPromotionShownCount) {
    return;
  }
  ScreenlockBridge::Get()->AddObserver(this);
}

void PromotionManager::OnFocusedUserChanged(const AccountId& account_id) {}

void PromotionManager::OnScreenDidLock(
    ScreenlockBridge::LockHandler::ScreenType screen_type) {}

void PromotionManager::OnScreenDidUnlock(
    ScreenlockBridge::LockHandler::ScreenType screen_type) {
  if (!HasFreshnessPeriodElapsed()) {
    return;
  }
  PA_LOG(INFO) << "Freshness period elapsed. Starting the flow.";
  if (!RollForPromotionCheck()) {
    return;
  }
  client_ = client_factory_->CreateInstance();

  std::string public_key;
  local_device_data_provider_->GetLocalDeviceData(&public_key, nullptr);
  cryptauth::FindEligibleForPromotionRequest request;
  request.set_promoter_public_key(public_key);
  client_->FindEligibleForPromotion(
      request,
      base::Bind(&PromotionManager::OnFindEligibleForPromotionSuccess,
                 weak_ptr_factory_.GetWeakPtr()),
      base::Bind(&PromotionManager::OnFindEligibleForPromotionFailure,
                 weak_ptr_factory_.GetWeakPtr()));
}

bool PromotionManager::HasFreshnessPeriodElapsed() {
  int64_t now_ms = clock_->Now().ToJavaTime();
  int64_t elapsed_time_since_last_check =
      now_ms - pref_manager_->GetLastPromotionCheckTimestampMs();
  if (elapsed_time_since_last_check < kFreshnessPeriodMs) {
    return false;
  }
  pref_manager_->SetLastPromotionCheckTimestampMs(now_ms);
  return true;
}

bool PromotionManager::RollForPromotionCheck() {
  bool success = base::RandDouble() < check_eligibility_probability_;
  if (!success) {
    PA_LOG(INFO) << "Roll uncessful. Stopping the flow.";
  }
  return success;
}

void PromotionManager::OnFindEligibleForPromotionFailure(
    const std::string& error) {
  client_.reset();
  PA_LOG(WARNING) << "FindEligibleForPromotion failed: " << error;
}

void PromotionManager::OnFindEligibleForPromotionSuccess(
    const cryptauth::FindEligibleForPromotionResponse& response) {
  client_.reset();
  if (!response.may_show_promo()) {
    PA_LOG(INFO) << "Local device not eligible for promo.";
    return;
  }
  task_runner_->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&PromotionManager::FindEligibleUnlockDevices,
                     weak_ptr_factory_.GetWeakPtr()),
      base::TimeDelta::FromSeconds(kWaitForPhoneOnlineDelaySec));
}

void PromotionManager::FindEligibleUnlockDevices() {
  client_ = client_factory_->CreateInstance();
  cryptauth::FindEligibleUnlockDevicesRequest request;
  request.set_max_last_update_time_delta_millis(kWaitForPhoneOnlineDelaySec *
                                                1000);
  client_->FindEligibleUnlockDevices(
      request,
      base::Bind(&PromotionManager::OnFindEligibleUnlockDevicesSuccess,
                 weak_ptr_factory_.GetWeakPtr()),
      base::Bind(&PromotionManager::OnFindEligibleUnlockDevicesFailure,
                 weak_ptr_factory_.GetWeakPtr()));
}

void PromotionManager::OnFindEligibleUnlockDevicesFailure(
    const std::string& error) {
  client_.reset();
  PA_LOG(WARNING) << "FindEligibleUnlockDevices failed: " << error;
}

void PromotionManager::OnFindEligibleUnlockDevicesSuccess(
    const cryptauth::FindEligibleUnlockDevicesResponse& response) {
  client_.reset();
  if (response.eligible_devices_size() == 0) {
    PA_LOG(INFO) << "No eligible unlock devices found.";
    return;
  }
  ShowPromotion();
}

void PromotionManager::ShowPromotion() {
  int previous_count = pref_manager_->GetPromotionShownCount();
  PA_LOG(INFO) << "Showing promotion for the user, previous count: "
               << previous_count;
  notification_controller_->ShowPromotionNotification();
  pref_manager_->SetPromotionShownCount(previous_count + 1);

  if (previous_count + 1 >= kMaxPromotionShownCount) {
    PA_LOG(INFO) << "Stop showing promotions";
    ScreenlockBridge::Get()->RemoveObserver(this);
  }
}

}  // namespace proximity_auth
