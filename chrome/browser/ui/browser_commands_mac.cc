// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/browser_commands_mac.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace chrome {

void ToggleFullscreenToolbar(Browser* browser) {
  DCHECK(browser);

  // Toggle the value of the preference.
  PrefService* prefs = browser->profile()->GetPrefs();
  bool show_toolbar = prefs->GetBoolean(prefs::kShowFullscreenToolbar);
  prefs->SetBoolean(prefs::kShowFullscreenToolbar, !show_toolbar);
}

}  // namespace chrome
