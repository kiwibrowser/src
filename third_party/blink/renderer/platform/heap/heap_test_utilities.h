/*
 * Copyright 2017 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_HEAP_TEST_UTILITIES_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_HEAP_TEST_UTILITIES_H_

namespace blink {

void PreciselyCollectGarbage();
void ConservativelyCollectGarbage();
void ClearOutOldGarbage();

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_HEAP_TEST_UTILITIES_H_
