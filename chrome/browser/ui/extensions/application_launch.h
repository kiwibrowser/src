// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_EXTENSIONS_APPLICATION_LAUNCH_H_
#define CHROME_BROWSER_UI_EXTENSIONS_APPLICATION_LAUNCH_H_

#include "chrome/browser/ui/extensions/app_launch_params.h"
#include "url/gurl.h"

class Browser;
class Profile;

namespace content {
class WebContents;
}

namespace extensions {
class Extension;
}

// Opens the application, possibly prompting the user to re-enable it.
void OpenApplicationWithReenablePrompt(const AppLaunchParams& params);

// Open the application in a way specified by |params|.
content::WebContents* OpenApplication(const AppLaunchParams& params);

// Open the application in a way specified by |params| in a new window.
content::WebContents* OpenApplicationWindow(const AppLaunchParams& params,
                                            const GURL& url);

// Open |url| in an app shortcut window.
// There are two kinds of app shortcuts: Shortcuts to a URL,
// and shortcuts that open an installed application.  This function
// is used to open the former.  To open the latter, use
// application_launch::OpenApplication().
content::WebContents* OpenAppShortcutWindow(Profile* profile,
                                            const GURL& url);

// Whether the extension can be launched by sending a
// chrome.app.runtime.onLaunched event.
bool CanLaunchViaEvent(const extensions::Extension* extension);

// Reparents |contents| into a new app browser for |extension|.
Browser* ReparentWebContentsIntoAppBrowser(
    content::WebContents* contents,
    const extensions::Extension* extension);

// Reparents the active tab into a new app browser for the PWA that has the
// tab's URL in its scope. Does nothing if the tab is not secure or there is no
// applicable PWA.
Browser* ReparentSecureActiveTabIntoPwaWindow(Browser* browser);

#endif  // CHROME_BROWSER_UI_EXTENSIONS_APPLICATION_LAUNCH_H_
