// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_ARCH_UTIL_H_
#define IOS_CHROME_BROWSER_ARCH_UTIL_H_

namespace arch_util {

// Architecture of the currently running binary. Depends on the combination of
// app binary archs and device's arch. e.g. for arm7/arm64 fat binary running
// on 64-bit processor the value will be "arm64", but for the same fat binary
// running on 32-bit processor the value will be "arm".
extern const char kCurrentArch[];

// Constant for 32-bit ARM architecture.
extern const char kARMArch[];

// Constant for 64-bit ARM architecture.
extern const char kARM64Arch[];

}  // namespace arch_util

#endif  // IOS_CHROME_BROWSER_ARCH_UTIL_H_
