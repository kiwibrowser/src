// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTEXTUAL_SEARCH_CONTEXTUAL_SEARCH_HIGHLIGHTER_VIEW_H_
#define IOS_CHROME_BROWSER_UI_CONTEXTUAL_SEARCH_CONTEXTUAL_SEARCH_HIGHLIGHTER_VIEW_H_

#import <UIKit/UIKit.h>

@protocol ContextualSearchHighlighterDelegate

// Redraw the highlight rects. This method does not change which text is
// highlighted but updates the highlighting in case the text has moved.
- (void)updateHighlight;

@end

// A view containing semitransparent rects to highlight searched terms.
@interface ContextualSearchHighlighterView : UIView

// Designated initializer.
// |owner updateHighlight| will be called when the view size change.
- (instancetype)initWithFrame:(CGRect)frame
                     delegate:(id<ContextualSearchHighlighterDelegate>)delegate
    NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithFrame:(CGRect)frame NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder*)aDecoder NS_UNAVAILABLE;

// Creates the highlights in |rect|. |offset| is vertically applied to all
// rects. |scroll| is store to compute the delta scroll on subsequent calls to
// |setScroll|.
- (void)highlightRects:(NSArray*)rects
            withOffset:(CGFloat)offset
                  zoom:(CGFloat)zoom
                scroll:(CGPoint)scroll;

// Offset all highlighted rects by the delta between |scroll| and the previous
// value of |scroll|.
- (void)setScroll:(CGPoint)scroll zoom:(CGFloat)zoom offset:(CGFloat)offset;

- (CGRect)boundingRect;

@end

#endif  // IOS_CHROME_BROWSER_UI_CONTEXTUAL_SEARCH_CONTEXTUAL_SEARCH_HIGHLIGHTER_VIEW_H_
