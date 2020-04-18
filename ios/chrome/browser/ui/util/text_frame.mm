// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/util/text_frame.h"

#include "base/logging.h"
#include "base/mac/foundation_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#pragma mark - FramedLine

@implementation FramedLine {
  // Backing object for property of the same name.
  id _line;
}

@synthesize stringRange = _stringRange;
@synthesize origin = _origin;

- (instancetype)initWithLine:(CTLineRef)line
                 stringRange:(NSRange)stringRange
                      origin:(CGPoint)origin {
  if ((self = [super init])) {
    DCHECK(line);
    // CTLines created by ManualTextFramers all have zero for their string range
    // locations, but its length should be equal to |stringRange|.
    NSRange lineRange;
    if (!base::mac::CFRangeToNSRange(CTLineGetStringRange(line), &lineRange)) {
      return nil;
    }
    DCHECK_EQ(lineRange.length, stringRange.length);
    _line = (__bridge id)(line);
    _stringRange = stringRange;
    _origin = origin;
  }
  return self;
}

- (NSString*)description {
  return [NSString stringWithFormat:@"%@ line:%@, stringRange:%@, origin:%@",
                                    [super description], _line,
                                    NSStringFromRange(_stringRange),
                                    NSStringFromCGPoint(_origin)];
}

- (CFIndex)lineOffsetForStringLocation:(NSUInteger)stringLocation {
  if (stringLocation < self.stringRange.location ||
      stringLocation >= self.stringRange.location + self.stringRange.length) {
    return kCFNotFound;
  }
  NSRange lineRange;
  if (!base::mac::CFRangeToNSRange(CTLineGetStringRange(self.line), &lineRange))
    return kCFNotFound;
  return lineRange.location + (stringLocation - self.stringRange.location);
}

#pragma mark Accessors

- (CTLineRef)line {
  return (__bridge CTLineRef)(_line);
}

@end

#pragma mark - CoreTextTextFrame

@interface CoreTextTextFrame () {
  // Backing object for property of the same name.
  NSAttributedString* _string;
  id _frame;
  NSMutableArray* _lines;
}

// The CTFrameRef passed on initializaton.
@property(nonatomic, readonly) CTFrameRef frame;

// Populates |lines| using |frame|.
- (void)createFramedLines;

@end

@implementation CoreTextTextFrame

@synthesize bounds = _bounds;

- (instancetype)initWithString:(NSAttributedString*)string
                        bounds:(CGRect)bounds
                         frame:(CTFrameRef)frame {
  if ((self = [super self])) {
    DCHECK(string.string.length);
    _string = string;
    _bounds = bounds;
    DCHECK(frame);
    _frame = (__bridge id)(frame);
  }
  return self;
}

#pragma mark Accessors

- (NSAttributedString*)string {
  return _string;
}

- (NSRange)framedRange {
  NSRange range;
  CFRange cfRange = CTFrameGetVisibleStringRange(self.frame);
  if (base::mac::CFRangeToNSRange(cfRange, &range))
    return range;
  return NSMakeRange(NSNotFound, 0);
}

- (NSArray*)lines {
  if (!_lines)
    [self createFramedLines];
  return _lines;
}

- (CTFrameRef)frame {
  return (__bridge CTFrameRef)(_frame);
}

#pragma mark Private

- (void)createFramedLines {
  NSArray* lines = base::mac::CFToNSCast(CTFrameGetLines(self.frame));
  CGPoint origins[lines.count];
  CTFrameGetLineOrigins(self.frame, CFRangeMake(0, 0), origins);
  _lines = [[NSMutableArray alloc] initWithCapacity:lines.count];
  for (NSUInteger line_idx = 0; line_idx < lines.count; ++line_idx) {
    CTLineRef line = (__bridge CTLineRef)(lines[line_idx]);
    NSRange stringRange;
    CFRange cfStringRange = CTLineGetStringRange(line);
    if (!base::mac::CFRangeToNSRange(cfStringRange, &stringRange))
      break;
    FramedLine* framedLine =
        [[FramedLine alloc] initWithLine:line
                             stringRange:stringRange
                                  origin:origins[line_idx]];
    [_lines addObject:framedLine];
  }
}

@end
