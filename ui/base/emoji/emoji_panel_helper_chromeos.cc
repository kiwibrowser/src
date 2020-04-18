// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/emoji/emoji_panel_helper.h"

#include "base/feature_list.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "ui/base/ui_base_features.h"

namespace ui {

namespace {

base::RepeatingClosure& GetShowEmojiKeyboardCallback() {
  static base::NoDestructor<base::RepeatingClosure> callback;
  return *callback;
}

}  // namespace

bool IsEmojiPanelSupported() {
  return base::FeatureList::IsEnabled(features::kEnableEmojiContextMenu);
}

void ShowEmojiPanel() {
  DCHECK(GetShowEmojiKeyboardCallback());
  GetShowEmojiKeyboardCallback().Run();
}

void SetShowEmojiKeyboardCallback(base::RepeatingClosure callback) {
  GetShowEmojiKeyboardCallback() = callback;
}

}  // namespace ui
