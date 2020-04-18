// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_MOCK_LOCATION_SETTINGS_H_
#define CHROME_BROWSER_ANDROID_MOCK_LOCATION_SETTINGS_H_

#include "base/macros.h"
#include "chrome/browser/android/location_settings.h"
#include "components/location/android/location_settings_dialog_context.h"
#include "components/location/android/location_settings_dialog_outcome.h"

// Mock implementation of LocationSettings for unit tests.
class MockLocationSettings : public LocationSettings {
 public:
  MockLocationSettings();
  ~MockLocationSettings() override;

  static void SetLocationStatus(bool has_android_location_permission,
                                bool is_system_location_setting_enabled);
  static void SetCanPromptForAndroidPermission(bool can_prompt);
  static void SetLocationSettingsDialogStatus(
      bool enabled,
      LocationSettingsDialogOutcome outcome);
  static bool HasShownLocationSettingsDialog();
  static void ClearHasShownLocationSettingsDialog();

  static void SetAsyncLocationSettingsDialog();
  static void ResolveAsyncLocationSettingsDialog();

  // LocationSettings implementation:
  bool HasAndroidLocationPermission() override;
  bool CanPromptForAndroidLocationPermission(
      content::WebContents* web_contents) override;
  bool IsSystemLocationSettingEnabled() override;
  bool CanPromptToEnableSystemLocationSetting() override;
  void PromptToEnableSystemLocationSetting(
      const LocationSettingsDialogContext prompt_context,
      content::WebContents* web_contents,
      LocationSettingsDialogOutcomeCallback callback) override;

  DISALLOW_COPY_AND_ASSIGN(MockLocationSettings);
};

#endif  // CHROME_BROWSER_ANDROID_MOCK_LOCATION_SETTINGS_H_
