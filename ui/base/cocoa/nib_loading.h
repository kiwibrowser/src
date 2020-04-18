// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_COCOA_NIB_LOADING_H_
#define UI_BASE_COCOA_NIB_LOADING_H_

#import <Cocoa/Cocoa.h>

#include "ui/base/ui_base_export.h"

namespace ui {

// Given the name of a nib file, gets an unowned reference to the NSView in the
// nib. Requires a nib with just a single root view.
UI_BASE_EXPORT NSView* GetViewFromNib(NSString* name);

}  // namespace ui

#endif  // UI_BASE_COCOA_NIB_LOADING_H_
