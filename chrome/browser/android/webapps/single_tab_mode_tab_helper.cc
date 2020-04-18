// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/webapps/single_tab_mode_tab_helper.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(SingleTabModeTabHelper);

SingleTabModeTabHelper::SingleTabModeTabHelper(
    content::WebContents* web_contents) {}

SingleTabModeTabHelper::~SingleTabModeTabHelper() {}

void SingleTabModeTabHelper::PermanentlyBlockAllNewWindows() {
  block_all_new_windows_ = true;
}
