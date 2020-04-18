// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_UTIL_TEXT_REGION_MAPPER_H_
#define IOS_CHROME_BROWSER_UI_UTIL_TEXT_REGION_MAPPER_H_

#import <UIKit/UIKit.h>

// A TextRegionMapper is a utility class that can, given an attributed string
// and bounds that the string will be rendered in, return the regions (rects)
// that ranges of the string occupy when rendered in the given bounds.

// For testing purposes the class interface is defined as a protocol.

@protocol TextRegionMapper<NSObject>
// Create a new mapper. The mapper can be used repeatedly as long as the
// string and its bounds do not change; if they do, another mapper must be
// created.
- (instancetype)initWithAttributedString:(NSAttributedString*)string
                                  bounds:(CGRect)bounds;

// Returns an array of (NSValue-boxed) CGRects which enclose the regions inside
// |bounds| that the characters of |string|'s substring at |range| occupy. If
// the string is spread across several lines, a given range might occupy several
// disjoint rects.
// Note that the rect encloses only the region occupied by the glyphs, not any
// leading or other space padding.
// If |range| is partially or wholly outside of the range of |string|, an
// empty array is returned.
- (NSArray*)rectsForRange:(NSRange)range;

@end

// The actual class implementation works by typesetting the text using CoreText.
@interface CoreTextRegionMapper : NSObject<TextRegionMapper>

- (instancetype)initWithAttributedString:(NSAttributedString*)string
                                  bounds:(CGRect)bounds
    NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_BROWSER_UI_UTIL_TEXT_REGION_MAPPER_H_
