// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_FADE_TRUNCATED_LABEL_H_
#define IOS_CHROME_BROWSER_UI_FADE_TRUNCATED_LABEL_H_

#import <UIKit/UIKit.h>

// A label class that applies a gradient fade to the end of a label whose bounds
// are too small to draw the entire string.  This class uses a CAGradientLayer
// as a mask instead of clipping the drawing context of the label.
@interface FadeTruncatedLabel : UILabel

// Returns a CAGadientLayer to use as a label's masking layer that will apply
// a fade truncation if |text| takes up more space than |bounds| when drawn
// with |attributes|.
+ (CAGradientLayer*)maskLayerForText:(NSString*)text
                      withAttributes:(NSDictionary*)attributes
                            inBounds:(CGRect)bounds;

// Adds animations from |beginFrame| to |endFrame| using the provided |duration|
// and |timingFunction|.
- (void)animateFromBeginFrame:(CGRect)beginFrame
                   toEndFrame:(CGRect)endFrame
                     duration:(CFTimeInterval)duration
               timingFunction:(CAMediaTimingFunction*)timingFunction;

// Reverses animations added by
// |-animateFromBeginFrame:toEndFrame:duration:timingFunction:|.
- (void)reverseAnimations;

// Removes animations added by
// |-animateFromBeginFrame:toEndFrame:duration:timingFunction:|.
- (void)cleanUpAnimations;

@end

#endif  // IOS_CHROME_BROWSER_UI_FADE_TRUNCATED_LABEL_H_
