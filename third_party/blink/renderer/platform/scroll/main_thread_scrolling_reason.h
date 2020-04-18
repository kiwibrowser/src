// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCROLL_MAIN_THREAD_SCROLLING_REASON_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCROLL_MAIN_THREAD_SCROLLING_REASON_H_

#include "cc/input/main_thread_scrolling_reason.h"

namespace blink {

// A wrapper around cc's structure to expose it to core.
struct MainThreadScrollingReason : public cc::MainThreadScrollingReason {};

}  // namespace blink

#endif
