// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_MAC_H_
#define CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_MAC_H_

#include <stddef.h>

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "chrome/browser/web_applications/web_app.h"

namespace base {
class CommandLine;
}

// Whether to enable update and launch of app shims in tests. (Normally shims
// are never created or launched in tests). Note that update only creates
// internal shim bundles, i.e. it does not create new shims in ~/Applications.
extern bool g_app_shims_allow_update_and_launch_in_tests;

namespace web_app {

// Returns the full path of the .app shim that would be created by
// CreateShortcuts().
base::FilePath GetAppInstallPath(const ShortcutInfo& shortcut_info);

// If necessary, launch the shortcut for an app.
void MaybeLaunchShortcut(std::unique_ptr<ShortcutInfo> shortcut_info);

// Rebuild the shortcut and relaunch it.
bool MaybeRebuildShortcut(const base::CommandLine& command_line);

// Reveals app shim in Finder given a profile and app.
// Calls RevealAppShimInFinderForAppOnFileThread and schedules it
// on the FILE thread.
void RevealAppShimInFinderForApp(Profile* profile,
                                 const extensions::Extension* app);

// Creates a shortcut for a web application. The shortcut is a stub app
// that simply loads the browser framework and runs the given app.
class WebAppShortcutCreator {
 public:
  // Creates a new shortcut based on information in |shortcut_info|.
  // A copy of the shortcut is placed in |app_data_dir|.
  // |chrome_bundle_id| is the CFBundleIdentifier of the Chrome browser bundle.
  // Retains the pointer |shortcut_info|; the ShortcutInfo object must outlive
  // the WebAppShortcutCreator.
  WebAppShortcutCreator(const base::FilePath& app_data_dir,
                        const ShortcutInfo* shortcut_info);

  virtual ~WebAppShortcutCreator();

  // Returns the base name for the shortcut.
  base::FilePath GetShortcutBasename() const;

  // Returns a path to the Chrome Apps folder in the relevant applications
  // folder. E.g. ~/Applications or /Applications.
  virtual base::FilePath GetApplicationsDirname() const;

  // The full path to the app bundle under the relevant Applications folder.
  base::FilePath GetApplicationsShortcutPath() const;

  // The full path to the app bundle under the profile folder.
  base::FilePath GetInternalShortcutPath() const;

  bool CreateShortcuts(ShortcutCreationReason creation_reason,
                       ShortcutLocations creation_locations);
  void DeleteShortcuts();
  bool UpdateShortcuts();

  // Show the bundle we just generated in the Finder.
  virtual void RevealAppShimInFinder() const;

 protected:
  // Returns a path to an app bundle with the given id. Or an empty path if no
  // matching bundle was found.
  // Protected and virtual so it can be mocked out for testing.
  virtual base::FilePath GetAppBundleById(const std::string& bundle_id) const;

 private:
  FRIEND_TEST_ALL_PREFIXES(WebAppShortcutCreatorTest, DeleteShortcuts);
  FRIEND_TEST_ALL_PREFIXES(WebAppShortcutCreatorTest, UpdateIcon);
  FRIEND_TEST_ALL_PREFIXES(WebAppShortcutCreatorTest, UpdateShortcuts);
  FRIEND_TEST_ALL_PREFIXES(WebAppShortcutCreatorTest,
                           UpdateBookmarkAppShortcut);

  // Returns the bundle identifier to use for this app bundle.
  std::string GetBundleIdentifier() const;

  // Returns the bundle identifier for the internal copy of the bundle.
  std::string GetInternalBundleIdentifier() const;

  // Copies the app loader template into a temporary directory and fills in all
  // relevant information.
  bool BuildShortcut(const base::FilePath& staging_path) const;

  // Builds a shortcut and copies it into the given destination folders.
  // Returns with the number of successful copies. Returns on the first failure.
  size_t CreateShortcutsIn(const std::vector<base::FilePath>& folders) const;

  // Updates the InfoPlist.string inside |app_path| with the display name for
  // the app.
  bool UpdateDisplayName(const base::FilePath& app_path) const;

  // Updates the bundle id of the internal copy of the app shim bundle.
  bool UpdateInternalBundleIdentifier() const;

  // Updates the plist inside |app_path| with information about the app.
  bool UpdatePlist(const base::FilePath& app_path) const;

  // Updates the icon for the shortcut.
  bool UpdateIcon(const base::FilePath& app_path) const;

  // Path to the data directory for this app. For example:
  // ~/Library/Application Support/Chromium/Default/Web Applications/_crx_abc/
  base::FilePath app_data_dir_;

  // Information about the app. Owned by the caller of the constructor.
  const ShortcutInfo* info_;

  DISALLOW_COPY_AND_ASSIGN(WebAppShortcutCreator);
};

}  // namespace web_app

#endif  // CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_MAC_H_
