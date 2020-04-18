// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PLUGINS_PLUGIN_UTILS_H_
#define CHROME_BROWSER_PLUGINS_PLUGIN_UTILS_H_

#include "base/macros.h"
#include "components/content_settings/core/common/content_settings.h"

class GURL;
class HostContentSettingsMap;

namespace content {
class ResourceContext;
struct WebPluginInfo;
}

namespace url {
class Origin;
}

class PluginUtils {
 public:
  // |is_default| and |is_managed| may be nullptr. In that case, they aren't
  // set.
  static void GetPluginContentSetting(
      const HostContentSettingsMap* host_content_settings_map,
      const content::WebPluginInfo& plugin,
      const url::Origin& main_frame_origin,
      const GURL& plugin_url,
      const std::string& resource,
      ContentSetting* setting,
      bool* is_default,
      bool* is_managed);

  // Returns the content setting for Flash. This is the same as
  // |GetPluginContentSetting| but flash-specific.
  static ContentSetting GetFlashPluginContentSetting(
      const HostContentSettingsMap* host_content_settings_map,
      const url::Origin& main_frame_origin,
      const GURL& plugin_url,
      bool* is_managed);

  // Remember that the user has changed the Flash permission for
  // |top_level_url|.
  static void RememberFlashChangedForSite(
      HostContentSettingsMap* host_content_settings_map,
      const GURL& top_level_url);

  // Returns true if HTML content should be prefered, by hiding Flash from the
  // plugin list. Use this instead of
  // base::FeatureList::IsEnabled(features::kPreferHtmlOverPlugins).
  static bool ShouldPreferHtmlOverPlugins(
      const HostContentSettingsMap* host_content_settings_map);

  // If there's an extension that is allowed to handle |mime_type|, returns its
  // ID. Otherwise returns an empty string.
  static std::string GetExtensionIdForMimeType(
      content::ResourceContext* resource_context,
      const std::string& mime_type);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(PluginUtils);
};

#endif  // CHROME_BROWSER_PLUGINS_PLUGIN_UTILS_H_
