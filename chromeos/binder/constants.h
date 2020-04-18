// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_BINDER_CONSTANTS_H_
#define CHROMEOS_BINDER_CONSTANTS_H_

#define BINDER_PACK_CHARS(c1, c2, c3, c4) \
  (((c1) << 24) | ((c2) << 16) | ((c3) << 8) | (c4))

namespace binder {

// Context manager's handle is always 0.
const uint32_t kContextManagerHandle = 0;

// Transaction code constants.
const uint32_t kFirstTransactionCode = 0x00000001;
const uint32_t kLastTransactionCode = 0x00ffffff;
const uint32_t kPingTransactionCode = BINDER_PACK_CHARS('_', 'P', 'N', 'G');

// Note: must be kept in sync with PENALTY_GATHER in Android's StrictMode.java.
const int32_t kStrictModePenaltyGather = (0x40 << 16);

}  // namespace binder

#endif  // CHROMEOS_BINDER_CONSTANTS_H_
