// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PERMISSIONS_PERMISSION_REQUEST_MANAGER_TEST_API_H_
#define CHROME_BROWSER_PERMISSIONS_PERMISSION_REQUEST_MANAGER_TEST_API_H_

#include "base/macros.h"
#include "chrome/browser/permissions/permission_request_manager.h"
#include "components/content_settings/core/common/content_settings_types.h"

class Browser;

namespace test {

class PermissionRequestManagerTestApi {
 public:
  explicit PermissionRequestManagerTestApi(PermissionRequestManager* manager);

  // Wraps the PermissionRequestManager for the active tab in |browser|.
  explicit PermissionRequestManagerTestApi(Browser* browser);

  PermissionRequestManager* manager() { return manager_; }

  // Add a "simple" permission request. One that uses PermissionRequestImpl,
  // such as for ContentSettingsType including MIDI_SYSEX, PUSH_MESSAGING,
  // NOTIFICATIONS, GEOLOCATON, or PLUGINS.
  void AddSimpleRequest(ContentSettingsType type);

  // Return the bubble window for the permission prompt or null if there is no
  // prompt currently showing.
  gfx::NativeWindow GetPromptWindow();

  void SimulateWebContentsDestroyed();

 private:
  PermissionRequestManager* manager_;

  DISALLOW_COPY_AND_ASSIGN(PermissionRequestManagerTestApi);
};

}  // namespace test

#endif  // CHROME_BROWSER_PERMISSIONS_PERMISSION_REQUEST_MANAGER_TEST_API_H_
