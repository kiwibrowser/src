// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"
#include "ui/gfx/switches.h"

namespace switches {

#if defined(OS_WIN)
// Disables DirectWrite font rendering for general UI elements.
const char kDisableDirectWriteForUI[] = "disable-directwrite-for-ui";
#endif

#if defined(OS_MACOSX)
// Enables the HarfBuzz port of RenderText on Mac (it's already used only for
// text editing; this enables it for everything else).
const char kEnableHarfBuzzRenderText[] = "enable-harfbuzz-rendertext";
#endif

// Force disables font subpixel positioning. This affects the character glyph
// sharpness, kerning, hinting and layout.
const char kDisableFontSubpixelPositioning[] =
    "disable-font-subpixel-positioning";

// Run in headless mode, i.e., without a UI or display server dependencies.
const char kHeadless[] = "headless";

}  // namespace switches

namespace features {

// Enables or disables an experimental Bidi URL rendering algorithm where the
// URL components are always shown in order from left to right, regardless of
// any RTL characters. (The contents of each component are still rendered with
// the normal Bidi algorithm.)
const base::Feature kLeftToRightUrls{"LeftToRightUrls",
                                     base::FEATURE_DISABLED_BY_DEFAULT};

}  // namespace features
