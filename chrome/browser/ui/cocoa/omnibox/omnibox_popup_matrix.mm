// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/omnibox/omnibox_popup_matrix.h"

#include "base/logging.h"
#include "base/mac/foundation_util.h"
#import "chrome/browser/ui/cocoa/omnibox/omnibox_popup_cell.h"
#include "chrome/browser/ui/cocoa/omnibox/omnibox_popup_view_mac.h"
#include "chrome/browser/ui/cocoa/omnibox/omnibox_view_mac.h"
#include "components/omnibox/browser/autocomplete_result.h"

namespace {

// NSEvent -buttonNumber for middle mouse button.
const NSInteger kMiddleButtonNumber = 2;

}  // namespace

@interface OmniboxPopupMatrix ()
- (OmniboxPopupTableController*)controller;
- (void)resetTrackingArea;
- (void)highlightRowUnder:(NSEvent*)theEvent;
- (BOOL)selectCellForEvent:(NSEvent*)theEvent;
@end

@implementation OmniboxPopupTableController

- (instancetype)initWithMatchResults:(const AutocompleteResult&)result
                           tableView:(OmniboxPopupMatrix*)tableView
                           popupView:(const OmniboxPopupViewMac&)popupView
                         answerImage:(NSImage*)answerImage {
  base::scoped_nsobject<NSMutableArray> array([[NSMutableArray alloc] init]);
  BOOL isDarkTheme = [tableView hasDarkTheme];
  for (const AutocompleteMatch& match : result) {
    base::scoped_nsobject<OmniboxPopupCellData> cellData(
        [[OmniboxPopupCellData alloc]
            initWithMatch:match
                    image:popupView.ImageForMatch(match)
              answerImage:(match.answer ? answerImage : nil)
             forDarkTheme:isDarkTheme]);
    [array addObject:cellData];
  }

  return [self initWithArray:array];
}

- (instancetype)initWithArray:(NSArray*)array {
  if ((self = [super init])) {
    hoveredIndex_ = -1;
    array_.reset([array copy]);
  }
  return self;
}

- (NSInteger)numberOfRowsInTableView:(NSTableView*)tableView {
  return [array_ count];
}

- (id)tableView:(NSTableView*)tableView
    objectValueForTableColumn:(NSTableColumn*)tableColumn
                          row:(NSInteger)rowIndex {
  return [array_ objectAtIndex:rowIndex];
}

- (void)tableView:(NSTableView*)tableView
    setObjectValue:(id)object
    forTableColumn:(NSTableColumn*)tableColumn
               row:(NSInteger)rowIndex {
  NOTREACHED();
}

- (void)tableView:(NSTableView*)tableView
    willDisplayCell:(id)cell
     forTableColumn:(NSTableColumn*)tableColumn
                row:(NSInteger)rowIndex {
  OmniboxPopupCell* popupCell =
      base::mac::ObjCCastStrict<OmniboxPopupCell>(cell);
  [popupCell
      setState:([tableView selectedRow] == rowIndex) ? NSOnState : NSOffState];
  [popupCell highlight:(hoveredIndex_ == rowIndex)
             withFrame:[tableView bounds]
                inView:tableView];
}

- (NSInteger)highlightedRow {
  return hoveredIndex_;
}

- (void)setHighlightedRow:(NSInteger)rowIndex {
  hoveredIndex_ = rowIndex;
}

- (void)setMatchIcon:(NSImage*)icon forRow:(NSInteger)rowIndex {
  OmniboxPopupCellData* cellData =
      base::mac::ObjCCastStrict<OmniboxPopupCellData>(
          [array_ objectAtIndex:rowIndex]);
  [cellData setImage:icon];
}

- (CGFloat)tableView:(NSTableView*)tableView heightOfRow:(NSInteger)row {
  BOOL isAnswer = [[array_ objectAtIndex:row] isAnswer];
  CGFloat height = [OmniboxPopupCell getContentTextHeight];

  if (isAnswer) {
    OmniboxPopupMatrix* matrix =
        base::mac::ObjCCastStrict<OmniboxPopupMatrix>(tableView);
    NSRect rowRect = [tableView rectOfColumn:0];
    OmniboxPopupCellData* cellData =
        base::mac::ObjCCastStrict<OmniboxPopupCellData>(
            [array_ objectAtIndex:row]);
    // Subtract any Material Design padding and/or icon.
    rowRect.size.width =
        [OmniboxPopupCell getTextContentAreaWidth:[matrix contentMaxWidth]];
    NSAttributedString* text = [cellData description];
    // Provide no more than 3 lines of space.
    rowRect.size.height =
        std::min(3, [cellData maxLines]) * [text size].height;
    NSRect textRect =
        [text boundingRectWithSize:rowRect.size
                           options:NSStringDrawingUsesLineFragmentOrigin |
                                   NSStringDrawingTruncatesLastVisibleLine];
    // Add a little padding or it looks cramped.
    int heightProvided = textRect.size.height + 2;
    height += heightProvided;
  }
  return height;
}

@end

@implementation OmniboxPopupMatrix

@synthesize separator = separator_;
@synthesize maxMatchContentsWidth = maxMatchContentsWidth_;
@synthesize contentLeftPadding = contentLeftPadding_;
@synthesize contentMaxWidth = contentMaxWidth_;
@synthesize answerLineHeight = answerLineHeight_;
@synthesize hasDarkTheme = hasDarkTheme_;

- (instancetype)initWithObserver:(OmniboxPopupMatrixObserver*)observer
                    forDarkTheme:(BOOL)isDarkTheme {
  if ((self = [super initWithFrame:NSZeroRect])) {
    observer_ = observer;
    hasDarkTheme_ = isDarkTheme;

    base::scoped_nsobject<NSTableColumn> column(
        [[NSTableColumn alloc] initWithIdentifier:@"MainColumn"]);
    [column setDataCell:[[[OmniboxPopupCell alloc] init] autorelease]];
    [self addTableColumn:column];

    // Cells pack with no spacing.
    [self setIntercellSpacing:NSMakeSize(0.0, 0.0)];

    [self setSelectionHighlightStyle:NSTableViewSelectionHighlightStyleNone];
    NSColor* backgroundColor =
        OmniboxPopupViewMac::BackgroundColor(hasDarkTheme_);
    [self setBackgroundColor:backgroundColor];
    [self setAllowsEmptySelection:YES];
    [self deselectAll:self];

    [self resetTrackingArea];

    base::scoped_nsobject<NSLayoutManager> layoutManager(
        [[NSLayoutManager alloc] init]);
    answerLineHeight_ =
        [layoutManager defaultLineHeightForFont:OmniboxViewMac::GetLargeFont()];
  }
  return self;
}

- (OmniboxPopupTableController*)controller {
  return base::mac::ObjCCastStrict<OmniboxPopupTableController>(
      [self delegate]);
}

- (void)setObserver:(OmniboxPopupMatrixObserver*)observer {
  observer_ = observer;
}

- (void)updateTrackingAreas {
  [self resetTrackingArea];
  [super updateTrackingAreas];
}

// Callbacks from tracking area.
- (void)mouseMoved:(NSEvent*)theEvent {
  [self highlightRowUnder:theEvent];
}

- (void)mouseExited:(NSEvent*)theEvent {
  [self highlightRowUnder:theEvent];
}

// The tracking area events aren't forwarded during a drag, so handle
// highlighting manually for middle-click and middle-drag.
- (void)otherMouseDown:(NSEvent*)theEvent {
  if ([theEvent buttonNumber] == kMiddleButtonNumber) {
    [self highlightRowUnder:theEvent];
  }
  [super otherMouseDown:theEvent];
}

- (void)otherMouseDragged:(NSEvent*)theEvent {
  if ([theEvent buttonNumber] == kMiddleButtonNumber) {
    [self highlightRowUnder:theEvent];
  }
  [super otherMouseDragged:theEvent];
}

- (void)otherMouseUp:(NSEvent*)theEvent {
  // Only intercept middle button.
  if ([theEvent buttonNumber] != kMiddleButtonNumber) {
    [super otherMouseUp:theEvent];
    return;
  }

  // -otherMouseDragged: should always have been called at this location, but
  // make sure the user is getting the right feedback.
  [self highlightRowUnder:theEvent];

  const NSInteger highlightedRow = [[self controller] highlightedRow];
  if (highlightedRow != -1) {
    DCHECK(observer_);
    observer_->OnMatrixRowMiddleClicked(self, highlightedRow);
  }
}

// Track the mouse until released, keeping the cell under the mouse selected.
// If the mouse wanders off-view, revert to the originally-selected cell. If
// the mouse is released over a cell, call the delegate to open the row's URL.
- (void)mouseDown:(NSEvent*)theEvent {
  NSCell* selectedCell = [self selectedCell];

  // Clear any existing highlight.
  [[self controller] setHighlightedRow:-1];

  do {
    if (![self selectCellForEvent:theEvent]) {
      [self selectCell:selectedCell];
    }

    const NSUInteger mask = NSLeftMouseUpMask | NSLeftMouseDraggedMask;
    theEvent = [[self window] nextEventMatchingMask:mask];
  } while ([theEvent type] == NSLeftMouseDragged);

  // Do not message the delegate if released outside view.
  if (![self selectCellForEvent:theEvent]) {
    [self selectCell:selectedCell];
  } else {
    const NSInteger selectedRow = [self selectedRow];

    // No row could be selected if the model failed to update.
    if (selectedRow == -1) {
      NOTREACHED();
      return;
    }

    DCHECK(observer_);
    observer_->OnMatrixRowClicked(self, selectedRow);
  }
}

- (void)selectRowIndex:(NSInteger)rowIndex {
  NSIndexSet* indexSet = [NSIndexSet indexSetWithIndex:rowIndex];
  [self selectRowIndexes:indexSet byExtendingSelection:NO];
}

- (NSInteger)highlightedRow {
  return [[self controller] highlightedRow];
}

- (void)setController:(OmniboxPopupTableController*)controller {
  matrixController_.reset([controller retain]);
  [self setDelegate:controller];
  [self setDataSource:controller];
  [self reloadData];
}

- (void)resetTrackingArea {
  if (trackingArea_.get())
    [self removeTrackingArea:trackingArea_.get()];

  trackingArea_.reset([[CrTrackingArea alloc]
      initWithRect:[self frame]
           options:NSTrackingMouseEnteredAndExited |
                   NSTrackingMouseMoved |
                   NSTrackingActiveInActiveApp |
                   NSTrackingInVisibleRect
             owner:self
          userInfo:nil]);
  [self addTrackingArea:trackingArea_.get()];
}

- (void)highlightRowUnder:(NSEvent*)theEvent {
  NSPoint point = [self convertPoint:[theEvent locationInWindow] fromView:nil];
  NSInteger oldRow = [[self controller] highlightedRow];
  NSInteger newRow = [self rowAtPoint:point];
  if (oldRow != newRow) {
    [[self controller] setHighlightedRow:newRow];
    [self setNeedsDisplayInRect:[self rectOfRow:oldRow]];
    [self setNeedsDisplayInRect:[self rectOfRow:newRow]];
  }
}

- (BOOL)selectCellForEvent:(NSEvent*)theEvent {
  NSPoint point = [self convertPoint:[theEvent locationInWindow] fromView:nil];

  NSInteger row = [self rowAtPoint:point];
  [self selectRowIndex:row];
  if (row != -1) {
    DCHECK(observer_);
    observer_->OnMatrixRowSelected(self, row);
    return YES;
  }
  return NO;
}

- (void)setMatchIcon:(NSImage*)icon forRow:(NSInteger)rowIndex {
  [[self controller] setMatchIcon:icon forRow:rowIndex];
  [self setNeedsDisplayInRect:[self rectOfRow:rowIndex]];
}

@end
