// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_PUBLIC_UTIL_FONTCONFIG_H_
#define HEADLESS_PUBLIC_UTIL_FONTCONFIG_H_

#include "headless/public/headless_export.h"

namespace headless {

// Initialize fontconfig by loading fonts from given path without following
// symlinks. This is a wrapper around FcInit from libfreetype bundled with
// Chromium modified to enable headless embedders to deploy in custom
// environments.
HEADLESS_EXPORT void InitFonts(const char* font_config_path);

HEADLESS_EXPORT void ReleaseFonts();

}  // namespace headless

#endif  // HEADLESS_PUBLIC_UTIL_FONTCONFIG_H_
