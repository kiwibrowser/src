// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <string>

#include "base/macros.h"
#include "build/build_config.h"

#if defined(OS_CHROMEOS)
#include "base/strings/string_util.h"
#endif

namespace l10n_util {

bool IsLocaleSupportedByOS(const std::string& locale) {
#if defined(OS_CHROMEOS)
  // We don't have translations yet for am, and sw.
  // TODO(jungshik): Once the above issues are resolved, change this back
  // to return true.
  static const char kUnsupportedLocales[][3] = {"am", "sw"};
  for (size_t i = 0; i < arraysize(kUnsupportedLocales); ++i) {
    if (base::LowerCaseEqualsASCII(locale, kUnsupportedLocales[i]))
      return false;
  }
  return true;
#else
  // Return true blindly for now.
  return true;
#endif
}

}  // namespace l10n_util
