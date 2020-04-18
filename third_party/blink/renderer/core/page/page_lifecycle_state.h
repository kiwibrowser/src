// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_PAGE_LIFECYCLE_STATE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_PAGE_LIFECYCLE_STATE_H_

namespace blink {

enum class PageLifecycleState {
  kUnknown,  // The page state is unknown.
  kActive,   // The page is visible and active.
  kHidden,   // The page is not visible but active.
  kFrozen,   // The page is frozen.
};

}  // namespace blink
#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_PAGE_LIFECYCLE_STATE_H_
