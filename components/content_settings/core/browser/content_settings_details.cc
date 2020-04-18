// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/content_settings/core/browser/content_settings_details.h"

ContentSettingsDetails::ContentSettingsDetails(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType type,
    const std::string& resource_identifier)
    : primary_pattern_(primary_pattern),
      secondary_pattern_(secondary_pattern),
      type_(type),
      resource_identifier_(resource_identifier) {
}
