// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_COMMON_EXTENSIONS_FEATURES_H_
#define EXTENSIONS_COMMON_EXTENSIONS_FEATURES_H_

#include "base/feature_list.h"

namespace extensions {
namespace features {

extern const base::Feature kNativeCrxBindings;
extern const base::Feature kNewExtensionUpdaterService;
extern const base::Feature kRuntimeHostPermissions;

}  // namespace features
}  // namespace extensions

#endif  // EXTENSIONS_COMMON_EXTENSIONS_FEATURES_H_
