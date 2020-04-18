// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/base/cocoa/controls/hyperlink_text_view.h"

#include "base/logging.h"
#include "base/mac/scoped_nsobject.h"
#include "ui/base/cocoa/nsview_additions.h"

// The baseline shift for text in the NSTextView.
const float kTextBaselineShift = -1.0;

@interface HyperlinkTextView(Private)
// Initialize the NSTextView properties for this subclass.
- (void)configureTextView;

// Change the current IBeamCursor to an arrowCursor.
- (void)fixupCursor;
@end

@implementation HyperlinkTextView

@synthesize drawsBackgroundUsingSuperview = drawsBackgroundUsingSuperview_;

- (id)initWithCoder:(NSCoder*)decoder {
  if ((self = [super initWithCoder:decoder]))
    [self configureTextView];
  return self;
}

- (id)initWithFrame:(NSRect)frameRect {
  if ((self = [super initWithFrame:frameRect]))
    [self configureTextView];
  return self;
}

- (BOOL)acceptsFirstResponder {
  return !refusesFirstResponder_;
}

- (void)drawViewBackgroundInRect:(NSRect)rect {
  if (drawsBackgroundUsingSuperview_)
    [self cr_drawUsingAncestor:[self superview] inRect:rect];
  else
    [super drawViewBackgroundInRect:rect];
}

// Never draw the insertion point (otherwise, it shows up without any user
// action if full keyboard accessibility is enabled).
- (BOOL)shouldDrawInsertionPoint {
  return NO;
}

- (NSRange)selectionRangeForProposedRange:(NSRange)proposedSelRange
                              granularity:(NSSelectionGranularity)granularity {
  // Return a range of length 0 to prevent text selection. Note that the start
  // of the range (the first argument) is treated as the position of the
  // subsequent click so it must not be 0. If it is, links that begin at a
  // non-zero position in the text will not function correctly when they are
  // clicked in such a way as to look like a possible text selection.
  return NSMakeRange(proposedSelRange.location, 0);
}

// Convince NSTextView to not show an I-Beam cursor when the cursor is over the
// text view but not over actual text.
//
// http://www.mail-archive.com/cocoa-dev@lists.apple.com/msg10791.html
// "NSTextView sets the cursor over itself dynamically, based on considerations
// including the text under the cursor. It does so in -mouseEntered:,
// -mouseMoved:, and -cursorUpdate:, so those would be points to consider
// overriding."
- (void)mouseMoved:(NSEvent*)e {
  [super mouseMoved:e];
  [self fixupCursor];
}

- (void)mouseEntered:(NSEvent*)e {
  [super mouseEntered:e];
  [self fixupCursor];
}

- (void)cursorUpdate:(NSEvent*)e {
  [super cursorUpdate:e];
  [self fixupCursor];
}

- (void)configureTextView {
  [self setEditable:NO];
  [self setDrawsBackground:NO];
  [self setHorizontallyResizable:NO];
  [self setVerticallyResizable:NO];

  // When text is rendered, linkTextAttributes override anything set via
  // addAttributes for text that has NSLinkAttributeName. Set to nil to allow
  // custom attributes to take precedence.
  [self setLinkTextAttributes:nil];
  [self setDisplaysLinkToolTips:NO];

  refusesFirstResponder_ = NO;
  drawsBackgroundUsingSuperview_ = NO;
  isValidLink_ = NO;
}

- (void)fixupCursor {
  if ([[NSCursor currentCursor] isEqual:[NSCursor IBeamCursor]])
    [[NSCursor arrowCursor] set];
}

// Only allow contextual menus (which allow copying of the link URL) if the link
// is a valid one.
- (NSMenu*)menuForEvent:(NSEvent*)e {
  if (isValidLink_)
    return [super menuForEvent:e];

  return nil;
}

// Only allow dragging of valid links.
- (BOOL)dragSelectionWithEvent:(NSEvent*)event
                        offset:(NSSize)mouseOffset
                     slideBack:(BOOL)slideBack {
  if (isValidLink_) {
    return [super dragSelectionWithEvent:event
                                  offset:mouseOffset
                               slideBack:slideBack];
  }

  return NO;
}

- (void)setMessage:(NSString*)message
          withFont:(NSFont*)font
      messageColor:(NSColor*)messageColor {
  // Create an attributes dictionary for the message and link.
  NSDictionary* attributes = @{
    NSForegroundColorAttributeName : messageColor,
    NSCursorAttributeName : [NSCursor arrowCursor],
    NSFontAttributeName : font,
    NSBaselineOffsetAttributeName : @(kTextBaselineShift)
  };

  // Create the attributed string for the message.
  base::scoped_nsobject<NSAttributedString> attributedMessage(
      [[NSMutableAttributedString alloc] initWithString:message
                                             attributes:attributes]);

  // Update the text view with the new text.
  [[self textStorage] setAttributedString:attributedMessage];
}

- (void)addLinkRange:(NSRange)range
             withURL:(NSString*)url
           linkColor:(NSColor*)linkColor {
  // If a URL is provided, make sure it is a valid one.
  if (url) {
    DCHECK_GT([url length], 0u);
    DCHECK([NSURL URLWithString:url]);
    isValidLink_ = YES;
  } else {
    url = @"";
    isValidLink_ = NO;
  }
  NSDictionary* attributes = @{
    NSForegroundColorAttributeName : linkColor,
    NSUnderlineStyleAttributeName : @(YES),
    NSCursorAttributeName : [NSCursor pointingHandCursor],
    NSLinkAttributeName : url,
    NSUnderlineStyleAttributeName : @(NSUnderlineStyleSingle)
  };

  [[self textStorage] addAttributes:attributes range:range];
}

- (void)setRefusesFirstResponder:(BOOL)refusesFirstResponder {
  refusesFirstResponder_ = refusesFirstResponder;
}

@end
