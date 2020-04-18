// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_LAUNCHER_CHROME_LAUNCHER_CONTROLLER_H_
#define CHROME_BROWSER_UI_ASH_LAUNCHER_CHROME_LAUNCHER_CONTROLLER_H_

#include <memory>
#include <string>
#include <vector>

#include "ash/public/cpp/shelf_item_delegate.h"
#include "ash/public/cpp/shelf_model_observer.h"
#include "ash/public/cpp/shelf_types.h"
#include "ash/public/interfaces/shelf.mojom.h"
#include "base/auto_reset.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/app_icon_loader_delegate.h"
#include "chrome/browser/ui/app_list/app_list_syncable_service.h"
#include "chrome/browser/ui/app_list/app_sync_ui_state_observer.h"
#include "chrome/browser/ui/ash/launcher/launcher_app_updater.h"
#include "chrome/browser/ui/ash/launcher/settings_window_observer.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/sync_preferences/pref_service_syncable_observer.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"

class AppIconLoader;
class AppSyncUIState;
class AppWindowLauncherController;
class BrowserShortcutLauncherItemController;
class BrowserStatusMonitor;
class ChromeLauncherControllerUserSwitchObserver;
class GURL;
class Profile;
class LauncherControllerHelper;
class ShelfSpinnerController;

namespace ash {
class ShelfModel;
}  // namespace ash

namespace content {
class WebContents;
}

namespace gfx {
class Image;
}

namespace ui {
class BaseWindow;
}

// ChromeLauncherController helps manage Ash's shelf for Chrome prefs and apps.
// It helps synchronize shelf state with profile preferences and app content.
// NOTE: Launcher is an old name for the shelf, this class should be renamed.
class ChromeLauncherController
    : public LauncherAppUpdater::Delegate,
      public AppIconLoaderDelegate,
      private ash::mojom::ShelfObserver,
      private ash::ShelfModelObserver,
      private AppSyncUIStateObserver,
      private app_list::AppListSyncableService::Observer,
      private sync_preferences::PrefServiceSyncableObserver {
 public:
  // Used to update the state of non plaform apps, as web contents change.
  enum AppState {
    APP_STATE_ACTIVE,
    APP_STATE_WINDOW_ACTIVE,
    APP_STATE_INACTIVE,
    APP_STATE_REMOVED
  };

  // Returns the single ChromeLauncherController instance.
  static ChromeLauncherController* instance() { return instance_; }

  ChromeLauncherController(Profile* profile, ash::ShelfModel* model);
  ~ChromeLauncherController() override;

  Profile* profile() const { return profile_; }
  ash::ShelfModel* shelf_model() const { return model_; }

  // Initializes this ChromeLauncherController.
  void Init();

  // Creates a new app item on the shelf for |item_delegate|.
  ash::ShelfID CreateAppLauncherItem(
      std::unique_ptr<ash::ShelfItemDelegate> item_delegate,
      ash::ShelfItemStatus status,
      const base::string16& title = base::string16());

  // Returns the shelf item with the given id, or null if |id| isn't found.
  const ash::ShelfItem* GetItem(const ash::ShelfID& id) const;

  // Updates the type of an item.
  void SetItemType(const ash::ShelfID& id, ash::ShelfItemType type);

  // Updates the running status of an item. It will also update the status of
  // browsers shelf item if needed.
  void SetItemStatus(const ash::ShelfID& id, ash::ShelfItemStatus status);

  // Updates the shelf item title (displayed in the tooltip).
  void SetItemTitle(const ash::ShelfID& id, const base::string16& title);

  // Closes or unpins the shelf item.
  void CloseLauncherItem(const ash::ShelfID& id);

  // Returns true if the item identified by |id| is pinned.
  bool IsPinned(const ash::ShelfID& id);

  // Set the shelf item status for the V1 application with the given |app_id|.
  // Adds or removes an item as needed to respect the running and pinned state.
  void SetV1AppStatus(const std::string& app_id, ash::ShelfItemStatus status);

  // Closes the specified item.
  void Close(const ash::ShelfID& id);

  // Returns true if the specified item is open.
  bool IsOpen(const ash::ShelfID& id);

  // Returns true if the specified item is for a platform app.
  bool IsPlatformApp(const ash::ShelfID& id);

  // Opens a new instance of the application identified by the ShelfID.
  // Used by the app-list, and by pinned-app shelf items. |display_id| is id of
  // the display from which the app is launched.
  void LaunchApp(const ash::ShelfID& id,
                 ash::ShelfLaunchSource source,
                 int event_flags,
                 int64_t display_id);

  // If |app_id| is running, reactivates the app's most recently active window,
  // otherwise launches and activates the app.
  // Used by the app-list, and by pinned-app shelf items.
  void ActivateApp(const std::string& app_id,
                   ash::ShelfLaunchSource source,
                   int event_flags);

  // Set the image for a specific shelf item (e.g. when set by the app).
  void SetLauncherItemImage(const ash::ShelfID& shelf_id,
                            const gfx::ImageSkia& image);

  // Updates the image for a specific shelf item from the app's icon loader.
  void UpdateLauncherItemImage(const std::string& app_id);

  // Notify the controller that the state of an non platform app's tabs
  // have changed,
  void UpdateAppState(content::WebContents* contents, AppState app_state);

  // Returns ShelfID for |contents|. If |contents| is not an app or is not
  // pinned, returns the id of browser shrotcut.
  ash::ShelfID GetShelfIDForWebContents(content::WebContents* contents);

  // Limits application refocusing to urls that match |url| for |id|.
  void SetRefocusURLPatternForTest(const ash::ShelfID& id, const GURL& url);

  // Activates a |window|. If |allow_minimize| is true and the system allows
  // it, the the window will get minimized instead.
  // Returns the action performed. Should be one of SHELF_ACTION_NONE,
  // SHELF_ACTION_WINDOW_ACTIVATED, or SHELF_ACTION_WINDOW_MINIMIZED.
  ash::ShelfAction ActivateWindowOrMinimizeIfActive(ui::BaseWindow* window,
                                                    bool allow_minimize);

  // Called when the active user has changed.
  void ActiveUserChanged(const std::string& user_email);

  // Called when a user got added to the session.
  void AdditionalUserAddedToSession(Profile* profile);

  // Get the list of all running incarnations of this item.
  ash::MenuItemList GetAppMenuItemsForTesting(const ash::ShelfItem& item);

  // Get the list of all tabs which belong to a certain application type.
  std::vector<content::WebContents*> GetV1ApplicationsFromAppId(
      const std::string& app_id);

  // Activates a specified shell application by app id and window index.
  void ActivateShellApp(const std::string& app_id, int window_index);

  // Checks if a given |web_contents| is known to be associated with an
  // application of type |app_id|.
  bool IsWebContentHandledByApplication(content::WebContents* web_contents,
                                        const std::string& app_id);

  // Check if the gMail app is loaded and it can handle the given web content.
  // This special treatment is required to address crbug.com/234268.
  bool ContentCanBeHandledByGmailApp(content::WebContents* web_contents);

  // Get the favicon for the application list entry for |web_contents|.
  // Note that for incognito windows the incognito icon will be returned.
  // If |web_contents| has not loaded, returns the default favicon.
  gfx::Image GetAppListIcon(content::WebContents* web_contents) const;

  // Get the title for the applicatoin list entry for |web_contents|.
  // If |web_contents| has not loaded, returns "Net Tab".
  base::string16 GetAppListTitle(content::WebContents* web_contents) const;

  // Returns the ash::ShelfItemDelegate of BrowserShortcut.
  BrowserShortcutLauncherItemController*
  GetBrowserShortcutLauncherItemController();

  // Called when the user profile is fully loaded and ready to switch to.
  void OnUserProfileReadyToSwitch(Profile* profile);

  // Controller to launch ARC and Crostini apps with a spinner.
  ShelfSpinnerController* GetShelfSpinnerController();

  // Temporarily prevent pinned shelf item changes from updating the sync model.
  using ScopedPinSyncDisabler = std::unique_ptr<base::AutoReset<bool>>;
  ScopedPinSyncDisabler GetScopedPinSyncDisabler();

  // Access to the BrowserStatusMonitor for tests.
  BrowserStatusMonitor* browser_status_monitor_for_test() {
    return browser_status_monitor_.get();
  }

  // Access to the AppWindowLauncherController list for tests.
  const std::vector<std::unique_ptr<AppWindowLauncherController>>&
  app_window_controllers_for_test() {
    return app_window_controllers_;
  }

  // Sets LauncherControllerHelper or AppIconLoader for test, taking ownership.
  void SetLauncherControllerHelperForTest(
      std::unique_ptr<LauncherControllerHelper> helper);
  void SetAppIconLoadersForTest(
      std::vector<std::unique_ptr<AppIconLoader>>& loaders);

  void SetProfileForTest(Profile* profile);

  // Flush responses to ash::mojom::ShelfObserver messages.
  void FlushForTesting();

  // Helpers that call through to corresponding ShelfModel functions.
  void PinAppWithID(const std::string& app_id);
  bool IsAppPinned(const std::string& app_id);
  void UnpinAppWithID(const std::string& app_id);

  // LauncherAppUpdater::Delegate:
  void OnAppInstalled(content::BrowserContext* browser_context,
                      const std::string& app_id) override;
  void OnAppUninstalledPrepared(content::BrowserContext* browser_context,
                                const std::string& app_id) override;

  // AppIconLoaderDelegate:
  void OnAppImageUpdated(const std::string& app_id,
                         const gfx::ImageSkia& image) override;

 protected:
  // Connects or reconnects to the mojom::ShelfController interface in ash.
  // Returns true if connected; virtual for unit tests.
  virtual bool ConnectToShelfController();

 private:
  friend class ChromeLauncherControllerTest;
  friend class LauncherPlatformAppBrowserTest;
  friend class ShelfAppBrowserTest;
  friend class TestChromeLauncherController;

  using WebContentsToAppIDMap = std::map<content::WebContents*, std::string>;

  // Creates a new app shortcut item and controller on the shelf at |index|.
  ash::ShelfID CreateAppShortcutLauncherItem(const ash::ShelfID& shelf_id,
                                             int index);

  // Remembers / restores list of running applications.
  // Note that this order will neither be stored in the preference nor will it
  // remember the order of closed applications since it is only temporary.
  void RememberUnpinnedRunningApplicationOrder();
  void RestoreUnpinnedRunningApplicationOrder(const std::string& user_id);

  // Invoked when the associated browser or app is closed.
  void RemoveShelfItem(const ash::ShelfID& id);

  // Internal helpers for pinning and unpinning that handle both
  // client-triggered and internal pinning operations.
  void DoPinAppWithID(const std::string& app_id);
  void DoUnpinAppWithID(const std::string& app_id, bool update_prefs);

  // Pin a running app with |shelf_id| internally to |index|.
  void PinRunningAppInternal(int index, const ash::ShelfID& shelf_id);

  // Unpin a locked application. This is an internal call which converts the
  // model type of the given app index from a shortcut into an unpinned running
  // app.
  void UnpinRunningAppInternal(int index);

  // Updates pin position for the item specified by |id| in sync model.
  void SyncPinPosition(const ash::ShelfID& id);

  // Re-syncs shelf model.
  void UpdateAppLaunchersFromPref();

  // Schedules re-sync of shelf model.
  void ScheduleUpdateAppLaunchersFromPref();

  // Update the policy-pinned flag for each shelf item.
  void UpdatePolicyPinnedAppsFromPrefs();

  // Sets whether the virtual keyboard is enabled from prefs.
  void SetVirtualKeyboardBehaviorFromPrefs();

  // Returns the shelf item status for the given |app_id|, which can be either
  // STATUS_RUNNING (if there is such an app) or STATUS_CLOSED.
  ash::ShelfItemStatus GetAppState(const std::string& app_id);

  // Creates an app launcher to insert at |index|. Note that |index| may be
  // adjusted by the model to meet ordering constraints.
  // The |shelf_item_type| will be set into the ShelfModel.
  ash::ShelfID InsertAppLauncherItem(
      std::unique_ptr<ash::ShelfItemDelegate> item_delegate,
      ash::ShelfItemStatus status,
      int index,
      ash::ShelfItemType shelf_item_type,
      const base::string16& title = base::string16());

  // Create the Chrome browser shortcut ShelfItem.
  void CreateBrowserShortcutLauncherItem();

  // Check if the given |web_contents| is in incognito mode.
  bool IsIncognito(const content::WebContents* web_contents) const;

  // Finds the index of where to insert the next item.
  int FindInsertionPoint();

  // Close all windowed V1 applications of a certain extension which was already
  // deleted.
  void CloseWindowedAppsFromRemovedExtension(const std::string& app_id,
                                             const Profile* profile);

  // Attach to a specific profile.
  void AttachProfile(Profile* profile_to_attach);

  // Forget the current profile to allow attaching to a new one.
  void ReleaseProfile();

  // ash::mojom::ShelfObserver:
  void OnShelfItemAdded(int32_t index, const ash::ShelfItem& item) override;
  void OnShelfItemRemoved(const ash::ShelfID& id) override;
  void OnShelfItemMoved(const ash::ShelfID& id, int32_t index) override;
  void OnShelfItemUpdated(const ash::ShelfItem& item) override;
  void OnShelfItemDelegateChanged(
      const ash::ShelfID& id,
      ash::mojom::ShelfItemDelegatePtr delegate) override;

  // ash::ShelfModelObserver:
  void ShelfItemAdded(int index) override;
  void ShelfItemRemoved(int index, const ash::ShelfItem& old_item) override;
  void ShelfItemMoved(int start_index, int target_index) override;
  void ShelfItemChanged(int index, const ash::ShelfItem& old_item) override;
  void ShelfItemDelegateChanged(const ash::ShelfID& id,
                                ash::ShelfItemDelegate* old_delegate,
                                ash::ShelfItemDelegate* delegate) override;

  // AppSyncUIStateObserver:
  void OnAppSyncUIStatusChanged() override;

  // app_list::AppListSyncableService::Observer:
  void OnSyncModelUpdated() override;

  // sync_preferences::PrefServiceSyncableObserver:
  void OnIsSyncingChanged() override;

  // An internal helper to unpin a shelf item; this does not update prefs.
  void UnpinShelfItemInternal(const ash::ShelfID& id);

  // Resolves the app icon image loader for the app.
  AppIconLoader* GetAppIconLoaderForApp(const std::string& app_id);

  static ChromeLauncherController* instance_;

  // The currently loaded profile used for prefs and loading extensions. This is
  // NOT necessarily the profile new windows are created with. Note that in
  // multi-profile use cases this might change over time.
  Profile* profile_ = nullptr;

  // In classic Ash, this the ShelfModel owned by ash::Shell's ShelfController.
  // In the mash config, this is a separate ShelfModel instance, owned by
  // ChromeBrowserMainExtraPartsAsh, and synchronized with Ash's ShelfModel.
  ash::ShelfModel* model_;

  // Ash's mojom::ShelfController used to change shelf state.
  ash::mojom::ShelfControllerPtr shelf_controller_;

  // The binding this instance uses to implment mojom::ShelfObserver
  mojo::AssociatedBinding<ash::mojom::ShelfObserver> observer_binding_;

  // True when applying changes from the remote ShelfModel owned by Ash.
  // Changes to the local ShelfModel should not be reported during this time.
  bool applying_remote_shelf_model_changes_ = false;

  // When true, changes to pinned shelf items should update the sync model.
  bool should_sync_pin_changes_ = true;

  // Used to get app info for tabs.
  std::unique_ptr<LauncherControllerHelper> launcher_controller_helper_;

  // An observer that manages the shelf title and icon for settings windows.
  SettingsWindowObserver settings_window_observer_;

  // Used to load the images for app items.
  std::vector<std::unique_ptr<AppIconLoader>> app_icon_loaders_;

  // Direct access to app_id for a web contents.
  WebContentsToAppIDMap web_contents_to_app_id_;

  // Used to track app windows.
  std::vector<std::unique_ptr<AppWindowLauncherController>>
      app_window_controllers_;

  // Used to handle app load/unload events.
  std::vector<std::unique_ptr<LauncherAppUpdater>> app_updaters_;

  PrefChangeRegistrar pref_change_registrar_;

  AppSyncUIState* app_sync_ui_state_ = nullptr;

  // The owned browser status monitor.
  std::unique_ptr<BrowserStatusMonitor> browser_status_monitor_;

  // A special observer class to detect user switches.
  std::unique_ptr<ChromeLauncherControllerUserSwitchObserver>
      user_switch_observer_;

  std::unique_ptr<ShelfSpinnerController> shelf_spinner_controller_;

  // The list of running & un-pinned applications for different users on hidden
  // desktops.
  using RunningAppListIds = std::vector<std::string>;
  using RunningAppListIdMap = std::map<std::string, RunningAppListIds>;
  RunningAppListIdMap last_used_running_application_order_;

  base::WeakPtrFactory<ChromeLauncherController> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ChromeLauncherController);
};

#endif  // CHROME_BROWSER_UI_ASH_LAUNCHER_CHROME_LAUNCHER_CONTROLLER_H_
