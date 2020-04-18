// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_API_HIT_TEST_ACTION_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_API_HIT_TEST_ACTION_H_

namespace blink {

enum HitTestAction {
  kHitTestBlockBackground,
  kHitTestChildBlockBackground,
  kHitTestChildBlockBackgrounds,
  kHitTestFloat,
  kHitTestForeground
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_API_HIT_TEST_ACTION_H_
