// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_STACK_VIEW_CARD_SET_H_
#define IOS_CHROME_BROWSER_UI_STACK_VIEW_CARD_SET_H_

#import <UIKit/UIKit.h>
#include <vector>

#import "ios/chrome/browser/tabs/tab_model_observer.h"
#import "ios/chrome/browser/ui/stack_view/card_view.h"
#import "ios/chrome/browser/ui/stack_view/stack_card.h"

@class CardSet;
@class CardStackLayoutManager;
struct LayoutRect;
@class TabModel;

// Observer delegate for clients interested in changes to cards in a CardSet.
@protocol CardSetObserver

// |cardSet| closed all of its tabs.
- (void)cardSetDidCloseAllTabs:(CardSet*)cardSet;

// |newCard| has been added to |cardSet|.
- (void)cardSet:(CardSet*)cardSet didAddCard:(StackCard*)newCard;

// |cardBeingRemoved| will be removed from |cardSet|.
- (void)cardSet:(CardSet*)cardSet
    willRemoveCard:(StackCard*)cardBeingRemoved
           atIndex:(NSUInteger)index;

// |removedCard| has been removed from |cardSet|.
- (void)cardSet:(CardSet*)cardSet
    didRemoveCard:(StackCard*)removedCard
          atIndex:(NSUInteger)index;

// |card| has been made visible for the first time.
- (void)cardSet:(CardSet*)cardSet displayedCard:(StackCard*)card;

// |cardSet| completely rebuilt the cards; any cached reference to cards should
// be cleared, and any card setup done again for the whole card set.
// This is also called on initial creation of the card set.
- (void)cardSetRecreatedCards:(CardSet*)cardSet;

@end

// Manager for constructing a set of StackCards and displaying them in a view.
@interface CardSet : NSObject
// The layout manager for the set.
// TODO(stuartmorgan): See if this can reasonably be internalized.
@property(nonatomic, readonly) CardStackLayoutManager* stackModel;
// An array of StackCards in the same order as the tabs in the tab model.
@property(nonatomic, readonly) NSArray* cards;
// The card corresponding to the currently selected tab in the tab model.
// Setting this property will change the selection in the tab model.
@property(nonatomic) StackCard* currentCard;
// Set to the card that is currently animating closed, if any.
@property(nonatomic) StackCard* closingCard;
// The view that cards should be displayed in. Changing the display view will
// remove cards from the previous view, but they will not be re-displayed in the
// new view without a call to updateCardVisibilities.
@property(nonatomic, strong, readwrite) UIView* displayView;
// The side on which the close button should be displayed.
@property(nonatomic, readonly) CardCloseButtonSide closeButtonSide;
// The object to be notified about addition and removal of cards.
@property(nonatomic, weak, readwrite) id<CardSetObserver> observer;
// While YES, changes to the tab model will not affect the card stack. When
// this is changed back to NO, the card stack will be completely rebuilt, so
// this should be used very carefully.
@property(nonatomic, assign, readwrite) BOOL ignoresTabModelChanges;
// While set to YES, |updateCardVisibilities| will elide setting the views
// of covered cards to hidden. Setting to NO will trigger a call to
// |updateCardVisibilities| to hide any covered views. Can be set to YES
// during animations to ensure that cards being animated to not-visible
// positions remain visible throughout the animation; should be set back to
// NO on the completion of such animations. Default value is NO.
@property(nonatomic, assign, readwrite) BOOL defersCardHiding;
// The maximum amount that the first card is allowed to overextend toward the
// start or the end.
@property(nonatomic, assign) CGFloat maximumOverextensionAmount;
// If set to YES, a card view is released once the corresponding card becomes
// covered (it will be transparently reloaded if the card is uncovered), saving
// memory at the cost of potentially introducing jankiness. Default value is
// NO. Setting this property to YES causes views of covered cards to be
// immediately released.
@property(nonatomic, assign) BOOL keepOnlyVisibleCardViewsAlive;

// Initializes a card set backed by |model|. |model| is not retained, and must
// outlive the card set.
- (id)initWithModel:(TabModel*)model;

// Tears down any observation or other state. After this method is called, the
// receiver should be set to nil or otherwise marked for deallocation.
- (void)disconnect;

// The tab model backing the card set.
// TODO(stuartmorgan): See if this can reasonably be internalized.
- (TabModel*)tabModel;

// Sets the tab model backing the card set. This should only be called when the
// TabModel will be deleted and replaced by a new one. When called both the
// current tab model and the new tab model must be either nil, or contain no
// tabs.
- (void)setTabModel:(TabModel*)tabModel;

// Configures the stack fan out behavior based on the current display view
// size, card size, and layout direction. Sets the margin in the layout
// direction to |margin|.
- (void)configureLayoutParametersWithMargin:(CGFloat)margin;

// Updates the stack's visible size based on the current display view size,
// followed by recalculating the end stack. Should be called any time that the
// display view's size is changed.
- (void)displayViewSizeWasChanged;

// Sets the size of the cards in the set. Updates existing cards, and sets
// the size that will be used to make new cards in the future. Preserves card
// origins with the exception that cards are moved as necessary to satisfy
// placement constraints (e.g., that a card is not placed too far away from its
// preceding neighbor).
- (void)setCardSize:(CGSize)cardSize;

// Sets the layout axis position (i.e., the center of the cards in the
// non-layout direction) and layout direction of the card model. Sets the
// cards' positions along the new layout axis to their positions along the
// previous layout axis.
- (void)setLayoutAxisPosition:(CGFloat)position
                   isVertical:(BOOL)layoutIsVertical;

// Based on the cards' current positions, caps cards that should be in the
// start stack.
- (void)layOutStartStack;

// Fans out the cards starting at the first card and then gathers them in to
// the end limit of the visible stack.
- (void)fanOutCards;

// Fans out the cards and then gathers them in such that the first card not
// collapsed into the start stack is the card at |startIndex|.
- (void)fanOutCardsWithStartIndex:(NSUInteger)startIndex;

// Returns the current LayoutRects of the cards.
- (std::vector<LayoutRect>)cardLayouts;

// Scrolls the card at |index| by |delta| (and trailing cards by a maximum of
// |delta| depending on whether evening out after pinching needs to occur).
// Scrolls leading cards by delta if |scrollLeadingCards| is |YES|; otherwise,
// does not scroll leading cards. Allows a certain amount of overscrolling
// toward the start and end. If |decayOnOverscroll| is |YES|, the impact of the
// scroll decays once the stack is overscrolled. If |allowEarlyOverscroll|
// is |YES|, overscrolling is allowed to occur naturally on the scrolled card;
// otherwise, overscrolling is not allowed to occur until the stack is fully
// collapsed/fanned out.
- (void)scrollCardAtIndex:(NSUInteger)index
                  byDelta:(CGFloat)delta
     allowEarlyOverscroll:(BOOL)allowEarlyOverscroll
        decayOnOverscroll:(BOOL)decayOnOverscroll
       scrollLeadingCards:(BOOL)scrollLeadingCards;

// Whether the stack is overextended.
- (BOOL)stackIsOverextended;
// Returns whether the card at |index| is overextended, defined as being
// overextended toward the start or end for the first card and overextended
// toward the start for any other card.
- (BOOL)overextensionOnCardAtIndex:(NSUInteger)index;
// Returns whether the card at |index| is overextended toward the start.
- (BOOL)overextensionTowardStartOnCardAtIndex:(NSUInteger)index;
// Moves the cards so that any overextension is eliminated, with the nature of
// the movement being dependent on whether a scroll or a pinch occurred last.
- (void)eliminateOverextension;

// Scrolls the card at |index| to be at the maximum separation from the
// neighboring card, followed by handling start and end capping. If |preceding|,
// the card at |index| is scrolled away from the previous card toward the end
// stack, otherwise it is scrolled away from the next card toward the start
// stack. Also scrolls the cards that the card at |index| is scrolled toward,
// but does not alter the positions of the cards it is being scrolled away from.
- (void)scrollCardAtIndex:(NSUInteger)index awayFromNeighbor:(BOOL)preceding;

// Updates the visibility of each card based on whether or not it is covered by
// other cards. Should be called any time the card layout changes.
- (void)updateCardVisibilities;

// Preloads (constructs the view and adds it to the view hierarchy, hidden) the
// next card, if there is one that still needs to be preloaded. Returns YES if
// a card was preloaded, NO if all cards were already loaded.
- (BOOL)preloadNextCard;

// Sets all of the cards' gesture recognizer targets/delegatesthat are equal to
// |object| to nil.
// TODO(stuartmorgan): This should probably move up out of CardSet.
- (void)clearGestureRecognizerTargetAndDelegateFromCards:(id)object;

// Removes the card at |index|. Does not modify the underlying TabModel.
// TODO(blundell): This public method is a hack only present to work around a
// crash. Do not add calls to it. b/8321162
- (void)removeCardAtIndex:(NSUInteger)index;

// The maximum length (without margins) that the stack could possibly grow to.
- (CGFloat)maximumStackLength;

// Returns whether |card| is collapsed behind its succeeding neighbor, defined
// as being separated by <= the separation distance between visible cards in the
// edge stacks.
- (BOOL)cardIsCollapsed:(StackCard*)card;

// Returns |YES| if all cards are scrolled into the start stack.
- (BOOL)stackIsFullyCollapsed;

// Returns |YES| if the stack is fully fanned out.
- (BOOL)stackIsFullyFannedOut;

// Returns |YES| if the stack is fully overextended toward the start or the
// end.
- (BOOL)stackIsFullyOverextended;

// The amount that the first card is currently overextended.
- (CGFloat)overextensionAmount;

// Returns whether |card| is collapsed into the start stagger region.
- (BOOL)isCardInStartStaggerRegion:(StackCard*)card;

// Returns whether |card| is collapsed into the end stagger region.
- (BOOL)isCardInEndStaggerRegion:(StackCard*)card;

// Updates the frame of the stack shadow to the card stack's current layout.
- (void)updateShadowLayout;

@end

@interface CardSet (Testing)
- (StackCard*)cardForTab:(Tab*)tab;
- (void)setStackModelForTesting:(CardStackLayoutManager*)stackModel;
@end

#endif  // IOS_CHROME_BROWSER_UI_STACK_VIEW_CARD_SET_H_
