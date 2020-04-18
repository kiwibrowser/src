// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_PREFERENCES_PREF_SERVICE_BRIDGE_H_
#define CHROME_BROWSER_ANDROID_PREFERENCES_PREF_SERVICE_BRIDGE_H_

#include <string>
#include <vector>

#include "components/content_settings/core/common/content_settings.h"

class PrefServiceBridge {
 public:
  // Use |locale| to create a language-region pair and language code to prepend
  // to the default accept languages. For Malay, we'll end up creating
  // "ms-MY,ms,en-US,en", and for Swiss-German, we will have
  // "de-CH,de-DE,de,en-US,en".
  static void PrependToAcceptLanguagesIfNecessary(
      const std::string& locale,
      std::string* accept_languages);

  // Populate the list of corresponding Android permissions associated with the
  // ContentSettingsType specified.
  static void GetAndroidPermissionsForContentSetting(
      ContentSettingsType content_type,
      std::vector<std::string>* out);

  static const char* GetPrefNameExposedToJava(int pref_index);
};

#endif  // CHROME_BROWSER_ANDROID_PREFERENCES_PREF_SERVICE_BRIDGE_H_
