// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CONTENT_SETTINGS_WEB_SITE_SETTINGS_UMA_UTIL_H_
#define CHROME_BROWSER_CONTENT_SETTINGS_WEB_SITE_SETTINGS_UMA_UTIL_H_

#include "base/logging.h"
#include "base/macros.h"
#include "components/content_settings/core/common/content_settings.h"

class WebSiteSettingsUmaUtil {
 public:
  static void LogPermissionChange(ContentSettingsType type,
                                  ContentSetting setting);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(WebSiteSettingsUmaUtil);
};

#endif  // CHROME_BROWSER_CONTENT_SETTINGS_WEB_SITE_SETTINGS_UMA_UTIL_H_
