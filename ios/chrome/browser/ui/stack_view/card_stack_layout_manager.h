// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_STACK_VIEW_CARD_STACK_LAYOUT_MANAGER_H_
#define IOS_CHROME_BROWSER_UI_STACK_VIEW_CARD_STACK_LAYOUT_MANAGER_H_

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>


@class StackCard;

// Encapsulates a stack of cards and their layout behavior, supporting
// fanning the cards out along an axis and gathering them to fit in a given
// area.
@interface CardStackLayoutManager : NSObject

// The size of a card. Setting this property preserves card origins with the
// exception that cards are moved as necessary to satisfy placement constraints
// (e.g., that a card is not placed too far away from its preceding neighbor).
@property(nonatomic, assign) CGSize cardSize;
// The center of a card along the axis that is not the layout axis.
@property(nonatomic, assign) CGFloat layoutAxisPosition;
// The amount cards should be staggered at full expansion.
@property(nonatomic, assign) CGFloat maxStagger;
// The maximum amount that the first card is allowed to overextend toward the
// start or the end.
@property(nonatomic, assign) CGFloat maximumOverextensionAmount;
// The offset denoting the start of the visible stack.
@property(nonatomic, assign) CGFloat startLimit;
// The offset denoting the end of the visible stack. Setting this property
// causes the end stack to be recomputed.
@property(nonatomic, assign) CGFloat endLimit;
// YES if the cards should be laid out vertically. Default is YES. A change in
// this property also sets the cards' positions along the new layout axis to
// their positions along the previous layout axis.
@property(nonatomic, assign) BOOL layoutIsVertical;
// TODO(blundell): The below two vars should be changed to NSUIntegers so that
// the implementation can uniformly use NSUIntegers. b/5956653
// The index of the last card that is in the initial stack.
@property(nonatomic, readonly, assign) NSInteger lastStartStackCardIndex;
// The index of the first card that is in the ending stack.
@property(nonatomic, readonly, assign) NSInteger firstEndStackCardIndex;

// Adds a |card| to the top (z-index) of the stack.
- (void)addCard:(StackCard*)card;

// Adds a |card| to the stack at the given index.
- (void)insertCard:(StackCard*)card atIndex:(NSUInteger)index;

// Removes |card| from the stack.
- (void)removeCard:(StackCard*)card;

// Removes all cards from the stack.
- (void)removeAllCards;

// Returns an array of cards, ordered from bottom to top of the stack (in
// z-order terms).
- (NSArray*)cards;

// Based on the cards' current positions and the value of |startLimit|, caps
// cards that should be in the start stack.
- (void)layOutStartStack;

// Fans out the cards and then gathers them in such that the first card not
// collapsed into the start stack is the card at |startIndex|.
- (void)fanOutCardsWithStartIndex:(NSUInteger)startIndex;

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

// Returns whether the card at |index| is overextended toward the start.
- (BOOL)overextensionTowardStartOnCardAtIndex:(NSUInteger)index;
// Returns whether the first card is overextended toward the end. Note that
// overextension is not a meaningful concept for other cards, as a pinch is
// allowed to leave cards other than the first card resting in arbitrary
// positions.
- (BOOL)overextensionTowardEndOnFirstCard;
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

// Updates the card positions based on the passed-in multitouch event, in which
// |firstCardIndex| represents the card closer to the start stack,
// |secondCardIndex > firstCardIndex| represents the card closer to the end
// stack, and |firstDelta| and |secondDelta| represent the two cards'
// respective movements. Caps card positions when cards get too close to each
// other. Allows overpinch toward the start stack, and allows the first card to
// overpinch toward the end stack when it is being pinched. If
// |decayOnOverpinch| is |YES|, the impact of each side of the pinch is
// decayed once the card being pinched is overpinched.
- (void)handleMultitouchWithFirstDelta:(CGFloat)firstDelta
                           secondDelta:(CGFloat)secondDelta
                        firstCardIndex:(NSInteger)firstCardIndex
                       secondCardIndex:(NSInteger)secondCardIndex
                      decayOnOverpinch:(BOOL)decayOnOverpinch;

// The length required to display a fully fanned-out stack.
- (CGFloat)fannedStackLength;

// The maximum length that the stack could possibly grow to accounting for the
// fact that the user can pinch to pull cards apart further than the fanned-out
// separation.
- (CGFloat)maximumStackLength;

// The length required to display a fully collapsed stack. Note that this does
// not depend on the number of cards in the stack; it will always return the
// amount needed to show an arbitrarily large stack, so that it has a
// consistent result that can be used to compute card sizes/margins.
- (CGFloat)fullyCollapsedStackLength;

// Returns YES if |card| is completely covered by other cards.
- (BOOL)cardIsCovered:(StackCard*)card;

// Returns whether |card| is collapsed behind its succeeding neighbor, defined
// as being separated by <= the separation distance between visible cards in the
// edge stacks.
- (BOOL)cardIsCollapsed:(StackCard*)card;

// Returns whether the title label for |card| is covered by its succeeding
// neighbor or the edge of the screen.
- (BOOL)cardLabelCovered:(StackCard*)card;

// Returns |YES| if all cards are scrolled into the start stack.
- (BOOL)stackIsFullyCollapsed;

// Returns |YES| if the stack is fully fanned out.
- (BOOL)stackIsFullyFannedOut;

// Returns |YES| if the stack is fully overextended toward the start or the
// end.
- (BOOL)stackIsFullyOverextended;

// The amount that the first card is currently overextended.
- (CGFloat)overextensionAmount;

// The number of cards visible when stack is fanned out.
- (NSUInteger)fannedStackCount;

@end

#endif  // IOS_CHROME_BROWSER_UI_STACK_VIEW_CARD_STACK_LAYOUT_MANAGER_H_
