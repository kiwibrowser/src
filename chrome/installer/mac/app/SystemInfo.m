// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <mach-o/arch.h>

#import "SystemInfo.h"

@implementation SystemInfo

+ (NSString*)getArch {
  // NOTE: It seems the below function `NSGetLocalArchInfo` returns an
  // arch->name that is either "x84_64h" or "i486".
  const NXArchInfo* arch = NXGetLocalArchInfo();
  NSString* archName = [NSString stringWithUTF8String:arch->name];
  return archName;
}

+ (NSString*)getOSVersion {
  NSDictionary* systemVersion =
      [NSDictionary dictionaryWithContentsOfFile:
                        @"/System/Library/CoreServices/SystemVersion.plist"];
  NSString* versionNumber = [systemVersion objectForKey:@"ProductVersion"];

  return versionNumber;
}

@end
