// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/font_names_testing.h"

#include "build/build_config.h"

namespace gfx {

#if defined(OS_LINUX)
const char kTestFontName[] = "Arimo";
#else
const char kTestFontName[] = "Arial";
#endif

#if defined(OS_LINUX)
const char kSymbolFontName[] = "DejaVu Sans";
#else
const char kSymbolFontName[] = "Symbol";
#endif

#if defined(OS_LINUX)
const char kCJKFontName[] = "Noto Sans CJK JP";
#elif defined(OS_MACOSX)
const char kCJKFontName[] = "Heiti SC";
#else
const char kCJKFontName[] = "SimSun";
#endif

} // namespace gfx
