// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/lib/browser/headless_shell_application_mac.h"

#include "base/auto_reset.h"

@implementation HeadlessShellCrApplication

- (BOOL)isHandlingSendEvent {
  // Since headless mode is non-interactive, always return false.
  return false;
}

// This method is required to allow handling some input events on Mac OS.
- (void)setHandlingSendEvent:(BOOL)handlingSendEvent {
}

@end
