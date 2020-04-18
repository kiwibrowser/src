// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_SEARCH_WEBSTORE_WEBSTORE_RESULT_H_
#define CHROME_BROWSER_UI_APP_LIST_SEARCH_WEBSTORE_WEBSTORE_RESULT_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/extensions/install_observer.h"
#include "chrome/browser/ui/app_list/search/chrome_search_result.h"
#include "chrome/common/extensions/webstore_install_result.h"
#include "extensions/browser/extension_registry_observer.h"
#include "extensions/common/manifest.h"
#include "url/gurl.h"

class AppListControllerDelegate;
class Profile;

namespace extensions {
class ExtensionRegistry;
class InstallTracker;
}

namespace app_list {

class WebstoreResult : public ChromeSearchResult,
                       public extensions::InstallObserver,
                       public extensions::ExtensionRegistryObserver {
 public:
  WebstoreResult(Profile* profile,
                 const std::string& app_id,
                 const GURL& icon_url,
                 bool is_paid,
                 extensions::Manifest::Type item_type,
                 AppListControllerDelegate* controller);
  ~WebstoreResult() override;

  static std::string GetResultIdFromExtensionId(
      const std::string& extension_id);

  const std::string& app_id() const { return app_id_; }
  const GURL& icon_url() const { return icon_url_; }
  extensions::Manifest::Type item_type() const { return item_type_; }
  bool is_paid() const { return is_paid_; }

  // ChromeSearchResult overrides:
  void Open(int event_flags) override;
  void InvokeAction(int action_index, int event_flags) override;

 private:
  // Set the initial state and start observing both InstallObserver and
  // ExtensionRegistryObserver.
  void InitAndStartObserving();

  void UpdateActions();
  void SetDefaultDetails();
  void OnIconLoaded();

  void StartInstall();
  void InstallCallback(bool success,
                       const std::string& error,
                       extensions::webstore_install::Result result);
  void LaunchCallback(extensions::webstore_install::Result result,
                      const std::string& error);

  void StopObservingInstall();
  void StopObservingRegistry();

  // extensions::InstallObserver overrides:
  void OnDownloadProgress(const std::string& extension_id,
                          int percent_downloaded) override;
  void OnShutdown() override;

  // extensions::ExtensionRegistryObserver overides:
  void OnExtensionInstalled(content::BrowserContext* browser_context,
                            const extensions::Extension* extension,
                            bool is_update) override;
  void OnShutdown(extensions::ExtensionRegistry* registry) override;

  Profile* profile_;
  const std::string app_id_;
  const GURL icon_url_;
  const bool is_paid_;
  extensions::Manifest::Type item_type_;

  gfx::ImageSkia icon_;

  AppListControllerDelegate* controller_;
  extensions::InstallTracker* install_tracker_;  // Not owned.
  extensions::ExtensionRegistry* extension_registry_;  // Not owned.

  base::WeakPtrFactory<WebstoreResult> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(WebstoreResult);
};

}  // namespace app_list

#endif  // CHROME_BROWSER_UI_APP_LIST_SEARCH_WEBSTORE_WEBSTORE_RESULT_H_
