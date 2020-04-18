// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_MEMORY_PRESSURE_LEVEL_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_MEMORY_PRESSURE_LEVEL_H_

namespace blink {

// These number must correspond to
// base::MemoryPressureListener::MemoryPressureLevel.
enum WebMemoryPressureLevel {
  kWebMemoryPressureLevelNone,
  kWebMemoryPressureLevelModerate,
  kWebMemoryPressureLevelCritical,
};

}  // namespace blink

#endif
