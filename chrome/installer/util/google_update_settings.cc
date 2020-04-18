// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/util/google_update_settings.h"

#include <stdint.h>

#include <algorithm>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/numerics/safe_conversions.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/lazy_task_runner.h"
#include "base/threading/thread_restrictions.h"
#include "base/time/time.h"
#include "base/win/registry.h"
#include "base/win/win_util.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/install_static/install_modes.h"
#include "chrome/install_static/install_util.h"
#include "chrome/installer/util/app_registration_data.h"
#include "chrome/installer/util/browser_distribution.h"
#include "chrome/installer/util/channel_info.h"
#include "chrome/installer/util/install_util.h"
#include "chrome/installer/util/installation_state.h"
#include "chrome/installer/util/product.h"

using base::win::RegKey;
using installer::InstallationState;

const wchar_t GoogleUpdateSettings::kPoliciesKey[] =
    L"SOFTWARE\\Policies\\Google\\Update";
const wchar_t GoogleUpdateSettings::kUpdatePolicyValue[] = L"UpdateDefault";
const wchar_t GoogleUpdateSettings::kDownloadPreferencePolicyValue[] =
    L"DownloadPreference";
const wchar_t GoogleUpdateSettings::kUpdateOverrideValuePrefix[] = L"Update";
const wchar_t GoogleUpdateSettings::kCheckPeriodOverrideMinutes[] =
    L"AutoUpdateCheckPeriodMinutes";

// Don't allow update periods longer than six weeks.
const int GoogleUpdateSettings::kCheckPeriodOverrideMinutesMax =
    60 * 24 * 7 * 6;

const GoogleUpdateSettings::UpdatePolicy
GoogleUpdateSettings::kDefaultUpdatePolicy =
#if defined(GOOGLE_CHROME_BUILD)
    GoogleUpdateSettings::AUTOMATIC_UPDATES;
#else
    GoogleUpdateSettings::UPDATES_DISABLED;
#endif

namespace {

base::LazySequencedTaskRunner g_collect_stats_consent_task_runner =
    LAZY_SEQUENCED_TASK_RUNNER_INITIALIZER(
        base::TaskTraits(base::TaskPriority::USER_VISIBLE,
                         base::TaskShutdownBehavior::BLOCK_SHUTDOWN));

bool ReadGoogleUpdateStrKey(const wchar_t* const name, base::string16* value) {
  BrowserDistribution* dist = BrowserDistribution::GetDistribution();
  base::string16 reg_path = dist->GetStateKey();
  RegKey key(HKEY_CURRENT_USER, reg_path.c_str(), KEY_READ | KEY_WOW64_32KEY);
  if (key.ReadValue(name, value) != ERROR_SUCCESS) {
    RegKey hklm_key(
        HKEY_LOCAL_MACHINE, reg_path.c_str(), KEY_READ | KEY_WOW64_32KEY);
    return (hklm_key.ReadValue(name, value) == ERROR_SUCCESS);
  }
  return true;
}

// Writes |value| into a user-specific value in the key |name| under
// |app_reg_data|'s ClientStateMedium key in HKLM along with the aggregation
// method |aggregate|. This function is solely for use by system-level installs.
bool WriteGoogleUpdateAggregateNumKeyInternal(
    const AppRegistrationData& app_reg_data,
    const wchar_t* const name,
    uint32_t value,
    const wchar_t* const aggregate) {
  DCHECK(aggregate);
  DCHECK(GoogleUpdateSettings::IsSystemInstall());
  const REGSAM kAccess = KEY_SET_VALUE | KEY_WOW64_32KEY;

  // Machine installs require each OS user to write a unique key under a
  // named key in HKLM as well as an "aggregation" function that describes
  // how the values of multiple users are to be combined.
  base::string16 uniquename;
  if (!base::win::GetUserSidString(&uniquename)) {
    NOTREACHED();
    return false;
  }

  base::string16 reg_path(app_reg_data.GetStateMediumKey());
  reg_path.append(L"\\");
  reg_path.append(name);
  RegKey key(HKEY_LOCAL_MACHINE, reg_path.c_str(), kAccess);
  key.WriteValue(google_update::kRegAggregateMethod, aggregate);

  return (key.WriteValue(uniquename.c_str(), value) == ERROR_SUCCESS);
}

// Updates a registry key |name| to be |value| for the given |app_reg_data|.
bool WriteGoogleUpdateStrKeyInternal(const AppRegistrationData& app_reg_data,
                                     const wchar_t* const name,
                                     const base::string16& value) {
  const REGSAM kAccess = KEY_SET_VALUE | KEY_WOW64_32KEY;
  RegKey key(HKEY_CURRENT_USER, app_reg_data.GetStateKey().c_str(), kAccess);
  return (key.WriteValue(name, value.c_str()) == ERROR_SUCCESS);
}

// Writes the per-user stat |value_name|=|value| either in ClientStateMedium
// using summation as the aggregation function or in ClientState directly,
// depending on whether this is is a per-machine or a per-user install.
void WritePerUserStat(const AppRegistrationData& app_reg_data,
                      bool is_system_install,
                      const wchar_t* value_name,
                      uint32_t value) {
  if (is_system_install) {
    // Write |value| as a DWORD in a per-user value of subkey |value_name|.
    WriteGoogleUpdateAggregateNumKeyInternal(app_reg_data, value_name, value,
                                             L"sum()");
  } else {
    // Write |value| as a string in value |value_name|.
    WriteGoogleUpdateStrKeyInternal(app_reg_data, value_name,
                                    base::UintToString16(value));
  }
}

bool WriteGoogleUpdateStrKey(const wchar_t* const name,
                             const base::string16& value) {
  BrowserDistribution* dist = BrowserDistribution::GetDistribution();
  return WriteGoogleUpdateStrKeyInternal(
      dist->GetAppRegistrationData(), name, value);
}

bool ClearGoogleUpdateStrKey(const wchar_t* const name) {
  BrowserDistribution* dist = BrowserDistribution::GetDistribution();
  base::string16 reg_path = dist->GetStateKey();
  RegKey key(HKEY_CURRENT_USER,
             reg_path.c_str(),
             KEY_READ | KEY_WRITE | KEY_WOW64_32KEY);
  base::string16 value;
  if (key.ReadValue(name, &value) != ERROR_SUCCESS)
    return false;
  return (key.WriteValue(name, L"") == ERROR_SUCCESS);
}

bool RemoveGoogleUpdateStrKey(const wchar_t* const name) {
  BrowserDistribution* dist = BrowserDistribution::GetDistribution();
  base::string16 reg_path = dist->GetStateKey();
  RegKey key(HKEY_CURRENT_USER,
             reg_path.c_str(),
             KEY_READ | KEY_WRITE | KEY_WOW64_32KEY);
  if (!key.HasValue(name))
    return true;
  return (key.DeleteValue(name) == ERROR_SUCCESS);
}

// Initializes |channel_info| based on |system_install|. Returns false on
// failure.
bool InitChannelInfo(bool system_install,
                     installer::ChannelInfo* channel_info) {
  HKEY root_key = system_install ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
  BrowserDistribution* dist = BrowserDistribution::GetDistribution();
  RegKey key(root_key, dist->GetStateKey().c_str(),
             KEY_QUERY_VALUE | KEY_WOW64_32KEY);
  return channel_info->Initialize(key);
}

#if defined(GOOGLE_CHROME_BUILD)
// Populates |update_policy| with the UpdatePolicy enum value corresponding to a
// DWORD read from the registry and returns true if |value| is within range.
// If |value| is out of range, returns false without modifying |update_policy|.
bool GetUpdatePolicyFromDword(
    const DWORD value,
    GoogleUpdateSettings::UpdatePolicy* update_policy) {
  switch (value) {
    case GoogleUpdateSettings::UPDATES_DISABLED:
    case GoogleUpdateSettings::AUTOMATIC_UPDATES:
    case GoogleUpdateSettings::MANUAL_UPDATES_ONLY:
    case GoogleUpdateSettings::AUTO_UPDATES_ONLY:
      *update_policy = static_cast<GoogleUpdateSettings::UpdatePolicy>(value);
      return true;
    default:
      LOG(WARNING) << "Unexpected update policy override value: " << value;
  }
  return false;
}
#endif  // defined(GOOGLE_CHROME_BUILD)

}  // namespace

// TODO(grt): Remove this now that it has no added value.
bool GoogleUpdateSettings::IsSystemInstall() {
  return !InstallUtil::IsPerUserInstall();
}

base::SequencedTaskRunner*
GoogleUpdateSettings::CollectStatsConsentTaskRunner() {
  // TODO(fdoray): Use LazySequencedTaskRunner::GetRaw() here instead of
  // .Get().get() when it's added to the API, http://crbug.com/730170.
  return g_collect_stats_consent_task_runner.Get().get();
}

bool GoogleUpdateSettings::GetCollectStatsConsent() {
  return GetCollectStatsConsentAtLevel(IsSystemInstall());
}

bool GoogleUpdateSettings::SetCollectStatsConsent(bool consented) {
  return SetCollectStatsConsentAtLevel(IsSystemInstall(), consented);
}

bool GoogleUpdateSettings::GetCollectStatsConsentAtLevel(bool system_install) {
  return GetCollectStatsConsentForApp(system_install,
                                      BrowserDistribution::GetDistribution()
                                          ->GetAppRegistrationData()) ==
         google_update::TRISTATE_TRUE;
}

google_update::Tristate GoogleUpdateSettings::GetCollectStatsConsentForApp(
    bool system_install,
    const AppRegistrationData& reg_data) {
  RegKey key;
  DWORD value = google_update::TRISTATE_NONE;
  bool have_value = false;
  const REGSAM kAccess = KEY_QUERY_VALUE | KEY_WOW64_32KEY;

  // For system-level installs, try ClientStateMedium first.
  have_value =
      system_install &&
      key.Open(HKEY_LOCAL_MACHINE, reg_data.GetStateMediumKey().c_str(),
               kAccess) == ERROR_SUCCESS &&
      key.ReadValueDW(google_update::kRegUsageStatsField, &value) ==
          ERROR_SUCCESS;

  // Otherwise, try ClientState.
  if (!have_value) {
    have_value =
        key.Open(system_install ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER,
                 reg_data.GetStateKey().c_str(), kAccess) == ERROR_SUCCESS &&
        key.ReadValueDW(google_update::kRegUsageStatsField, &value) ==
            ERROR_SUCCESS;
  }

  if (!have_value)
    return google_update::TRISTATE_NONE;

  return value == google_update::TRISTATE_TRUE ? google_update::TRISTATE_TRUE
                                               : google_update::TRISTATE_FALSE;
}

// static
bool GoogleUpdateSettings::SetCollectStatsConsentAtLevel(bool system_install,
                                                         bool consented) {
  DWORD value =
      consented ? google_update::TRISTATE_TRUE : google_update::TRISTATE_FALSE;
  BrowserDistribution* dist = BrowserDistribution::GetDistribution();

  // Write to ClientStateMedium for system-level; ClientState otherwise.
  HKEY root_key = system_install ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
  base::string16 reg_path =
      system_install ? dist->GetStateMediumKey() : dist->GetStateKey();
  RegKey key;
  LONG result = key.Create(
      root_key, reg_path.c_str(), KEY_SET_VALUE | KEY_WOW64_32KEY);
  if (result != ERROR_SUCCESS) {
    LOG(ERROR) << "Failed opening key " << reg_path << " to set "
               << google_update::kRegUsageStatsField << "; result: " << result;
  } else {
    result = key.WriteValue(google_update::kRegUsageStatsField, value);
    LOG_IF(ERROR, result != ERROR_SUCCESS) << "Failed setting "
        << google_update::kRegUsageStatsField << " in key " << reg_path
        << "; result: " << result;
  }

  // When opting out, clear registry backup of client id and related values.
  if (result == ERROR_SUCCESS && !consented)
    StoreMetricsClientInfo(metrics::ClientInfo());

  return (result == ERROR_SUCCESS);
}

// static
bool GoogleUpdateSettings::GetCollectStatsConsentDefault(
    bool* stats_consent_default) {
  installer::ChannelInfo channel_info;
  if (InitChannelInfo(IsSystemInstall(), &channel_info)) {
    base::string16 stats_default = channel_info.GetStatsDefault();
    if (stats_default == L"0" || stats_default == L"1") {
      *stats_consent_default = (stats_default == L"1");
      return true;
    }
  }
  return false;
}

std::unique_ptr<metrics::ClientInfo>
GoogleUpdateSettings::LoadMetricsClientInfo() {
  base::string16 client_id_16;
  if (!ReadGoogleUpdateStrKey(google_update::kRegMetricsId, &client_id_16) ||
      client_id_16.empty()) {
    return std::unique_ptr<metrics::ClientInfo>();
  }

  std::unique_ptr<metrics::ClientInfo> client_info(new metrics::ClientInfo);
  client_info->client_id = base::UTF16ToUTF8(client_id_16);

  base::string16 installation_date_str;
  if (ReadGoogleUpdateStrKey(google_update::kRegMetricsIdInstallDate,
                             &installation_date_str)) {
    base::StringToInt64(installation_date_str, &client_info->installation_date);
  }

  base::string16 reporting_enbaled_date_date_str;
  if (ReadGoogleUpdateStrKey(google_update::kRegMetricsIdEnabledDate,
                             &reporting_enbaled_date_date_str)) {
    base::StringToInt64(reporting_enbaled_date_date_str,
                        &client_info->reporting_enabled_date);
  }

  return client_info;
}

void GoogleUpdateSettings::StoreMetricsClientInfo(
    const metrics::ClientInfo& client_info) {
  // Attempt a best-effort at backing |client_info| in the registry (but don't
  // handle/report failures).
  WriteGoogleUpdateStrKey(google_update::kRegMetricsId,
                          base::UTF8ToUTF16(client_info.client_id));
  WriteGoogleUpdateStrKey(google_update::kRegMetricsIdInstallDate,
                          base::Int64ToString16(client_info.installation_date));
  WriteGoogleUpdateStrKey(
      google_update::kRegMetricsIdEnabledDate,
      base::Int64ToString16(client_info.reporting_enabled_date));
}

// EULA consent is only relevant for system-level installs.
bool GoogleUpdateSettings::SetEULAConsent(
    const InstallationState& machine_state,
    BrowserDistribution* dist,
    bool consented) {
  DCHECK(dist);
  const DWORD eula_accepted = consented ? 1 : 0;
  const REGSAM kAccess = KEY_SET_VALUE | KEY_WOW64_32KEY;
  RegKey key;

  // Write the consent value into the product's ClientStateMedium key.
  return key.Create(HKEY_LOCAL_MACHINE, dist->GetStateMediumKey().c_str(),
                    kAccess) == ERROR_SUCCESS &&
         key.WriteValue(google_update::kRegEULAAceptedField, eula_accepted) ==
             ERROR_SUCCESS;
}

int GoogleUpdateSettings::GetLastRunTime() {
  base::string16 time_s;
  if (!ReadGoogleUpdateStrKey(google_update::kRegLastRunTimeField, &time_s))
    return -1;
  int64_t time_i;
  if (!base::StringToInt64(time_s, &time_i))
    return -1;
  base::TimeDelta td =
      base::Time::NowFromSystemTime() - base::Time::FromInternalValue(time_i);
  return td.InDays();
}

bool GoogleUpdateSettings::SetLastRunTime() {
  int64_t time = base::Time::NowFromSystemTime().ToInternalValue();
  return WriteGoogleUpdateStrKey(google_update::kRegLastRunTimeField,
                                 base::Int64ToString16(time));
}

bool GoogleUpdateSettings::RemoveLastRunTime() {
  return RemoveGoogleUpdateStrKey(google_update::kRegLastRunTimeField);
}

bool GoogleUpdateSettings::GetBrowser(base::string16* browser) {
  return ReadGoogleUpdateStrKey(google_update::kRegBrowserField, browser);
}

bool GoogleUpdateSettings::GetLanguage(base::string16* language) {
  return ReadGoogleUpdateStrKey(google_update::kRegLangField, language);
}

bool GoogleUpdateSettings::GetBrand(base::string16* brand) {
  return ReadGoogleUpdateStrKey(google_update::kRegRLZBrandField, brand);
}

bool GoogleUpdateSettings::GetReactivationBrand(base::string16* brand) {
  return ReadGoogleUpdateStrKey(google_update::kRegRLZReactivationBrandField,
                                brand);
}

bool GoogleUpdateSettings::GetClient(base::string16* client) {
  return ReadGoogleUpdateStrKey(google_update::kRegClientField, client);
}

bool GoogleUpdateSettings::SetClient(const base::string16& client) {
  return WriteGoogleUpdateStrKey(google_update::kRegClientField, client);
}

bool GoogleUpdateSettings::GetReferral(base::string16* referral) {
  return ReadGoogleUpdateStrKey(google_update::kRegReferralField, referral);
}

bool GoogleUpdateSettings::ClearReferral() {
  return ClearGoogleUpdateStrKey(google_update::kRegReferralField);
}

bool GoogleUpdateSettings::UpdateDidRunState(bool did_run) {
  BrowserDistribution* dist = BrowserDistribution::GetDistribution();
  return WriteGoogleUpdateStrKeyInternal(dist->GetAppRegistrationData(),
                                         google_update::kRegDidRunField,
                                         did_run ? L"1" : L"0");
}

void GoogleUpdateSettings::UpdateInstallStatus(bool system_install,
    installer::ArchiveType archive_type, int install_return_code,
    const base::string16& product_guid) {
  DCHECK(archive_type != installer::UNKNOWN_ARCHIVE_TYPE ||
         install_return_code != 0);
  HKEY reg_root = (system_install) ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;

  RegKey key;
  installer::ChannelInfo channel_info;
  base::string16 reg_key(google_update::kRegPathClientState);
  reg_key.append(L"\\");
  reg_key.append(product_guid);
  LONG result = key.Open(reg_root,
                         reg_key.c_str(),
                         KEY_QUERY_VALUE | KEY_SET_VALUE | KEY_WOW64_32KEY);
  if (result == ERROR_SUCCESS)
    channel_info.Initialize(key);
  else if (result != ERROR_FILE_NOT_FOUND)
    LOG(ERROR) << "Failed to open " << reg_key << "; Error: " << result;

  if (UpdateGoogleUpdateApKey(archive_type, install_return_code,
                              &channel_info)) {
    // We have a modified channel_info value to write.
    // Create the app's ClientState key if it doesn't already exist.
    if (!key.Valid()) {
      result = key.Open(reg_root,
                        google_update::kRegPathClientState,
                        KEY_CREATE_SUB_KEY | KEY_WOW64_32KEY);
      if (result == ERROR_SUCCESS)
        result = key.CreateKey(product_guid.c_str(),
                               KEY_SET_VALUE | KEY_WOW64_32KEY);

      if (result != ERROR_SUCCESS) {
        LOG(ERROR) << "Failed to create " << reg_key << "; Error: " << result;
        return;
      }
    }
    if (!channel_info.Write(&key)) {
      LOG(ERROR) << "Failed to write to application's ClientState key "
                 << google_update::kRegApField << " = " << channel_info.value();
    }
  }
}

void GoogleUpdateSettings::SetProgress(bool system_install,
                                       const base::string16& path,
                                       int progress) {
  DCHECK_GE(progress, 0);
  DCHECK_LE(progress, 100);
  const HKEY root = system_install ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
  base::win::RegKey key(root, path.c_str(), KEY_SET_VALUE | KEY_WOW64_32KEY);
  if (key.Valid()) {
    key.WriteValue(google_update::kRegInstallerProgress,
                   static_cast<DWORD>(progress));
  }
}

bool GoogleUpdateSettings::UpdateGoogleUpdateApKey(
    installer::ArchiveType archive_type, int install_return_code,
    installer::ChannelInfo* value) {
  DCHECK(archive_type != installer::UNKNOWN_ARCHIVE_TYPE ||
         install_return_code != 0);
  bool modified = false;

  if (archive_type == installer::FULL_ARCHIVE_TYPE || !install_return_code) {
    if (value->SetFullSuffix(false)) {
      VLOG(1) << "Removed incremental installer failure key; "
                 "switching to channel: "
              << value->value();
      modified = true;
    }
  } else if (archive_type == installer::INCREMENTAL_ARCHIVE_TYPE) {
    if (value->SetFullSuffix(true)) {
      VLOG(1) << "Incremental installer failed; switching to channel: "
              << value->value();
      modified = true;
    } else {
      VLOG(1) << "Incremental installer failure; already on channel: "
              << value->value();
    }
  } else {
    // It's okay if we don't know the archive type.  In this case, leave the
    // "-full" suffix as we found it.
    DCHECK_EQ(installer::UNKNOWN_ARCHIVE_TYPE, archive_type);
  }

  // The mini_installer in Chrome 10 through 12 added "-multifail" to the "ap"
  // value if "--multi-install" was on the command line. Unconditionally remove
  // it if present.
  // TODO(grt): Move this cleanup into mini_installer.cc's SetInstallerFlags.
  if (value->SetMultiFailSuffix(false)) {
    VLOG(1) << "Removed multi-install failure key; switching to channel: "
            << value->value();
    modified = true;
  }

  if (value->ClearStage()) {
    VLOG(1) << "Removed (legacy) stage information; switching to channel: "
            << value->value();
    modified = true;
  }

  return modified;
}

void GoogleUpdateSettings::UpdateProfileCounts(size_t profiles_active,
                                               size_t profiles_signedin) {
  const auto& app_reg_data =
      BrowserDistribution::GetDistribution()->GetAppRegistrationData();
  const bool is_system_install = IsSystemInstall();
  WritePerUserStat(app_reg_data, is_system_install,
                   google_update::kRegProfilesActive,
                   base::saturated_cast<uint32_t>(profiles_active));
  WritePerUserStat(app_reg_data, is_system_install,
                   google_update::kRegProfilesSignedIn,
                   base::saturated_cast<uint32_t>(profiles_signedin));
}

int GoogleUpdateSettings::DuplicateGoogleUpdateSystemClientKey() {
  BrowserDistribution* dist = BrowserDistribution::GetDistribution();
  base::string16 reg_path = dist->GetStateKey();

  // Minimum access needed is to be able to write to this key.
  RegKey reg_key(
      HKEY_LOCAL_MACHINE, reg_path.c_str(), KEY_SET_VALUE | KEY_WOW64_32KEY);
  if (!reg_key.Valid())
    return 0;

  HANDLE target_handle = 0;
  if (!DuplicateHandle(GetCurrentProcess(), reg_key.Handle(),
                       GetCurrentProcess(), &target_handle, KEY_SET_VALUE,
                       TRUE, DUPLICATE_SAME_ACCESS)) {
    return 0;
  }
  return static_cast<int>(reinterpret_cast<uintptr_t>(target_handle));
}

bool GoogleUpdateSettings::WriteGoogleUpdateSystemClientKey(
    int handle, const base::string16& key, const base::string16& value) {
  HKEY reg_key = reinterpret_cast<HKEY>(
      reinterpret_cast<void*>(static_cast<uintptr_t>(handle)));
  DWORD size = static_cast<DWORD>(value.size()) * sizeof(wchar_t);
  LSTATUS status = RegSetValueEx(reg_key, key.c_str(), 0, REG_SZ,
      reinterpret_cast<const BYTE*>(value.c_str()), size);
  return status == ERROR_SUCCESS;
}

GoogleUpdateSettings::UpdatePolicy GoogleUpdateSettings::GetAppUpdatePolicy(
    base::StringPiece16 app_guid,
    bool* is_overridden) {
  bool found_override = false;
  UpdatePolicy update_policy = kDefaultUpdatePolicy;

#if defined(GOOGLE_CHROME_BUILD)
  DCHECK(!app_guid.empty());
  RegKey policy_key;

  // Google Update Group Policy settings are always in HKLM.
  // TODO(wfh): Check if policies should go into Wow6432Node or not.
  if (policy_key.Open(HKEY_LOCAL_MACHINE, kPoliciesKey, KEY_QUERY_VALUE) ==
          ERROR_SUCCESS) {
    DWORD value = 0;
    base::string16 app_update_override(kUpdateOverrideValuePrefix);
    app_guid.AppendToString(&app_update_override);
    // First try to read and comprehend the app-specific override.
    found_override = (policy_key.ReadValueDW(app_update_override.c_str(),
                                             &value) == ERROR_SUCCESS &&
                      GetUpdatePolicyFromDword(value, &update_policy));

    // Failing that, try to read and comprehend the default override.
    if (!found_override &&
        policy_key.ReadValueDW(kUpdatePolicyValue, &value) == ERROR_SUCCESS) {
      GetUpdatePolicyFromDword(value, &update_policy);
    }
  }
#endif  // defined(GOOGLE_CHROME_BUILD)

  if (is_overridden != NULL)
    *is_overridden = found_override;

  return update_policy;
}

// static
bool GoogleUpdateSettings::AreAutoupdatesEnabled() {
#if defined(GOOGLE_CHROME_BUILD)
  // Check the auto-update check period override. If it is 0 or exceeds the
  // maximum timeout, then for all intents and purposes auto updates are
  // disabled.
  RegKey policy_key;
  DWORD value = 0;
  if (policy_key.Open(HKEY_LOCAL_MACHINE, kPoliciesKey,
                      KEY_QUERY_VALUE) == ERROR_SUCCESS &&
      policy_key.ReadValueDW(kCheckPeriodOverrideMinutes,
                             &value) == ERROR_SUCCESS &&
      (value == 0 || value > kCheckPeriodOverrideMinutesMax)) {
    return false;
  }

  UpdatePolicy app_policy =
      GetAppUpdatePolicy(install_static::GetAppGuid(), nullptr);
  return app_policy == AUTOMATIC_UPDATES || app_policy == AUTO_UPDATES_ONLY;
#else  // defined(GOOGLE_CHROME_BUILD)
  // Chromium does not auto update.
  return false;
#endif  // !defined(GOOGLE_CHROME_BUILD)
}

// static
bool GoogleUpdateSettings::ReenableAutoupdates() {
#if defined(GOOGLE_CHROME_BUILD)
  int needs_reset_count = 0;
  int did_reset_count = 0;

  UpdatePolicy update_policy = kDefaultUpdatePolicy;
  RegKey policy_key;
  if (policy_key.Open(HKEY_LOCAL_MACHINE, kPoliciesKey,
                      KEY_SET_VALUE | KEY_QUERY_VALUE) == ERROR_SUCCESS) {
    // Set to true while app-specific overrides are present that allow automatic
    // updates. When this is the case, the defaults are irrelevant and don't
    // need to be checked or reset.
    bool automatic_updates_allowed_by_overrides = true;
    DWORD value = 0;

    // First check the app-specific override value and reset that if needed.
    // Note that this intentionally sets the override to AUTOMATIC_UPDATES even
    // if it was previously AUTO_UPDATES_ONLY. The thinking is that
    // AUTOMATIC_UPDATES is marginally more likely to let a user update and this
    // code is only called when a stuck user asks for updates.
    base::string16 app_update_override(kUpdateOverrideValuePrefix);
    app_update_override.append(install_static::GetAppGuid());
    if (policy_key.ReadValueDW(app_update_override.c_str(), &value) !=
        ERROR_SUCCESS) {
      automatic_updates_allowed_by_overrides = false;
    } else if (!GetUpdatePolicyFromDword(value, &update_policy) ||
               update_policy != GoogleUpdateSettings::AUTOMATIC_UPDATES) {
      automatic_updates_allowed_by_overrides = false;
      ++needs_reset_count;
      if (policy_key.WriteValue(
              app_update_override.c_str(),
              static_cast<DWORD>(GoogleUpdateSettings::AUTOMATIC_UPDATES)) ==
          ERROR_SUCCESS) {
        ++did_reset_count;
      }
    }

    // If there were no app-specific override policies, see if there's a global
    // policy preventing updates and delete it if so.
    if (!automatic_updates_allowed_by_overrides &&
        policy_key.ReadValueDW(kUpdatePolicyValue, &value) == ERROR_SUCCESS &&
        (!GetUpdatePolicyFromDword(value, &update_policy) ||
         update_policy != GoogleUpdateSettings::AUTOMATIC_UPDATES)) {
      ++needs_reset_count;
      if (policy_key.DeleteValue(kUpdatePolicyValue) == ERROR_SUCCESS)
        ++did_reset_count;
    }

    // Check the auto-update check period override. If it is 0 or exceeds
    // the maximum timeout, delete the override value.
    if (policy_key.ReadValueDW(kCheckPeriodOverrideMinutes,
                               &value) == ERROR_SUCCESS &&
        (value == 0 || value > kCheckPeriodOverrideMinutesMax)) {
      ++needs_reset_count;
      if (policy_key.DeleteValue(kCheckPeriodOverrideMinutes) == ERROR_SUCCESS)
        ++did_reset_count;
    }

    // Return whether the number of successful resets is the same as the
    // number of things that appeared to need resetting.
    return (needs_reset_count == did_reset_count);
  } else {
    // For some reason we couldn't open the policy key with the desired
    // permissions to make changes (the most likely reason is that there is no
    // policy set). Simply return whether or not we think updates are enabled.
    return AreAutoupdatesEnabled();
  }
#endif
  // Non Google Chrome isn't going to autoupdate.
  return true;
}

// Reads and sanitizes the value of
// "HKLM\SOFTWARE\Policies\Google\Update\DownloadPreference". A valid
// group policy option must be a single alpha numeric word of up to 32
// characters.
base::string16 GoogleUpdateSettings::GetDownloadPreference() {
  RegKey policy_key;
  base::string16 value;
  if (policy_key.Open(HKEY_LOCAL_MACHINE, kPoliciesKey, KEY_QUERY_VALUE) ==
          ERROR_SUCCESS &&
      policy_key.ReadValue(kDownloadPreferencePolicyValue, &value) ==
          ERROR_SUCCESS) {
    // Validates that |value| matches `[a-zA-z]{0-32}`.
    const size_t kMaxValueLength = 32;
    if (value.size() > kMaxValueLength)
      return base::string16();
    for (auto ch : value) {
      if (!base::IsAsciiAlpha(ch))
        return base::string16();
    }
    return value;
  }
  return base::string16();
}

void GoogleUpdateSettings::RecordChromeUpdatePolicyHistograms() {
  bool is_overridden = false;
  const UpdatePolicy update_policy =
      GetAppUpdatePolicy(install_static::GetAppGuid(), &is_overridden);
  UMA_HISTOGRAM_BOOLEAN("GoogleUpdate.UpdatePolicyIsOverridden", is_overridden);
  UMA_HISTOGRAM_ENUMERATION("GoogleUpdate.EffectivePolicy", update_policy,
                            UPDATE_POLICIES_COUNT);
}

base::string16 GoogleUpdateSettings::GetUninstallCommandLine(
    bool system_install) {
  const HKEY root_key = system_install ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
  base::string16 cmd_line;
  RegKey update_key;

  if (update_key.Open(root_key, google_update::kRegPathGoogleUpdate,
                      KEY_QUERY_VALUE | KEY_WOW64_32KEY) == ERROR_SUCCESS) {
    update_key.ReadValue(google_update::kRegUninstallCmdLine, &cmd_line);
  }

  return cmd_line;
}

base::Version GoogleUpdateSettings::GetGoogleUpdateVersion(
    bool system_install) {
  const HKEY root_key = system_install ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
  base::string16 version;
  RegKey key;

  if (key.Open(root_key,
               google_update::kRegPathGoogleUpdate,
               KEY_QUERY_VALUE | KEY_WOW64_32KEY) == ERROR_SUCCESS &&
      key.ReadValue(google_update::kRegGoogleUpdateVersion, &version) ==
          ERROR_SUCCESS) {
    return base::Version(base::UTF16ToUTF8(version));
  }

  return base::Version();
}

base::Time GoogleUpdateSettings::GetGoogleUpdateLastStartedAU(
    bool system_install) {
  const HKEY root_key = system_install ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
  RegKey update_key;

  if (update_key.Open(root_key,
                      google_update::kRegPathGoogleUpdate,
                      KEY_QUERY_VALUE | KEY_WOW64_32KEY) == ERROR_SUCCESS) {
    DWORD last_start;
    if (update_key.ReadValueDW(google_update::kRegLastStartedAUField,
                               &last_start) == ERROR_SUCCESS) {
      return base::Time::FromTimeT(last_start);
    }
  }

  return base::Time();
}

base::Time GoogleUpdateSettings::GetGoogleUpdateLastChecked(
    bool system_install) {
  const HKEY root_key = system_install ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
  RegKey update_key;

  if (update_key.Open(root_key,
                      google_update::kRegPathGoogleUpdate,
                      KEY_QUERY_VALUE | KEY_WOW64_32KEY) == ERROR_SUCCESS) {
    DWORD last_check;
    if (update_key.ReadValueDW(google_update::kRegLastCheckedField,
                               &last_check) == ERROR_SUCCESS) {
      return base::Time::FromTimeT(last_check);
    }
  }

  return base::Time();
}

bool GoogleUpdateSettings::GetUpdateDetailForApp(bool system_install,
                                                 const wchar_t* app_guid,
                                                 ProductData* data) {
  DCHECK(app_guid);
  DCHECK(data);

  bool product_found = false;

  const HKEY root_key = system_install ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
  base::string16 clientstate_reg_path(google_update::kRegPathClientState);
  clientstate_reg_path.append(L"\\");
  clientstate_reg_path.append(app_guid);

  RegKey clientstate;
  if (clientstate.Open(root_key,
                       clientstate_reg_path.c_str(),
                       KEY_QUERY_VALUE | KEY_WOW64_32KEY) == ERROR_SUCCESS) {
    base::string16 version;
    DWORD dword_value;
    if ((clientstate.ReadValueDW(google_update::kRegLastCheckSuccessField,
                                 &dword_value) == ERROR_SUCCESS) &&
        (clientstate.ReadValue(google_update::kRegVersionField,
                               &version) == ERROR_SUCCESS)) {
      product_found = true;
      data->version = base::UTF16ToASCII(version);
      data->last_success = base::Time::FromTimeT(dword_value);
      data->last_result = 0;
      data->last_error_code = 0;
      data->last_extra_code = 0;

      if (clientstate.ReadValueDW(google_update::kRegLastInstallerResultField,
                                  &dword_value) == ERROR_SUCCESS) {
        // Google Update convention is that if an installer writes an result
        // code that is invalid, it is clamped to an exit code result.
        const DWORD kMaxValidInstallResult = 4;  // INSTALLER_RESULT_EXIT_CODE
        data->last_result = std::min(dword_value, kMaxValidInstallResult);
      }
      if (clientstate.ReadValueDW(google_update::kRegLastInstallerErrorField,
                                  &dword_value) == ERROR_SUCCESS) {
        data->last_error_code = dword_value;
      }
      if (clientstate.ReadValueDW(google_update::kRegLastInstallerExtraField,
                                  &dword_value) == ERROR_SUCCESS) {
        data->last_extra_code = dword_value;
      }
    }
  }

  return product_found;
}

bool GoogleUpdateSettings::GetUpdateDetailForGoogleUpdate(ProductData* data) {
  return GetUpdateDetailForApp(!InstallUtil::IsPerUserInstall(),
                               google_update::kGoogleUpdateUpgradeCode, data);
}

bool GoogleUpdateSettings::GetUpdateDetail(ProductData* data) {
  return GetUpdateDetailForApp(!InstallUtil::IsPerUserInstall(),
                               install_static::GetAppGuid(), data);
}

bool GoogleUpdateSettings::SetExperimentLabels(
    bool system_install,
    const base::string16& experiment_labels) {
  HKEY reg_root = system_install ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
  // Use the browser distribution and install level to write to the correct
  // client state/app guid key.
  bool success = false;
  BrowserDistribution* dist = BrowserDistribution::GetDistribution();
  base::string16 client_state_path(system_install ? dist->GetStateMediumKey()
                                                  : dist->GetStateKey());
  RegKey client_state(reg_root, client_state_path.c_str(),
                      KEY_SET_VALUE | KEY_WOW64_32KEY);
  // It is possible that the registry keys do not yet exist or have not yet
  // been ACLed by Google Update to be user writable.
  if (!client_state.Valid())
    return false;
  if (experiment_labels.empty()) {
    success = client_state.DeleteValue(google_update::kExperimentLabels) ==
              ERROR_SUCCESS;
  } else {
    success =
        client_state.WriteValue(google_update::kExperimentLabels,
                                experiment_labels.c_str()) == ERROR_SUCCESS;
  }

  return success;
}

bool GoogleUpdateSettings::ReadExperimentLabels(
    bool system_install,
    base::string16* experiment_labels) {
  HKEY reg_root = system_install ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
  BrowserDistribution* dist = BrowserDistribution::GetDistribution();
  base::string16 client_state_path(
      system_install ? dist->GetStateMediumKey() : dist->GetStateKey());

  RegKey client_state;
  LONG result = client_state.Open(
      reg_root, client_state_path.c_str(), KEY_QUERY_VALUE | KEY_WOW64_32KEY);
  if (result == ERROR_SUCCESS) {
    result = client_state.ReadValue(google_update::kExperimentLabels,
                                    experiment_labels);
  }

  // If the key or value was not present, return the empty string.
  if (result == ERROR_FILE_NOT_FOUND || result == ERROR_PATH_NOT_FOUND) {
    experiment_labels->clear();
    return true;
  }

  return result == ERROR_SUCCESS;
}
