// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/util/core_text_util.h"

#import <UIKit/UIKit.h>

#include "base/mac/foundation_util.h"
#import "ios/chrome/browser/ui/util/manual_text_framer.h"
#import "ios/chrome/browser/ui/util/text_frame.h"
#import "ios/chrome/browser/ui/util/unicode_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace core_text_util {

namespace {
// Returns the range within |string| for which |text_frame|'s glyph information
// is non-repeating.
CFRange GetValidRangeForTextFrame(CTFrameRef text_frame,
                                  NSAttributedString* string) {
  DCHECK(text_frame);
  DCHECK(string.length);
  CFRange range = CFRangeMake(0, 0);
  bool is_rtl =
      GetEffectiveWritingDirection(string) == NSWritingDirectionRightToLeft;
  for (id line_id in base::mac::CFToNSCast(CTFrameGetLines(text_frame))) {
    CTLineRef line = (__bridge CTLineRef)(line_id);
    NSArray* runs = base::mac::CFToNSCast(CTLineGetGlyphRuns(line));
    NSInteger run_idx = is_rtl ? runs.count - 1 : 0;
    while (run_idx >= 0 && run_idx < static_cast<NSInteger>(runs.count)) {
      CTRunRef run = (__bridge CTRunRef)(runs[run_idx]);
      CFRange run_range = CTRunGetStringRange(run);
      if (run_range.location == range.location + range.length)
        range.length += run_range.length;
      else
        break;
      run_idx += is_rtl ? -1 : 1;
    }
  }
  return range;
}
}  // namespace

base::ScopedCFTypeRef<CTFrameRef> CreateTextFrameForStringInBounds(
    NSAttributedString* string,
    CGRect bounds) {
  base::ScopedCFTypeRef<CTFramesetterRef> frame_setter(
      CTFramesetterCreateWithAttributedString(base::mac::NSToCFCast(string)));
  DCHECK(frame_setter);
  base::ScopedCFTypeRef<CGPathRef> path(CGPathCreateWithRect(bounds, nullptr));
  DCHECK(path);
  base::ScopedCFTypeRef<CTFrameRef> text_frame(
      CTFramesetterCreateFrame(frame_setter, CFRangeMake(0, 0), path, nullptr));
  DCHECK(text_frame);
  return text_frame;
}

bool IsTextFrameValid(CTFrameRef text_frame,
                      ManualTextFramer* manual_framer,
                      NSAttributedString* string) {
  DCHECK(text_frame);
  DCHECK(manual_framer);
  CFRange visible_range = CTFrameGetVisibleStringRange(text_frame);
  CFRange valid_range = GetValidRangeForTextFrame(text_frame, string);
  NSRange unsigned_visible_range;
  // If |visible_range| has invalid values, return early.
  if (!base::mac::CFRangeToNSRange(visible_range, &unsigned_visible_range))
    return false;
  NSRange manual_range = manual_framer.textFrame.framedRange;
  return visible_range.location == valid_range.location &&
         visible_range.length == valid_range.length &&
         manual_range.length == unsigned_visible_range.length;
}

CGFloat GetTrimmedLineWidth(CTLineRef line) {
  DCHECK(line);
  return CTLineGetTypographicBounds(line, nullptr, nullptr, nullptr) -
         CTLineGetTrailingWhitespaceWidth(line);
}

CGFloat GetRunWidthWithRange(CTRunRef run, CFRange range) {
  DCHECK(run);
  CFIndex glyph_count = CTRunGetGlyphCount(run);
  if (range.location < 0 || range.location >= glyph_count ||
      range.location + range.length > glyph_count || !range.length) {
    return 0;
  }
  return CTRunGetTypographicBounds(run, range, nullptr, nullptr, nullptr);
}

CGFloat GetGlyphWidth(CTRunRef run, CFIndex glyph_idx) {
  return GetRunWidthWithRange(run, CFRangeMake(glyph_idx, 1));
}

CFIndex GetGlyphIdxForCharInSet(CTRunRef run,
                                CFRange range,
                                NSAttributedString* string,
                                NSCharacterSet* set) {
  DCHECK(run);
  CFIndex glyph_count = CTRunGetGlyphCount(run);
  DCHECK_LT(range.location, glyph_count);
  DCHECK_LE(range.location + range.length, glyph_count);
  DCHECK(string.length);
  DCHECK(set);
  BOOL isRTL =
      GetEffectiveWritingDirection(string) == NSWritingDirectionRightToLeft;
  CFIndex glyph_idx =
      isRTL ? range.location + range.length - 1 : range.location;
  CFIndex string_idx = GetStringIdxForGlyphIdx(run, glyph_idx);
  while (![set characterIsMember:[string.string characterAtIndex:string_idx]]) {
    glyph_idx += isRTL ? -1 : 1;
    string_idx = GetStringIdxForGlyphIdx(run, glyph_idx);
    if (string_idx == NSNotFound)
      return kCFNotFound;
  }
  return glyph_idx;
}

NSUInteger GetStringIdxForGlyphIdx(CTRunRef run, CFIndex glyph_idx) {
  DCHECK(run);
  if (glyph_idx < 0 || glyph_idx >= CTRunGetGlyphCount(run))
    return NSNotFound;
  CFIndex string_idx;
  CTRunGetStringIndices(run, CFRangeMake(glyph_idx, 1), &string_idx);
  return static_cast<NSUInteger>(string_idx);
}

NSRange GetStringRangeForRun(CTRunRef run) {
  DCHECK(run);
  NSRange stringRange;
  if (base::mac::CFRangeToNSRange(CTRunGetStringRange(run), &stringRange))
    return stringRange;
  return NSMakeRange(NSNotFound, 0);
}

void EnumerateAttributesInString(NSAttributedString* string,
                                 NSRange range,
                                 AttributesBlock block) {
  DCHECK(string.length);
  DCHECK_LT(range.location, string.length);
  DCHECK_LE(range.location + range.length, string.length);
  DCHECK(block);
  NSUInteger char_idx = range.location;
  NSRange effectiveRange = NSMakeRange(0, 0);
  while (char_idx < range.location + range.length) {
    NSDictionary* attributes =
        [string attributesAtIndex:char_idx effectiveRange:&effectiveRange];
    block(attributes);
    char_idx += range.length;
  }
}

CGFloat GetLineHeight(NSAttributedString* string, NSRange range) {
  __block CGFloat line_height = 0;
  AttributesBlock block = ^(NSDictionary* attributes) {
    NSParagraphStyle* style = attributes[NSParagraphStyleAttributeName];
    CGFloat run_line_height = 0;
    UIFont* font = attributes[NSFontAttributeName];
    if (!font)
      font = [UIFont systemFontOfSize:[UIFont systemFontSize]];
    DCHECK(font);
    run_line_height = font.ascender - font.descender;
    if (style.lineHeightMultiple > 0)
      run_line_height *= style.lineHeightMultiple;
    if (style.minimumLineHeight > 0)
      run_line_height = std::max(run_line_height, style.minimumLineHeight);
    if (style.maximumLineHeight > 0)
      run_line_height = std::min(run_line_height, style.maximumLineHeight);
    line_height = std::max(line_height, run_line_height);
  };
  EnumerateAttributesInString(string, range, block);
  return line_height;
}

CGFloat GetLineSpacing(NSAttributedString* string, NSRange range) {
  __block CGFloat line_spacing = 0;
  AttributesBlock block = ^(NSDictionary* attributes) {
    NSParagraphStyle* style = attributes[NSParagraphStyleAttributeName];
    line_spacing = std::max(line_spacing, style.lineSpacing);
  };
  EnumerateAttributesInString(string, range, block);
  return line_spacing;
}

NSTextAlignment GetEffectiveTextAlignment(NSAttributedString* string) {
  DCHECK(string.length);
  NSTextAlignment alignment = NSTextAlignmentLeft;
  NSParagraphStyle* style = [string attribute:NSParagraphStyleAttributeName
                                      atIndex:0
                               effectiveRange:nullptr];
  if (style) {
    alignment = style.alignment;
    if (alignment == NSTextAlignmentNatural ||
        alignment == NSTextAlignmentJustified) {
      NSWritingDirection direction = GetEffectiveWritingDirection(string);
      alignment = direction == NSWritingDirectionRightToLeft
                      ? NSTextAlignmentRight
                      : NSTextAlignmentLeft;
    }
  }
  return alignment;
}

NSWritingDirection GetEffectiveWritingDirection(NSAttributedString* string) {
  DCHECK(string.length);
  // Search for unicode bidirectionality characters within |string|.
  NSWritingDirection direction =
      unicode_util::UnicodeWritingDirectionForString(string.string);
  if (direction == NSWritingDirectionNatural) {
    // If there are no characters with bidirectionality information, default to
    // NSWritingDirectionLeftToRight.
    direction = NSWritingDirectionLeftToRight;
  }
  NSParagraphStyle* style = [string attribute:NSParagraphStyleAttributeName
                                      atIndex:0
                               effectiveRange:nullptr];
  if (style && style.baseWritingDirection != NSWritingDirectionNatural) {
    // Use the NSParagraphStyle's writing direction if specified.
    direction = style.baseWritingDirection;
  }
  return direction;
}

}  // namespace core_text_util
