// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs.h"

#include <stddef.h>

#include <string>
#include <utility>

#include "base/files/file_util.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "base/values.h"
#include "chrome/browser/chromeos/arc/arc_session_manager.h"
#include "chrome/browser/chromeos/arc/arc_util.h"
#include "chrome/browser/chromeos/arc/policy/arc_policy_util.h"
#include "chrome/browser/chromeos/login/session/user_session_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs_factory.h"
#include "chrome/browser/ui/app_list/arc/arc_app_utils.h"
#include "chrome/browser/ui/app_list/arc/arc_package_syncable_service.h"
#include "chrome/browser/ui/app_list/arc/arc_pai_starter.h"
#include "chrome/grit/generated_resources.h"
#include "components/arc/arc_prefs.h"
#include "components/arc/arc_service_manager.h"
#include "components/arc/arc_util.h"
#include "components/arc/connection_holder.h"
#include "components/crx_file/id_util.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/user_manager/user_manager.h"
#include "content/public/browser/browser_thread.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

constexpr char kActivity[] = "activity";
constexpr char kIconResourceId[] = "icon_resource_id";
constexpr char kInstallTime[] = "install_time";
constexpr char kIntentUri[] = "intent_uri";
constexpr char kInvalidatedIcons[] = "invalidated_icons";
constexpr char kLastBackupAndroidId[] = "last_backup_android_id";
constexpr char kLastBackupTime[] = "last_backup_time";
constexpr char kLastLaunchTime[] = "lastlaunchtime";
constexpr char kLaunchable[] = "launchable";
constexpr char kName[] = "name";
constexpr char kNotificationsEnabled[] = "notifications_enabled";
constexpr char kPackageName[] = "package_name";
constexpr char kPackageVersion[] = "package_version";
constexpr char kSticky[] = "sticky";
constexpr char kShortcut[] = "shortcut";
constexpr char kShouldSync[] = "should_sync";
constexpr char kSystem[] = "system";
constexpr char kUninstalled[] = "uninstalled";
constexpr char kVPNProvider[] = "vpnprovider";

constexpr base::TimeDelta kDetectDefaultAppAvailabilityTimeout =
    base::TimeDelta::FromMinutes(1);

// Provider of write access to a dictionary storing ARC prefs.
class ScopedArcPrefUpdate : public DictionaryPrefUpdate {
 public:
  ScopedArcPrefUpdate(PrefService* service,
                      const std::string& id,
                      const std::string& path)
      : DictionaryPrefUpdate(service, path), id_(id) {}

  ~ScopedArcPrefUpdate() override {}

  // DictionaryPrefUpdate overrides:
  base::DictionaryValue* Get() override {
    base::DictionaryValue* dict = DictionaryPrefUpdate::Get();
    base::Value* dict_item =
        dict->FindKeyOfType(id_, base::Value::Type::DICTIONARY);
    if (!dict_item) {
      dict_item = dict->SetKey(id_, base::Value(base::Value::Type::DICTIONARY));
    }
    return static_cast<base::DictionaryValue*>(dict_item);
  }

 private:
  const std::string id_;

  DISALLOW_COPY_AND_ASSIGN(ScopedArcPrefUpdate);
};

// Accessor for deferred set notifications enabled requests in prefs.
class SetNotificationsEnabledDeferred {
 public:
  explicit SetNotificationsEnabledDeferred(PrefService* prefs)
      : prefs_(prefs) {}

  void Put(const std::string& app_id, bool enabled) {
    DictionaryPrefUpdate update(
        prefs_, arc::prefs::kArcSetNotificationsEnabledDeferred);
    base::DictionaryValue* const dict = update.Get();
    dict->SetKey(app_id, base::Value(enabled));
  }

  bool Get(const std::string& app_id, bool* enabled) {
    const base::DictionaryValue* dict =
        prefs_->GetDictionary(arc::prefs::kArcSetNotificationsEnabledDeferred);
    return dict->GetBoolean(app_id, enabled);
  }

  void Remove(const std::string& app_id) {
    DictionaryPrefUpdate update(
        prefs_, arc::prefs::kArcSetNotificationsEnabledDeferred);
    base::DictionaryValue* const dict = update.Get();
    dict->RemoveWithoutPathExpansion(app_id, /* out_value */ nullptr);
  }

 private:
  PrefService* const prefs_;
};

bool InstallIconFromFileThread(const base::FilePath& icon_path,
                               const std::vector<uint8_t>& content_png) {
  DCHECK(!content_png.empty());

  base::CreateDirectory(icon_path.DirName());

  int wrote =
      base::WriteFile(icon_path, reinterpret_cast<const char*>(&content_png[0]),
                      content_png.size());
  if (wrote != static_cast<int>(content_png.size())) {
    VLOG(2) << "Failed to write ARC icon file: " << icon_path.MaybeAsASCII()
            << ".";
    if (!base::DeleteFile(icon_path, false)) {
      VLOG(2) << "Couldn't delete broken icon file" << icon_path.MaybeAsASCII()
              << ".";
    }
    return false;
  }

  return true;
}

void DeleteAppFolderFromFileThread(const base::FilePath& path) {
  DCHECK(path.DirName().BaseName().MaybeAsASCII() == arc::prefs::kArcApps &&
         (!base::PathExists(path) || base::DirectoryExists(path)));
  const bool deleted = base::DeleteFile(path, true);
  DCHECK(deleted);
}

// TODO(crbug.com/672829): Due to shutdown procedure dependency,
// ArcAppListPrefs may try to touch ArcSessionManager related stuff.
// Specifically, this returns false on shutdown phase.
// Remove this check after the shutdown behavior is fixed.
bool IsArcAlive() {
  const auto* arc_session_manager = arc::ArcSessionManager::Get();
  return arc_session_manager && arc_session_manager->IsAllowed();
}

// Returns true if ARC Android instance is supposed to be enabled for the
// profile.  This can happen for if the user has opted in for the given profile,
// or when ARC always starts after login.
bool IsArcAndroidEnabledForProfile(const Profile* profile) {
  return arc::ShouldArcAlwaysStart() ||
         arc::IsArcPlayStoreEnabledForProfile(profile);
}

bool GetInt64FromPref(const base::DictionaryValue* dict,
                      const std::string& key,
                      int64_t* value) {
  DCHECK(dict);
  std::string value_str;
  if (!dict->GetStringWithoutPathExpansion(key, &value_str)) {
    VLOG(2) << "Can't find key in local pref dictionary. Invalid key: " << key
            << ".";
    return false;
  }

  if (!base::StringToInt64(value_str, value)) {
    VLOG(2) << "Can't change string to int64_t. Invalid string value: "
            << value_str << ".";
    return false;
  }

  return true;
}

base::FilePath ToIconPath(const base::FilePath& app_path,
                          ui::ScaleFactor scale_factor) {
  DCHECK(!app_path.empty());
  switch (scale_factor) {
    case ui::SCALE_FACTOR_100P:
      return app_path.AppendASCII("icon_100p.png");
    case ui::SCALE_FACTOR_125P:
      return app_path.AppendASCII("icon_125p.png");
    case ui::SCALE_FACTOR_133P:
      return app_path.AppendASCII("icon_133p.png");
    case ui::SCALE_FACTOR_140P:
      return app_path.AppendASCII("icon_140p.png");
    case ui::SCALE_FACTOR_150P:
      return app_path.AppendASCII("icon_150p.png");
    case ui::SCALE_FACTOR_180P:
      return app_path.AppendASCII("icon_180p.png");
    case ui::SCALE_FACTOR_200P:
      return app_path.AppendASCII("icon_200p.png");
    case ui::SCALE_FACTOR_250P:
      return app_path.AppendASCII("icon_250p.png");
    case ui::SCALE_FACTOR_300P:
      return app_path.AppendASCII("icon_300p.png");
    default:
      NOTREACHED();
      return base::FilePath();
  }
}

}  // namespace

// static
ArcAppListPrefs* ArcAppListPrefs::Create(
    Profile* profile,
    arc::ConnectionHolder<arc::mojom::AppInstance, arc::mojom::AppHost>*
        app_connection_holder) {
  return new ArcAppListPrefs(profile, app_connection_holder);
}

// static
void ArcAppListPrefs::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterDictionaryPref(arc::prefs::kArcApps);
  registry->RegisterDictionaryPref(arc::prefs::kArcPackages);
  registry->RegisterDictionaryPref(
      arc::prefs::kArcSetNotificationsEnabledDeferred);
}

// static
ArcAppListPrefs* ArcAppListPrefs::Get(content::BrowserContext* context) {
  return ArcAppListPrefsFactory::GetInstance()->GetForBrowserContext(context);
}

// static
std::string ArcAppListPrefs::GetAppId(const std::string& package_name,
                                      const std::string& activity) {
  if (package_name == arc::kPlayStorePackage &&
      activity == arc::kPlayStoreActivity) {
    return arc::kPlayStoreAppId;
  }
  const std::string input = package_name + "#" + activity;
  const std::string app_id = crx_file::id_util::GenerateId(input);
  return app_id;
}

std::string ArcAppListPrefs::GetAppIdByPackageName(
    const std::string& package_name) const {
  const base::DictionaryValue* apps =
      prefs_->GetDictionary(arc::prefs::kArcApps);
  if (!apps)
    return std::string();

  for (const auto& it : apps->DictItems()) {
    const base::Value& value = it.second;
    const base::Value* installed_package_name =
        value.FindKeyOfType(kPackageName, base::Value::Type::STRING);
    if (!installed_package_name ||
        installed_package_name->GetString() != package_name)
      continue;

    const base::Value* activity_name =
        value.FindKeyOfType(kActivity, base::Value::Type::STRING);
    return activity_name ? GetAppId(package_name, activity_name->GetString())
                         : std::string();
  }
  return std::string();
}

ArcAppListPrefs::ArcAppListPrefs(
    Profile* profile,
    arc::ConnectionHolder<arc::mojom::AppInstance, arc::mojom::AppHost>*
        app_connection_holder)
    : profile_(profile),
      prefs_(profile->GetPrefs()),
      app_connection_holder_(app_connection_holder),
      default_apps_(this, profile),
      weak_ptr_factory_(this) {
  DCHECK(profile);
  DCHECK(app_connection_holder);
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  const base::FilePath& base_path = profile->GetPath();
  base_path_ = base_path.AppendASCII(arc::prefs::kArcApps);

  invalidated_icon_scale_factor_mask_ = 0;
  for (ui::ScaleFactor scale_factor : ui::GetSupportedScaleFactors())
    invalidated_icon_scale_factor_mask_ |= (1U << scale_factor);

  arc::ArcSessionManager* arc_session_manager = arc::ArcSessionManager::Get();
  if (!arc_session_manager)
    return;

  DCHECK(arc::IsArcAllowedForProfile(profile));

  const std::vector<std::string> existing_app_ids = GetAppIds();
  tracked_apps_.insert(existing_app_ids.begin(), existing_app_ids.end());
  // Once default apps are ready OnDefaultAppsReady is called.
}

ArcAppListPrefs::~ArcAppListPrefs() {
  arc::ArcSessionManager* arc_session_manager = arc::ArcSessionManager::Get();
  if (!arc_session_manager)
    return;
  DCHECK(arc::ArcServiceManager::Get());
  arc_session_manager->RemoveObserver(this);
  app_connection_holder_->RemoveObserver(this);
}

void ArcAppListPrefs::StartPrefs() {
  // Don't tie ArcAppListPrefs created with sync test profile in sync
  // integration test to ArcSessionManager.
  if (!ArcAppListPrefsFactory::IsFactorySetForSyncTest()) {
    arc::ArcSessionManager* arc_session_manager = arc::ArcSessionManager::Get();
    CHECK(arc_session_manager);

    if (arc_session_manager->profile()) {
      // Note: If ArcSessionManager has profile, it should be as same as the one
      // this instance has, because ArcAppListPrefsFactory creates an instance
      // only if the given Profile meets ARC's requirement.
      // Anyway, just in case, check it here.
      DCHECK_EQ(profile_, arc_session_manager->profile());
      OnArcPlayStoreEnabledChanged(
          arc::IsArcPlayStoreEnabledForProfile(profile_));
    }
    arc_session_manager->AddObserver(this);
  }

  app_connection_holder_->SetHost(this);
  app_connection_holder_->AddObserver(this);
  if (!app_connection_holder_->IsConnected())
    OnConnectionClosed();
}

base::FilePath ArcAppListPrefs::GetAppPath(const std::string& app_id) const {
  return base_path_.AppendASCII(app_id);
}

base::FilePath ArcAppListPrefs::MaybeGetIconPathForDefaultApp(
    const std::string& app_id,
    ui::ScaleFactor scale_factor) const {
  const ArcDefaultAppList::AppInfo* default_app = default_apps_.GetApp(app_id);
  if (!default_app || default_app->app_path.empty())
    return base::FilePath();

  return ToIconPath(default_app->app_path, scale_factor);
}

base::FilePath ArcAppListPrefs::GetIconPath(
    const std::string& app_id,
    ui::ScaleFactor scale_factor) const {
  return ToIconPath(GetAppPath(app_id), scale_factor);
}

bool ArcAppListPrefs::IsIconRequestRecorded(
    const std::string& app_id,
    ui::ScaleFactor scale_factor) const {
  const auto iter = request_icon_recorded_.find(app_id);
  if (iter == request_icon_recorded_.end())
    return false;
  return iter->second & (1 << scale_factor);
}

void ArcAppListPrefs::MaybeRemoveIconRequestRecord(const std::string& app_id) {
  request_icon_recorded_.erase(app_id);
}

void ArcAppListPrefs::ClearIconRequestRecord() {
  request_icon_recorded_.clear();
}

void ArcAppListPrefs::RequestIcon(const std::string& app_id,
                                  ui::ScaleFactor scale_factor) {
  DCHECK_NE(app_id, arc::kPlayStoreAppId);

  // ArcSessionManager can be terminated during test tear down, before callback
  // into this function.
  // TODO(victorhsieh): figure out the best way/place to handle this situation.
  if (arc::ArcSessionManager::Get() == nullptr)
    return;

  if (!IsRegistered(app_id)) {
    VLOG(2) << "Request to load icon for non-registered app: " << app_id << ".";
    return;
  }

  // In case app is not ready, recorded request will be send to ARC when app
  // becomes ready.
  // This record will prevent ArcAppIcon from resending request to ARC for app
  // icon when icon file decode failure is suffered in case app sends bad icon.
  request_icon_recorded_[app_id] |= (1 << scale_factor);

  if (!ready_apps_.count(app_id))
    return;

  if (!app_connection_holder_->IsConnected()) {
    // AppInstance should be ready since we have app_id in ready_apps_. This
    // can happen in browser_tests.
    return;
  }

  std::unique_ptr<AppInfo> app_info = GetApp(app_id);
  if (!app_info) {
    VLOG(2) << "Failed to get app info: " << app_id << ".";
    return;
  }

  if (app_info->icon_resource_id.empty()) {
    auto* app_instance =
        ARC_GET_INSTANCE_FOR_METHOD(app_connection_holder_, RequestAppIcon);
    // Version 0 instance should always be available here because IsConnected()
    // returned true above.
    DCHECK(app_instance);
    app_instance->RequestAppIcon(
        app_info->package_name, app_info->activity,
        static_cast<arc::mojom::ScaleFactor>(scale_factor));
  } else {
    auto* app_instance =
        ARC_GET_INSTANCE_FOR_METHOD(app_connection_holder_, RequestIcon);
    if (!app_instance)
      return;  // The instance version on ARC side was too old.
    app_instance->RequestIcon(
        app_info->icon_resource_id,
        static_cast<arc::mojom::ScaleFactor>(scale_factor),
        base::Bind(&ArcAppListPrefs::OnIcon, base::Unretained(this), app_id,
                   static_cast<arc::mojom::ScaleFactor>(scale_factor)));
  }
}

void ArcAppListPrefs::MaybeRequestIcon(const std::string& app_id,
                                       ui::ScaleFactor scale_factor) {
  if (!IsIconRequestRecorded(app_id, scale_factor))
    RequestIcon(app_id, scale_factor);
}

void ArcAppListPrefs::SetNotificationsEnabled(const std::string& app_id,
                                              bool enabled) {
  if (!IsRegistered(app_id)) {
    VLOG(2) << "Request to set notifications enabled flag for non-registered "
            << "app:" << app_id << ".";
    return;
  }

  std::unique_ptr<AppInfo> app_info = GetApp(app_id);
  if (!app_info) {
    VLOG(2) << "Failed to get app info: " << app_id << ".";
    return;
  }

  // In case app is not ready, defer this request.
  if (!ready_apps_.count(app_id)) {
    SetNotificationsEnabledDeferred(prefs_).Put(app_id, enabled);
    for (auto& observer : observer_list_)
      observer.OnNotificationsEnabledChanged(app_info->package_name, enabled);
    return;
  }

  auto* app_instance = ARC_GET_INSTANCE_FOR_METHOD(app_connection_holder_,
                                                   SetNotificationsEnabled);
  if (!app_instance)
    return;

  SetNotificationsEnabledDeferred(prefs_).Remove(app_id);
  app_instance->SetNotificationsEnabled(app_info->package_name, enabled);
}

void ArcAppListPrefs::AddObserver(Observer* observer) {
  observer_list_.AddObserver(observer);
}

void ArcAppListPrefs::RemoveObserver(Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

bool ArcAppListPrefs::HasObserver(Observer* observer) {
  return observer_list_.HasObserver(observer);
}

base::RepeatingCallback<std::string(const std::string&)>
ArcAppListPrefs::GetAppIdByPackageNameCallback() {
  return base::BindRepeating(
      [](base::WeakPtr<ArcAppListPrefs> self, const std::string& package_name) {
        if (!self)
          return std::string();
        return self->GetAppIdByPackageName(package_name);
      },
      weak_ptr_factory_.GetWeakPtr());
}

std::unique_ptr<ArcAppListPrefs::PackageInfo> ArcAppListPrefs::GetPackage(
    const std::string& package_name) const {
  if (!IsArcAlive() || !IsArcAndroidEnabledForProfile(profile_))
    return nullptr;

  const base::DictionaryValue* package = nullptr;
  const base::DictionaryValue* packages =
      prefs_->GetDictionary(arc::prefs::kArcPackages);
  if (!packages ||
      !packages->GetDictionaryWithoutPathExpansion(package_name, &package))
    return std::unique_ptr<PackageInfo>();

  bool uninstalled = false;
  if (package->GetBoolean(kUninstalled, &uninstalled) && uninstalled)
    return nullptr;

  int32_t package_version = 0;
  int64_t last_backup_android_id = 0;
  int64_t last_backup_time = 0;
  bool should_sync = false;
  bool system = false;
  bool vpn_provider = false;

  GetInt64FromPref(package, kLastBackupAndroidId, &last_backup_android_id);
  GetInt64FromPref(package, kLastBackupTime, &last_backup_time);
  package->GetInteger(kPackageVersion, &package_version);
  package->GetBoolean(kShouldSync, &should_sync);
  package->GetBoolean(kSystem, &system);
  package->GetBoolean(kVPNProvider, &vpn_provider);

  return std::make_unique<PackageInfo>(package_name, package_version,
                                       last_backup_android_id, last_backup_time,
                                       should_sync, system, vpn_provider);
}

std::vector<std::string> ArcAppListPrefs::GetAppIds() const {
  if (arc::ShouldArcAlwaysStart())
    return GetAppIdsNoArcEnabledCheck();

  if (!IsArcAlive() || !IsArcAndroidEnabledForProfile(profile_)) {
    // Default ARC apps available before OptIn.
    std::vector<std::string> ids;
    for (const auto& default_app : default_apps_.app_map()) {
      if (default_apps_.HasApp(default_app.first))
        ids.push_back(default_app.first);
    }
    return ids;
  }
  return GetAppIdsNoArcEnabledCheck();
}

std::vector<std::string> ArcAppListPrefs::GetAppIdsNoArcEnabledCheck() const {
  std::vector<std::string> ids;
  const base::DictionaryValue* apps =
      prefs_->GetDictionary(arc::prefs::kArcApps);
  DCHECK(apps);

  // crx_file::id_util is de-facto utility for id generation.
  for (base::DictionaryValue::Iterator app_id(*apps); !app_id.IsAtEnd();
       app_id.Advance()) {
    if (!crx_file::id_util::IdIsValid(app_id.key()))
      continue;

    ids.push_back(app_id.key());
  }

  return ids;
}

std::unique_ptr<ArcAppListPrefs::AppInfo> ArcAppListPrefs::GetApp(
    const std::string& app_id) const {
  // Information for default app is available before ARC enabled.
  if ((!IsArcAlive() || !IsArcAndroidEnabledForProfile(profile_)) &&
      !default_apps_.HasApp(app_id))
    return std::unique_ptr<AppInfo>();

  const base::DictionaryValue* app = nullptr;
  const base::DictionaryValue* apps =
      prefs_->GetDictionary(arc::prefs::kArcApps);
  if (!apps || !apps->GetDictionaryWithoutPathExpansion(app_id, &app))
    return std::unique_ptr<AppInfo>();

  std::string name;
  std::string package_name;
  std::string activity;
  std::string intent_uri;
  std::string icon_resource_id;
  bool sticky = false;
  bool notifications_enabled = true;
  bool shortcut = false;
  bool launchable = true;
  app->GetString(kName, &name);
  app->GetString(kPackageName, &package_name);
  app->GetString(kActivity, &activity);
  app->GetString(kIntentUri, &intent_uri);
  app->GetString(kIconResourceId, &icon_resource_id);
  app->GetBoolean(kSticky, &sticky);
  app->GetBoolean(kNotificationsEnabled, &notifications_enabled);
  app->GetBoolean(kShortcut, &shortcut);
  app->GetBoolean(kLaunchable, &launchable);

  DCHECK(!name.empty());
  DCHECK(!shortcut || activity.empty());
  DCHECK(!shortcut || !intent_uri.empty());

  int64_t last_launch_time_internal = 0;
  base::Time last_launch_time;
  if (GetInt64FromPref(app, kLastLaunchTime, &last_launch_time_internal)) {
    last_launch_time = base::Time::FromInternalValue(last_launch_time_internal);
  }

  bool deferred;
  if (SetNotificationsEnabledDeferred(prefs_).Get(app_id, &deferred))
    notifications_enabled = deferred;

  return std::make_unique<AppInfo>(
      name, package_name, activity, intent_uri, icon_resource_id,
      last_launch_time, GetInstallTime(app_id), sticky, notifications_enabled,
      ready_apps_.count(app_id) > 0,
      launchable && arc::ShouldShowInLauncher(app_id), shortcut, launchable);
}

bool ArcAppListPrefs::IsRegistered(const std::string& app_id) const {
  if ((!IsArcAlive() || !IsArcAndroidEnabledForProfile(profile_)) &&
      !default_apps_.HasApp(app_id))
    return false;

  const base::DictionaryValue* app = nullptr;
  const base::DictionaryValue* apps =
      prefs_->GetDictionary(arc::prefs::kArcApps);
  return apps && apps->GetDictionaryWithoutPathExpansion(app_id, &app);
}

bool ArcAppListPrefs::IsDefault(const std::string& app_id) const {
  return default_apps_.HasApp(app_id);
}

bool ArcAppListPrefs::IsOem(const std::string& app_id) const {
  const ArcDefaultAppList::AppInfo* app_info = default_apps_.GetApp(app_id);
  return app_info && app_info->oem;
}

bool ArcAppListPrefs::IsShortcut(const std::string& app_id) const {
  std::unique_ptr<ArcAppListPrefs::AppInfo> app_info = GetApp(app_id);
  return app_info && app_info->shortcut;
}

void ArcAppListPrefs::SetLastLaunchTime(const std::string& app_id) {
  if (!IsRegistered(app_id)) {
    NOTREACHED();
    return;
  }

  // Usage time on hidden should not be tracked.
  if (!arc::ShouldShowInLauncher(app_id))
    return;

  const base::Time time = base::Time::Now();
  ScopedArcPrefUpdate update(prefs_, app_id, arc::prefs::kArcApps);
  base::DictionaryValue* app_dict = update.Get();
  const std::string string_value = base::Int64ToString(time.ToInternalValue());
  app_dict->SetString(kLastLaunchTime, string_value);

  for (auto& observer : observer_list_)
    observer.OnAppLastLaunchTimeUpdated(app_id);

  if (first_launch_app_request_) {
    first_launch_app_request_ = false;
    // UI Shown time may not be set in unit tests.
    const user_manager::UserManager* user_manager =
        user_manager::UserManager::Get();
    if (arc::ArcSessionManager::Get()->is_directly_started() &&
        !user_manager->IsLoggedInAsKioskApp() &&
        !user_manager->IsLoggedInAsArcKioskApp() &&
        !chromeos::UserSessionManager::GetInstance()
             ->ui_shown_time()
             .is_null()) {
      UMA_HISTOGRAM_CUSTOM_TIMES(
          "Arc.FirstAppLaunchRequest.TimeDelta",
          time - chromeos::UserSessionManager::GetInstance()->ui_shown_time(),
          base::TimeDelta::FromSeconds(1), base::TimeDelta::FromMinutes(2), 20);
    }
  }
}

void ArcAppListPrefs::DisableAllApps() {
  std::unordered_set<std::string> old_ready_apps;
  old_ready_apps.swap(ready_apps_);
  for (auto& app_id : old_ready_apps)
    NotifyAppReadyChanged(app_id, false);
}

void ArcAppListPrefs::NotifyRegisteredApps() {
  if (apps_restored_)
    return;

  DCHECK(ready_apps_.empty());
  std::vector<std::string> app_ids = GetAppIdsNoArcEnabledCheck();
  for (const auto& app_id : app_ids) {
    std::unique_ptr<AppInfo> app_info = GetApp(app_id);
    if (!app_info) {
      NOTREACHED();
      continue;
    }

    // Default apps are reported earlier.
    if (tracked_apps_.insert(app_id).second) {
      for (auto& observer : observer_list_)
        observer.OnAppRegistered(app_id, *app_info);
    }
  }

  apps_restored_ = true;
}

void ArcAppListPrefs::RemoveAllAppsAndPackages() {
  std::vector<std::string> app_ids = GetAppIdsNoArcEnabledCheck();
  for (const auto& app_id : app_ids) {
    if (!default_apps_.HasApp(app_id)) {
      RemoveApp(app_id);
    } else {
      if (ready_apps_.count(app_id)) {
        ready_apps_.erase(app_id);
        NotifyAppReadyChanged(app_id, false);
      }
    }
  }
  DCHECK(ready_apps_.empty());

  const std::vector<std::string> package_names_to_remove =
      GetPackagesFromPrefs(false /* check_arc_alive */, true /* installed */);
  for (const auto& package_name : package_names_to_remove) {
    if (!default_apps_.HasPackage(package_name))
      RemovePackageFromPrefs(prefs_, package_name);
    for (auto& observer : observer_list_)
      observer.OnPackageRemoved(package_name, false);
  }
}

void ArcAppListPrefs::OnArcPlayStoreEnabledChanged(bool enabled) {
  SetDefaultAppsFilterLevel();

  // TODO(victorhsieh): Implement opt-in and opt-out.
  if (arc::ShouldArcAlwaysStart())
    return;

  if (enabled)
    NotifyRegisteredApps();
  else
    RemoveAllAppsAndPackages();
}

void ArcAppListPrefs::SetDefaultAppsFilterLevel() {
  // There is no a blacklisting mechanism for Android apps. Until there is
  // one, we have no option but to ban all pre-installed apps on Android side.
  // Match this requirement and don't show pre-installed apps for managed users
  // in app list.
  if (arc::policy_util::IsAccountManaged(profile_)) {
    default_apps_.set_filter_level(
        arc::IsArcPlayStoreEnabledForProfile(profile_)
            ? ArcDefaultAppList::FilterLevel::OPTIONAL_APPS
            : ArcDefaultAppList::FilterLevel::ALL);
  } else {
    default_apps_.set_filter_level(ArcDefaultAppList::FilterLevel::NOTHING);
  }

  // Register default apps if it was not registered before.
  RegisterDefaultApps();
}

void ArcAppListPrefs::OnDefaultAppsReady() {
  // Apply uninstalled packages now.
  const std::vector<std::string> uninstalled_package_names =
      GetPackagesFromPrefs(false /* check_arc_alive */, false /* installed */);
  for (const auto& uninstalled_package_name : uninstalled_package_names)
    default_apps_.MaybeMarkPackageUninstalled(uninstalled_package_name, true);

  SetDefaultAppsFilterLevel();

  default_apps_ready_ = true;
  if (!default_apps_ready_callback_.is_null())
    default_apps_ready_callback_.Run();

  StartPrefs();
}

void ArcAppListPrefs::RegisterDefaultApps() {
  // Report default apps first, note, app_map includes uninstalled and filtered
  // out apps as well.
  for (const auto& default_app : default_apps_.app_map()) {
    const std::string& app_id = default_app.first;
    if (!default_apps_.HasApp(app_id))
      continue;
    // Skip already tracked app.
    if (tracked_apps_.count(app_id)) {
      // Icon should be already taken from the cache. Play Store icon is loaded
      // from internal resources.
      if (ready_apps_.count(app_id) || app_id == arc::kPlayStoreAppId)
        continue;
      // Notify that icon is ready for default app.
      for (auto& observer : observer_list_) {
        for (ui::ScaleFactor scale_factor : ui::GetSupportedScaleFactors())
          observer.OnAppIconUpdated(app_id, scale_factor);
      }
      continue;
    }

    const ArcDefaultAppList::AppInfo& app_info = *default_app.second.get();
    AddAppAndShortcut(false /* app_ready */, app_info.name,
                      app_info.package_name, app_info.activity,
                      std::string() /* intent_uri */,
                      std::string() /* icon_resource_id */, false /* sticky */,
                      false /* notifications_enabled */, false /* shortcut */,
                      true /* launchable */);
  }
}

base::Value* ArcAppListPrefs::GetPackagePrefs(const std::string& package_name,
                                              const std::string& key) {
  if (!GetPackage(package_name)) {
    LOG(ERROR) << package_name << " can not be found.";
    return nullptr;
  }
  ScopedArcPrefUpdate update(prefs_, package_name, arc::prefs::kArcPackages);
  return update.Get()->FindKey(key);
}

void ArcAppListPrefs::SetPackagePrefs(const std::string& package_name,
                                      const std::string& key,
                                      base::Value value) {
  if (!GetPackage(package_name)) {
    LOG(ERROR) << package_name << " can not be found.";
    return;
  }
  ScopedArcPrefUpdate update(prefs_, package_name, arc::prefs::kArcPackages);
  update.Get()->SetKey(key, std::move(value));
}

void ArcAppListPrefs::SetDefaltAppsReadyCallback(base::Closure callback) {
  DCHECK(!callback.is_null());
  DCHECK(default_apps_ready_callback_.is_null());
  default_apps_ready_callback_ = callback;
  if (default_apps_ready_)
    default_apps_ready_callback_.Run();
}

void ArcAppListPrefs::SimulateDefaultAppAvailabilityTimeoutForTesting() {
  if (!detect_default_app_availability_timeout_.IsRunning())
    return;
  detect_default_app_availability_timeout_.Stop();
  DetectDefaultAppAvailability();
}

void ArcAppListPrefs::OnConnectionReady() {
  // Note, sync_service_ may be nullptr in testing.
  sync_service_ = arc::ArcPackageSyncableService::Get(profile_);
  is_initialized_ = false;

  if (!app_list_refreshed_callback_.is_null())
    std::move(app_list_refreshed_callback_).Run();
}

void ArcAppListPrefs::OnConnectionClosed() {
  DisableAllApps();
  installing_packages_count_ = 0;
  default_apps_installations_.clear();
  detect_default_app_availability_timeout_.Stop();
  ClearIconRequestRecord();

  if (sync_service_) {
    sync_service_->StopSyncing(syncer::ARC_PACKAGE);
    sync_service_ = nullptr;
  }

  is_initialized_ = false;
  package_list_initial_refreshed_ = false;
  app_list_refreshed_callback_.Reset();
}

void ArcAppListPrefs::HandleTaskCreated(const base::Optional<std::string>& name,
                                        const std::string& package_name,
                                        const std::string& activity) {
  DCHECK(IsArcAndroidEnabledForProfile(profile_));
  const std::string app_id = GetAppId(package_name, activity);
  if (IsRegistered(app_id)) {
    SetLastLaunchTime(app_id);
  } else {
    // Create runtime app entry that is valid for the current user session. This
    // entry is not shown in App Launcher and only required for shelf
    // integration.
    AddAppAndShortcut(true /* app_ready */, name.has_value() ? *name : "",
                      package_name, activity, std::string() /* intent_uri */,
                      std::string() /* icon_resource_id */, false /* sticky */,
                      false /* notifications_enabled */, false /* shortcut */,
                      false /* launchable */);
  }
}

void ArcAppListPrefs::AddAppAndShortcut(bool app_ready,
                                        const std::string& name,
                                        const std::string& package_name,
                                        const std::string& activity,
                                        const std::string& intent_uri,
                                        const std::string& icon_resource_id,
                                        const bool sticky,
                                        const bool notifications_enabled,
                                        const bool shortcut,
                                        const bool launchable) {
  const std::string app_id = shortcut ? GetAppId(package_name, intent_uri)
                                      : GetAppId(package_name, activity);

  // Do not add Play Store app for Public Session and Kiosk modes.
  if (app_id == arc::kPlayStoreAppId && arc::IsRobotAccountMode())
    return;

  std::string updated_name = name;
  // Add "(beta)" string to Play Store. See crbug.com/644576 for details.
  if (app_id == arc::kPlayStoreAppId)
    updated_name = l10n_util::GetStringUTF8(IDS_ARC_PLAYSTORE_ICON_TITLE_BETA);

  const bool was_tracked = tracked_apps_.count(app_id);
  if (was_tracked) {
    std::unique_ptr<ArcAppListPrefs::AppInfo> app_old_info = GetApp(app_id);
    DCHECK(app_old_info);
    DCHECK(launchable);
    if (updated_name != app_old_info->name) {
      for (auto& observer : observer_list_)
        observer.OnAppNameUpdated(app_id, updated_name);
    }
  }

  ScopedArcPrefUpdate update(prefs_, app_id, arc::prefs::kArcApps);
  base::DictionaryValue* app_dict = update.Get();
  app_dict->SetString(kName, updated_name);
  app_dict->SetString(kPackageName, package_name);
  app_dict->SetString(kActivity, activity);
  app_dict->SetString(kIntentUri, intent_uri);
  app_dict->SetString(kIconResourceId, icon_resource_id);
  app_dict->SetBoolean(kSticky, sticky);
  app_dict->SetBoolean(kNotificationsEnabled, notifications_enabled);
  app_dict->SetBoolean(kShortcut, shortcut);
  app_dict->SetBoolean(kLaunchable, launchable);

  // Note the install time is the first time the Chrome OS sees the app, not the
  // actual install time in Android side.
  if (GetInstallTime(app_id).is_null()) {
    std::string install_time_str =
        base::Int64ToString(base::Time::Now().ToInternalValue());
    app_dict->SetString(kInstallTime, install_time_str);
  }

  const bool was_disabled = ready_apps_.count(app_id) == 0;
  DCHECK(!(!was_disabled && !app_ready));
  if (was_disabled && app_ready)
    ready_apps_.insert(app_id);

  if (was_tracked) {
    if (was_disabled && app_ready)
      NotifyAppReadyChanged(app_id, true);
  } else {
    AppInfo app_info(updated_name, package_name, activity, intent_uri,
                     icon_resource_id, base::Time(), GetInstallTime(app_id),
                     sticky, notifications_enabled, app_ready,
                     launchable && arc::ShouldShowInLauncher(app_id), shortcut,
                     launchable);
    for (auto& observer : observer_list_)
      observer.OnAppRegistered(app_id, app_info);
    tracked_apps_.insert(app_id);
  }

  if (app_ready) {
    int icon_update_mask = 0;
    app_dict->GetInteger(kInvalidatedIcons, &icon_update_mask);
    auto pending_icons = request_icon_recorded_.find(app_id);
    if (pending_icons != request_icon_recorded_.end())
      icon_update_mask |= pending_icons->second;
    for (ui::ScaleFactor scale_factor : ui::GetSupportedScaleFactors()) {
      if (icon_update_mask & (1 << scale_factor))
        RequestIcon(app_id, scale_factor);
    }

    bool deferred_notifications_enabled;
    if (SetNotificationsEnabledDeferred(prefs_).Get(
            app_id, &deferred_notifications_enabled)) {
      SetNotificationsEnabled(app_id, deferred_notifications_enabled);
    }
  }
}

void ArcAppListPrefs::RemoveApp(const std::string& app_id) {
  // Delete cached icon if there is any.
  std::unique_ptr<ArcAppListPrefs::AppInfo> app_info = GetApp(app_id);
  if (app_info && !app_info->icon_resource_id.empty()) {
    arc::RemoveCachedIcon(app_info->icon_resource_id);
  }

  MaybeRemoveIconRequestRecord(app_id);

  // From now, app is not available.
  ready_apps_.erase(app_id);

  // app_id may be released by observers, get the path first. It should be done
  // before removing prefs entry in order not to mix with pre-build default apps
  // files.
  const base::FilePath app_path = GetAppPath(app_id);

  // Remove from prefs.
  DictionaryPrefUpdate update(prefs_, arc::prefs::kArcApps);
  base::DictionaryValue* apps = update.Get();
  const bool removed = apps->Remove(app_id, nullptr);
  DCHECK(removed);

  DCHECK(tracked_apps_.count(app_id));
  for (auto& observer : observer_list_)
    observer.OnAppRemoved(app_id);
  tracked_apps_.erase(app_id);

  // Remove local data on file system.
  base::PostTaskWithTraits(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::Bind(&DeleteAppFolderFromFileThread, app_path));
}

void ArcAppListPrefs::AddOrUpdatePackagePrefs(
    PrefService* prefs, const arc::mojom::ArcPackageInfo& package) {
  DCHECK(IsArcAndroidEnabledForProfile(profile_));
  const std::string& package_name = package.package_name;

  default_apps_.MaybeMarkPackageUninstalled(package_name, false);
  if (package_name.empty()) {
    VLOG(2) << "Package name cannot be empty.";
    return;
  }

  ScopedArcPrefUpdate update(prefs, package_name, arc::prefs::kArcPackages);
  base::DictionaryValue* package_dict = update.Get();
  const std::string id_str =
      base::Int64ToString(package.last_backup_android_id);
  const std::string time_str = base::Int64ToString(package.last_backup_time);

  int old_package_version = -1;
  package_dict->GetInteger(kPackageVersion, &old_package_version);

  package_dict->SetBoolean(kShouldSync, package.sync);
  package_dict->SetInteger(kPackageVersion, package.package_version);
  package_dict->SetString(kLastBackupAndroidId, id_str);
  package_dict->SetString(kLastBackupTime, time_str);
  package_dict->SetBoolean(kSystem, package.system);
  package_dict->SetBoolean(kUninstalled, false);
  package_dict->SetBoolean(kVPNProvider, package.vpn_provider);

  if (old_package_version == -1 ||
      old_package_version == package.package_version) {
    return;
  }

  InvalidatePackageIcons(package_name);
}

void ArcAppListPrefs::RemovePackageFromPrefs(PrefService* prefs,
                                             const std::string& package_name) {
  default_apps_.MaybeMarkPackageUninstalled(package_name, true);
  if (!default_apps_.HasPackage(package_name)) {
    DictionaryPrefUpdate update(prefs, arc::prefs::kArcPackages);
    base::DictionaryValue* packages = update.Get();
    const bool removed = packages->RemoveWithoutPathExpansion(package_name,
                                                              nullptr);
    DCHECK(removed);
  } else {
    ScopedArcPrefUpdate update(prefs, package_name, arc::prefs::kArcPackages);
    base::DictionaryValue* package_dict = update.Get();
    package_dict->SetBoolean(kUninstalled, true);
  }
}

void ArcAppListPrefs::OnAppListRefreshed(
    std::vector<arc::mojom::AppInfoPtr> apps) {
  DCHECK(app_list_refreshed_callback_.is_null());
  if (!app_connection_holder_->IsConnected()) {
    LOG(ERROR) << "App instance is not connected. Delaying app list refresh. "
               << "See b/70566216.";
    app_list_refreshed_callback_ =
        base::BindOnce(&ArcAppListPrefs::OnAppListRefreshed,
                       weak_ptr_factory_.GetWeakPtr(), std::move(apps));
    return;
  }

  DCHECK(IsArcAndroidEnabledForProfile(profile_));
  std::vector<std::string> old_apps = GetAppIds();

  ready_apps_.clear();
  for (const auto& app : apps) {
    AddAppAndShortcut(true /* app_ready */, app->name, app->package_name,
                      app->activity, std::string() /* intent_uri */,
                      std::string() /* icon_resource_id */, app->sticky,
                      app->notifications_enabled, false /* shortcut */,
                      true /* launchable */);
  }

  // Detect removed ARC apps after current refresh.
  for (const auto& app_id : old_apps) {
    if (ready_apps_.count(app_id))
      continue;

    if (IsShortcut(app_id)) {
      // If this is a shortcut, we just mark it as ready.
      ready_apps_.insert(app_id);
      NotifyAppReadyChanged(app_id, true);
    } else {
      // Default apps may not be installed yet at this moment.
      if (!default_apps_.HasApp(app_id))
        RemoveApp(app_id);
    }
  }

  if (!is_initialized_) {
    is_initialized_ = true;

    UMA_HISTOGRAM_COUNTS_1000("Arc.AppsInstalledAtStartup", ready_apps_.size());

    arc::ArcPaiStarter* pai_starter =
        arc::ArcSessionManager::Get()->pai_starter();

    if (pai_starter) {
      pai_starter->AddOnStartCallback(
          base::BindOnce(&ArcAppListPrefs::MaybeSetDefaultAppLoadingTimeout,
                         weak_ptr_factory_.GetWeakPtr()));
    } else {
      MaybeSetDefaultAppLoadingTimeout();
    }
  }
}

void ArcAppListPrefs::DetectDefaultAppAvailability() {
  for (const auto& package : default_apps_.GetActivePackages()) {
    // Check if already installed or installation in progress.
    if (!GetPackage(package) && !default_apps_installations_.count(package))
      HandlePackageRemoved(package);
  }
}

void ArcAppListPrefs::MaybeSetDefaultAppLoadingTimeout() {
  // Find at least one not installed default app package.
  for (const auto& package : default_apps_.GetActivePackages()) {
    if (!GetPackage(package)) {
      detect_default_app_availability_timeout_.Start(FROM_HERE,
          kDetectDefaultAppAvailabilityTimeout, this,
          &ArcAppListPrefs::DetectDefaultAppAvailability);
      break;
    }
  }
}

void ArcAppListPrefs::AddApp(const arc::mojom::AppInfo& app_info) {
  if ((app_info.name.empty() || app_info.package_name.empty() ||
       app_info.activity.empty())) {
    VLOG(2) << "App Name, package name, and activity cannot be empty.";
    return;
  }

  AddAppAndShortcut(true /* app_ready */, app_info.name, app_info.package_name,
                    app_info.activity, std::string() /* intent_uri */,
                    std::string() /* icon_resource_id */, app_info.sticky,
                    app_info.notifications_enabled, false /* shortcut */,
                    true /* launchable */);
}

void ArcAppListPrefs::OnAppAddedDeprecated(arc::mojom::AppInfoPtr app) {
  AddApp(*app);
}

void ArcAppListPrefs::InvalidateAppIcons(const std::string& app_id) {
  // Ignore Play Store app since we provide its icon in Chrome resources.
  if (app_id == arc::kPlayStoreAppId)
    return;

  // Clean up previous icon records. They may refer to outdated icons.
  MaybeRemoveIconRequestRecord(app_id);

  {
    ScopedArcPrefUpdate update(prefs_, app_id, arc::prefs::kArcApps);
    base::DictionaryValue* app_dict = update.Get();
    app_dict->SetInteger(kInvalidatedIcons,
                         invalidated_icon_scale_factor_mask_);
  }

  for (ui::ScaleFactor scale_factor : ui::GetSupportedScaleFactors())
    MaybeRequestIcon(app_id, scale_factor);
}

void ArcAppListPrefs::InvalidatePackageIcons(const std::string& package_name) {
  for (const std::string& app_id : GetAppsForPackage(package_name))
    InvalidateAppIcons(app_id);
}

void ArcAppListPrefs::OnPackageAppListRefreshed(
    const std::string& package_name,
    std::vector<arc::mojom::AppInfoPtr> apps) {
  if (package_name.empty()) {
    VLOG(2) << "Package name cannot be empty.";
    return;
  }

  std::unordered_set<std::string> apps_to_remove =
      GetAppsForPackage(package_name);
  default_apps_.MaybeMarkPackageUninstalled(package_name, false);

  for (const auto& app : apps) {
    const std::string app_id = GetAppId(app->package_name, app->activity);
    apps_to_remove.erase(app_id);

    AddApp(*app);
  }

  for (const auto& app_id : apps_to_remove)
    RemoveApp(app_id);
}

void ArcAppListPrefs::OnInstallShortcut(arc::mojom::ShortcutInfoPtr shortcut) {
  if ((shortcut->name.empty() || shortcut->intent_uri.empty())) {
    VLOG(2) << "Shortcut Name, and intent_uri cannot be empty.";
    return;
  }

  AddAppAndShortcut(true /* app_ready */, shortcut->name,
                    shortcut->package_name, std::string() /* activity */,
                    shortcut->intent_uri, shortcut->icon_resource_id,
                    false /* sticky */, false /* notifications_enabled */,
                    true /* shortcut */, true /* launchable */);
}

void ArcAppListPrefs::OnUninstallShortcut(const std::string& package_name,
                                          const std::string& intent_uri) {
  std::vector<std::string> shortcuts_to_remove;
  const base::DictionaryValue* apps =
      prefs_->GetDictionary(arc::prefs::kArcApps);
  for (base::DictionaryValue::Iterator app_it(*apps); !app_it.IsAtEnd();
       app_it.Advance()) {
    const base::Value* value = &app_it.value();
    const base::DictionaryValue* app;
    bool shortcut;
    std::string installed_package_name;
    std::string installed_intent_uri;
    if (!value->GetAsDictionary(&app) ||
        !app->GetBoolean(kShortcut, &shortcut) ||
        !app->GetString(kPackageName, &installed_package_name) ||
        !app->GetString(kIntentUri, &installed_intent_uri)) {
      VLOG(2) << "Failed to extract information for " << app_it.key() << ".";
      continue;
    }

    if (!shortcut || installed_package_name != package_name ||
        installed_intent_uri != intent_uri) {
      continue;
    }

    shortcuts_to_remove.push_back(app_it.key());
  }

  for (const auto& shortcut_id : shortcuts_to_remove)
    RemoveApp(shortcut_id);
}

std::unordered_set<std::string> ArcAppListPrefs::GetAppsForPackage(
    const std::string& package_name) const {
  return GetAppsAndShortcutsForPackage(package_name,
                                       false /* include_shortcuts */);
}

std::unordered_set<std::string> ArcAppListPrefs::GetAppsAndShortcutsForPackage(
    const std::string& package_name,
    bool include_shortcuts) const {
  std::unordered_set<std::string> app_set;
  const base::DictionaryValue* apps =
      prefs_->GetDictionary(arc::prefs::kArcApps);
  for (base::DictionaryValue::Iterator app_it(*apps); !app_it.IsAtEnd();
       app_it.Advance()) {
    const base::Value* value = &app_it.value();
    const base::DictionaryValue* app;
    if (!value->GetAsDictionary(&app)) {
      NOTREACHED();
      continue;
    }

    std::string app_package;
    if (!app->GetString(kPackageName, &app_package)) {
      NOTREACHED();
      continue;
    }

    if (package_name != app_package)
      continue;

    if (!include_shortcuts) {
      bool shortcut = false;
      if (app->GetBoolean(kShortcut, &shortcut) && shortcut)
        continue;
    }

    app_set.insert(app_it.key());
  }

  return app_set;
}

void ArcAppListPrefs::HandlePackageRemoved(const std::string& package_name) {
  DCHECK(IsArcAndroidEnabledForProfile(profile_));
  const std::unordered_set<std::string> apps_to_remove =
      GetAppsAndShortcutsForPackage(package_name, true /* include_shortcuts */);
  for (const auto& app_id : apps_to_remove)
    RemoveApp(app_id);

  RemovePackageFromPrefs(prefs_, package_name);
}

void ArcAppListPrefs::OnPackageRemoved(const std::string& package_name) {
  HandlePackageRemoved(package_name);

  for (auto& observer : observer_list_)
    observer.OnPackageRemoved(package_name, true);
}

void ArcAppListPrefs::OnAppIcon(const std::string& package_name,
                                const std::string& activity,
                                arc::mojom::ScaleFactor scale_factor,
                                const std::vector<uint8_t>& icon_png_data) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK_NE(0u, icon_png_data.size());

  std::string app_id = GetAppId(package_name, activity);
  if (!IsRegistered(app_id)) {
    VLOG(2) << "Request to update icon for non-registered app: " << app_id;
    return;
  }

  InstallIcon(app_id, static_cast<ui::ScaleFactor>(scale_factor),
              icon_png_data);
}

void ArcAppListPrefs::OnIcon(const std::string& app_id,
                             arc::mojom::ScaleFactor scale_factor,
                             const std::vector<uint8_t>& icon_png_data) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK_NE(0u, icon_png_data.size());

  if (!IsRegistered(app_id)) {
    VLOG(2) << "Request to update icon for non-registered app: " << app_id;
    return;
  }

  InstallIcon(app_id, static_cast<ui::ScaleFactor>(scale_factor),
              icon_png_data);
}

void ArcAppListPrefs::OnTaskCreated(int32_t task_id,
                                    const std::string& package_name,
                                    const std::string& activity,
                                    const base::Optional<std::string>& name,
                                    const base::Optional<std::string>& intent) {
  HandleTaskCreated(name, package_name, activity);
  for (auto& observer : observer_list_) {
    observer.OnTaskCreated(task_id,
                           package_name,
                           activity,
                           intent.value_or(std::string()));
  }
}

void ArcAppListPrefs::OnTaskDescriptionUpdated(
    int32_t task_id,
    const std::string& label,
    const std::vector<uint8_t>& icon_png_data) {
  for (auto& observer : observer_list_)
    observer.OnTaskDescriptionUpdated(task_id, label, icon_png_data);
}

void ArcAppListPrefs::OnTaskDestroyed(int32_t task_id) {
  for (auto& observer : observer_list_)
    observer.OnTaskDestroyed(task_id);
}

void ArcAppListPrefs::OnTaskSetActive(int32_t task_id) {
  for (auto& observer : observer_list_)
    observer.OnTaskSetActive(task_id);
}

void ArcAppListPrefs::OnNotificationsEnabledChanged(
    const std::string& package_name,
    bool enabled) {
  const base::DictionaryValue* apps =
      prefs_->GetDictionary(arc::prefs::kArcApps);
  for (base::DictionaryValue::Iterator app(*apps); !app.IsAtEnd();
       app.Advance()) {
    const base::DictionaryValue* app_dict;
    std::string app_package_name;
    if (!app.value().GetAsDictionary(&app_dict) ||
        !app_dict->GetString(kPackageName, &app_package_name)) {
      NOTREACHED();
      continue;
    }
    if (app_package_name != package_name) {
      continue;
    }
    ScopedArcPrefUpdate update(prefs_, app.key(), arc::prefs::kArcApps);
    base::DictionaryValue* updateing_app_dict = update.Get();
    updateing_app_dict->SetBoolean(kNotificationsEnabled, enabled);
  }
  for (auto& observer : observer_list_)
    observer.OnNotificationsEnabledChanged(package_name, enabled);
}

bool ArcAppListPrefs::IsUnknownPackage(const std::string& package_name) const {
  return !GetPackage(package_name) && sync_service_ &&
         !sync_service_->IsPackageSyncing(package_name);
}

void ArcAppListPrefs::OnPackageAdded(
    arc::mojom::ArcPackageInfoPtr package_info) {
  DCHECK(IsArcAndroidEnabledForProfile(profile_));

  AddOrUpdatePackagePrefs(prefs_, *package_info);
  for (auto& observer : observer_list_)
    observer.OnPackageInstalled(*package_info);
}

void ArcAppListPrefs::OnPackageModified(
    arc::mojom::ArcPackageInfoPtr package_info) {
  DCHECK(IsArcAndroidEnabledForProfile(profile_));
  AddOrUpdatePackagePrefs(prefs_, *package_info);
  for (auto& observer : observer_list_)
    observer.OnPackageModified(*package_info);
}

void ArcAppListPrefs::OnPackageListRefreshed(
    std::vector<arc::mojom::ArcPackageInfoPtr> packages) {
  DCHECK(IsArcAndroidEnabledForProfile(profile_));

  const std::vector<std::string> old_packages(GetPackagesFromPrefs());
  std::unordered_set<std::string> current_packages;

  for (const auto& package : packages) {
    AddOrUpdatePackagePrefs(prefs_, *package);
    current_packages.insert((*package).package_name);
  }

  for (const auto& package_name : old_packages) {
    if (!current_packages.count(package_name))
      RemovePackageFromPrefs(prefs_, package_name);
  }

  package_list_initial_refreshed_ = true;
  for (auto& observer : observer_list_)
    observer.OnPackageListInitialRefreshed();
}

std::vector<std::string> ArcAppListPrefs::GetPackagesFromPrefs() const {
  return GetPackagesFromPrefs(true /* check_arc_alive */, true /* installed */);
}

std::vector<std::string> ArcAppListPrefs::GetPackagesFromPrefs(
    bool check_arc_alive,
    bool installed) const {
  std::vector<std::string> packages;
  if (check_arc_alive &&
      (!IsArcAlive() || !IsArcAndroidEnabledForProfile(profile_))) {
    return packages;
  }

  const base::DictionaryValue* package_prefs =
      prefs_->GetDictionary(arc::prefs::kArcPackages);
  for (base::DictionaryValue::Iterator package(*package_prefs);
       !package.IsAtEnd(); package.Advance()) {
    const base::DictionaryValue* package_info;
    if (!package.value().GetAsDictionary(&package_info)) {
      NOTREACHED();
      continue;
    }

    bool uninstalled = false;
    package_info->GetBoolean(kUninstalled, &uninstalled);
    if (installed != !uninstalled)
      continue;

    packages.push_back(package.key());
  }

  return packages;
}

base::Time ArcAppListPrefs::GetInstallTime(const std::string& app_id) const {
  const base::DictionaryValue* app = nullptr;
  const base::DictionaryValue* apps =
      prefs_->GetDictionary(arc::prefs::kArcApps);
  if (!apps || !apps->GetDictionaryWithoutPathExpansion(app_id, &app))
    return base::Time();

  std::string install_time_str;
  if (!app->GetString(kInstallTime, &install_time_str))
    return base::Time();

  int64_t install_time_i64;
  if (!base::StringToInt64(install_time_str, &install_time_i64))
    return base::Time();
  return base::Time::FromInternalValue(install_time_i64);
}

void ArcAppListPrefs::InstallIcon(const std::string& app_id,
                                  ui::ScaleFactor scale_factor,
                                  const std::vector<uint8_t>& content_png) {
  const base::FilePath icon_path = GetIconPath(app_id, scale_factor);
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::Bind(&InstallIconFromFileThread, icon_path, content_png),
      base::Bind(&ArcAppListPrefs::OnIconInstalled,
                 weak_ptr_factory_.GetWeakPtr(), app_id, scale_factor));
}

void ArcAppListPrefs::OnIconInstalled(const std::string& app_id,
                                      ui::ScaleFactor scale_factor,
                                      bool install_succeed) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!install_succeed)
    return;

  ScopedArcPrefUpdate update(prefs_, app_id, arc::prefs::kArcApps);
  int invalidated_icon_mask = 0;
  base::DictionaryValue* app_dict = update.Get();
  app_dict->GetInteger(kInvalidatedIcons, &invalidated_icon_mask);
  invalidated_icon_mask &= (~(1 << scale_factor));
  app_dict->SetInteger(kInvalidatedIcons, invalidated_icon_mask);

  for (auto& observer : observer_list_)
    observer.OnAppIconUpdated(app_id, scale_factor);
}

void ArcAppListPrefs::OnInstallationStarted(
    const base::Optional<std::string>& package_name) {
  ++installing_packages_count_;

  if (!package_name.has_value())
    return;

  if (default_apps_.HasPackage(*package_name))
    default_apps_installations_.insert(*package_name);

  for (auto& observer : observer_list_)
    observer.OnInstallationStarted(*package_name);
}

void ArcAppListPrefs::OnInstallationFinished(
    arc::mojom::InstallationResultPtr result) {
  if (result && default_apps_.HasPackage(result->package_name)) {
    default_apps_installations_.erase(result->package_name);

    if (!result->success && !GetPackage(result->package_name))
      HandlePackageRemoved(result->package_name);
  }

  if (result) {
    for (auto& observer : observer_list_)
      observer.OnInstallationFinished(result->package_name, result->success);
  }

  if (!installing_packages_count_) {
    VLOG(2) << "Received unexpected installation finished event";
    return;
  }
  --installing_packages_count_;
}

void ArcAppListPrefs::NotifyAppReadyChanged(const std::string& app_id,
                                            bool ready) {
  for (auto& observer : observer_list_)
    observer.OnAppReadyChanged(app_id, ready);
}

ArcAppListPrefs::AppInfo::AppInfo(const std::string& name,
                                  const std::string& package_name,
                                  const std::string& activity,
                                  const std::string& intent_uri,
                                  const std::string& icon_resource_id,
                                  const base::Time& last_launch_time,
                                  const base::Time& install_time,
                                  bool sticky,
                                  bool notifications_enabled,
                                  bool ready,
                                  bool showInLauncher,
                                  bool shortcut,
                                  bool launchable)
    : name(name),
      package_name(package_name),
      activity(activity),
      intent_uri(intent_uri),
      icon_resource_id(icon_resource_id),
      last_launch_time(last_launch_time),
      install_time(install_time),
      sticky(sticky),
      notifications_enabled(notifications_enabled),
      ready(ready),
      showInLauncher(showInLauncher),
      shortcut(shortcut),
      launchable(launchable) {}

// Need to add explicit destructor for chromium style checker error:
// Complex class/struct needs an explicit out-of-line destructor
ArcAppListPrefs::AppInfo::~AppInfo() {}

ArcAppListPrefs::PackageInfo::PackageInfo(const std::string& package_name,
                                          int32_t package_version,
                                          int64_t last_backup_android_id,
                                          int64_t last_backup_time,
                                          bool should_sync,
                                          bool system,
                                          bool vpn_provider)
    : package_name(package_name),
      package_version(package_version),
      last_backup_android_id(last_backup_android_id),
      last_backup_time(last_backup_time),
      should_sync(should_sync),
      system(system),
      vpn_provider(vpn_provider) {}
