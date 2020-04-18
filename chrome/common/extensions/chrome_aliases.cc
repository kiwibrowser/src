// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/extensions/chrome_aliases.h"

namespace extensions {

std::vector<Alias> GetChromePermissionAliases() {
  // In alias constructor, first value is the alias name; second value is the
  // real name. See also alias.h.
  return {Alias("windows", "tabs")};
}

}  // namespace extensions
