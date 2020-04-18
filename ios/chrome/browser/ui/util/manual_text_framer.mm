// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/util/manual_text_framer.h"

#import <UIKit/UIKit.h>

#include "base/logging.h"
#include "base/mac/foundation_util.h"
#import "ios/chrome/browser/ui/util/core_text_util.h"
#import "ios/chrome/browser/ui/util/text_frame.h"
#import "ios/chrome/browser/ui/util/unicode_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// NOTE: When RTL text is laid out into glyph runs, the glyphs appear in the
// visual order in which they appear on screen.  In other words, the glyphs are
// arranged in the reverse order of their corresponding characters in the
// original string.

namespace {
// Aligns |value| to the nearest pixel value, rounding by the function indicated
// by |function|.  AlignmentFunction::CEIL should be used to align size values,
// while AlignmentFunction::FLOOR should be used to align location values.
enum class AlignmentFunction : short { CEIL = 0, FLOOR };
CGFloat AlignValueToPixel(CGFloat value, AlignmentFunction function) {
  static CGFloat scale = [[UIScreen mainScreen] scale];
  return function == AlignmentFunction::CEIL ? ceil(value * scale) / scale
                                             : floor(value * scale) / scale;
}

// Returns an NSArray of NSAttributedStrings corresponding to newline-separated
// paragraphs within |string|.
NSArray* GetParagraphStringsForString(NSAttributedString* string) {
  NSMutableArray* paragraph_strings = [NSMutableArray array];
  NSCharacterSet* newline_char_set = [NSCharacterSet newlineCharacterSet];
  NSUInteger string_length = string.string.length;
  NSRange remaining_range = NSMakeRange(0, string_length);
  while (remaining_range.location < string_length) {
    NSRange newline_range =
        [string.string rangeOfCharacterFromSet:newline_char_set
                                       options:0
                                         range:remaining_range];
    NSRange paragraph_range = NSMakeRange(0, 0);
    if (newline_range.location == NSNotFound) {
      // There's no newline in the remaining portion of the string.
      paragraph_range = remaining_range;
      remaining_range = NSMakeRange(string_length, 0);
    } else {
      // A newline character was encountered.  Compute approximate text lines
      // for the substring within |remaining_range| up to the newline.
      NSUInteger newline_end = newline_range.location + newline_range.length;
      paragraph_range = NSMakeRange(remaining_range.location,
                                    newline_end - remaining_range.location);
      remaining_range.location = newline_end;
      remaining_range.length = string_length - remaining_range.location;
    }
    // Create an attributed substring for the current paragraph and add it to
    // |paragraphs|.
    [paragraph_strings
        addObject:[string attributedSubstringFromRange:paragraph_range]];
  }
  return paragraph_strings;
}
}  // namespace

#pragma mark - ManualTextFrame

// A TextFrame implementation that is manually created by ManualTextFramer.
@interface ManualTextFrame : NSObject<TextFrame> {
  // Backing objects for properties of the same name.
  NSAttributedString* _string;
  NSMutableArray* _lines;
}

// Designated initializer.
- (instancetype)initWithString:(NSAttributedString*)string
                      inBounds:(CGRect)bounds NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;

// Creates a FramedLine out of |line|, |stringRange|, and |origin|, then adds it
// to |lines|.
- (void)addFramedLineWithLine:(CTLineRef)line
                  stringRange:(NSRange)stringRange
                       origin:(CGPoint)origin;

// Redefine property as readwrite.
@property(nonatomic, readwrite) NSRange framedRange;

@end

@implementation ManualTextFrame

@synthesize framedRange = _framedRange;
@synthesize bounds = _bounds;

- (instancetype)initWithString:(NSAttributedString*)string
                      inBounds:(CGRect)bounds {
  if ((self = [super init])) {
    DCHECK(string.string.length);
    _string = string;
    _bounds = bounds;
    _lines = [[NSMutableArray alloc] init];
  }
  return self;
}

#pragma mark Accessors

- (NSAttributedString*)string {
  return _string;
}

- (NSArray*)lines {
  return _lines;
}

#pragma mark Private

- (void)addFramedLineWithLine:(CTLineRef)line
                  stringRange:(NSRange)stringRange
                       origin:(CGPoint)origin {
  FramedLine* framedLine = [[FramedLine alloc] initWithLine:line
                                                stringRange:stringRange
                                                     origin:origin];
  [_lines addObject:framedLine];
}

@end

#pragma mark - ManualTextFramer Private Interface

@interface ManualTextFramer ()

// The string passed upon initialization.
@property(strong, nonatomic, readonly) NSAttributedString* string;

// The bounds passed upon initialization.
@property(nonatomic, readonly) CGRect bounds;

// The width of the bounds passed upon initialization.
@property(nonatomic, readonly) CGFloat boundingWidth;

// The remaining height into which text can be framed.
@property(nonatomic, assign) CGFloat remainingHeight;

// The text frame constructed by |-frameText|.
@property(strong, nonatomic, readonly) ManualTextFrame* manualTextFrame;

// Creates a ManualTextFrame and assigns it to |_manualTextFrame|.  Returns YES
// if a new text frame was successfully created.
- (BOOL)setupManualTextFrame;

@end

#pragma mark - ParagraphFramer

// ManualTextFramer subclass that frames a single paragraph.  A paragraph is
// defined as an NSAttributedString which contains either zero newlines or one
// newline as its last character.
@interface ParagraphFramer : ManualTextFramer {
  // Backing objects for properties of the same name.
  base::ScopedCFTypeRef<CTLineRef> _line;
}

// The CTLine created from |string|.
@property(nonatomic, readonly) CTLineRef line;

// The effective text alignment for |line|.
@property(nonatomic, readonly) NSTextAlignment effectiveAlignment;

// Character set containing characters that are appropriate for line endings.
// These characters include whitespaces and newlines (denoting a word boundary),
// in addition to line-ending characters like hyphens, em dashes, and en dashes.
@property(strong, nonatomic, readonly) NSCharacterSet* lineEndSet;

// The index of the current run that is being framed.  Setting |runIdx| also
// updates |currentRun| and |currentGlyphCount|.
@property(nonatomic, assign) CFIndex runIdx;

// The CTRun corresponding with |runIdx| in |line|.
@property(nonatomic, readonly) CTRunRef currentRun;

// The glyph count in |currentRun|.
@property(nonatomic, readonly) CFIndex currentGlyphCount;

// The number of glyphs in |currentRun| that have been successfully framed.
@property(nonatomic, assign) CFIndex framedGlyphCount;

// The range in |string| that has successfully been framed for the current line.
@property(nonatomic, assign) NSRange currentLineRange;

// The width of the typographic bounds for the glyphs framed on the current
// line.  This is the width of the substring of |string| corresponding to
// |currentLineRange|.
@property(nonatomic, assign) CGFloat currentLineWidth;

// The width of the trailing whitespace for the current line.  This whitespace
// is not counted against the line width if it's the end of the line, but needs
// to be added in if non-whitespace characters from subsequent runs fit on the
// same line.
@property(nonatomic, assign) CGFloat currentWhitespaceWidth;

// Whether the paragraph's writing direction is in RTL.
@property(nonatomic, readonly) BOOL isRTL;

// Either 1 or -1 depending on |isRTL|.
@property(nonatomic, readonly) CFIndex incrementAmount;

// Glyphs are laid out differently for RTL and LTR languages (see note at top of
// file).  These functions return a range with |range|'s length incremented or
// decremented and an updated location that would include the next glyph in the
// trailing direction.
- (CFRange)incrementRange:(CFRange)range byAmount:(CFIndex)amount;
- (CFRange)incrementRange:(CFRange)range;
- (CFRange)decrementRange:(CFRange)range;

// Updates |range| such that its trailing glyph index is |trailingGlyphIdx|.
- (CFRange)updateRange:(CFRange)range
    forTrailingGlyphIdx:(CFIndex)trailingGlyphIdx;

// Returns the index of the trailing glyph in |range| for |currentRun|.
- (CFIndex)trailingGlyphIdxForRange:(CFRange)range;

// Returns the index of the leading or trailing glyph in |currentRun|.
- (CFIndex)trailingGlyphIdxForCurrentRun;

// Manually frames the glyphs in |currentRun| following |framedGlyphCount|.
// This function updates |framedGlyphCount|, |currentLineWidth|, and
// |currentLineRange|.
- (void)frameCurrentRun;

// Returns the character associated with the glyph at |glyphIdx| in
// |currentRun|.
- (unichar)charForGlyphAtIdx:(CFIndex)glyphIdx;

// Returns the index within the original string corresponding to the glyph at
// |glyphIdx| in |currentRun|.
- (CFIndex)stringIdxForGlyphAtIdx:(CFIndex)glyphIdx;

// Returns YES if |runIdx| is within the range of |line|'s glyph runs array.
- (BOOL)runIdxIsValid:(CFIndex)runIdx;

// Returns YES if |glyphIdx| is within [0, |currentGlyphCount|).
- (BOOL)glyphIdxIsValid:(CFIndex)glyphIdx;

// Creates a line from |currentLineRange| and adds it to |lines|.
- (void)addCurrentLine;

// Returns the baselines origin for the current line.  This function depends on
// |currentLineRange|, |currentLineWidth|, and |remainingHeight|, and must be
// called before updating those bookkeeping variables when adding the line.
- (CGPoint)originForCurrentLine;
@end

@implementation ParagraphFramer

@synthesize effectiveAlignment = _effectiveTextAlignment;
@synthesize runIdx = _runIdx;
@synthesize currentRun = _currentRun;
@synthesize currentGlyphCount = _currentGlyphCount;
@synthesize framedGlyphCount = _framedGlyphCount;
@synthesize currentLineRange = _currentLineRange;
@synthesize currentLineWidth = _currentLineWidth;
@synthesize currentWhitespaceWidth = _currentWhitespaceWidth;
@synthesize isRTL = _isRTL;
@synthesize lineEndSet = _lineEndSet;

- (instancetype)initWithString:(NSAttributedString*)string
                      inBounds:(CGRect)bounds {
  if ((self = [super initWithString:string inBounds:bounds])) {
    NSRange newlineRange = [string.string
        rangeOfCharacterFromSet:[NSCharacterSet newlineCharacterSet]];
    DCHECK(newlineRange.location == NSNotFound ||
           newlineRange.location == string.string.length - 1);
    CTLineRef line =
        CTLineCreateWithAttributedString(base::mac::NSToCFCast(string));
    _line.reset(line);
    _effectiveTextAlignment = core_text_util::GetEffectiveTextAlignment(string);
    NSWritingDirection direction =
        core_text_util::GetEffectiveWritingDirection(string);
    _isRTL = direction == NSWritingDirectionRightToLeft;
  }
  return self;
}

- (void)frameText {
  if (![self setupManualTextFrame])
    return;
  self.runIdx =
      self.isRTL ? CFArrayGetCount(CTLineGetGlyphRuns(self.line)) - 1 : 0;
  while (self.currentRun) {
    NSRange runStringRange =
        core_text_util::GetStringRangeForRun(self.currentRun);
    DCHECK_NE(runStringRange.location, static_cast<NSUInteger>(NSNotFound));
    DCHECK_NE(runStringRange.length, 0U);
    CGFloat runLineHeight =
        core_text_util::GetLineHeight(self.string, runStringRange);
    // Count of the number of times the framing process (-frameCurrentRun) has
    // "stalled" -- run without changing the total number of glyphs framed. In
    // some cases the process may stall once at the end of a run, but if it
    // stalls twice, it won't make any further progress and should halt.
    NSUInteger stallCount = 0;
    // Loop as long as framed glyph count is less that the total glyph count,
    // and the framer is making progress.
    while (self.framedGlyphCount < self.currentGlyphCount && stallCount < 2U) {
      // Stop framing glyphs if there is not enough vertical space for the run.
      if (self.remainingHeight < runLineHeight)
        break;
      CFIndex initialFramedGlyphCount = self.framedGlyphCount;
      [self frameCurrentRun];
      if (self.framedGlyphCount == initialFramedGlyphCount)
        stallCount++;
      if (self.framedGlyphCount < self.currentGlyphCount) {
        // The entire run didn't fit onto the current line, so create a CTLine
        // from |currentLineRange| and add it to |lines|.
        [self addCurrentLine];
      }
    }
    self.runIdx += self.incrementAmount;
  }
  // Add the final line.
  [self addCurrentLine];
  // Update |manualTextFrame|'s |framedRange|.
  self.manualTextFrame.framedRange =
      NSMakeRange(0, self.currentLineRange.location);
}

#pragma mark Accessors

- (CTLineRef)line {
  return _line.get();
}

- (NSCharacterSet*)lineEndSet {
  if (!_lineEndSet) {
    NSMutableCharacterSet* lineEndSet =
        [NSMutableCharacterSet whitespaceAndNewlineCharacterSet];
    [lineEndSet addCharactersInString:@"-\u2013\u2014"];
    _lineEndSet = lineEndSet;
  }
  return _lineEndSet;
}

- (void)setRunIdx:(CFIndex)runIdx {
  _runIdx = runIdx;
  self.framedGlyphCount = 0;
  if ([self runIdxIsValid:runIdx]) {
    NSArray* runs = base::mac::CFToNSCast(CTLineGetGlyphRuns(self.line));
    _currentRun = (__bridge CTRunRef)(runs[_runIdx]);
    _currentGlyphCount = CTRunGetGlyphCount(self.currentRun);
  } else {
    _currentRun = nullptr;
    _currentGlyphCount = 0;
  }
}

- (CFIndex)incrementAmount {
  return self.isRTL ? -1 : 1;
}

#pragma mark Private

- (CFRange)incrementRange:(CFRange)range byAmount:(CFIndex)amount {
  CFRange incrementedRange = range;
  incrementedRange.length += amount;
  if (self.isRTL)
    incrementedRange.location += self.incrementAmount * amount;
  return incrementedRange;
}

- (CFRange)incrementRange:(CFRange)range {
  return [self incrementRange:range byAmount:1];
}

- (CFRange)decrementRange:(CFRange)range {
  return [self incrementRange:range byAmount:-1];
}

- (CFRange)updateRange:(CFRange)range
    forTrailingGlyphIdx:(CFIndex)trailingGlyphIdx {
  DCHECK(self.isRTL ? trailingGlyphIdx <= range.location + range.length
                    : trailingGlyphIdx >= range.location);
  DCHECK([self glyphIdxIsValid:trailingGlyphIdx]);
  CFIndex currentTrailingGlyphIdx = [self trailingGlyphIdxForRange:range];
  CFIndex updateAmount = self.isRTL
                             ? currentTrailingGlyphIdx - trailingGlyphIdx
                             : trailingGlyphIdx - currentTrailingGlyphIdx;
  return [self incrementRange:range byAmount:updateAmount];
}

- (CFIndex)trailingGlyphIdxForRange:(CFRange)range {
  if (self.isRTL)
    return range.location;
  return range.location + range.length - 1;
}

- (CFIndex)trailingGlyphIdxForCurrentRun {
  return self.isRTL ? 0 : self.currentGlyphCount - 1;
}

- (void)frameCurrentRun {
  DCHECK(self.currentRun);
  DCHECK_LT(self.framedGlyphCount, self.currentGlyphCount);
  DCHECK_LT(self.currentLineWidth, self.boundingWidth);

  // Calculate the range that will fit in the remaining portion of the line.
  NSCharacterSet* whitespaceSet =
      [NSCharacterSet whitespaceAndNewlineCharacterSet];
  CFIndex startGlyphIdx = self.isRTL
                              ? self.currentGlyphCount - self.framedGlyphCount
                              : self.framedGlyphCount;
  CFRange remainingRunRange =
      CFRangeMake(self.isRTL ? 0 : startGlyphIdx,
                  self.currentGlyphCount - self.framedGlyphCount);
  CFRange range = CFRangeMake(startGlyphIdx, 0);
  while (remainingRunRange.length > 0) {
    // Find the range for the next word that can be added to the line.  If no
    // delimiters were found, frame the rest of the run.
    CFIndex delimIdx = core_text_util::GetGlyphIdxForCharInSet(
        self.currentRun, remainingRunRange, self.string, self.lineEndSet);
    if (delimIdx == kCFNotFound)
      delimIdx = [self trailingGlyphIdxForCurrentRun];
    CFRange wordGlyphRange =
        [self updateRange:remainingRunRange forTrailingGlyphIdx:delimIdx];
    CFIndex wordFramedGlyphCount = wordGlyphRange.length;
    // Trim any whitespace and record its width.
    CGFloat wordTrailingWhitespaceWidth = 0.0;
    if ([whitespaceSet characterIsMember:[self charForGlyphAtIdx:delimIdx]]) {
      wordTrailingWhitespaceWidth =
          core_text_util::GetGlyphWidth(self.currentRun, delimIdx);
      wordGlyphRange = [self decrementRange:wordGlyphRange];
    }
    // Check if the word will fit on the line.
    CGFloat wordWidth =
        core_text_util::GetRunWidthWithRange(self.currentRun, wordGlyphRange);
    CGFloat cumulativeLineWidth =
        self.currentLineWidth + self.currentWhitespaceWidth + wordWidth;
    if (cumulativeLineWidth <= self.boundingWidth) {
      // The word at |wordGlyphRange| fits on the line.
      self.currentLineWidth = cumulativeLineWidth;
      self.framedGlyphCount += wordFramedGlyphCount;
      self.currentWhitespaceWidth = wordTrailingWhitespaceWidth;
      remainingRunRange.length -= wordFramedGlyphCount;
      if (!self.isRTL)
        remainingRunRange.location += wordFramedGlyphCount;
      range = [self incrementRange:range byAmount:wordFramedGlyphCount];
    } else {
      break;
    }
  }
  // Early return if no glyphs were framed.
  if (!range.length)
    return;
  // Use the string index of the next glyph to determine the string range for
  // the current line, since a glyph may correspond with multiple characters
  // when ligatures are used.
  CFIndex nextGlyphIdx =
      [self trailingGlyphIdxForRange:range] + self.incrementAmount;
  CFIndex nextGlyphStringIdx;
  if ([self glyphIdxIsValid:nextGlyphIdx]) {
    nextGlyphStringIdx = [self stringIdxForGlyphAtIdx:nextGlyphIdx];
  } else {
    CFRange runStringRange = CTRunGetStringRange(self.currentRun);
    nextGlyphStringIdx = runStringRange.location + runStringRange.length;
  }
  self.currentLineRange =
      NSMakeRange(self.currentLineRange.location,
                  nextGlyphStringIdx - self.currentLineRange.location);
}

- (unichar)charForGlyphAtIdx:(CFIndex)glyphIdx {
  DCHECK([self glyphIdxIsValid:glyphIdx]);
  return [self.string.string
      characterAtIndex:[self stringIdxForGlyphAtIdx:glyphIdx]];
}

- (CFIndex)stringIdxForGlyphAtIdx:(CFIndex)glyphIdx {
  DCHECK([self glyphIdxIsValid:glyphIdx]);
  CFIndex stringIdx = 0;
  CTRunGetStringIndices(self.currentRun, CFRangeMake(glyphIdx, 1), &stringIdx);
  return stringIdx;
}

- (BOOL)runIdxIsValid:(CFIndex)runIdx {
  NSArray* runs = base::mac::CFToNSCast(CTLineGetGlyphRuns(self.line));
  return runIdx >= 0 && runIdx < static_cast<CFIndex>(runs.count);
}

- (BOOL)glyphIdxIsValid:(CFIndex)glyphIdx {
  return glyphIdx >= 0 && glyphIdx < self.currentGlyphCount;
}

- (void)addCurrentLine {
  // Don't attempt to add a line if |currentLineRange| is empty.
  if (!self.currentLineRange.length)
    return;
  // Add the new line and its corresponding string range and baseline origin.
  NSAttributedString* currentLineString =
      [self.string attributedSubstringFromRange:self.currentLineRange];
  CTLineRef currentLine = CTLineCreateWithAttributedString(
      base::mac::NSToCFCast(currentLineString));
  [self.manualTextFrame addFramedLineWithLine:currentLine
                                  stringRange:self.currentLineRange
                                       origin:[self originForCurrentLine]];
  CFRelease(currentLine);
  // Update bookkeeping variables for next line.
  CGFloat usedHeight =
      core_text_util::GetLineHeight(self.string, self.currentLineRange) +
      core_text_util::GetLineSpacing(self.string, self.currentLineRange);
  self.currentLineRange = NSMakeRange(
      self.currentLineRange.location + self.currentLineRange.length, 0);
  self.currentLineWidth = 0;
  self.currentWhitespaceWidth = 0;
  self.remainingHeight -= usedHeight;
}

- (CGPoint)originForCurrentLine {
  CGPoint origin = CGPointZero;
  CGFloat alignedWidth =
      AlignValueToPixel(self.currentLineWidth, AlignmentFunction::CEIL);
  switch (self.effectiveAlignment) {
    case NSTextAlignmentLeft:
      // Left-aligned lines begin at 0.0.
      break;
    case NSTextAlignmentRight:
      origin.x = AlignValueToPixel(self.boundingWidth - alignedWidth,
                                   AlignmentFunction::FLOOR);
      break;
    case NSTextAlignmentCenter:
      origin.x = AlignValueToPixel((self.boundingWidth - alignedWidth) / 2.0,
                                   AlignmentFunction::FLOOR);
      break;
    default:
      // Only left, right, and center effective alignment is supported.
      NOTREACHED();
      break;
  }
  UIFont* font = [self.string attribute:NSFontAttributeName
                                atIndex:self.currentLineRange.location
                         effectiveRange:nullptr];
  CGFloat lineHeight =
      core_text_util::GetLineHeight(self.string, self.currentLineRange);
  origin.y =
      AlignValueToPixel(self.remainingHeight - lineHeight - font.descender,
                        AlignmentFunction::FLOOR);
  return origin;
}

@end

#pragma mark - ManualTextFramer

@implementation ManualTextFramer

@synthesize bounds = _bounds;
@synthesize boundingWidth = _boundingWidth;
@synthesize remainingHeight = _remainingHeight;
@synthesize string = _string;
@synthesize manualTextFrame = _manualTextFrame;

- (instancetype)initWithString:(NSAttributedString*)string
                      inBounds:(CGRect)bounds {
  if ((self = [super init])) {
    DCHECK(string.string.length);
    _string = string;
    _bounds = bounds;
    _boundingWidth = CGRectGetWidth(bounds);
    _remainingHeight = CGRectGetHeight(bounds);
  }
  return self;
}

- (void)frameText {
  if (![self setupManualTextFrame])
    return;
  NSRange framedRange = NSMakeRange(0, 0);
  NSArray* paragraphs = GetParagraphStringsForString(self.string);
  NSUInteger stringRangeOffset = 0;
  for (NSAttributedString* paragraph in paragraphs) {
    // Frame each paragraph using a ParagraphFramer, then update bookkeeping
    // variables for the top-level ManualTextFramer.
    CGRect remainingBounds =
        CGRectMake(0, 0, self.boundingWidth, self.remainingHeight);
    ParagraphFramer* framer =
        [[ParagraphFramer alloc] initWithString:paragraph
                                       inBounds:remainingBounds];
    [framer frameText];
    id<TextFrame> frame = [framer textFrame];
    DCHECK(frame);
    framedRange.length += frame.framedRange.length;
    CGFloat paragraphHeight = 0.0;
    for (FramedLine* line in frame.lines) {
      NSRange lineRange = line.stringRange;
      lineRange.location += stringRangeOffset;
      [self.manualTextFrame addFramedLineWithLine:line.line
                                      stringRange:lineRange
                                           origin:line.origin];
      paragraphHeight += core_text_util::GetLineHeight(self.string, lineRange) +
                         core_text_util::GetLineSpacing(self.string, lineRange);
    }
    self.remainingHeight -= paragraphHeight;
    stringRangeOffset += paragraph.string.length;
  }
  self.manualTextFrame.framedRange = framedRange;
}

#pragma mark Accessors

- (id<TextFrame>)textFrame {
  return _manualTextFrame;
}

#pragma mark Private

- (BOOL)setupManualTextFrame {
  if (_manualTextFrame)
    return NO;
  _manualTextFrame =
      [[ManualTextFrame alloc] initWithString:self.string inBounds:self.bounds];
  return YES;
}

@end
