// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/setup/setup_install_details.h"

#include "base/command_line.h"
#include "base/strings/string16.h"
#include "base/win/registry.h"
#include "chrome/install_static/install_constants.h"
#include "chrome/install_static/install_details.h"
#include "chrome/install_static/install_modes.h"
#include "chrome/install_static/install_util.h"
#include "chrome/installer/util/google_update_constants.h"
#include "chrome/installer/util/master_preferences.h"
#include "chrome/installer/util/master_preferences_constants.h"
#include "chrome/installer/util/util_constants.h"

namespace {

const install_static::InstallConstants* FindInstallMode(
    const base::CommandLine& command_line) {
  // Search for a mode whose switch is on the command line.
  for (int i = 1; i < install_static::NUM_INSTALL_MODES; ++i) {
    const install_static::InstallConstants& mode =
        install_static::kInstallModes[i];
    if (command_line.HasSwitch(mode.install_switch))
      return &mode;
  }
  // The first mode is always the default if all else fails.
  return &install_static::kInstallModes[0];
}

// Returns true if Chrome is installed (i.e., has a "pv" value) and its
// UninstallArguments contains "--multi-install".
bool IsUpdatingFromMulti(const install_static::InstallConstants& mode,
                         bool system_level) {
  const HKEY root = system_level ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
  const wchar_t* const app_guid = mode.app_guid;
  base::win::RegKey key;
  base::string16 value;

  return key.Open(root, install_static::GetClientsKeyPath(app_guid).c_str(),
                  KEY_QUERY_VALUE | KEY_WOW64_32KEY) == ERROR_SUCCESS &&
         key.ReadValue(google_update::kRegVersionField, &value) ==
             ERROR_SUCCESS &&
         !value.empty() &&
         key.Open(root, install_static::GetClientStateKeyPath(app_guid).c_str(),
                  KEY_QUERY_VALUE | KEY_WOW64_32KEY) == ERROR_SUCCESS &&
         key.ReadValue(installer::kUninstallArgumentsField, &value) ==
             ERROR_SUCCESS &&
         value.find(L"--multi-install") != base::string16::npos;
}

}  // namespace

void InitializeInstallDetails(
    const base::CommandLine& command_line,
    const installer::MasterPreferences& master_preferences) {
  install_static::InstallDetails::SetForProcess(
      MakeInstallDetails(command_line, master_preferences));
}

std::unique_ptr<install_static::PrimaryInstallDetails> MakeInstallDetails(
    const base::CommandLine& command_line,
    const installer::MasterPreferences& master_preferences) {
  std::unique_ptr<install_static::PrimaryInstallDetails> details(
      std::make_unique<install_static::PrimaryInstallDetails>());

  // The mode is determined by brand-specific command line switches.
  const install_static::InstallConstants* const mode =
      FindInstallMode(command_line);
  details->set_mode(mode);

  // The install level may be set by any of:
  // - distribution.system_level=true in master_preferences,
  // - --system-level on the command line, or
  // - the GoogleUpdateIsMachine=1 environment variable.
  // In all three cases the value is sussed out in MasterPreferences
  // initialization.
  bool system_level = false;
  master_preferences.GetBool(installer::master_preferences::kSystemLevel,
                             &system_level);
  details->set_system_level(system_level);

  // The channel is determined based on the brand and the mode's
  // ChannelStrategy. For brands that do not support Google Update, the channel
  // is an empty string. For modes using the FIXED strategy, the channel is the
  // default_channel_name in the mode. For modes using the ADDITIONAL_PARAMETERS
  // strategy, the channel is parsed from the "ap" value in either the binaries'
  // ClientState registry key or the mode's ClientState registry key. Which one
  // is used depends on whether or not this Chrome is updating from a legacy
  // multi-install Chrome.

  // Cache the ap and cohort name values found in the registry for use in crash
  // keys.
  base::string16 update_ap;
  base::string16 update_cohort_name;
  details->set_channel(install_static::DetermineChannel(
      *mode, system_level, IsUpdatingFromMulti(*mode, system_level), &update_ap,
      &update_cohort_name));
  details->set_update_ap(update_ap);
  details->set_update_cohort_name(update_cohort_name);

  return details;
}
