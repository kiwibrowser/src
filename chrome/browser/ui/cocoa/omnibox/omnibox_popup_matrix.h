// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_OMNIBOX_OMNIBOX_POPUP_MATRIX_H_
#define CHROME_BROWSER_UI_COCOA_OMNIBOX_OMNIBOX_POPUP_MATRIX_H_

#import <Cocoa/Cocoa.h>
#include <stddef.h>

#import "ui/base/cocoa/tracking_area.h"
#include "ui/base/window_open_disposition.h"

class AutocompleteResult;
@class OmniboxPopupCell;
@class OmniboxPopupMatrix;
class OmniboxPopupViewMac;

@interface OmniboxPopupTableController
    : NSObject<NSTableViewDelegate, NSTableViewDataSource> {
 @private
  base::scoped_nsobject<NSArray> array_;
  NSInteger hoveredIndex_;
};

// Setup the information used by the NSTableView data source.
- (instancetype)initWithMatchResults:(const AutocompleteResult&)result
                           tableView:(OmniboxPopupMatrix*)tableView
                           popupView:(const OmniboxPopupViewMac&)popupView
                         answerImage:(NSImage*)answerImage;

// Set the hovered highlight.
- (void)setHighlightedRow:(NSInteger)rowIndex;

// Sets a custom match icon.
- (void)setMatchIcon:(NSImage*)icon forRow:(NSInteger)rowIndex;

// Which row has the hovered highlight.
- (NSInteger)highlightedRow;

@end

@interface OmniboxPopupTableController (TestingAPI)
- (instancetype)initWithArray:(NSArray*)array;
@end

@class OmniboxPopupMatrix;

class OmniboxPopupMatrixObserver {
 public:
  // Called when the selection in the matrix changes.
  virtual void OnMatrixRowSelected(OmniboxPopupMatrix* matrix, size_t row) = 0;

  // Called when the user clicks on a row.
  virtual void OnMatrixRowClicked(OmniboxPopupMatrix* matrix, size_t row) = 0;

  // Called when the user middle clicks on a row.
  virtual void OnMatrixRowMiddleClicked(OmniboxPopupMatrix* matrix,
                                        size_t row) = 0;
};

// Sets up a tracking area to implement hover by highlighting the cell the mouse
// is over.
@interface OmniboxPopupMatrix : NSTableView {
  base::scoped_nsobject<OmniboxPopupTableController> matrixController_;
  OmniboxPopupMatrixObserver* observer_;  // weak
  ui::ScopedCrTrackingArea trackingArea_;
  NSAttributedString* separator_;

  // The width of widest match contents in a set of tail suggestions.
  CGFloat maxMatchContentsWidth_;

  CGFloat answerLineHeight_;

  // Left margin padding for the content (i.e. icon and text) in a cell.
  CGFloat contentLeftPadding_;

  // Max width for the content in the cell.
  CGFloat contentMaxWidth_;

  // true if the OmniboxPopupMatrix should use the dark theme style.
  BOOL hasDarkTheme_;
}

@property(retain, nonatomic) NSAttributedString* separator;
@property(nonatomic) CGFloat maxMatchContentsWidth;
@property(nonatomic) CGFloat answerLineHeight;
@property(nonatomic) CGFloat contentLeftPadding;
@property(nonatomic) CGFloat contentMaxWidth;
@property(readonly, nonatomic) BOOL hasDarkTheme;

// Create a zero-size matrix.
- (instancetype)initWithObserver:(OmniboxPopupMatrixObserver*)observer
                    forDarkTheme:(BOOL)isDarkTheme;

// Sets the observer.
- (void)setObserver:(OmniboxPopupMatrixObserver*)observer;

// Return the currently highlighted row.  Returns -1 if no row is highlighted.
- (NSInteger)highlightedRow;

// Move the selection to |rowIndex|.
- (void)selectRowIndex:(NSInteger)rowIndex;

// Setup the NSTableView data source.
- (void)setController:(OmniboxPopupTableController*)controller;

// Sets a custom match icon.
- (void)setMatchIcon:(NSImage*)icon forRow:(NSInteger)rowIndex;

@end

#endif  // CHROME_BROWSER_UI_COCOA_OMNIBOX_OMNIBOX_POPUP_MATRIX_H_
