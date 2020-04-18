// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_PLUGIN_UTILS_H_
#define CHROME_COMMON_PLUGIN_UTILS_H_

namespace content {
struct WebPluginInfo;
}

// Returns true if |plugin| should use the JavaScript Content Settings.
bool ShouldUseJavaScriptSettingForPlugin(const content::WebPluginInfo& plugin);

#endif  // CHROME_COMMON_PLUGIN_UTILS_H_
