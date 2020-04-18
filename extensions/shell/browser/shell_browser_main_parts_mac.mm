// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/shell/browser/shell_browser_main_parts.h"

#import "content/shell/browser/shell_application_mac.h"

namespace extensions {

void MainPartsPreMainMessageLoopStartMac() {
  // Force the NSApplication subclass to be used.
  [ShellCrApplication sharedApplication];
}

}  // namespace extensions
