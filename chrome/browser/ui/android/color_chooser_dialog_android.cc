// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/browser_dialogs.h"

// The actual android color chooser is at
// components/web_contents_delegate_android/color_chooser_android.cc

namespace chrome {

content::ColorChooser* ShowColorChooser(content::WebContents* web_contents,
                                        SkColor initial_color) {
  return NULL;
}

}  // namespace chrome
