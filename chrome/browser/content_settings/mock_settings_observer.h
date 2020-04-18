// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CONTENT_SETTINGS_MOCK_SETTINGS_OBSERVER_H_
#define CHROME_BROWSER_CONTENT_SETTINGS_MOCK_SETTINGS_OBSERVER_H_

#include "base/scoped_observer.h"
#include "components/content_settings/core/browser/content_settings_observer.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "testing/gmock/include/gmock/gmock.h"

class ContentSettingsPattern;
class HostContentSettingsMap;

class MockSettingsObserver : public content_settings::Observer {
 public:
  explicit MockSettingsObserver(HostContentSettingsMap* map);
  ~MockSettingsObserver() override;

  void OnContentSettingChanged(const ContentSettingsPattern& primary_pattern,
                               const ContentSettingsPattern& secondary_pattern,
                               ContentSettingsType content_type,
                               std::string resource_identifier) override;

  MOCK_METHOD6(OnContentSettingsChanged,
               void(HostContentSettingsMap*,
                    ContentSettingsType,
                    bool,
                    const ContentSettingsPattern&,
                    const ContentSettingsPattern&,
                    bool));

 private:
  // The map that this Observer is watching.
  HostContentSettingsMap* map_;

  // Observer to watch for content settings changes.
  ScopedObserver<HostContentSettingsMap, content_settings::Observer> observer_;
};

#endif  // CHROME_BROWSER_CONTENT_SETTINGS_MOCK_SETTINGS_OBSERVER_H_
