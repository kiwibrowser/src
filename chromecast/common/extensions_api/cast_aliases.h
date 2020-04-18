// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_COMMON_EXTENSIONS_API_CAST_ALIASES_H_
#define CHROMECAST_COMMON_EXTENSIONS_API_CAST_ALIASES_H_

#include <vector>

#include "extensions/common/alias.h"

namespace extensions {

std::vector<Alias> GetCastPermissionAliases();

}  // namespace extensions

#endif  // CHROMECAST_COMMON_EXTENSIONS_API_CAST_ALIASES_H_
