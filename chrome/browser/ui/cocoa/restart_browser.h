// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_RESTART_BROWSER_H_
#define CHROME_BROWSER_UI_COCOA_RESTART_BROWSER_H_

#import <Cocoa/Cocoa.h>

namespace restart_browser {

// Puts up an alert telling the user to restart their browser.  The alert
// will be hung off |parent| or global otherise.
void RequestRestart(NSWindow* parent);

}  // namespace restart_browser

#endif  // CHROME_BROWSER_UI_COCOA_RESTART_BROWSER_H_
