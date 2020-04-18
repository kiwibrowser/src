// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/background_sync/background_sync_controller_impl.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "components/rappor/public/rappor_utils.h"
#include "components/rappor/rappor_service_impl.h"
#include "components/variations/variations_associated_data.h"
#include "content/public/browser/background_sync_parameters.h"

#if defined(OS_ANDROID)
#include "chrome/browser/android/background_sync_launcher_android.h"
#endif

// static
const char BackgroundSyncControllerImpl::kFieldTrialName[] = "BackgroundSync";
const char BackgroundSyncControllerImpl::kDisabledParameterName[] = "disabled";
const char BackgroundSyncControllerImpl::kMaxAttemptsParameterName[] =
    "max_sync_attempts";
const char BackgroundSyncControllerImpl::kInitialRetryParameterName[] =
    "initial_retry_delay_sec";
const char BackgroundSyncControllerImpl::kRetryDelayFactorParameterName[] =
    "retry_delay_factor";
const char BackgroundSyncControllerImpl::kMinSyncRecoveryTimeName[] =
    "min_recovery_time_sec";
const char BackgroundSyncControllerImpl::kMaxSyncEventDurationName[] =
    "max_sync_event_duration_sec";

BackgroundSyncControllerImpl::BackgroundSyncControllerImpl(Profile* profile)
    : profile_(profile) {}

BackgroundSyncControllerImpl::~BackgroundSyncControllerImpl() = default;

void BackgroundSyncControllerImpl::GetParameterOverrides(
    content::BackgroundSyncParameters* parameters) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

#if defined(OS_ANDROID)
  if (BackgroundSyncLauncherAndroid::ShouldDisableBackgroundSync()) {
    parameters->disable = true;
  }
#endif

  std::map<std::string, std::string> field_params;
  if (!variations::GetVariationParams(kFieldTrialName, &field_params))
    return;

  if (base::LowerCaseEqualsASCII(field_params[kDisabledParameterName],
                                 "true")) {
    parameters->disable = true;
  }

  if (base::ContainsKey(field_params, kMaxAttemptsParameterName)) {
    int max_attempts;
    if (base::StringToInt(field_params[kMaxAttemptsParameterName],
                          &max_attempts)) {
      parameters->max_sync_attempts = max_attempts;
    }
  }

  if (base::ContainsKey(field_params, kInitialRetryParameterName)) {
    int initial_retry_delay_sec;
    if (base::StringToInt(field_params[kInitialRetryParameterName],
                          &initial_retry_delay_sec)) {
      parameters->initial_retry_delay =
          base::TimeDelta::FromSeconds(initial_retry_delay_sec);
    }
  }

  if (base::ContainsKey(field_params, kRetryDelayFactorParameterName)) {
    int retry_delay_factor;
    if (base::StringToInt(field_params[kRetryDelayFactorParameterName],
                          &retry_delay_factor)) {
      parameters->retry_delay_factor = retry_delay_factor;
    }
  }

  if (base::ContainsKey(field_params, kMinSyncRecoveryTimeName)) {
    int min_sync_recovery_time_sec;
    if (base::StringToInt(field_params[kMinSyncRecoveryTimeName],
                          &min_sync_recovery_time_sec)) {
      parameters->min_sync_recovery_time =
          base::TimeDelta::FromSeconds(min_sync_recovery_time_sec);
    }
  }

  if (base::ContainsKey(field_params, kMaxSyncEventDurationName)) {
    int max_sync_event_duration_sec;
    if (base::StringToInt(field_params[kMaxSyncEventDurationName],
                          &max_sync_event_duration_sec)) {
      parameters->max_sync_event_duration =
          base::TimeDelta::FromSeconds(max_sync_event_duration_sec);
    }
  }

  return;
}

void BackgroundSyncControllerImpl::NotifyBackgroundSyncRegistered(
    const GURL& origin) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK_EQ(origin, origin.GetOrigin());

  if (profile_->IsOffTheRecord())
    return;

  rappor::SampleDomainAndRegistryFromGURL(
      GetRapporServiceImpl(), "BackgroundSync.Register.Origin", origin);
}

void BackgroundSyncControllerImpl::RunInBackground(bool enabled,
                                                   int64_t min_ms) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (profile_->IsOffTheRecord())
    return;
#if defined(OS_ANDROID)
  BackgroundSyncLauncherAndroid::LaunchBrowserIfStopped(enabled, min_ms);
#else
// TODO(jkarlin): Use BackgroundModeManager to enter background mode. See
// https://crbug.com/484201.
#endif
}

rappor::RapporServiceImpl*
BackgroundSyncControllerImpl::GetRapporServiceImpl() {
  return g_browser_process->rappor_service();
}
