// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "content/browser/renderer_host/pepper/pepper_truetype_font.h"

namespace content {

// static
PepperTrueTypeFont* PepperTrueTypeFont::Create() {
  NOTIMPLEMENTED();  // Not implemented on Android.
  return NULL;
}

}  // namespace content
