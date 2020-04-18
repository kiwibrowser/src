// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/extensions_aliases.h"

namespace extensions {

std::vector<Alias> GetExtensionsPermissionAliases() {
  // In alias constructor, first value is the alias name; second value is the
  // real name. See also alias.h.
  return {Alias("alwaysOnTopWindows", "app.window.alwaysOnTop"),
          Alias("fullscreen", "app.window.fullscreen"),
          Alias("overrideEscFullscreen", "app.window.fullscreen.overrideEsc"),
          Alias("unlimited_storage", "unlimitedStorage")};
}

}  // namespace extensions
