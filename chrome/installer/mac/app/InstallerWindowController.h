// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_MAC_APP_INSTALLERWINDOWCONTROLLER_H_
#define CHROME_INSTALLER_MAC_APP_INSTALLERWINDOWCONTROLLER_H_

#import <AppKit/AppKit.h>

@interface InstallerWindowController : NSWindowController

- (id)initWithWindow:(NSWindow*)window;
- (void)updateStatusDescription:(NSString*)text;
- (void)updateDownloadProgress:(double)progressPercent;
- (void)enableLaunchButton;

- (BOOL)isUserMetricsChecked;
- (BOOL)isDefaultBrowserChecked;
@end

#endif  // CHROME_INSTALLER_MAC_APP_INSTALLERWINDOWCONTROLLER_H_
