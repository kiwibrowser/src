// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/policy/arc_android_management_checker.h"

#include <algorithm>
#include <utility>

#include "base/bind.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/browser_process_platform_part.h"
#include "chrome/browser/chromeos/arc/policy/arc_policy_util.h"
#include "chrome/browser/chromeos/policy/browser_policy_connector_chromeos.h"
#include "chrome/browser/profiles/profile.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/cloud/device_management_service.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"

namespace arc {

namespace {

constexpr base::TimeDelta kRetryDelayMin = base::TimeDelta::FromSeconds(10);
constexpr base::TimeDelta kRetryDelayMax = base::TimeDelta::FromHours(1);

policy::DeviceManagementService* GetDeviceManagementService() {
  policy::BrowserPolicyConnectorChromeOS* const connector =
      g_browser_process->platform_part()->browser_policy_connector_chromeos();
  return connector->device_management_service();
}

}  // namespace

ArcAndroidManagementChecker::ArcAndroidManagementChecker(
    Profile* profile,
    ProfileOAuth2TokenService* token_service,
    const std::string& account_id,
    bool retry_on_error)
    : profile_(profile),
      token_service_(token_service),
      account_id_(account_id),
      retry_on_error_(retry_on_error),
      retry_delay_(kRetryDelayMin),
      android_management_client_(GetDeviceManagementService(),
                                 g_browser_process->system_request_context(),
                                 account_id,
                                 token_service),
      weak_ptr_factory_(this) {}

ArcAndroidManagementChecker::~ArcAndroidManagementChecker() {
  token_service_->RemoveObserver(this);
}

// static
void ArcAndroidManagementChecker::StartClient() {
  GetDeviceManagementService()->ScheduleInitialization(0);
}

void ArcAndroidManagementChecker::StartCheck(const CheckCallback& callback) {
  DCHECK(callback_.is_null());

  // Do not send requests for Chrome OS managed users, nor for well-known
  // consumer domains.
  if (policy_util::IsAccountManaged(profile_) ||
      policy::BrowserPolicyConnector::IsNonEnterpriseUser(
          profile_->GetProfileUserName())) {
    callback.Run(policy::AndroidManagementClient::Result::UNMANAGED);
    return;
  }

  callback_ = callback;
  EnsureRefreshTokenLoaded();
}

void ArcAndroidManagementChecker::EnsureRefreshTokenLoaded() {
  if (token_service_->RefreshTokenIsAvailable(account_id_)) {
    // If the refresh token is already available, just start the management
    // check immediately.
    StartCheckInternal();
    return;
  }

  // Set the observer to the token service so the callback will be called
  // when the token is loaded.
  token_service_->AddObserver(this);
}

void ArcAndroidManagementChecker::OnRefreshTokenAvailable(
    const std::string& account_id) {
  if (account_id != account_id_)
    return;
  OnRefreshTokensLoaded();
}

void ArcAndroidManagementChecker::OnRefreshTokensLoaded() {
  token_service_->RemoveObserver(this);
  StartCheckInternal();
}

void ArcAndroidManagementChecker::StartCheckInternal() {
  DCHECK(!callback_.is_null());

  if (!token_service_->RefreshTokenIsAvailable(account_id_)) {
    VLOG(2) << "No refresh token is available for android management check.";
    std::move(callback_).Run(policy::AndroidManagementClient::Result::ERROR);
    return;
  }

  VLOG(2) << "Start android management check.";
  android_management_client_.StartCheckAndroidManagement(
      base::Bind(&ArcAndroidManagementChecker::OnAndroidManagementChecked,
                 weak_ptr_factory_.GetWeakPtr()));
}

void ArcAndroidManagementChecker::OnAndroidManagementChecked(
    policy::AndroidManagementClient::Result result) {
  DCHECK(!callback_.is_null());
  VLOG(2) << "Android management check done " << result << ".";
  if (retry_on_error_ &&
      result == policy::AndroidManagementClient::Result::ERROR) {
    ScheduleRetry();
    return;
  }

  std::move(callback_).Run(result);
}

void ArcAndroidManagementChecker::ScheduleRetry() {
  DCHECK(retry_on_error_);
  DCHECK(!callback_.is_null());
  VLOG(2) << "Schedule next android management check in " << retry_delay_;

  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&ArcAndroidManagementChecker::StartCheckInternal,
                     weak_ptr_factory_.GetWeakPtr()),
      retry_delay_);
  retry_delay_ = std::min(retry_delay_ * 2, kRetryDelayMax);
}

}  // namespace arc
