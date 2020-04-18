// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/content_settings/chrome_content_settings_utils.h"

#include "base/metrics/histogram_macros.h"

namespace content_settings {

void RecordMixedScriptAction(MixedScriptAction action) {
  UMA_HISTOGRAM_ENUMERATION("ContentSettings.MixedScript", action,
                            MIXED_SCRIPT_ACTION_COUNT);
}

void RecordPluginsAction(PluginsAction action) {
  UMA_HISTOGRAM_ENUMERATION("ContentSettings.Plugins", action,
                            PLUGINS_ACTION_COUNT);
}

void RecordPopupsAction(PopupsAction action) {
  UMA_HISTOGRAM_ENUMERATION("ContentSettings.Popups", action,
                            POPUPS_ACTION_COUNT);
}

}  // namespace content_settings
