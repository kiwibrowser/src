// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_PEPPER_PLUGIN_LIST_H_
#define CONTENT_COMMON_PEPPER_PLUGIN_LIST_H_

#include <vector>

#include "ppapi/buildflags/buildflags.h"

#if !BUILDFLAG(ENABLE_PLUGINS)
#error "Plugins should be enabled"
#endif

namespace content {

struct PepperPluginInfo;
struct WebPluginInfo;

// Constructs a PepperPluginInfo from a WebPluginInfo. Returns false if
// the operation is not possible, in particular the WebPluginInfo::type
// must be one of the pepper types.
bool MakePepperPluginInfo(const WebPluginInfo& webplugin_info,
                          PepperPluginInfo* pepper_info);

// Computes the list of known pepper plugins.
void ComputePepperPluginList(std::vector<PepperPluginInfo>* plugins);

}  // namespace content

#endif  // CONTENT_COMMON_PEPPER_PLUGIN_LIST_H_
