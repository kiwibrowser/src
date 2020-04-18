// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_LOCATION_SETTINGS_IMPL_H_
#define CHROME_BROWSER_ANDROID_LOCATION_SETTINGS_IMPL_H_

#include <memory>

#include "base/android/jni_weak_ref.h"
#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "chrome/browser/android/location_settings.h"

class LocationSettingsImpl : public LocationSettings {
 public:
  LocationSettingsImpl();
  ~LocationSettingsImpl() override;

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

 private:
  DISALLOW_COPY_AND_ASSIGN(LocationSettingsImpl);
};

#endif  // CHROME_BROWSER_ANDROID_LOCATION_SETTINGS_IMPL_H_
