// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file intentionally does not have header guards, it's included
// inside a macro to generate enum values.

#ifndef DEFINE_FONT_FAMILY
#error "DEFINE_FONT_FAMILY should be defined before including this file"
#endif

// First argument represents the enum name, second argument represents enum
// value.
// These must be kept in sync with the resource strings in
// chrome/android/java/res/values/arrays.xml
DEFINE_FONT_FAMILY(SANS_SERIF, 0)
DEFINE_FONT_FAMILY(SERIF, 1)
DEFINE_FONT_FAMILY(MONOSPACE, 2)
DEFINE_FONT_FAMILY(FONT_FAMILY_COUNT, 3)
