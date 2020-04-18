// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "NSAlert+ChromeInstallerAdditions.h"

@implementation NSAlert (ChromeInstallerAdditions)
// In the one-button scenario, the button would be just "Quit." In the
// two-button scenario, the first button would allow the user to "Retry" and
// the second button would provide the "Quit" option.
- (NSModalResponse)quitResponse {
  return ([[self buttons] count] == 1) ? NSAlertFirstButtonReturn
                                       : NSAlertSecondButtonReturn;
}
@end
