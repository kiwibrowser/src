// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NACL_COMMON_NACL_NONSFI_UTIL_H_
#define COMPONENTS_NACL_COMMON_NACL_NONSFI_UTIL_H_

namespace nacl {

// Returns true if non-SFI mode *can* run on the current platform and if non-SFI
// manifest entries are preferred.  There can be other restrictions which
// prevent a particular module from launching.  See NaClProcessHost::Launch
// which makes the final determination.
bool IsNonSFIModeEnabled();

}  // namespace nacl

#endif  // COMPONENTS_NACL_COMMON_NACL_NONSFI_UTIL_H_
