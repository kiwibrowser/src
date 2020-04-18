// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_UTIL_MANUAL_TEXT_FRAMER_H_
#define IOS_CHROME_BROWSER_UI_UTIL_MANUAL_TEXT_FRAMER_H_

#import <CoreText/CoreText.h>
#import <Foundation/Foundation.h>

@protocol TextFrame;

// Object encapsulating the manual framing of an attributed string within a
// bounding rect.
// Known limitations:
// - Justified-aligned text is not supported.
// - Hyphenation is never attempted, regardless of NSParagraphStyle's
//   |hyphenationFactor|.
@interface ManualTextFramer : NSObject

// Creates an instance that frames |string| within |bounds|.
- (instancetype)initWithString:(NSAttributedString*)string
                      inBounds:(CGRect)bounds NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;

// Manually Frames the paragraph within the bounds provided upon initialization.
// Calling this method populates |textFrame|.
- (void)frameText;

// The TextFrame created by |-frameText|.  Will be nil before |-frameText| is
// called.
@property(strong, nonatomic, readonly) id<TextFrame> textFrame;

@end

#endif  // IOS_CHROME_BROWSER_UI_UTIL_MANUAL_TEXT_FRAMER_H_
