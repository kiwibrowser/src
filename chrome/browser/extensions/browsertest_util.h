// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_BROWSERTEST_UTIL_H_
#define CHROME_BROWSER_EXTENSIONS_BROWSERTEST_UTIL_H_

class Browser;
class Profile;
struct WebApplicationInfo;

namespace extensions {

class Extension;

namespace browsertest_util {

// On chromeos, the extension cache directory must be initialized before
// extensions can be installed in some situations (e.g. policy force installs
// via update urls). The chromeos device setup scripts take care of this in
// actual production devices, but some tests need to do it manually.
void CreateAndInitializeLocalCache();

// Installs a Bookmark App into |profile| using |info|.
const Extension* InstallBookmarkApp(Profile* profile, WebApplicationInfo info);

// Launches a new app window for |app| in |profile|.
Browser* LaunchAppBrowser(Profile* profile, const Extension* app);

}  // namespace browsertest_util
}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_BROWSERTEST_UTIL_H_
