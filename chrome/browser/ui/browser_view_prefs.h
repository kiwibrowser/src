// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_BROWSER_VIEW_PREFS_H_
#define CHROME_BROWSER_UI_BROWSER_VIEW_PREFS_H_

class PrefRegistrySimple;
class PrefService;

namespace user_prefs {
class PrefRegistrySyncable;
}

// Register local state preferences specific to BrowserView.
void RegisterBrowserViewLocalPrefs(PrefRegistrySimple* registry);

// Register profile-specific preferences specific to BrowserView. These
// preferences may be synced, depending on the pref's |sync_status| parameter.
void RegisterBrowserViewProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry);

// Converts deprecated int tabstrip layout type into a boolean value indicating
// stacked layout preference.
void MigrateBrowserTabStripPrefs(PrefService* pref);

#endif  // CHROME_BROWSER_UI_BROWSER_VIEW_PREFS_H_
