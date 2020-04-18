// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/font_fallback.h"

#include <string>
#include <vector>

namespace gfx {

std::vector<Font> GetFallbackFonts(const Font& font) {
  // TODO(fuchsia): Stubbed while bringing up headless build, see
  // https://crbug.com/743296.
  return std::vector<Font>();
}

}  // namespace gfx
