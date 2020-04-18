// Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONSTANTS_CRYPTOHOME_H_
#define CONSTANTS_CRYPTOHOME_H_

#include <stdint.h>

namespace cryptohome {

// Cleanup is trigerred if the amount of free disk space goes below this value.
const int64_t kMinFreeSpaceInBytes = 512 * 1LL << 20;

}  // namespace cryptohome

#endif  // CONSTANTS_CRYPTOHOME_H_
