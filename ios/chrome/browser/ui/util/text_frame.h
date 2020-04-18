// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_UTIL_TEXT_FRAME_H_
#define IOS_CHROME_BROWSER_UI_UTIL_TEXT_FRAME_H_

#import <CoreText/CoreText.h>
#import <UIKit/UIKit.h>

// An object encapsulating a line of text that has been framed within a bounding
// rect.
@interface FramedLine : NSObject

// Designated initializer.
- (instancetype)initWithLine:(CTLineRef)line
                 stringRange:(NSRange)stringRange
                      origin:(CGPoint)origin NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;

// The CTLine that was framed.
@property(nonatomic, readonly) CTLineRef line;
// The range within the original string that corresponds with |line|.  CTLines
// created by ManualTextFramers report string ranges with 0 for their locations,
// so the actual range is stored here separately.
@property(nonatomic, readonly) NSRange stringRange;
// The baseline origin (in Quartz coordinates) of the line within its bounds.
@property(nonatomic, readonly) CGPoint origin;

// Returns the offset into |line| of the glyph corresponding with the character
// in the original string at |stringLocation|.  Returns kCFNotFound if
// |stringLocation| is outside of |stringRange|.
- (CFIndex)lineOffsetForStringLocation:(NSUInteger)stringLocation;

@end

// Protocol for objects that contain NSAttributedStrings laid out within a
// bounding rect.
@protocol TextFrame<NSObject>

// The string that was framed within |bounds|.
@property(nonatomic, readonly) NSAttributedString* string;
// The range of |string| that was successfully framed.
@property(nonatomic, readonly) NSRange framedRange;
// An NSArray of FramedLines corresponding to |string| as laid out in |bounds|.
@property(nonatomic, readonly) NSArray* lines;
// The bounds in which |string| is laid out.
@property(nonatomic, readonly) CGRect bounds;

@end

// A TextFrame implementation that is backed by a CTFrameRef.
@interface CoreTextTextFrame : NSObject<TextFrame>

// Designated initializer.
- (instancetype)initWithString:(NSAttributedString*)string
                        bounds:(CGRect)bounds
                         frame:(CTFrameRef)frame NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_BROWSER_UI_UTIL_TEXT_FRAME_H_
