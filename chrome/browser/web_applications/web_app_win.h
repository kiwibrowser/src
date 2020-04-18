// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_WIN_H_
#define CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_WIN_H_

#include "base/files/file_path.h"
#include "chrome/browser/web_applications/web_app.h"

class Profile;

namespace extensions {
class Extension;
}

namespace gfx {
class ImageFamily;
}

namespace web_app {

// Create a shortcut in the given web app data dir, returning the name of the
// created shortcut.
base::FilePath CreateShortcutInWebAppDir(
    const base::FilePath& web_app_path,
    std::unique_ptr<ShortcutInfo> shortcut_info);

// Update the relaunch details for the given app's window, making the taskbar
// group's "Pin to the taskbar" button function correctly.
void UpdateRelaunchDetailsForApp(Profile* profile,
                                 const extensions::Extension* extension,
                                 HWND hwnd);

namespace internals {

// Saves |image| to |icon_file| if the file is outdated. Returns true if
// icon_file is up to date or successfully updated.
// If |refresh_shell_icon_cache| is true, the shell's icon cache will be
// refreshed, ensuring the correct icon is displayed, but causing a flicker.
// Refreshing the icon cache is not necessary on shortcut creation as the shell
// will be notified when the shortcut is created.
bool CheckAndSaveIcon(const base::FilePath& icon_file,
                      const gfx::ImageFamily& image,
                      bool refresh_shell_icon_cache);

base::FilePath GetIconFilePath(const base::FilePath& web_app_path,
                               const base::string16& title);

}  // namespace internals

}  // namespace web_app

#endif  // CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_WIN_H_
