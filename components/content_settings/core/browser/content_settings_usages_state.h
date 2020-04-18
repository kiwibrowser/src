// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_CONTENT_SETTINGS_USAGES_STATE_H_
#define COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_CONTENT_SETTINGS_USAGES_STATE_H_

#include <map>
#include <set>

#include "base/macros.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "url/gurl.h"

class HostContentSettingsMap;

// This class manages a content setting state per tab for a given
// |ContentSettingsType|, and provides information and presentation data about
// the content setting usage.
class ContentSettingsUsagesState {
 public:
  ContentSettingsUsagesState(
      HostContentSettingsMap* host_content_settings_map,
      ContentSettingsType type);
  ~ContentSettingsUsagesState();

  typedef std::map<GURL, ContentSetting> StateMap;
  const StateMap& state_map() const {
    return state_map_;
  }

  // Sets the state for |requesting_origin|.
  void OnPermissionSet(const GURL& requesting_origin, bool allowed);

  // Delegated by WebContents to indicate a navigation has happened and we
  // may need to clear our settings.
  void DidNavigate(const GURL& url, const GURL& previous_url);

  void ClearStateMap();

  enum TabState {
    TABSTATE_NONE = 0,
    // There's at least one entry with non-default setting.
    TABSTATE_HAS_EXCEPTION = 1 << 1,
    // There's at least one entry with a non-ASK setting.
    TABSTATE_HAS_ANY_ICON = 1 << 2,
    // There's at least one entry with ALLOWED setting.
    TABSTATE_HAS_ANY_ALLOWED = 1 << 3,
    // There's at least one entry that doesn't match the saved setting.
    TABSTATE_HAS_CHANGED = 1 << 4,
  };

  // Maps ContentSetting to a set of hosts formatted for presentation.
  typedef std::map<ContentSetting, std::set<std::string> >
      FormattedHostsPerState;

  void GetDetailedInfo(FormattedHostsPerState* formatted_hosts_per_state,
                       unsigned int* tab_state_flags) const;

 private:
  std::string GURLToFormattedHost(const GURL& url) const;

  HostContentSettingsMap* const host_content_settings_map_;
  ContentSettingsType type_;
  StateMap state_map_;
  GURL embedder_url_;

  DISALLOW_COPY_AND_ASSIGN(ContentSettingsUsagesState);
};

#endif  // COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_CONTENT_SETTINGS_USAGES_STATE_H_
