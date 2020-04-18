// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/find_bar/find_bar_text_field_cell.h"

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

// How far to offset the keyword token into the field.
const NSInteger kResultsXOffset = 3;

// How much width (beyond text) to add to the keyword token on each
// side.
const NSInteger kResultsTokenInset = 3;

// How far to shift bounding box of hint down from top of field.
// Assumes -setFlipped:YES.
const NSInteger kResultsYOffset = 4;

// Conveniences to centralize width+offset calculations.
CGFloat WidthForResults(NSAttributedString* resultsString) {
  return kResultsXOffset + ceil([resultsString size].width) +
      2 * kResultsTokenInset;
}

}  // namespace

@implementation FindBarTextFieldCell

- (CGFloat)topTextFrameOffset {
  return 1.0;
}

- (CGFloat)bottomTextFrameOffset {
  return 1.0;
}

- (CGFloat)cornerRadius {
  return 4.0;
}

- (rect_path_utils::RoundedCornerFlags)roundedCornerFlags {
  return rect_path_utils::RoundedCornerLeft;
}

// Convenience for the attributes used in the right-justified info
// cells.  Sets the background color to red if |foundMatches| is YES.
- (NSDictionary*)resultsAttributes:(BOOL)foundMatches {
  base::scoped_nsobject<NSMutableParagraphStyle> style(
      [[NSMutableParagraphStyle alloc] init]);
  [style setAlignment:NSRightTextAlignment];

  return [NSDictionary dictionaryWithObjectsAndKeys:
              [self font], NSFontAttributeName,
              [NSColor lightGrayColor], NSForegroundColorAttributeName,
              [NSColor whiteColor], NSBackgroundColorAttributeName,
              style.get(), NSParagraphStyleAttributeName,
              nil];
}

- (void)setActiveMatch:(NSInteger)current of:(NSInteger)total {
  NSString* results =
      base::SysUTF16ToNSString(l10n_util::GetStringFUTF16(
          IDS_FIND_IN_PAGE_COUNT,
          base::IntToString16(current),
          base::IntToString16(total)));
  resultsString_.reset([[NSAttributedString alloc]
                         initWithString:results
                         attributes:[self resultsAttributes:(total > 0)]]);
}

- (void)clearResults {
  resultsString_.reset(nil);
}

- (NSString*)resultsString {
  return [resultsString_ string];
}

- (NSRect)textFrameForFrame:(NSRect)cellFrame {
  NSRect textFrame([super textFrameForFrame:cellFrame]);
  if (resultsString_)
    textFrame.size.width -= WidthForResults(resultsString_);
  return textFrame;
}

// Do not show the I-beam cursor over the results label.
- (NSRect)textCursorFrameForFrame:(NSRect)cellFrame {
  return [self textFrameForFrame:cellFrame];
}

- (void)drawResultsWithFrame:(NSRect)cellFrame inView:(NSView*)controlView {
  DCHECK(resultsString_);

  NSRect textFrame = [self textFrameForFrame:cellFrame];
  NSRect infoFrame(NSMakeRect(NSMaxX(textFrame),
                              cellFrame.origin.y + kResultsYOffset,
                              ceil([resultsString_ size].width),
                              cellFrame.size.height - kResultsYOffset));
  [resultsString_.get() drawInRect:infoFrame];
}

- (void)drawInteriorWithFrame:(NSRect)cellFrame inView:(NSView*)controlView {
  if (resultsString_)
    [self drawResultsWithFrame:cellFrame inView:controlView];
  [super drawInteriorWithFrame:cellFrame inView:controlView];
}

@end
