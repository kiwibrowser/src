// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/embedder/mac_init.h"

#import <Cocoa/Cocoa.h>

namespace service_manager {

void InitializeMac() {
  [[NSUserDefaults standardUserDefaults] registerDefaults:@{
    // Exceptions routed to -[NSApplication reportException:] should crash
    // immediately, as opposed being swallowed or presenting UI that gives the
    // user a choice in the matter.
    @"NSApplicationCrashOnExceptions" : @YES,

    // Prevent Cocoa from turning command-line arguments into -[NSApplication
    // application:openFile:], because they are handled directly. @"NO" looks
    // like a mistake, but the value really is supposed to be a string.
    @"NSTreatUnknownArgumentsAsOpen" : @"NO",
  }];
}

}  // namespace service_manager
