// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/metrics/metrics_state_manager.h"

#include <stddef.h>
#include <utility>

#include <memory>

#include "base/command_line.h"
#include "base/guid.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "components/metrics/cloned_install_detector.h"
#include "components/metrics/enabled_state_provider.h"
#include "components/metrics/machine_id_provider.h"
#include "components/metrics/metrics_log.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/metrics/metrics_provider.h"
#include "components/metrics/metrics_switches.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/variations/caching_permuted_entropy_provider.h"
#include "third_party/metrics_proto/chrome_user_metrics_extension.pb.h"
#include "third_party/metrics_proto/system_profile.pb.h"

namespace metrics {

namespace {

// The argument used to generate a non-identifying entropy source. We want no
// more than 13 bits of entropy, so use this max to return a number in the range
// [0, 7999] as the entropy source (12.97 bits of entropy).
const int kMaxLowEntropySize = 8000;

// Default prefs value for prefs::kMetricsLowEntropySource to indicate that
// the value has not yet been set.
const int kLowEntropySourceNotSet = -1;

// Generates a new non-identifying entropy source used to seed persistent
// activities.
int GenerateLowEntropySource() {
  return base::RandInt(0, kMaxLowEntropySize - 1);
}

// Records the given |low_entorpy_source_value| in a histogram.
void LogLowEntropyValue(int low_entropy_source_value) {
  base::UmaHistogramSparse("UMA.LowEntropySourceValue",
                           low_entropy_source_value);
}

int64_t ReadEnabledDate(PrefService* local_state) {
  return local_state->GetInt64(prefs::kMetricsReportingEnabledTimestamp);
}

int64_t ReadInstallDate(PrefService* local_state) {
  return local_state->GetInt64(prefs::kInstallDate);
}

// Round a timestamp measured in seconds since epoch to one with a granularity
// of an hour. This can be used before uploaded potentially sensitive
// timestamps.
int64_t RoundSecondsToHour(int64_t time_in_seconds) {
  return 3600 * (time_in_seconds / 3600);
}

// Records the cloned install histogram.
void LogClonedInstall() {
  // Equivalent to UMA_HISTOGRAM_BOOLEAN with the stability flag set.
  UMA_STABILITY_HISTOGRAM_ENUMERATION("UMA.IsClonedInstall", 1, 2);
}

class MetricsStateMetricsProvider : public MetricsProvider {
 public:
  MetricsStateMetricsProvider(PrefService* local_state,
                              bool metrics_ids_were_reset,
                              std::string previous_client_id)
      : local_state_(local_state),
        metrics_ids_were_reset_(metrics_ids_were_reset),
        previous_client_id_(std::move(previous_client_id)) {}

  // MetricsProvider:
  void ProvideSystemProfileMetrics(
      SystemProfileProto* system_profile) override {
    system_profile->set_uma_enabled_date(
        RoundSecondsToHour(ReadEnabledDate(local_state_)));
    system_profile->set_install_date(
        RoundSecondsToHour(ReadInstallDate(local_state_)));
  }

  void ProvidePreviousSessionData(
      ChromeUserMetricsExtension* uma_proto) override {
    if (metrics_ids_were_reset_) {
      LogClonedInstall();
      if (!previous_client_id_.empty()) {
        // If we know the previous client id, overwrite the client id for the
        // previous session log so the log contains the client id at the time
        // of the previous session. This allows better attribution of crashes
        // to earlier behavior. If the previous client id is unknown, leave
        // the current client id.
        uma_proto->set_client_id(MetricsLog::Hash(previous_client_id_));
      }
    }
  }

  void ProvideCurrentSessionData(
      ChromeUserMetricsExtension* uma_proto) override {
    if (local_state_->GetBoolean(prefs::kMetricsResetIds))
      LogClonedInstall();
  }

 private:
  PrefService* const local_state_;
  const bool metrics_ids_were_reset_;
  // |previous_client_id_| is set only (if known) when |metrics_ids_were_reset_|
  const std::string previous_client_id_;

  DISALLOW_COPY_AND_ASSIGN(MetricsStateMetricsProvider);
};

}  // namespace

// static
bool MetricsStateManager::instance_exists_ = false;

MetricsStateManager::MetricsStateManager(
    PrefService* local_state,
    EnabledStateProvider* enabled_state_provider,
    const base::string16& backup_registry_key,
    const StoreClientInfoCallback& store_client_info,
    const LoadClientInfoCallback& retrieve_client_info)
    : local_state_(local_state),
      enabled_state_provider_(enabled_state_provider),
      store_client_info_(store_client_info),
      load_client_info_(retrieve_client_info),
      clean_exit_beacon_(backup_registry_key, local_state),
      low_entropy_source_(kLowEntropySourceNotSet),
      entropy_source_returned_(ENTROPY_SOURCE_NONE),
      metrics_ids_were_reset_(false) {
  ResetMetricsIDsIfNecessary();
  if (enabled_state_provider_->IsConsentGiven())
    ForceClientIdCreation();

  // Set the install date if this is our first run.
  int64_t install_date = local_state_->GetInt64(prefs::kInstallDate);
  if (install_date == 0)
    local_state_->SetInt64(prefs::kInstallDate, base::Time::Now().ToTimeT());

  DCHECK(!instance_exists_);
  instance_exists_ = true;
}

MetricsStateManager::~MetricsStateManager() {
  DCHECK(instance_exists_);
  instance_exists_ = false;
}

std::unique_ptr<MetricsProvider> MetricsStateManager::GetProvider() {
  return std::make_unique<MetricsStateMetricsProvider>(
      local_state_, metrics_ids_were_reset_, previous_client_id_);
}

bool MetricsStateManager::IsMetricsReportingEnabled() {
  return enabled_state_provider_->IsReportingEnabled();
}

int64_t MetricsStateManager::GetInstallDate() const {
  return ReadInstallDate(local_state_);
}

void MetricsStateManager::ForceClientIdCreation() {
  {
    std::string client_id_from_prefs =
        local_state_->GetString(prefs::kMetricsClientID);
    // If client id in prefs matches the cached copy, return early.
    if (!client_id_from_prefs.empty() && client_id_from_prefs == client_id_)
      return;
    client_id_.swap(client_id_from_prefs);
  }

  if (!client_id_.empty())
    return;

  const std::unique_ptr<ClientInfo> client_info_backup = LoadClientInfo();
  if (client_info_backup) {
    client_id_ = client_info_backup->client_id;

    const base::Time now = base::Time::Now();

    // Save the recovered client id and also try to reinstantiate the backup
    // values for the dates corresponding with that client id in order to avoid
    // weird scenarios where we could report an old client id with a recent
    // install date.
    local_state_->SetString(prefs::kMetricsClientID, client_id_);
    local_state_->SetInt64(prefs::kInstallDate,
                           client_info_backup->installation_date != 0
                               ? client_info_backup->installation_date
                               : now.ToTimeT());
    local_state_->SetInt64(prefs::kMetricsReportingEnabledTimestamp,
                           client_info_backup->reporting_enabled_date != 0
                               ? client_info_backup->reporting_enabled_date
                               : now.ToTimeT());

    base::TimeDelta recovered_installation_age;
    if (client_info_backup->installation_date != 0) {
      recovered_installation_age =
          now - base::Time::FromTimeT(client_info_backup->installation_date);
    }
    UMA_HISTOGRAM_COUNTS_10000("UMA.ClientIdBackupRecoveredWithAge",
                               recovered_installation_age.InHours());

    // Flush the backup back to persistent storage in case we re-generated
    // missing data above.
    BackUpCurrentClientInfo();
    return;
  }

  // Failing attempts at getting an existing client ID, generate a new one.
  client_id_ = base::GenerateGUID();
  local_state_->SetString(prefs::kMetricsClientID, client_id_);

  // Record the timestamp of when the user opted in to UMA.
  local_state_->SetInt64(prefs::kMetricsReportingEnabledTimestamp,
                         base::Time::Now().ToTimeT());

  BackUpCurrentClientInfo();
}

void MetricsStateManager::CheckForClonedInstall() {
  DCHECK(!cloned_install_detector_);

  if (!MachineIdProvider::HasId())
    return;

  cloned_install_detector_ = std::make_unique<ClonedInstallDetector>();
  cloned_install_detector_->CheckForClonedInstall(local_state_);
}

std::unique_ptr<const base::FieldTrial::EntropyProvider>
MetricsStateManager::CreateDefaultEntropyProvider() {
  if (enabled_state_provider_->IsConsentGiven()) {
    // For metrics reporting-enabled users, we combine the client ID and low
    // entropy source to get the final entropy source. Otherwise, only use the
    // low entropy source.
    // This has two useful properties:
    //  1) It makes the entropy source less identifiable for parties that do not
    //     know the low entropy source.
    //  2) It makes the final entropy source resettable.
    const int low_entropy_source_value = GetLowEntropySource();

    UpdateEntropySourceReturnedValue(ENTROPY_SOURCE_HIGH);
    const std::string high_entropy_source =
        client_id_ + base::IntToString(low_entropy_source_value);
    return std::unique_ptr<const base::FieldTrial::EntropyProvider>(
        new variations::SHA1EntropyProvider(high_entropy_source));
  }

  UpdateEntropySourceReturnedValue(ENTROPY_SOURCE_LOW);
  return CreateLowEntropyProvider();
}

std::unique_ptr<const base::FieldTrial::EntropyProvider>
MetricsStateManager::CreateLowEntropyProvider() {
  const int low_entropy_source_value = GetLowEntropySource();

#if defined(OS_ANDROID) || defined(OS_IOS)
  return std::unique_ptr<const base::FieldTrial::EntropyProvider>(
      new variations::CachingPermutedEntropyProvider(
          local_state_, low_entropy_source_value, kMaxLowEntropySize));
#else
  return std::unique_ptr<const base::FieldTrial::EntropyProvider>(
      new variations::PermutedEntropyProvider(low_entropy_source_value,
                                              kMaxLowEntropySize));
#endif
}

// static
std::unique_ptr<MetricsStateManager> MetricsStateManager::Create(
    PrefService* local_state,
    EnabledStateProvider* enabled_state_provider,
    const base::string16& backup_registry_key,
    const StoreClientInfoCallback& store_client_info,
    const LoadClientInfoCallback& retrieve_client_info) {
  std::unique_ptr<MetricsStateManager> result;
  // Note: |instance_exists_| is updated in the constructor and destructor.
  if (!instance_exists_) {
    result.reset(new MetricsStateManager(local_state, enabled_state_provider,
                                         backup_registry_key, store_client_info,
                                         retrieve_client_info));
  }
  return result;
}

// static
void MetricsStateManager::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kMetricsResetIds, false);
  registry->RegisterStringPref(prefs::kMetricsClientID, std::string());
  registry->RegisterInt64Pref(prefs::kMetricsReportingEnabledTimestamp, 0);
  registry->RegisterIntegerPref(prefs::kMetricsLowEntropySource,
                                kLowEntropySourceNotSet);
  registry->RegisterInt64Pref(prefs::kInstallDate, 0);

  ClonedInstallDetector::RegisterPrefs(registry);
  variations::CachingPermutedEntropyProvider::RegisterPrefs(registry);
}

void MetricsStateManager::BackUpCurrentClientInfo() {
  ClientInfo client_info;
  client_info.client_id = client_id_;
  client_info.installation_date = ReadInstallDate(local_state_);
  client_info.reporting_enabled_date = ReadEnabledDate(local_state_);
  store_client_info_.Run(client_info);
}

std::unique_ptr<ClientInfo> MetricsStateManager::LoadClientInfo() {
  std::unique_ptr<ClientInfo> client_info = load_client_info_.Run();

  // The GUID retrieved should be valid unless retrieval failed.
  // If not, return nullptr. This will result in a new GUID being generated by
  // the calling function ForceClientIdCreation().
  if (client_info && !base::IsValidGUID(client_info->client_id))
    return nullptr;

  return client_info;
}

int MetricsStateManager::GetLowEntropySource() {
  UpdateLowEntropySource();
  return low_entropy_source_;
}

void MetricsStateManager::UpdateLowEntropySource() {
  // Note that the default value for the low entropy source and the default pref
  // value are both kLowEntropySourceNotSet, which is used to identify if the
  // value has been set or not.
  if (low_entropy_source_ != kLowEntropySourceNotSet)
    return;

  const base::CommandLine* command_line(base::CommandLine::ForCurrentProcess());
  // Only try to load the value from prefs if the user did not request a
  // reset.
  // Otherwise, skip to generating a new value.
  if (!command_line->HasSwitch(switches::kResetVariationState)) {
    int value = local_state_->GetInteger(prefs::kMetricsLowEntropySource);
    // If the value is outside the [0, kMaxLowEntropySize) range, re-generate
    // it below.
    if (value >= 0 && value < kMaxLowEntropySize) {
      low_entropy_source_ = value;
      LogLowEntropyValue(low_entropy_source_);
      return;
    }
  }

  low_entropy_source_ = GenerateLowEntropySource();
  LogLowEntropyValue(low_entropy_source_);
  local_state_->SetInteger(prefs::kMetricsLowEntropySource,
                           low_entropy_source_);
  variations::CachingPermutedEntropyProvider::ClearCache(local_state_);
}

void MetricsStateManager::UpdateEntropySourceReturnedValue(
    EntropySourceType type) {
  if (entropy_source_returned_ != ENTROPY_SOURCE_NONE)
    return;

  entropy_source_returned_ = type;
  UMA_HISTOGRAM_ENUMERATION("UMA.EntropySourceType", type,
                            ENTROPY_SOURCE_ENUM_SIZE);
}

void MetricsStateManager::ResetMetricsIDsIfNecessary() {
  if (!local_state_->GetBoolean(prefs::kMetricsResetIds))
    return;
  metrics_ids_were_reset_ = true;
  previous_client_id_ = local_state_->GetString(prefs::kMetricsClientID);

  UMA_HISTOGRAM_BOOLEAN("UMA.MetricsIDsReset", true);

  DCHECK(client_id_.empty());
  DCHECK_EQ(kLowEntropySourceNotSet, low_entropy_source_);

  local_state_->ClearPref(prefs::kMetricsClientID);
  local_state_->ClearPref(prefs::kMetricsLowEntropySource);
  local_state_->ClearPref(prefs::kMetricsResetIds);

  // Also clear the backed up client info.
  store_client_info_.Run(ClientInfo());
}

}  // namespace metrics
