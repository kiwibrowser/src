// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/plugin_utils.h"

#include "base/strings/utf_string_conversions.h"
#include "content/public/common/content_constants.h"
#include "content/public/common/webplugininfo.h"

bool ShouldUseJavaScriptSettingForPlugin(const content::WebPluginInfo& plugin) {
  if (plugin.name == base::ASCIIToUTF16(content::kFlashPluginName))
    return false;

  // Since all the UI surfaces for Plugin content settings display "Flash",
  // treat all other plugins as JavaScript. These include all of:
  //  - Internally registered plugins such as:
  //    - NaCl
  //    - Widevine
  //    - PDF
  //  - Custom plugins loaded from the command line
  return true;
}
