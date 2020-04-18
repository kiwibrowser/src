// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SHELL_INTEGRATION_LINUX_H_
#define CHROME_BROWSER_SHELL_INTEGRATION_LINUX_H_

#include <string>

#include "base/files/file_path.h"
#include "chrome/browser/web_applications/web_app.h"
#include "chrome/common/buildflags.h"
#include "url/gurl.h"

namespace base {
class CommandLine;
class Environment;
}

namespace shell_integration_linux {

// Get the path to write user-specific application data files to, as specified
// in the XDG Base Directory Specification:
// http://standards.freedesktop.org/basedir-spec/latest/
base::FilePath GetDataWriteLocation(base::Environment* env);

// Get the list of paths to search for application data files, in order of
// preference, as specified in the XDG Base Directory Specification:
// http://standards.freedesktop.org/basedir-spec/latest/
// Called on the FILE thread.
std::vector<base::FilePath> GetDataSearchLocations(base::Environment* env);

// Gets the name for use as the res_name of the window's WM_CLASS property.
std::string GetProgramClassName();

// Gets the name for use as the res_class of the window's WM_CLASS property.
std::string GetProgramClassClass();

// Returns filename of the desktop shortcut used to launch the browser.
std::string GetDesktopName(base::Environment* env);

// Returns name of the browser icon (without a path or file extension).
std::string GetIconName();

// Returns the set of locations in which shortcuts are installed for the
// extension with |extension_id| in |profile_path|.
// This searches the file system for .desktop files in appropriate locations. A
// shortcut with NoDisplay=true causes hidden to become true, instead of
// creating at APP_MENU_LOCATIONS_SUBDIR_CHROMEAPPS.
web_app::ShortcutLocations GetExistingShortcutLocations(
    base::Environment* env,
    const base::FilePath& profile_path,
    const std::string& extension_id);

// Version of GetExistingShortcutLocations which takes an explicit path
// to the user's desktop directory. Useful for testing.
// If |desktop_path| is empty, the desktop is not searched.
web_app::ShortcutLocations GetExistingShortcutLocations(
    base::Environment* env,
    const base::FilePath& profile_path,
    const std::string& extension_id,
    const base::FilePath& desktop_path);

// Returns the contents of an existing .desktop file installed in the system.
// Searches the "applications" subdirectory of each XDG data directory for a
// file named |desktop_filename|. If the file is found, populates |output| with
// its contents and returns true. Else, returns false.
bool GetExistingShortcutContents(base::Environment* env,
                                 const base::FilePath& desktop_filename,
                                 std::string* output);

// Returns filename for .desktop file based on |url|, sanitized for security.
base::FilePath GetWebShortcutFilename(const GURL& url);

// Returns filename for .desktop file based on |profile_path| and
// |extension_id|, sanitized for security.
base::FilePath GetExtensionShortcutFilename(const base::FilePath& profile_path,
                                            const std::string& extension_id);

// Returns a list of filenames for all existing .desktop files corresponding to
// on |profile_path| in a given |directory|.
std::vector<base::FilePath> GetExistingProfileShortcutFilenames(
    const base::FilePath& profile_path,
    const base::FilePath& directory);

// Returns contents for .desktop file based on |url| and |title|. If
// |no_display| is true, the shortcut will not be visible to the user in menus.
std::string GetDesktopFileContents(const base::FilePath& chrome_exe_path,
                                   const std::string& app_name,
                                   const GURL& url,
                                   const std::string& extension_id,
                                   const base::string16& title,
                                   const std::string& icon_name,
                                   const base::FilePath& profile_path,
                                   const std::string& categories,
                                   bool no_display);

// Returns contents for .desktop file that executes command_line. This is a more
// general form of GetDesktopFileContents. If |no_display| is true, the shortcut
// will not be visible to the user in menus.
std::string GetDesktopFileContentsForCommand(
    const base::CommandLine& command_line,
    const std::string& app_name,
    const GURL& url,
    const base::string16& title,
    const std::string& icon_name,
    const std::string& categories,
    bool no_display);

// Returns contents for .directory file named |title| with icon |icon_name|. If
// |icon_name| is empty, will use the Chrome icon.
std::string GetDirectoryFileContents(const base::string16& title,
                                     const std::string& icon_name);

// Create shortcuts on the desktop or in the application menu (as specified by
// |shortcut_info|), for the web page or extension in |shortcut_info|.
// For extensions, duplicate shortcuts are avoided, so if a requested shortcut
// already exists it is deleted first.
bool CreateDesktopShortcut(
    const web_app::ShortcutInfo& shortcut_info,
    const web_app::ShortcutLocations& creation_locations);

#if BUILDFLAG(ENABLE_APP_LIST)
// Create shortcuts in the application menu for the app launcher. Duplicate
// shortcuts are avoided, so if a requested shortcut already exists it is
// deleted first. Also creates the icon required by the shortcut.
bool CreateAppListDesktopShortcut(const std::string& wm_class,
                                  const std::string& title);
#endif

// Delete any desktop shortcuts on desktop or in the application menu that have
// been added for the extension with |extension_id| in |profile_path|.
void DeleteDesktopShortcuts(const base::FilePath& profile_path,
                            const std::string& extension_id);

// Delete any desktop shortcuts on desktop or in the application menu that have
// for the profile in |profile_path|.
void DeleteAllDesktopShortcuts(const base::FilePath& profile_path);

namespace internal {

// Exposed for testing.  Clients should use the corresponding functions in
// shell_integration_linux instead.
std::string GetProgramClassName(const base::CommandLine& command_line,
                                const std::string& desktop_file_name);
std::string GetProgramClassClass(const base::CommandLine& command_line,
                                 const std::string& desktop_file_name);

}  // namespace internal

}  // namespace shell_integration_linux

#endif  // CHROME_BROWSER_SHELL_INTEGRATION_LINUX_H_
