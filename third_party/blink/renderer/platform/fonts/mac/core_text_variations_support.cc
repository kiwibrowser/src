// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/fonts/mac/core_text_variations_support.h"

#include <CoreText/CoreText.h>

namespace blink {

// Compare CoreText.h in an up to date SDK, redefining here since we don't seem
// to have access to this value when building against the 10.10 SDK in our
// standard Chrome build configuration.
static const long kBlinkLocalCTVersionNumber10_12 = 0x00090000;

bool CoreTextVersionSupportsVariations() {
  return &CTGetCoreTextVersion &&
         CTGetCoreTextVersion() >= kBlinkLocalCTVersionNumber10_12;
}
}
