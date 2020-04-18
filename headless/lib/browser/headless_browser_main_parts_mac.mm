// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/lib/browser/headless_browser_main_parts.h"

#import <Cocoa/Cocoa.h>

#include "headless/lib/browser/headless_shell_application_mac.h"

namespace headless {

void HeadlessBrowserMainParts::PreMainMessageLoopStart() {
  // Force the NSApplication subclass to be used.
  [HeadlessShellCrApplication sharedApplication];
  // Force hide dock and menu bar.
  [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];
}

}  // namespace headless
