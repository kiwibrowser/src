// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_MAC_APP_NSERROR_CHROMEINSTALLERADDITIONS_H_
#define CHROME_INSTALLER_MAC_APP_NSERROR_CHROMEINSTALLERADDITIONS_H_

#import <Foundation/Foundation.h>

@interface NSError (ChromeInstallerAdditions)
// Creates a custom error object to be used to create an alert to be shown to
// the user.
+ (NSError*)errorForAlerts:(NSString*)message
           withDescription:(NSString*)description
             isRecoverable:(BOOL)recoverable;
@end

#endif  // CHROME_INSTALLER_MAC_APP_NSERROR_CHROMEINSTALLERADDITIONS_H_
