// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "google_apis/drive/drive_switches.h"

namespace google_apis {
// Enables or disables Team Drives integration.
const char kEnableTeamDrives[] = "team-drives";

TeamDrivesIntegrationStatus GetTeamDrivesIntegrationSwitch() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(kEnableTeamDrives) ?
      TEAM_DRIVES_INTEGRATION_ENABLED : TEAM_DRIVES_INTEGRATION_DISABLED;
}

}  // namespace google_apis
