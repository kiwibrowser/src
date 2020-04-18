// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APPS_APP_INFO_DIALOG_H_
#define CHROME_BROWSER_UI_APPS_APP_INFO_DIALOG_H_

#include "base/callback_forward.h"
#include "chrome/common/buildflags.h"

class Profile;

namespace content {
class WebContents;
}

namespace extensions {
class Extension;
}

namespace gfx {
class Rect;
}

// Used for UMA to track where the App Info dialog is launched from.
enum AppInfoLaunchSource {
  FROM_APP_LIST,         // Launched from the app list context menu (ChromeOS).
  FROM_EXTENSIONS_PAGE,  // Launched from the chrome://extensions page.
  FROM_APPS_PAGE,        // Launched from chrome://apps context menu.
  NUM_LAUNCH_SOURCES,
};

// TODO(tsergeant): Move these methods into a class
// Returns true if the app info dialog is available on the current platform.
bool CanShowAppInfoDialog();

#if BUILDFLAG(ENABLE_APP_LIST)
// Shows the chrome app information as a frameless window for the given |app|
// and |profile| at the given |app_info_bounds|.
void ShowAppInfoInAppList(const gfx::Rect& app_info_bounds,
                          Profile* profile,
                          const extensions::Extension* app,
                          const base::Closure& close_callback);
#endif

// Shows the chrome app information in a native dialog box.
void ShowAppInfoInNativeDialog(content::WebContents* web_contents,
                               Profile* profile,
                               const extensions::Extension* app,
                               const base::Closure& close_callback);

#endif  // CHROME_BROWSER_UI_APPS_APP_INFO_DIALOG_H_
