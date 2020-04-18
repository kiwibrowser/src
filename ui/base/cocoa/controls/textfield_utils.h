// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_COCOA_CONTROLS_TEXTFIELD_UTILS_H_
#define UI_BASE_COCOA_CONTROLS_TEXTFIELD_UTILS_H_

#include "ui/base/ui_base_export.h"

#include <Cocoa/Cocoa.h>

UI_BASE_EXPORT
@interface TextFieldUtils : NSObject

// This method is a polyfill for a method on NSTextField on macOS 10.12+.
// TODO(ellyjones): Once we target only 10.12+, delete this and convert uses
// over to NSTextField.
+ (NSTextField*)labelWithString:(NSString*)text;

@end

#endif  // UI_BASE_COCOA_CONTROLS_TEXTFIELD_UTILS_H_
