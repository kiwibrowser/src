// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_PLUGINS_POWER_SAVER_INFO_H_
#define CHROME_RENDERER_PLUGINS_POWER_SAVER_INFO_H_

#include <string>

#include "ui/gfx/geometry/size.h"
#include "url/gurl.h"

namespace blink {
struct WebPluginParams;
}

namespace content {
class RenderFrame;
struct WebPluginInfo;
}

// This contains information specifying the plugin's Power Saver behavior.
// The default constructor has Plugin Power Saver disabled.
struct PowerSaverInfo {
  PowerSaverInfo();
  PowerSaverInfo(const PowerSaverInfo& other);

  // Determines the PowerSaverInfo using the peripheral content heuristic.
  static PowerSaverInfo Get(content::RenderFrame* render_frame,
                            bool power_saver_setting_on,
                            const blink::WebPluginParams& params,
                            const content::WebPluginInfo& plugin_info,
                            const GURL& document_url);

  // Whether power saver should be enabled.
  bool power_saver_enabled;

  // Whether the plugin should be deferred because it is in a background tab.
  bool blocked_for_background_tab;

  // The poster image specified in image 'srcset' attribute format.
  std::string poster_attribute;

  // Used to resolve relative paths in |poster_attribute|.
  GURL base_url;

  // Specify this to provide partially obscured plugins a centered poster image.
  gfx::Size custom_poster_size;
};

#endif  // CHROME_RENDERER_PLUGINS_POWER_SAVER_INFO_H_
