// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file intentionally does not have header guards, it's included
// inside a macro to generate enum values.

#ifndef DEFINE_THEME
#error "DEFINE_THEME should be defined before including this file"
#endif

// First argument represents the enum name, second argument represents enum
// value. THEME_COUNT used only by native enum.
DEFINE_THEME(LIGHT, 0)
DEFINE_THEME(DARK, 1)
DEFINE_THEME(SEPIA, 2)
DEFINE_THEME(THEME_COUNT, 3)
