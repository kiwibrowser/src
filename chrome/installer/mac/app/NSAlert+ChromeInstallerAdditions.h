// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_MAC_APP_NSALERT_CHROMEINSTALLERADDITIONS_H_
#define CHROME_INSTALLER_MAC_APP_NSALERT_CHROMEINSTALLERADDITIONS_H_

#import <AppKit/AppKit.h>

typedef NSInteger NSModalResponse;
@interface NSAlert (ChromeInstallerAdditions)
// Allows the caller to determine whether to determine the app's quit button was
// pressed or not.
- (NSModalResponse)quitResponse;
@end

#endif  // CHROME_INSTALLER_MAC_APP_NSALERT_CHROMEINSTALLERADDITIONS_H_
