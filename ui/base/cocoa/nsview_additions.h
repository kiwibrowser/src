// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_COCOA_NSVIEW_ADDITIONS_H_
#define UI_BASE_COCOA_NSVIEW_ADDITIONS_H_

#import <Cocoa/Cocoa.h>

@interface NSView (ChromeAdditions)

// Returns the line width that will generate a 1 pixel wide line.
- (CGFloat)cr_lineWidth;

// Checks if the mouse is currently in this view.
- (BOOL)cr_isMouseInView;

// Returns YES if this view is below |otherView|.
- (BOOL)cr_isBelowView:(NSView*)otherView;

// Returns YES if this view is aobve |otherView|.
- (BOOL)cr_isAboveView:(NSView*)otherView;

// Ensures that the z-order of |subview| is correct relative to |otherView|.
- (void)cr_ensureSubview:(NSView*)subview
            isPositioned:(NSWindowOrderingMode)place
              relativeTo:(NSView *)otherView;

// Return best color for keyboard focus ring.
- (NSColor*)cr_keyboardFocusIndicatorColor;

// Invoke |block| on this view and all descendants.
- (void)cr_recursivelyInvokeBlock:(void (^)(id view))block;

// Set needsDisplay for this view and all descendants.
- (void)cr_recursivelySetNeedsDisplay:(BOOL)flag;

// Draw using ancestorView's drawRect function into this view's rect. Do any
// required translating or flipping to transform between the two coordinate
// systems, and optionally clip to the ancestor view's bounds.
- (void)cr_drawUsingAncestor:(NSView*)ancestorView inRect:(NSRect)dirtyRect
     clippedToAncestorBounds:(BOOL)clipToAncestorBounds;

// Same as cr_drawUsingAncestor:inRect:clippedToAncestorBounds: except always
// clips to the ancestor view's bounds.
- (void)cr_drawUsingAncestor:(NSView*)ancestorView inRect:(NSRect)dirtyRect;

// Used by ancestorView in the above draw call, to look up the child view that
// it is actually drawing to.
- (NSView*)cr_viewBeingDrawnTo;

// Set a view's accessibilityLabel in a way that's compatible with 10.9.
- (void)cr_setAccessibilityLabel:(NSString*)label;

@end

#endif  // UI_BASE_COCOA_NSVIEW_ADDITIONS_H_
