// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_EXTENSION_VIEW_HOST_FACTORY_H_
#define CHROME_BROWSER_EXTENSIONS_EXTENSION_VIEW_HOST_FACTORY_H_

#include "base/macros.h"

class Browser;
class GURL;
class Profile;

namespace extensions {

class ExtensionViewHost;

// A utility class to make ExtensionViewHosts for UI views that are backed
// by extensions.
class ExtensionViewHostFactory {
 public:
  // Creates a new ExtensionHost with its associated view, grouping it in the
  // appropriate SiteInstance (and therefore process) based on the URL and
  // profile.
  static ExtensionViewHost* CreatePopupHost(const GURL& url, Browser* browser);

  // Some dialogs may not be associated with a particular browser window and
  // hence only require a |profile|.
  static ExtensionViewHost* CreateDialogHost(const GURL& url, Profile* profile);

 private:
  DISALLOW_COPY_AND_ASSIGN(ExtensionViewHostFactory);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_EXTENSION_VIEW_HOST_FACTORY_H_
