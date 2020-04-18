// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/platform_font.h"

#include "base/logging.h"

namespace gfx {

// static
PlatformFont* PlatformFont::CreateDefault() {
  // TODO(fuchsia): Stubbed during headless bringup, https://crbug.com/743296.
  NOTIMPLEMENTED();
  return NULL;
}

// static
PlatformFont* PlatformFont::CreateFromNameAndSize(const std::string& font_name,
                                                  int font_size) {
  // TODO(fuchsia): Stubbed during headless bringup, https://crbug.com/743296.
  NOTIMPLEMENTED();
  return NULL;
}

}  // namespace gfx
