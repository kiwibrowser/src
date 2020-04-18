// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_OMNIBOX_OMNIBOX_POPUP_CELL_H_
#define CHROME_BROWSER_UI_COCOA_OMNIBOX_OMNIBOX_POPUP_CELL_H_

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#include "components/omnibox/browser/autocomplete_match.h"

class OmniboxPopupViewMac;

@interface OmniboxPopupCellData : NSObject<NSCopying>

// Left hand side of the separator (e.g. a hyphen).
@property(readonly, retain, nonatomic) NSAttributedString* contents;

// Right hand side of the separator (e.g. a hyphen).
@property(readonly, retain, nonatomic) NSAttributedString* description;

// NOTE: While |prefix_| is used only for tail suggestions, it still needs
// to be a member of the class. This allows the |NSAttributedString| instance
// to stay alive between the call to |drawTitle| and the actual paint event
// which accesses the |NSAttributedString| instance.
@property(readonly, retain, nonatomic) NSAttributedString* prefix;

// Common icon that shows next to most rows in the list.
@property(retain, nonatomic) NSImage* image;

// Uncommon icon that only shows on answer rows (e.g. weather).
@property(readonly, retain, nonatomic) NSImage* answerImage;

@property(readonly, nonatomic) BOOL isContentsRTL;

// Is this suggestion an answer or calculator result.
@property(readonly, nonatomic) bool isAnswer;
@property(readonly, nonatomic) AutocompleteMatch::Type matchType;
@property(readonly, nonatomic) int maxLines;

- (instancetype)initWithMatch:(const AutocompleteMatch&)matchFromModel
                        image:(NSImage*)image
                  answerImage:(NSImage*)answerImage
                 forDarkTheme:(BOOL)isDarkTheme;

// Returns the width of the match contents.
- (CGFloat)getMatchContentsWidth;

@end

// Overrides how cells are displayed.  Uses information from
// OmniboxPopupCellData to draw suggestion results.
@interface OmniboxPopupCell : NSCell

- (void)drawMatchWithFrame:(NSRect)cellFrame inView:(NSView*)controlView;

// Returns the offset of the start of the contents in the input text for the
// given match. It is costly to compute this offset, so it is computed once and
// shared by all OmniboxPopupCell instances through OmniboxPopupViewMac parent.
+ (CGFloat)computeContentsOffset:(const AutocompleteMatch&)match;

+ (NSAttributedString*)createSeparatorStringForDarkTheme:(BOOL)isDarkTheme;

+ (CGFloat)getTextContentAreaWidth:(CGFloat)cellContentMaxWidth;

+ (CGFloat)getContentTextHeight;

@end

#endif  // CHROME_BROWSER_UI_COCOA_OMNIBOX_OMNIBOX_POPUP_CELL_H_
