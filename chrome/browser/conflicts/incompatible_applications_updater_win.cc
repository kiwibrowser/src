// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/conflicts/incompatible_applications_updater_win.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "base/win/registry.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/conflicts/module_database_win.h"
#include "chrome/browser/conflicts/module_info_util_win.h"
#include "chrome/browser/conflicts/module_list_filter_win.h"
#include "chrome/browser/conflicts/third_party_metrics_recorder_win.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/browser/browser_thread.h"

namespace {

// Serializes a vector of IncompatibleApplications to JSON.
base::Value ConvertToDictionary(
    const std::vector<IncompatibleApplicationsUpdater::IncompatibleApplication>&
        applications) {
  base::Value result(base::Value::Type::DICTIONARY);

  for (const auto& application : applications) {
    base::Value element(base::Value::Type::DICTIONARY);

    // The registry location is necessary to quickly figure out if that
    // application is still installed on the computer.
    element.SetKey(
        "registry_is_hkcu",
        base::Value(application.info.registry_root == HKEY_CURRENT_USER));
    element.SetKey("registry_key_path",
                   base::Value(application.info.registry_key_path));
    element.SetKey(
        "registry_wow64_access",
        base::Value(static_cast<int>(application.info.registry_wow64_access)));

    // And then the actual information needed to display a warning to the user.
    element.SetKey("allow_load",
                   base::Value(application.blacklist_action->allow_load()));
    element.SetKey("type",
                   base::Value(application.blacklist_action->message_type()));
    element.SetKey("message_url",
                   base::Value(application.blacklist_action->message_url()));

    result.SetKey(base::UTF16ToUTF8(application.info.name), std::move(element));
  }

  return result;
}

// Deserializes a IncompatibleApplication named |name| from |value|. Returns
// null if |value| is not a dict containing all required fields.
std::unique_ptr<IncompatibleApplicationsUpdater::IncompatibleApplication>
ConvertToIncompatibleApplication(const std::string& name,
                                 const base::Value& value) {
  if (!value.is_dict())
    return nullptr;

  const base::Value* registry_is_hkcu_value =
      value.FindKeyOfType("registry_is_hkcu", base::Value::Type::BOOLEAN);
  const base::Value* registry_key_path_value =
      value.FindKeyOfType("registry_key_path", base::Value::Type::STRING);
  const base::Value* registry_wow64_access_value =
      value.FindKeyOfType("registry_wow64_access", base::Value::Type::INTEGER);
  const base::Value* allow_load_value =
      value.FindKeyOfType("allow_load", base::Value::Type::BOOLEAN);
  const base::Value* type_value =
      value.FindKeyOfType("type", base::Value::Type::INTEGER);
  const base::Value* message_url_value =
      value.FindKeyOfType("message_url", base::Value::Type::STRING);

  // All of the above are required for a valid application.
  if (!registry_is_hkcu_value || !registry_key_path_value ||
      !registry_wow64_access_value || !allow_load_value || !type_value ||
      !message_url_value) {
    return nullptr;
  }

  InstalledApplications::ApplicationInfo application_info = {
      base::UTF8ToUTF16(name),
      registry_is_hkcu_value->GetBool() ? HKEY_CURRENT_USER
                                        : HKEY_LOCAL_MACHINE,
      base::UTF8ToUTF16(registry_key_path_value->GetString()),
      static_cast<REGSAM>(registry_wow64_access_value->GetInt())};

  auto blacklist_action =
      std::make_unique<chrome::conflicts::BlacklistAction>();
  blacklist_action->set_allow_load(allow_load_value->GetBool());
  blacklist_action->set_message_type(
      static_cast<chrome::conflicts::BlacklistMessageType>(
          type_value->GetInt()));
  blacklist_action->set_message_url(message_url_value->GetString());

  return std::make_unique<
      IncompatibleApplicationsUpdater::IncompatibleApplication>(
      std::move(application_info), std::move(blacklist_action));
}

// Returns true if |application| references an existing application in the
// registry.
//
// Used to filter out stale applications from the cache. This can happen if a
// application was uninstalled between the time it was found and Chrome was
// relaunched.
bool IsValidApplication(
    const IncompatibleApplicationsUpdater::IncompatibleApplication&
        application) {
  return base::win::RegKey(
             application.info.registry_root,
             application.info.registry_key_path.c_str(),
             KEY_QUERY_VALUE | application.info.registry_wow64_access)
      .Valid();
}

// Clears the cache of all the applications whose name is in
// |state_application_names|.
void RemoveStaleApplications(
    const std::vector<std::string>& stale_application_names) {
  // Early exit because DictionaryPrefUpdate will write to the pref even if it
  // doesn't contain a value.
  if (stale_application_names.empty())
    return;

  DictionaryPrefUpdate update(g_browser_process->local_state(),
                              prefs::kIncompatibleApplications);
  base::Value* existing_applications = update.Get();

  for (const auto& application_name : stale_application_names) {
    bool removed = existing_applications->RemoveKey(application_name);
    DCHECK(removed);
  }
}

// Applies the given |function| object to each valid IncompatibleApplication
// found in the kIncompatibleApplications preference.
//
// The signature of the function must be equivalent to the following:
//   bool Function(std::unique_ptr<IncompatibleApplication> application));
//
// The return value of |function| indicates if the enumeration should continue
// (true) or be stopped (false).
//
// This function takes care of removing invalid entries that are found during
// the enumeration.
template <class UnaryFunction>
void EnumerateAndTrimIncompatibleApplications(UnaryFunction function) {
  std::vector<std::string> stale_application_names;
  for (const auto& item : g_browser_process->local_state()
                              ->FindPreference(prefs::kIncompatibleApplications)
                              ->GetValue()
                              ->DictItems()) {
    auto application =
        ConvertToIncompatibleApplication(item.first, item.second);

    if (!application || !IsValidApplication(*application)) {
      // Mark every invalid application as stale so they are removed from the
      // cache.
      stale_application_names.push_back(item.first);
      continue;
    }

    // Notify the caller and stop the enumeration if requested by the function.
    if (!function(std::move(application)))
      break;
  }

  RemoveStaleApplications(stale_application_names);
}

}  // namespace

// -----------------------------------------------------------------------------
// IncompatibleApplication

IncompatibleApplicationsUpdater::IncompatibleApplication::
    IncompatibleApplication(
        InstalledApplications::ApplicationInfo info,
        std::unique_ptr<chrome::conflicts::BlacklistAction> blacklist_action)
    : info(std::move(info)), blacklist_action(std::move(blacklist_action)) {}

IncompatibleApplicationsUpdater::IncompatibleApplication::
    ~IncompatibleApplication() = default;

IncompatibleApplicationsUpdater::IncompatibleApplication::
    IncompatibleApplication(
        IncompatibleApplication&& incompatible_application) = default;

IncompatibleApplicationsUpdater::IncompatibleApplication&
IncompatibleApplicationsUpdater::IncompatibleApplication::operator=(
    IncompatibleApplication&& incompatible_application) = default;

// -----------------------------------------------------------------------------
// IncompatibleApplicationsUpdater

IncompatibleApplicationsUpdater::IncompatibleApplicationsUpdater(
    const CertificateInfo& exe_certificate_info,
    const ModuleListFilter& module_list_filter,
    const InstalledApplications& installed_applications)
    : exe_certificate_info_(exe_certificate_info),
      module_list_filter_(module_list_filter),
      installed_applications_(installed_applications) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

IncompatibleApplicationsUpdater::~IncompatibleApplicationsUpdater() = default;

// static
void IncompatibleApplicationsUpdater::RegisterLocalStatePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(prefs::kIncompatibleApplications);
  registry->RegisterDictionaryPref(prefs::kProblematicPrograms);
}

// static
bool IncompatibleApplicationsUpdater::IsWarningEnabled() {
  return ModuleDatabase::GetInstance() &&
         ModuleDatabase::GetInstance()->third_party_conflicts_manager();
}

// static
bool IncompatibleApplicationsUpdater::HasCachedApplications() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  bool found_valid_application = false;

  EnumerateAndTrimIncompatibleApplications(
      [&found_valid_application](
          std::unique_ptr<IncompatibleApplication> application) {
        found_valid_application = true;

        // Break the enumeration.
        return false;
      });

  return found_valid_application;
}

// static
std::vector<IncompatibleApplicationsUpdater::IncompatibleApplication>
IncompatibleApplicationsUpdater::GetCachedApplications() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  std::vector<IncompatibleApplication> valid_applications;

  EnumerateAndTrimIncompatibleApplications(
      [&valid_applications](
          std::unique_ptr<IncompatibleApplication> application) {
        valid_applications.push_back(std::move(*application));

        // Continue the enumeration.
        return true;
      });

  return valid_applications;
}

void IncompatibleApplicationsUpdater::OnNewModuleFound(
    const ModuleInfoKey& module_key,
    const ModuleInfoData& module_data) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // Only consider loaded modules that are not shell extensions or IMEs.
  static constexpr uint32_t kModuleTypesBitmask =
      ModuleInfoData::kTypeLoadedModule | ModuleInfoData::kTypeShellExtension |
      ModuleInfoData::kTypeIme;
  if ((module_data.module_types & kModuleTypesBitmask) !=
      ModuleInfoData::kTypeLoadedModule) {
    return;
  }

  // Explicitly whitelist modules whose signing cert's Subject field matches the
  // one in the current executable. No attempt is made to check the validity of
  // module signatures or of signing certs.
  if (exe_certificate_info_.type != CertificateType::NO_CERTIFICATE &&
      exe_certificate_info_.subject ==
          module_data.inspection_result->certificate_info.subject) {
    return;
  }

  // Skip modules whitelisted by the Module List component.
  if (module_list_filter_.IsWhitelisted(module_key, module_data))
    return;

  // Also skip a module if it cannot be associated with an installed application
  // on the user's computer.
  std::vector<InstalledApplications::ApplicationInfo> associated_applications;
  if (!installed_applications_.GetInstalledApplications(
          module_key.module_path, &associated_applications)) {
    return;
  }

  std::unique_ptr<chrome::conflicts::BlacklistAction> blacklist_action =
      module_list_filter_.IsBlacklisted(module_key, module_data);
  if (!blacklist_action) {
    // The default behavior is to suggest to uninstall.
    blacklist_action = std::make_unique<chrome::conflicts::BlacklistAction>();
    blacklist_action->set_allow_load(true);
    blacklist_action->set_message_type(
        chrome::conflicts::BlacklistMessageType::UNINSTALL);
    blacklist_action->set_message_url(std::string());
  }

  for (auto&& associated_application : associated_applications) {
    incompatible_applications_.emplace_back(
        std::move(associated_application),
        std::make_unique<chrome::conflicts::BlacklistAction>(
            *blacklist_action));
  }
}

void IncompatibleApplicationsUpdater::OnModuleDatabaseIdle() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // On the first call to OnModuleDatabaseIdle(), the previous value must always
  // be overwritten.
  if (before_first_idle_)
    g_browser_process->local_state()->ClearPref(
        prefs::kIncompatibleApplications);
  before_first_idle_ = false;

  // If there is no new incompatible application, there is nothing to do.
  if (incompatible_applications_.empty())
    return;

  // The conversion of the accumulated applications to a json dictionary takes
  // care of eliminating duplicates.
  base::Value new_applications =
      ConvertToDictionary(incompatible_applications_);
  incompatible_applications_.clear();

  // Update the existing dictionary.
  DictionaryPrefUpdate update(g_browser_process->local_state(),
                              prefs::kIncompatibleApplications);
  base::Value* existing_applications = update.Get();
  for (auto&& element : new_applications.DictItems()) {
    existing_applications->SetKey(std::move(element.first),
                                  std::move(element.second));
  }
}
