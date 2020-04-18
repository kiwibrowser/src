// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_EXTENSION_STORAGE_MONITOR_H_
#define CHROME_BROWSER_EXTENSIONS_EXTENSION_STORAGE_MONITOR_H_

#include <stdint.h>

#include <set>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/scoped_observer.h"
#include "chrome/browser/extensions/extension_uninstall_dialog.h"
#include "components/keyed_service/core/keyed_service.h"
#include "extensions/browser/extension_registry_observer.h"

namespace gfx {
class Image;
}

class Profile;

namespace extensions {

class Extension;
class ExtensionPrefs;
class ExtensionRegistry;
class ExtensionStorageMonitorIOHelper;

// ExtensionStorageMonitor monitors the storage usage of extensions and apps
// that are granted unlimited storage and displays notifications when high
// usage is detected.
class ExtensionStorageMonitor : public KeyedService,
                                public ExtensionRegistryObserver,
                                public ExtensionUninstallDialog::Delegate {
 public:
  static ExtensionStorageMonitor* Get(Profile* profile);

  // Indices of buttons in the notification. Exposed for testing.
  enum ButtonIndex {
    BUTTON_DISABLE_NOTIFICATION = 0,
    BUTTON_UNINSTALL
  };

  explicit ExtensionStorageMonitor(Profile* profile);
  ~ExtensionStorageMonitor() override;

 private:
  // ExtensionRegistryObserver overrides:
  void OnExtensionLoaded(content::BrowserContext* browser_context,
                         const Extension* extension) override;
  void OnExtensionUnloaded(content::BrowserContext* browser_context,
                           const Extension* extension,
                           UnloadedExtensionReason reason) override;
  void OnExtensionWillBeInstalled(content::BrowserContext* browser_context,
                                  const Extension* extension,
                                  bool is_update,
                                  const std::string& old_name) override;
  void OnExtensionUninstalled(content::BrowserContext* browser_context,
                              const Extension* extension,
                              extensions::UninstallReason reason) override;

  // Overridden from ExtensionUninstallDialog::Delegate:
  void OnExtensionUninstallDialogClosed(bool did_start_uninstall,
                                        const base::string16& error) override;

  std::string GetNotificationId(const std::string& extension_id);

  void OnStorageThresholdExceeded(const std::string& extension_id,
                                  int64_t next_threshold,
                                  int64_t current_usage);
  void OnImageLoaded(const std::string& extension_id,
                     int64_t current_usage,
                     const gfx::Image& image);
  void OnNotificationButtonClick(const std::string& extension_id,
                                 base::Optional<int> button_index);

  void DisableStorageMonitoring(const std::string& extension_id);
  void StartMonitoringStorage(const Extension* extension);
  void StopMonitoringStorage(const std::string& extension_id);

  void RemoveNotificationForExtension(const std::string& extension_id);

  // Displays the prompt for uninstalling the extension.
  void ShowUninstallPrompt(const std::string& extension_id);

  // Returns/sets the next threshold for displaying a notification if an
  // extension or app consumes excessive disk space.
  int64_t GetNextStorageThreshold(const std::string& extension_id) const;
  void SetNextStorageThreshold(const std::string& extension_id,
                               int64_t next_threshold);

  // Returns the raw next storage threshold value stored in prefs. Returns 0 if
  // the initial threshold has not yet been reached.
  int64_t GetNextStorageThresholdFromPrefs(
      const std::string& extension_id) const;

  // Returns/sets whether notifications should be shown if an extension or app
  // consumes too much disk space.
  bool IsStorageNotificationEnabled(const std::string& extension_id) const;
  void SetStorageNotificationEnabled(const std::string& extension_id,
                                     bool enable_notifications);

  // Initially, monitoring will only be applied to ephemeral apps. This flag
  // is set by tests to enable for all extensions and apps.
  bool enable_for_all_extensions_;

  // The first notification is shown after the initial threshold is exceeded.
  // A lower threshold is set by tests.
  int64_t initial_extension_threshold_;

  // The rate at which we would like to receive storage updates
  // from QuotaManager. Overridden in tests.
  base::TimeDelta observer_rate_;

  // IDs of extensions that notifications were shown for.
  std::set<std::string> notified_extension_ids_;

  Profile* profile_;
  extensions::ExtensionPrefs* extension_prefs_;

  ScopedObserver<extensions::ExtensionRegistry,
                 extensions::ExtensionRegistryObserver>
      extension_registry_observer_;

  // ExtensionStorageMonitorIOHelper maintains, on the IO thread, an instance of
  // SingleExtensionStorageObserver for each extension.
  scoped_refptr<ExtensionStorageMonitorIOHelper> io_helper_;

  // Modal dialog used to confirm removal of an extension.
  std::unique_ptr<ExtensionUninstallDialog> uninstall_dialog_;

  // The ID of the extension that is the subject of the uninstall confirmation
  // dialog.
  std::string uninstall_extension_id_;

  base::WeakPtrFactory<ExtensionStorageMonitor> weak_ptr_factory_;

  friend class SingleExtensionStorageObserver;
  friend class ExtensionStorageMonitorTest;

  DISALLOW_COPY_AND_ASSIGN(ExtensionStorageMonitor);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_EXTENSION_STORAGE_MONITOR_H_
