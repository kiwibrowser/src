// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_STACK_VIEW_STACK_CARD_H_
#define IOS_CHROME_BROWSER_UI_STACK_VIEW_STACK_CARD_H_

#import <UIKit/UIKit.h>

@class CardView;
struct LayoutRect;
@class StackCard;

@protocol StackCardViewProvider
// Returns the view to use for the given card.
- (CardView*)cardViewWithFrame:(CGRect)frame forStackCard:(StackCard*)card;
@end

// Abstract representation of a card in the card stack. Allows operating on
// the layout aspects of the card without the card actually having a view, so
// that the view can easily be constructed lazily, purged for low memory, etc.
// |StackCard| does not pixel-align its coordinates to avoid losing precision,
// but does ensure that the origin of its view is always pixel-aligned.
@interface StackCard : NSObject

// The view corresponding to this card.
// WARNING: The coordinates of the view should not be set directly, as doing so
// will result in inconsistency.
@property(nonatomic, readonly, retain) CardView* view;
// The layout of the underyling CardView.
@property(nonatomic, readwrite, assign) LayoutRect layout;
// The size of the underlying CardView.  Setting this property updates |layout|
// with thew new size while maintaining the center of the card.
@property(nonatomic, readwrite, assign) CGSize size;
// If set to |YES|, updates to the frame/bounds/center of the card will also
// update the corresponding properties of its view; if |NO|, they will not
// (but immediate synchronization will occur the next time that the property is
// set to |YES|). Default is |YES|.
// NOTE: This property exists to enable updating a card's coordinates multiple
// times within an animation, as setting the coordinates of a |UIView| more
// than once inside an animation can result in undesired animation. Use this
// property with care.
@property(nonatomic, readwrite, assign) BOOL synchronizeView;
// |YES| if this card represents the current active tab.
@property(nonatomic, assign) BOOL isActiveTab;
// Opaque value representing the Tab to which this card corresponds. Do not add
// uses of this property.
// TODO(blundell): Remove this. b/8321162
@property(nonatomic, assign) NSUInteger tabID;
// Returns YES if the view for the card currently exists. Normally clients don't
// need to know this, since |view| will create the view on demand, but this can
// be used in cases where an operation should only be done if the view exists.
@property(nonatomic, readonly) BOOL viewIsLive;

// Initializes a new card which uses |viewProvider| to create its view.
// |viewProvider| is not retained, and must outlive this object.
- (instancetype)initWithViewProvider:(id<StackCardViewProvider>)viewProvider
    NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

// Releases the view; it will be re-created on next access. Can be used to
// temporarily free memory.
- (void)releaseView;

@end

#endif  // IOS_CHROME_BROWSER_UI_STACK_VIEW_STACK_CARD_H_
