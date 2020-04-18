// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GOOGLE_APIS_DRIVE_DRIVE_SWITHES_H_
#define GOOGLE_APIS_DRIVE_DRIVE_SWITHES_H_

namespace google_apis {

enum TeamDrivesIntegrationStatus {
  TEAM_DRIVES_INTEGRATION_DISABLED,
  TEAM_DRIVES_INTEGRATION_ENABLED
};

// Whether Team Drives integration is enabled or not.
TeamDrivesIntegrationStatus GetTeamDrivesIntegrationSwitch();

// For tests which require specific commandline switch settings.
extern const char kEnableTeamDrives[];

}  // namespace switches

#endif  // GOOGLE_APIS_DRIVE_DRIVE_SWITHES_H_
