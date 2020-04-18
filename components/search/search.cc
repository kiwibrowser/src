// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/search/search.h"

#include "build/build_config.h"

namespace search {

bool IsInstantExtendedAPIEnabled() {
#if defined(OS_IOS) || defined(OS_ANDROID)
  return false;
#else
  return true;
#endif
}

}  // namespace search
