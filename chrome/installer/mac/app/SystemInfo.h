// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_MAC_APP_SYSTEMINFO_H_
#define CHROME_INSTALLER_MAC_APP_SYSTEMINFO_H_

#if !defined(__x86_64__)
#error "Your machine's system architecture may not be compatible with Chrome."
#endif

#import <Foundation/Foundation.h>

@interface SystemInfo : NSObject
// Gets the CPU architecture type of the client's system, which will be used
// when crafting the query to Omaha. This should return either "x84_64h" for
// systems running on Intel Haswell chips, "i486" for other Intel machines, or
// strings representing other CPU types ("amd", "pentium", and "i686", for
// example, are all possible; however, due to the above macro, the possible
// return strings are limited to either "x84_64h" or "i486").
+ (NSString*)getArch;

// Gets the operating system version of the client. This function may return
// values such as "10.11" or "10.10.5".
+ (NSString*)getOSVersion;

@end

#endif  // CHROME_INSTALLER_MAC_APP_SYSTEMINFO_H_
