// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/common/extensions_api/cast_aliases.h"

namespace extensions {

std::vector<Alias> GetCastPermissionAliases() {
  // In alias constructor, first value is the alias name; second value is the
  // real name. See also alias.h.
  return {Alias("windows", "tabs")};
}

}  // namespace extensions
