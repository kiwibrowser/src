// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_EXTENSION_APP_MODEL_BUILDER_H_
#define CHROME_BROWSER_UI_APP_LIST_EXTENSION_APP_MODEL_BUILDER_H_

#include <stddef.h>

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "chrome/browser/extensions/install_observer.h"
#include "chrome/browser/ui/app_list/app_list_model_builder.h"
#include "chrome/browser/ui/ash/launcher/launcher_app_updater.h"
#include "components/prefs/pref_change_registrar.h"
#include "ui/base/models/list_model_observer.h"

class AppListControllerDelegate;
class LauncherExtensionAppUpdater;
class ExtensionAppItem;

namespace extensions {
class Extension;
class InstallTracker;
}

namespace gfx {
class ImageSkia;
}

// This class populates and maintains the given |model| for extension items
// with information from |profile|.
class ExtensionAppModelBuilder : public AppListModelBuilder,
                                 public extensions::InstallObserver,
                                 public LauncherAppUpdater::Delegate {
 public:
  explicit ExtensionAppModelBuilder(AppListControllerDelegate* controller);
  ~ExtensionAppModelBuilder() override;

 private:
  // AppListModelBuilder
  void BuildModel() override;

  // extensions::InstallObserver.
  void OnBeginExtensionInstall(const ExtensionInstallParams& params) override;
  void OnDownloadProgress(const std::string& extension_id,
                          int percent_downloaded) override;
  void OnInstallFailure(const std::string& extension_id) override;
  void OnDisabledExtensionUpdated(
      const extensions::Extension* extension) override;
  void OnShutdown() override;

  // LauncherAppUpdater::Delegate:
  void OnAppInstalled(content::BrowserContext* browser_context,
                      const std::string& app_id) override;
  void OnAppUninstalled(content::BrowserContext* browser_context,
                        const std::string& app_id) override;

  std::unique_ptr<ExtensionAppItem> CreateAppItem(
      const std::string& extension_id,
      const std::string& extension_name,
      const gfx::ImageSkia& installing_icon,
      bool is_platform_app);

  // Populates the model with apps.
  void PopulateApps();

  // Returns app instance matching |extension_id| or NULL.
  ExtensionAppItem* GetExtensionAppItem(const std::string& extension_id);

  // Initializes the |profile_pref_change_registrar_| and the
  // |extension_pref_change_registrar_| to listen for changes to profile and
  // extension prefs, and call OnProfilePreferenceChanged() or
  // OnExtensionPreferenceChanged().
  void InitializePrefChangeRegistrars();

  // Handles profile prefs changes.
  void OnProfilePreferenceChanged();

  // Registrar used to monitor the profile prefs.
  PrefChangeRegistrar profile_pref_change_registrar_;

  // We listen to this to show app installing progress.
  extensions::InstallTracker* tracker_ = nullptr;

  // Dispatches extension lifecycle events.
  std::unique_ptr<LauncherExtensionAppUpdater> app_updater_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionAppModelBuilder);
};

#endif  // CHROME_BROWSER_UI_APP_LIST_EXTENSION_APP_MODEL_BUILDER_H_
