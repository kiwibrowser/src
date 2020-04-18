// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/test/scoped_block_popups_pref.h"

#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "ios/chrome/browser/content_settings/host_content_settings_map_factory.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

ScopedBlockPopupsPref::ScopedBlockPopupsPref(
    ContentSetting setting,
    ios::ChromeBrowserState* browser_state)
    : browser_state_(browser_state), original_setting_(GetPrefValue()) {
  SetPrefValue(setting);
}

ScopedBlockPopupsPref::~ScopedBlockPopupsPref() {
  SetPrefValue(original_setting_);
}

ContentSetting ScopedBlockPopupsPref::GetPrefValue() {
  return ios::HostContentSettingsMapFactory::GetForBrowserState(browser_state_)
      ->GetDefaultContentSetting(CONTENT_SETTINGS_TYPE_POPUPS, NULL);
}

void ScopedBlockPopupsPref::SetPrefValue(ContentSetting setting) {
  DCHECK(setting == CONTENT_SETTING_BLOCK || setting == CONTENT_SETTING_ALLOW);
  ios::HostContentSettingsMapFactory::GetForBrowserState(browser_state_)
      ->SetDefaultContentSetting(CONTENT_SETTINGS_TYPE_POPUPS, setting);
}
