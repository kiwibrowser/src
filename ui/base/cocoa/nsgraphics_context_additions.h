// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_COCOA_NSGRAPHICS_CONTEXT_ADDITIONS_H_
#define UI_BASE_COCOA_NSGRAPHICS_CONTEXT_ADDITIONS_H_

#import <Cocoa/Cocoa.h>

@interface NSGraphicsContext (CrAdditions)

// When a view is not layer backed the pattern phase is relative to the origin
// of the window's content view. With a layer backed view the pattern phase is
// relative to the origin of the view.
//
// For layer backed view this method offsets the pattern phase to match the
// behavior of non layer backed views.
- (void)cr_setPatternPhase:(NSPoint)phase
                   forView:(NSView*)view;

@end

#endif  // UI_BASE_COCOA_NSGRAPHICS_CONTEXT_ADDITIONS_H_
