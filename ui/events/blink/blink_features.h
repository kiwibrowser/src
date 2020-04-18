// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_BLINK_BLINK_FEATURES_H_
#define UI_EVENTS_BLINK_BLINK_FEATURES_H_

#include "base/feature_list.h"

namespace features {

extern const base::Feature kVsyncAlignedInputEvents;

// This feature allows native ET_MOUSE_EXIT events to be passed
// through to blink as mouse leave events. Traditionally these events were
// converted to mouse move events due to a number of inconsistencies on
// the native platforms. crbug.com/450631
extern const base::Feature kSendMouseLeaveEvents;
}

#endif  // UI_EVENTS_BLINK_BLINK_FEATURES_H_
