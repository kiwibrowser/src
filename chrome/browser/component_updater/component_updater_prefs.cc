// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/component_updater/component_updater_prefs.h"

#include "chrome/browser/component_updater/chrome_component_updater_configurator.h"
#include "chrome/browser/component_updater/recovery_component_installer.h"
#include "chrome/browser/component_updater/recovery_improved_component_installer.h"
#include "chrome/browser/component_updater/supervised_user_whitelist_installer.h"

namespace component_updater {

void RegisterPrefs(PrefRegistrySimple* registry) {
  RegisterPrefsForChromeComponentUpdaterConfigurator(registry);
  RegisterPrefsForRecoveryComponent(registry);
  RegisterPrefsForRecoveryImprovedComponent(registry);
  SupervisedUserWhitelistInstaller::RegisterPrefs(registry);
}

}  // namespace component_updater
