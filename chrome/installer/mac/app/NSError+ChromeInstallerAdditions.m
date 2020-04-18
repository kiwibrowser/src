// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "NSError+ChromeInstallerAdditions.h"

@implementation NSError (ChromeInstallerAdditions)
+ (NSError*)errorForAlerts:(NSString*)message
           withDescription:(NSString*)description
             isRecoverable:(BOOL)recoverable {
  NSArray* options = @[];
  if (recoverable) {
    options = @[ @"Try Again", @"Quit" ];
  } else {
    options = @[ @"Quit" ];
  }

  NSDictionary* errorContents = @{
    NSLocalizedDescriptionKey : NSLocalizedString(message, nil),
    NSLocalizedRecoveryOptionsErrorKey : options,
    NSLocalizedRecoverySuggestionErrorKey : NSLocalizedString(description, nil)
  };
  return [NSError errorWithDomain:@"ChromeErrorDomain"
                             code:-1
                         userInfo:errorContents];
}
@end
