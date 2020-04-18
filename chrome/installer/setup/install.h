// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file contains the specification of setup main functions.

#ifndef CHROME_INSTALLER_SETUP_INSTALL_H_
#define CHROME_INSTALLER_SETUP_INSTALL_H_

#include "base/strings/string16.h"
#include "chrome/installer/util/util_constants.h"

namespace base {
class FilePath;
class Version;
}

namespace installer {

class InstallationState;
class InstallerState;
class MasterPreferences;
class Product;

enum InstallShortcutOperation {
  // Create all shortcuts (potentially skipping those explicitly stated not to
  // be installed in the InstallShortcutPreferences).
  INSTALL_SHORTCUT_CREATE_ALL,
  // Create each per-user shortcut (potentially skipping those explicitly stated
  // not to be installed in the InstallShortcutPreferences), but only if the
  // system-level equivalent of that shortcut is not present on the system.
  INSTALL_SHORTCUT_CREATE_EACH_IF_NO_SYSTEM_LEVEL,
  // Replace all shortcuts that still exist with the most recent version of
  // each individual shortcut.
  INSTALL_SHORTCUT_REPLACE_EXISTING,
};

enum InstallShortcutLevel {
  // Install shortcuts for the current user only.
  CURRENT_USER,
  // Install global shortcuts visible to all users. Note: the Quick Launch
  // and taskbar pin shortcuts are still installed per-user (as they have no
  // all-users version).
  ALL_USERS,
};

// Creates chrome.VisualElementsManifest.xml in |src_path| if
// |src_path|\VisualElements exists. |supports_dark_text| indicates whether or
// not the OS supports drawing dark text on light assets. When true, and such
// light assets are present, the generated manifest references those light
// assets. Returns true unless the manifest is supposed to be created, but fails
// to be.
bool CreateVisualElementsManifest(const base::FilePath& src_path,
                                  const base::Version& version,
                                  bool supports_dark_text);

// Updates chrome.VisualElementsManifest.xml in |target_path| if
// |target_path|\VisualElements exists. |supports_dark_text| indicates whether
// or not the OS supports drawing dark text on light assets. When true, and such
// light assets are present, the generated manifest references those light
// assets. The file is not modified if no changes are needed. If it is modified,
// Chrome's start menu shortcut is touched so that the Start Menu refreshes its
// representation of the tile.
void UpdateVisualElementsManifest(const base::FilePath& target_path,
                                  const base::Version& version,
                                  bool supports_dark_text);

// Overwrites shortcuts (desktop, quick launch, and start menu) if they are
// present on the system.
// |prefs| can affect the behavior of this method through
// kDoNotCreateDesktopShortcut, kDoNotCreateQuickLaunchShortcut, and
// kAltShortcutText.
// |install_level| specifies whether to install per-user shortcuts or shortcuts
// for all users on the system (this should only be used to update legacy
// system-level installs).
// If |install_operation| is a creation command, appropriate shortcuts will be
// created even if they don't exist.
// If creating the Start menu shortcut is successful, it is also pinned to the
// taskbar.
void CreateOrUpdateShortcuts(
    const base::FilePath& target,
    const Product& product,
    const MasterPreferences& prefs,
    InstallShortcutLevel install_level,
    InstallShortcutOperation install_operation);

// Registers Chrome on this machine.
// If |make_chrome_default|, also attempts to make Chrome default where doing so
// requires no more user interaction than a UAC prompt. In practice, this means
// on versions of Windows prior to Windows 8.
void RegisterChromeOnMachine(const InstallerState& installer_state,
                             const Product& product,
                             bool make_chrome_default);

// This function installs or updates a new version of Chrome. It returns
// install status (failed, new_install, updated etc).
//
// setup_path: Path to the executable (setup.exe) as it will be copied
//           to Chrome install folder after install is complete
// archive_path: Path to the archive (chrome.7z) as it will be copied
//               to Chrome install folder after install is complete
// install_temp_path: working directory used during install/update. It should
//                    also has a sub dir source that contains a complete
//                    and unpacked Chrome package.
// src_path: the unpacked Chrome package (inside |install_temp_path|).
// prefs: master preferences. See chrome/installer/util/master_preferences.h.
// new_version: new Chrome version that needs to be installed
// package: Represents the target installation folder and all distributions
//          to be installed in that folder.
//
// Note: since caller unpacks Chrome to install_temp_path\source, the caller
// is responsible for cleaning up install_temp_path.
InstallStatus InstallOrUpdateProduct(
    const InstallationState& original_state,
    const InstallerState& installer_state,
    const base::FilePath& setup_path,
    const base::FilePath& archive_path,
    const base::FilePath& install_temp_path,
    const base::FilePath& src_path,
    const base::FilePath& prefs_path,
    const installer::MasterPreferences& prefs,
    const base::Version& new_version);

// Launches a process that deletes files that belong to old versions of Chrome.
// |setup_path| is the path to the setup.exe executable to use.
void LaunchDeleteOldVersionsProcess(const base::FilePath& setup_path,
                                    const InstallerState& installer_state);

// Performs installation-related tasks following an OS upgrade.
// |chrome| The installed product (must be a browser).
// |installed_version| the current version of this install.
void HandleOsUpgradeForBrowser(const InstallerState& installer_state,
                               const Product& chrome,
                               const base::Version& installed_version);

// Performs per-user installation-related tasks on Active Setup (ran on first
// login for each user post system-level Chrome install). Shortcut creation is
// skipped if the First Run beacon is present (unless |force| is set to true).
void HandleActiveSetupForBrowser(const InstallerState& installer_state,
                                 bool force);

}  // namespace installer

#endif  // CHROME_INSTALLER_SETUP_INSTALL_H_
