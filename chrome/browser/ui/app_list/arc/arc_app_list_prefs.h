// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_ARC_ARC_APP_LIST_PREFS_H_
#define CHROME_BROWSER_UI_APP_LIST_ARC_ARC_APP_LIST_PREFS_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "chrome/browser/chromeos/arc/arc_session_manager.h"
#include "chrome/browser/ui/app_list/arc/arc_default_app_list.h"
#include "components/arc/common/app.mojom.h"
#include "components/arc/connection_observer.h"
#include "components/keyed_service/core/keyed_service.h"
#include "ui/base/layout.h"

class PrefService;
class Profile;

namespace arc {
class ArcPackageSyncableService;
template <typename InstanceType, typename HostType>
class ConnectionHolder;
}  // namespace arc

namespace content {
class BrowserContext;
}  // namespace content

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

// Declares shareable ARC app specific preferences, that keep information
// about app attributes (name, package_name, activity) and its state. This
// information is used to pre-create non-ready app items while ARC bridge
// service is not ready to provide information about available ARC apps.
// NOTE: ArcAppListPrefs is only created for the primary user.
class ArcAppListPrefs : public KeyedService,
                        public arc::mojom::AppHost,
                        public arc::ConnectionObserver<arc::mojom::AppInstance>,
                        public arc::ArcSessionManager::Observer,
                        public ArcDefaultAppList::Delegate {
 public:
  struct AppInfo {
    AppInfo(const std::string& name,
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
            bool launchable);
    ~AppInfo();

    std::string name;
    std::string package_name;
    std::string activity;
    std::string intent_uri;
    std::string icon_resource_id;
    base::Time last_launch_time;
    base::Time install_time;
    bool sticky;
    bool notifications_enabled;
    bool ready;
    bool showInLauncher;
    bool shortcut;
    bool launchable;
  };

  struct PackageInfo {
    PackageInfo(const std::string& package_name,
                int32_t package_version,
                int64_t last_backup_android_id,
                int64_t last_backup_time,
                bool should_sync,
                bool system,
                bool vpn_provider);

    std::string package_name;
    int32_t package_version;
    int64_t last_backup_android_id;
    int64_t last_backup_time;
    bool should_sync;
    bool system;
    bool vpn_provider;
  };

  class Observer {
   public:
    // Notifies an observer that new app is registered.
    virtual void OnAppRegistered(const std::string& app_id,
                                 const AppInfo& app_info) {}
    // Notifies an observer that app ready state has been changed.
    virtual void OnAppReadyChanged(const std::string& id, bool ready) {}
    // Notifies an observer that app was removed.
    virtual void OnAppRemoved(const std::string& id) {}
    // Notifies an observer that app icon has been installed or updated.
    virtual void OnAppIconUpdated(const std::string& id,
                                  ui::ScaleFactor scale_factor) {}
    // Notifies an observer that the name of an app has changed.
    virtual void OnAppNameUpdated(const std::string& id,
                                  const std::string& name) {}
    // Notifies an observer that the last launch time of an app has changed.
    virtual void OnAppLastLaunchTimeUpdated(const std::string& app_id) {}
    // Notifies that task has been created and provides information about
    // initial activity.
    virtual void OnTaskCreated(int32_t task_id,
                               const std::string& package_name,
                               const std::string& activity,
                               const std::string& intent) {}
    // Notifies that task description has been updated.
    virtual void OnTaskDescriptionUpdated(
        int32_t task_id,
        const std::string& label,
        const std::vector<uint8_t>& icon_png_data) {}
    // Notifies that task has been destroyed.
    virtual void OnTaskDestroyed(int32_t task_id) {}
    // Notifies that task has been activated and moved to the front.
    virtual void OnTaskSetActive(int32_t task_id) {}

    virtual void OnNotificationsEnabledChanged(
        const std::string& package_name, bool enabled) {}
    // Notifies that package has been installed.
    virtual void OnPackageInstalled(
        const arc::mojom::ArcPackageInfo& package_info) {}
    // Notifies that package has been modified.
    virtual void OnPackageModified(
        const arc::mojom::ArcPackageInfo& package_info) {}
    // Notifies that package has been removed from the system. |uninstalled| is
    // set to true in case package was uninstalled by user or sync.
    // OnPackageRemoved is called for each active package with |uninstalled| set
    // to false in case the user opts out the Play Store.
    virtual void OnPackageRemoved(const std::string& package_name,
                                  bool uninstalled) {}
    // Notifies sync date type controller the model is ready to start.
    virtual void OnPackageListInitialRefreshed() {}

    // Notifies that installation of package started.
    virtual void OnInstallationStarted(const std::string& package_name) {}

    // Notifies that installation of package finished. |succeed| is set to true
    // in case of success.
    virtual void OnInstallationFinished(const std::string& package_name,
                                        bool success) {}

   protected:
    virtual ~Observer() {}
  };

  static ArcAppListPrefs* Create(
      Profile* profile,
      arc::ConnectionHolder<arc::mojom::AppInstance, arc::mojom::AppHost>*
          app_connection_holder);

  // Convenience function to get the ArcAppListPrefs for a BrowserContext. It
  // will only return non-null pointer for the primary user.
  static ArcAppListPrefs* Get(content::BrowserContext* context);

  // Constructs unique id based on package name and activity information. This
  // id is safe to use at file paths and as preference keys.
  static std::string GetAppId(const std::string& package_name,
                              const std::string& activity);

  // Constructs a unique id based on package name and activity name. Activity
  // name is found by iterating through the |prefs_| arc app dictionary to find
  // the app which has a matching |package_name|. This id is safe to use in file
  // paths and as preference keys.
  std::string GetAppIdByPackageName(const std::string& package_name) const;

  // It is called from chrome/browser/prefs/browser_prefs.cc.
  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  ~ArcAppListPrefs() override;

  // Returns a list of all app ids, including ready and non-ready apps.
  std::vector<std::string> GetAppIds() const;

  // Extracts attributes of an app based on its id. Returns NULL if the app is
  // not found.
  std::unique_ptr<AppInfo> GetApp(const std::string& app_id) const;

  // Get current installed package names.
  std::vector<std::string> GetPackagesFromPrefs() const;

  // Extracts attributes of a package based on its package name. Returns
  // nullptr if the package is not found.
  std::unique_ptr<PackageInfo> GetPackage(
      const std::string& package_name) const;

  // Constructs path to app local data.
  base::FilePath GetAppPath(const std::string& app_id) const;

  // Constructs path to app icon for specific scale factor.
  base::FilePath GetIconPath(const std::string& app_id,
                             ui::ScaleFactor scale_factor) const;
  // Constructs path to default app icon for specific scale factor. This path
  // is used to resolve icon if no icon is available at |GetIconPath|.
  base::FilePath MaybeGetIconPathForDefaultApp(
      const std::string& app_id,
      ui::ScaleFactor scale_factor) const;

  // Sets last launched time for the requested app.
  void SetLastLaunchTime(const std::string& app_id);

  // Calls RequestIcon if no request is recorded.
  void MaybeRequestIcon(const std::string& app_id,
                        ui::ScaleFactor scale_factor);

  // Sets notification enabled flag for given value. If the app or ARC bridge
  // service is not ready, then defer this request until the app gets
  // available. Once new value is set notifies an observer
  // OnNotificationsEnabledChanged.
  void SetNotificationsEnabled(const std::string& app_id, bool enabled);

  // Returns true if app is registered.
  bool IsRegistered(const std::string& app_id) const;
  // Returns true if app is a default app.
  bool IsDefault(const std::string& app_id) const;
  // Returns true if app is an OEM app.
  bool IsOem(const std::string& app_id) const;
  // Returns true if app is a shortcut
  bool IsShortcut(const std::string& app_id) const;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);
  bool HasObserver(Observer* observer);

  base::RepeatingCallback<std::string(const std::string&)>
  GetAppIdByPackageNameCallback();

  // arc::ArcSessionManager::Observer:
  void OnArcPlayStoreEnabledChanged(bool enabled) override;

  // ArcDefaultAppList::Delegate:
  void OnDefaultAppsReady() override;

  // Removes app with the given app_id.
  void RemoveApp(const std::string& app_id);

  arc::ConnectionHolder<arc::mojom::AppInstance, arc::mojom::AppHost>*
  app_connection_holder() {
    return app_connection_holder_;
  }

  bool package_list_initial_refreshed() const {
    return package_list_initial_refreshed_;
  }

  // Returns set of ARC apps for provided package name, not including shortcuts,
  // associated with this package.
  std::unordered_set<std::string> GetAppsForPackage(
      const std::string& package_name) const;

  // Gets Chrome prefs for given |package_name| and |key|.
  base::Value* GetPackagePrefs(const std::string& package_name,
                               const std::string& key);
  // Sets Chrome prefs for given |package_name| and |key| to |value|.
  void SetPackagePrefs(const std::string& package_name,
                       const std::string& key,
                       base::Value value);

  void SetDefaltAppsReadyCallback(base::Closure callback);
  void SimulateDefaultAppAvailabilityTimeoutForTesting();

 private:
  friend class ChromeLauncherControllerTest;
  friend class ArcAppModelBuilderTest;

  // See the Create methods.
  ArcAppListPrefs(
      Profile* profile,
      arc::ConnectionHolder<arc::mojom::AppInstance, arc::mojom::AppHost>*
          app_connection_holder);

  // arc::ConnectionObserver<arc::mojom::AppInstance>:
  void OnConnectionReady() override;
  void OnConnectionClosed() override;

  // arc::mojom::AppHost:
  void OnAppListRefreshed(std::vector<arc::mojom::AppInfoPtr> apps) override;
  void OnAppAddedDeprecated(arc::mojom::AppInfoPtr app) override;
  void OnPackageAppListRefreshed(
      const std::string& package_name,
      std::vector<arc::mojom::AppInfoPtr> apps) override;
  void OnInstallShortcut(arc::mojom::ShortcutInfoPtr shortcut) override;
  void OnUninstallShortcut(const std::string& package_name,
                           const std::string& intent_uri) override;
  void OnPackageRemoved(const std::string& package_name) override;
  void OnAppIcon(const std::string& package_name,
                 const std::string& activity,
                 arc::mojom::ScaleFactor scale_factor,
                 const std::vector<uint8_t>& icon_png_data) override;
  void OnIcon(const std::string& app_id,
              arc::mojom::ScaleFactor scale_factor,
              const std::vector<uint8_t>& icon_png_data);
  void OnTaskCreated(int32_t task_id,
                     const std::string& package_name,
                     const std::string& activity,
                     const base::Optional<std::string>& name,
                     const base::Optional<std::string>& intent) override;
  void OnTaskDescriptionUpdated(
      int32_t task_id,
      const std::string& label,
      const std::vector<uint8_t>& icon_png_data) override;
  void OnTaskDestroyed(int32_t task_id) override;
  void OnTaskSetActive(int32_t task_id) override;
  void OnNotificationsEnabledChanged(const std::string& package_name,
                                     bool enabled) override;
  void OnPackageAdded(arc::mojom::ArcPackageInfoPtr package_info) override;
  void OnPackageModified(arc::mojom::ArcPackageInfoPtr package_info) override;
  void OnPackageListRefreshed(
      std::vector<arc::mojom::ArcPackageInfoPtr> packages) override;
  void OnInstallationStarted(
      const base::Optional<std::string>& package_name) override;
  void OnInstallationFinished(
      arc::mojom::InstallationResultPtr result) override;

  void StartPrefs();

  void SetDefaultAppsFilterLevel();
  void RegisterDefaultApps();

  // Returns list of packages from prefs. If |installed| is set to true then
  // returns currently installed packages. If not, returns list of packages that
  // where uninstalled. Note, we store uninstall packages only for packages of
  // default apps. If |check_arc_alive| is set to true then package list is
  // filled only in case ARC is currently active.
  std::vector<std::string> GetPackagesFromPrefs(bool check_arc_alive,
                                                bool installed) const;

  void AddApp(const arc::mojom::AppInfo& app_info);
  void AddAppAndShortcut(bool app_ready,
                         const std::string& name,
                         const std::string& package_name,
                         const std::string& activity,
                         const std::string& intent_uri,
                         const std::string& icon_resource_id,
                         const bool sticky,
                         const bool notifications_enabled,
                         const bool shortcut,
                         const bool launchable);
  // Adds or updates local pref for given package.
  void AddOrUpdatePackagePrefs(PrefService* prefs,
                               const arc::mojom::ArcPackageInfo& package);
  // Removes given package from local pref.
  void RemovePackageFromPrefs(PrefService* prefs,
                              const std::string& package_name);

  void DisableAllApps();
  void RemoveAllAppsAndPackages();
  std::vector<std::string> GetAppIdsNoArcEnabledCheck() const;
  std::unordered_set<std::string> GetAppsAndShortcutsForPackage(
      const std::string& package_name,
      bool include_shortcuts) const;

  // Enumerates apps from preferences and notifies listeners about available
  // apps while ARC is not started yet. All apps in this case have disabled
  // state.
  void NotifyRegisteredApps();
  base::Time GetInstallTime(const std::string& app_id) const;

  // Installs an icon to file system in the special folder of the profile
  // directory.
  void InstallIcon(const std::string& app_id,
                   ui::ScaleFactor scale_factor,
                   const std::vector<uint8_t>& contentPng);
  void OnIconInstalled(const std::string& app_id,
                       ui::ScaleFactor scale_factor,
                       bool install_succeed);

  // Requests to load an app icon for specific scale factor. If the app or ARC
  // bridge service is not ready, then defer this request until the app gets
  // available. Once new icon is installed notifies an observer
  // OnAppIconUpdated.
  void RequestIcon(const std::string& app_id, ui::ScaleFactor scale_factor);

  // This checks if app is not registered yet and in this case creates
  // non-launchable app entry. In case app is already registered then updates
  // last launch time.
  void HandleTaskCreated(const base::Optional<std::string>& name,
                         const std::string& package_name,
                         const std::string& activity);

  // Returns true is specified package is new in the system, was not installed
  // and it is not scheduled to install by sync.
  bool IsUnknownPackage(const std::string& package_name) const;

  // Detects that default apps either exist or installation session is started.
  void DetectDefaultAppAvailability();

  // Performs data clean up for removed package.
  void HandlePackageRemoved(const std::string& package_name);

  // Sets timeout to wait for default app installed or installation started if
  // some default app is not available yet.
  void MaybeSetDefaultAppLoadingTimeout();

  bool IsIconRequestRecorded(const std::string& app_id,
                             ui::ScaleFactor scale_factor) const;

  // Removes the IconRequestRecord associated with app_id.
  void MaybeRemoveIconRequestRecord(const std::string& app_id);

  void ClearIconRequestRecord();

  // Dispatches OnAppReadyChanged event to observers.
  void NotifyAppReadyChanged(const std::string& app_id, bool ready);

  // Marks app icons as invalidated and request icons updated.
  void InvalidateAppIcons(const std::string& app_id);

  // Marks package icons as invalidated and request icons updated.
  void InvalidatePackageIcons(const std::string& package_name);

  Profile* const profile_;

  // Owned by the BrowserContext.
  PrefService* const prefs_;

  arc::ConnectionHolder<arc::mojom::AppInstance, arc::mojom::AppHost>* const
      app_connection_holder_;

  // List of observers.
  base::ObserverList<Observer> observer_list_;
  // Keeps root folder where ARC app icons for different scale factor are
  // stored.
  base::FilePath base_path_;
  // Contains set of ARC apps that are currently ready.
  std::unordered_set<std::string> ready_apps_;
  // Contains set of ARC apps that are currently tracked.
  std::unordered_set<std::string> tracked_apps_;
  // Contains number of ARC packages that are currently installing.
  int installing_packages_count_ = 0;
  // Keeps record for icon request. Each app may contain several requests for
  // different scale factor. Scale factor is defined by specific bit position.
  // Mainly two usages:
  // 1. Keeps deferred icon load requests when apps are not ready. Request will
  // be sent when apps becomes ready.
  // 2. Keeps record of icon request sent to Android. In each user session, one
  // request per app per scale_factor is allowed once.
  // When ARC is disabled or the app is uninstalled, the record will be erased.
  std::map<std::string, uint32_t> request_icon_recorded_;
  // True if this preference has been initialized once.
  bool is_initialized_ = false;
  // True if apps were restored.
  bool apps_restored_ = false;
  // True is ARC package list has been successfully refreshed.
  bool package_list_initial_refreshed_ = false;
  // Used to detect first ARC app launch request.
  bool first_launch_app_request_ = true;
  // Play Store does not have publicly available observers for default app
  // installations. This timeout is for validating default app availability.
  // Default apps should be either already installed or their installations
  // should be started soon after initial app list refresh.
  base::OneShotTimer detect_default_app_availability_timeout_;
  // Set of currently installing default apps_.
  std::unordered_set<std::string> default_apps_installations_;
  // Mask of scale factors for which the app's icon needs invalidation.
  uint32_t invalidated_icon_scale_factor_mask_;

  arc::ArcPackageSyncableService* sync_service_ = nullptr;

  bool default_apps_ready_ = false;
  ArcDefaultAppList default_apps_;
  base::Closure default_apps_ready_callback_;

  // TODO (b/70566216): Remove this once fixed.
  base::OnceClosure app_list_refreshed_callback_;

  base::WeakPtrFactory<ArcAppListPrefs> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ArcAppListPrefs);
};

#endif  // CHROME_BROWSER_UI_APP_LIST_ARC_ARC_APP_LIST_PREFS_H_
