// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/usage_stats_consent.h"

#include <memory>
#include <string>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/values.h"
#include "remoting/host/config_file_watcher.h"
#include "remoting/host/host_config.h"

namespace remoting {

bool GetUsageStatsConsent(bool* allowed, bool* set_by_policy) {
  *set_by_policy = false;
  *allowed = false;

  // Normally, the ConfigFileWatcher class would be used for retrieving config
  // settings, but this code needs to execute before Breakpad is initialised,
  // which itself should happen as early as possible during startup.
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(kHostConfigSwitchName)) {
    base::FilePath config_file_path =
        command_line->GetSwitchValuePath(kHostConfigSwitchName);
    std::unique_ptr<base::DictionaryValue> host_config(
        HostConfigFromJsonFile(config_file_path));
    if (host_config) {
      return host_config->GetBoolean(kUsageStatsConsentConfigPath, allowed);
    }
  }
  return false;
}

bool IsUsageStatsAllowed() {
  bool allowed;
  bool set_by_policy;
  return GetUsageStatsConsent(&allowed, &set_by_policy) && allowed;
}

bool SetUsageStatsConsent(bool allowed) {
  NOTIMPLEMENTED();
  return false;
}

}  // namespace remoting
