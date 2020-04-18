// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/cocoa/touch_bar_forward_declarations.h"

#if !defined(MAC_OS_X_VERSION_10_12_1) || \
    MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_12_1
NSString* const NSTouchBarItemIdentifierFixedSpaceSmall =
    @"NSTouchBarItemIdentifierFixedSpaceSmall";
NSString* const NSTouchBarItemIdentifierFlexibleSpace =
    @"NSTouchBarItemIdentifierFlexibleSpace";
#endif  // MAC_OS_X_VERSION_10_12_1
