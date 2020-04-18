// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/platform_font.h"

#include "base/logging.h"

namespace gfx {

// static
PlatformFont* PlatformFont::CreateDefault() {
  NOTIMPLEMENTED();
  return NULL;
}

// static
PlatformFont* PlatformFont::CreateFromNameAndSize(const std::string& font_name,
                                                  int font_size) {
  // If you implement this, enable FontTest.DeriveFont on Android,
  // https://crbug.com/642010
  NOTIMPLEMENTED();
  return NULL;
}

} // namespace gfx
