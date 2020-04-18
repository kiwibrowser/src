// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "ui/base/ui_base_export.h"

@class NSColor;

// HyperlinkTextView is an NSTextView subclass for unselectable, linkable text.
// This subclass doesn't show the text caret or IBeamCursor, whereas the base
// class NSTextView displays both with full keyboard accessibility enabled.
UI_BASE_EXPORT
@interface HyperlinkTextView : NSTextView {
 @private
  BOOL refusesFirstResponder_;
  BOOL drawsBackgroundUsingSuperview_;
  BOOL isValidLink_;
}

@property(nonatomic, assign) BOOL drawsBackgroundUsingSuperview;

// Set the |message| displayed by the HyperlinkTextView, using |font| and
// |messageColor|.
- (void)setMessage:(NSString*)message
          withFont:(NSFont*)font
      messageColor:(NSColor*)messageColor;

// Marks a |range| within the given message as a link. Pass nil as the url to
// create a link that can neither be copied nor dragged.
- (void)addLinkRange:(NSRange)range
             withURL:(NSString*)url
           linkColor:(NSColor*)linkColor;

// This is NO (by default) if the view rejects first responder status.
- (void)setRefusesFirstResponder:(BOOL)refusesFirstResponder;

@end
