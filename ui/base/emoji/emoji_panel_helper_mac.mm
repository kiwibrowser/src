// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/emoji/emoji_panel_helper.h"

#import <Cocoa/Cocoa.h>

#include "base/feature_list.h"
#include "ui/base/ui_base_features.h"

namespace ui {

bool IsEmojiPanelSupported() {
  return base::FeatureList::IsEnabled(features::kEnableEmojiContextMenu);
}

void ShowEmojiPanel() {
  [NSApp orderFrontCharacterPalette:nil];
}

}  // namespace ui
