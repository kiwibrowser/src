// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_SHAPE_PROPERTIES_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_SHAPE_PROPERTIES_H_

namespace blink {

// Bit field values indicating available display shapes.
enum DisplayShape {
  kDisplayShapeRect = 1 << 0,
  kDisplayShapeRound = 1 << 1,
};

}  // namespace blink

#endif
