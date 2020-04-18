// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_SPINNER_VIEW_H_
#define CHROME_BROWSER_UI_COCOA_SPINNER_VIEW_H_

#import <Cocoa/Cocoa.h>

#import "base/mac/scoped_nsobject.h"
#include "base/mac/sdk_forward_declarations.h"
#include "third_party/skia/include/core/SkColor.h"

// A view that displays a Material Design Circular Activity Indicator (aka a
// "spinner") for Mac Chrome. To use, create a SpinnerView of the desired size
// and add to a view hierarchy. SpinnerView uses Core Animation to achieve GPU-
// accelerated animation and smooth scaling to any size.
@interface SpinnerView : NSView
@property(retain, nonatomic) CAAnimation* spinnerAnimation;
@property(retain, nonatomic) CAAnimation* rotationAnimation;

// Returns the time in seconds for one full arc rotation.
+ (CGFloat)arcRotationTime;

// Returns the spinner arc's radius when unscaled.
+ (CGFloat)arcUnitRadius;

// Returns the spinner arc's starting angle.
- (CGFloat)arcStartAngle;

// Returns the spinner arc's end angle delta (i.e. the spinner adds the
// absolute value of this delta to arcStartAngle to compute the end angle).
// Return a negative delta to rotate the spinner counter clockwise.
- (CGFloat)arcEndAngleDelta;

// Returns the arc's length (the distance from arcStart to arcEnd along the
// circumference of a circle with radius arcRadius).
- (CGFloat)arcLength;

// Returns the scale factor for sizing the vector strokes to fit the
// SpinnerView's bounds.
- (CGFloat)scaleFactor;

// Returns the SpinnerView's custom CALayer (called from setWantsLayer:).
- (CALayer*)makeBackingLayer;

// Override point for subclasses to perform animation initialization.
- (void)initializeAnimation;

// Return YES if the spinner is animating.
- (BOOL)isAnimating;

// Returns the default theme's throbber color. Subclasses can override to
// return a custom spinner color.
- (NSColor*)spinnerColor;

// Updates the spinner to the color returned from -spinnerColor.
- (void)updateSpinnerColor;

// Restarts the spinner's animation.
- (void)restartAnimation;

@end

#endif  // CHROME_BROWSER_UI_COCOA_SPINNER_VIEW_H_
