// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_MAC_APP_AUTHORIZEDINSTALL_H_
#define CHROME_INSTALLER_MAC_APP_AUTHORIZEDINSTALL_H_

#import <Foundation/Foundation.h>
#import <Security/Security.h>

@interface AuthorizedInstall : NSObject

// Attempts to gain elevated permissions, then starts the subprocess with the
// appropriate level of privilege.
- (BOOL)loadInstallationTool;

// Signals the tool to begin the installation. Returns the path to the
// installed app.
- (NSString*)startInstall:(NSString*)appBundlePath;

@end

#endif  // CHROME_INSTALLER_MAC_APP_AUTHORIZEDINSTALL_H_
