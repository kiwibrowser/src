// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_LAYER_TREE_SETTINGS_FACTORY_H_
#define CONTENT_COMMON_LAYER_TREE_SETTINGS_FACTORY_H_

#include "base/command_line.h"
#include "base/macros.h"
#include "cc/trees/layer_tree_settings.h"
#include "content/common/content_export.h"

namespace content {

// LayerTreeSettingsFactory holds utilities functions to generate
// LayerTreeSettings.
class CONTENT_EXPORT LayerTreeSettingsFactory {
  // TODO(xingliu): Refactor LayerTreeSettings generation logic.
  // crbug.com/577985
 public:
  static void SetBrowserControlsSettings(cc::LayerTreeSettings& settings,
                                         const base::CommandLine& command_line);
};

}  // namespace content

#endif  // CONTENT_COMMON_LAYER_TREE_SETTINGS_FACTORY_H_
