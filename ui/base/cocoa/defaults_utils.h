// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_COCOA_DEFAULTS_UTILS_H_
#define UI_BASE_COCOA_DEFAULTS_UTILS_H_

#include "base/time/time.h"
#include "ui/base/ui_base_export.h"

namespace ui {

// Returns the text insertion caret blink period, if one is configured in
// NSUserDefaults.
UI_BASE_EXPORT bool TextInsertionCaretBlinkPeriod(base::TimeDelta* period);

}

#endif
