// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_COCOA_VIEW_DESCRIPTION_H_
#define UI_BASE_COCOA_VIEW_DESCRIPTION_H_

#import <Cocoa/Cocoa.h>

#if !NDEBUG

@interface NSView (CrDebugging)

// Returns a description of all the subviews and each's frame for debugging.
- (NSString*)cr_recursiveDescription;

@end

#endif  // !NDEBUG

#endif  // UI_BASE_COCOA_VIEW_DESCRIPTION_H_
