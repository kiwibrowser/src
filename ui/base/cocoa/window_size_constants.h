// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_COCOA_WINDOW_SIZE_CONSTANTS_H_
#define UI_BASE_COCOA_WINDOW_SIZE_CONSTANTS_H_

#include "ui/base/ui_base_export.h"

#import <Foundation/Foundation.h>

namespace ui {

// It is not valid to make a zero-sized window. Use this constant instead.
UI_BASE_EXPORT extern const NSRect kWindowSizeDeterminedLater;

}  // namespace ui

#endif  // UI_BASE_COCOA_WINDOW_SIZE_CONSTANTS_H_
