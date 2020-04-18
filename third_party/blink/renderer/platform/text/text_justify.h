// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_TEXT_TEXT_JUSTIFY_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_TEXT_TEXT_JUSTIFY_H_

namespace blink {

enum class TextJustify : unsigned {
  kAuto = 0x0,
  kNone = 0x1,
  kInterWord = 0x2,
  kDistribute = 0x3
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_TEXT_TEXT_JUSTIFY_H_
