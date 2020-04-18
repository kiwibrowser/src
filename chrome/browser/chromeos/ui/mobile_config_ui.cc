// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/ui/mobile_config_ui.h"

#include <string>

#include "base/logging.h"
#include "chrome/browser/chromeos/mobile_config.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/scoped_tabbed_browser_displayer.h"
#include "chrome/browser/ui/singleton_tabs.h"

namespace chromeos {
namespace mobile_config_ui {

bool DisplayConfigDialog() {
  MobileConfig* config = MobileConfig::GetInstance();
  if (!config->IsReady()) {
    LOG(ERROR) << "MobileConfig not ready";
    return false;
  }
  const MobileConfig::LocaleConfig* locale_config = config->GetLocaleConfig();
  if (!locale_config) {
    LOG(ERROR) << "MobileConfig::LocaleConfig not available";
    return false;
  }
  std::string setup_url = locale_config->setup_url();
  if (setup_url.empty()) {
    LOG(ERROR) << "MobileConfig setup url is empty";
    return false;
  }
  // The mobile device will be managed by the primary user.
  chrome::ScopedTabbedBrowserDisplayer displayer(
      ProfileManager::GetPrimaryUserProfile());
  ShowSingletonTab(displayer.browser(), GURL(setup_url));
  return true;
}

}  // namespace mobile_config_ui
}  // namespace chromeos
