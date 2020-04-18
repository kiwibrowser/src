// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_LOCAL_DISCOVERY_LOCAL_DISCOVERY_UI_H_
#define CHROME_BROWSER_UI_WEBUI_LOCAL_DISCOVERY_LOCAL_DISCOVERY_UI_H_

#include "base/macros.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "content/public/browser/web_ui_controller.h"

class LocalDiscoveryUI : public content::WebUIController {
 public:
  explicit LocalDiscoveryUI(content::WebUI* web_ui);

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);
 private:
  DISALLOW_COPY_AND_ASSIGN(LocalDiscoveryUI);
};

#endif  // CHROME_BROWSER_UI_WEBUI_LOCAL_DISCOVERY_LOCAL_DISCOVERY_UI_H_
