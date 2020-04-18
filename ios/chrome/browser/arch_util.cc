// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/arch_util.h"

namespace arch_util {

// const char[] can be initialized only with literal.
#define _ARM_ARCH "arm"
#define _ARM_64_ARCH "arm64"

const char kARMArch[] = _ARM_ARCH;

const char kARM64Arch[] = _ARM_64_ARCH;

#if defined(__LP64__)
const char kCurrentArch[] = _ARM_64_ARCH;
#else
const char kCurrentArch[] = _ARM_ARCH;
#endif

}  // namespace arch_util
