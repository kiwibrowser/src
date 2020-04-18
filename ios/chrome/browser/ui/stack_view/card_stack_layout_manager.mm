// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/stack_view/card_stack_layout_manager.h"

#include <algorithm>
#include <cmath>

#include "base/logging.h"
#include "ios/chrome/browser/ui/rtl_geometry.h"
#import "ios/chrome/browser/ui/stack_view/card_view.h"
#import "ios/chrome/browser/ui/stack_view/stack_card.h"
#import "ios/chrome/browser/ui/stack_view/title_label.h"
#import "ios/chrome/browser/ui/ui_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// The maximum number of cards that should be staggered at a collapse point.
const NSInteger kMaxVisibleStaggerCount = 4;
// The amount that each of the staggered cards in a stack should be staggered
// when fully collapsed.
const CGFloat kMinStackStaggerAmount = 4.0;
// The amount that a card should overlap with a previous/subsequent card when
// it is extended the maximum distance away (e.g., after a multitouch event).
const CGFloat kFullyExtendedCardOverlap = 8.0;
// The amount that a card's position is allowed to drift toward overextension
// before the card is considered to be overextended (i.e., an epsilon to allow
// for floating-point imprecision).
const CGFloat kDistanceBeforeOverextension = 0.0001;
// The factor by which scroll is decayed on overscroll.
const CGFloat kOverextensionDecayFactor = 2.0;
// The amount by which a card is scrolled when asked to scroll it away from its
// preceding neighbor.
const CGFloat kScrollAwayFromNeighborAmount = 200;

}  // namespace

@interface CardStackLayoutManager () {
  NSMutableArray* cards_;
  // YES if the previous call to one of {|scrollCardAtIndex|,
  // |handleMultitouchWithFirstDelta|} was to the former method; NO otherwise.
  BOOL treatOverExtensionAsScroll_;
  NSUInteger previousFirstPinchCardIndex_;
  NSUInteger previousSecondPinchCardIndex_;
}

// Exposes |kMinStackStaggerAmount| for tests.
- (CGFloat)minStackStaggerAmount;
// Exposes |kScrollAwayFromNeighborAmount| for tests.
- (CGFloat)scrollCardAwayFromNeighborAmount;
// Returns the current start stack limit allowing for overextension as follows:
// - If the card at |index| is not overextended toward the start, returns
// |startLimit_|.
// - Otherwise, returns the value of the start limit such that the position of
// the card at |index| in the start stack is its current position (with the
// exception that the value is capped at |limitOfOverextensionTowardStart|).
- (CGFloat)startStackLimitAllowingForOverextensionOnCardAtIndex:
    (NSUInteger)index;
// Based on cards' current positions, |startLimit|, and |endLimit_|, caps cards
// that should be in the start and end stack. The reason that |startLimit| is
// a parameter is that the position of the start stack can change due to
// overextension.
- (void)layOutEdgeStacksWithStartLimit:(CGFloat)startLimit;
// Based on cards' current positions and |limit|, caps cards that should be in
// the start stack. The reason that |limit| is a parameter is that the desired
// position for the visual start of the start stack can change due to
// overextension.
- (void)layOutStartStackWithLimit:(CGFloat)limit;
// Positions the cards in the end stack based on |endLimit_|, leaving enough
// margin so that the last card in the stack has |kMinStackStaggerAmount|
// amount of visibility before |endLimit_|.
- (void)layOutEndStack;
// Computes the index of what should be the the inner boundary card in the
// indicated stack based on the current positions of the cards and the desired
// |visualStackLimit|.
- (NSInteger)computeEdgeStackBoundaryIndex:(BOOL)startStack
                      withVisualStackLimit:(CGFloat)visualStackLimit;
// Computes what the origin of the inner boundary card in the indicated stack
// based on |visualStackLimit|.
- (CGFloat)computeEdgeStackInnerEdge:(BOOL)startStack
                withVisualStackLimit:(CGFloat)visualStackLimit;
// Fans out the cards in the end stack and then recalculates the end stack.
- (void)recomputeEndStack;
// Fans out the cards in the start stack/end stack to be |maxStagger_| away
// from each other, with the first card in the stack being the greater of
// |maxStagger_| and its current distance away from its neighboring non-
// collapsed card.
- (void)fanOutCardsInEdgeStack:(BOOL)startStack;
// Returns the distance separating the origin of the card at |firstIndex| from
// that of the card at |secondIndex|.
- (CGFloat)distanceBetweenCardAtIndex:(NSUInteger)firstIndex
                       andCardAtIndex:(NSUInteger)secondIndex;
// Returns the minimum offset that the first card is allowed to over-extend to
// toward the start.
- (CGFloat)limitOfOverextensionTowardStart;
// Returns the maximum offset that the first card is allowed to overscroll to
// toward the end.
- (CGFloat)limitOfOverscrollTowardEnd;
// Caps overscroll toward start and end to maximum allowed amounts and re-lays
// out the start and end stacks. If |allowEarlyOverscroll| is |YES|,
// overscrolling is allowed to occur naturally on the scrolled card; otherwise,
// overscrolling is not allowed to occur until the stack is fully
// collapsed/fanned out.
- (void)capOverscrollWithScrolledIndex:(NSUInteger)scrolledIndex
                  allowEarlyOverscroll:(BOOL)allowEarlyOverscroll;
// Caps overscroll toward end to maximum allowed amount.
- (void)capOverscrollTowardEnd;
// Moves the cards so that any overscroll is eliminated.
- (void)eliminateOverscroll;
// Moves the cards so that any overpinch is eliminated.
- (void)eliminateOverpinch;
// Returns the maximum amount that a card can be offset from a
// preceding/following card: |cardSize - kFullyExtendedCardOverlap|.
- (CGFloat)maximumCardSeparation;
// Returns the maximum offset that the card at |index| can have given the
// constraint that no card can start more than
// |maximumCardSeparation:| away from the previous card.
- (CGFloat)maximumOffsetForCardAtIndex:(NSInteger)index;
// Returns the offset that the card at |index| would have after calling
// |fanOutCardsWithStartIndex:0|.
- (CGFloat)cappedFanoutOffsetForCardAtIndex:(NSInteger)index;
// Moves the card at |index| by |amount| along the layout axis, centered in the
// other direction at layoutAxisPosition_.
- (void)moveCardAtIndex:(NSUInteger)index byAmount:(CGFloat)amount;
// Moves |card|'s layout by |amount| along the layout axis.
- (void)moveCard:(StackCard*)card byAmount:(CGFloat)amount;
// Moves each of the cards between |startIndex| and |endIndex| inclusive by
// |delta| along the layout axis.
- (void)moveCardsFromIndex:(NSUInteger)startIndex
                   toIndex:(NSUInteger)endIndex
                  byAmount:(CGFloat)amount;

// Moves each of the cards before/after |index| (as indicated by |toEnd|)
// by |amount| with the constraint that for a non-edge-stack card (and for
// cards in the start stack if |restoreFanOutInStartStack| is |YES|), the
// amount that the card is moved is decreased by the amount necessary to
// restore the separation between that card and its next/previous neighbor to
// |maxStagger_|.  Assumes that the card at |index| has been moved by |amount|
// prior to calling this method. `
- (void)moveCardsrestoringFanoutFromIndex:(NSUInteger)index
                                    toEnd:(BOOL)toEnd
                                 byAmount:(CGFloat)amount
                restoreFanOutInStartStack:(BOOL)restoreFanOutInStartStack;
// Moves the origin of the card at |index| to |offset| along the layout axis,
// centered in the other direction at layoutAxisPosition_.
- (void)moveOriginOfCardAtIndex:(NSUInteger)index toOffset:(CGFloat)offset;
// Returns |offset| modified as necessary to make sure that it is not too
// close or too far from the origin of its constraining neighbor (previous or
// next, as determined by |constrainingNeighborIsPrevious|).
- (CGFloat)constrainedOffset:(CGFloat)offset
                    forCardAtIndex:(NSInteger)index
    constrainingNeighborIsPrevious:(BOOL)isPrevious;
// Moves the cards starting at |index| by an amount that decays from
// |drivingDelta| with each card that gets moved.
- (void)moveCardsStartingAtIndex:(NSInteger)index
                      towardsEnd:(BOOL)towardsEnd
                withDrivingDelta:(CGFloat)delta;
// Moves the cards in-between |firstIndex| and |secondIndex > firstIndex|
// inclusive via a proportional blend of |firstDelta| and |secondDelta|.
- (void)blendOffsetsOfCardsBetweenFirstIndex:(NSInteger)firstIndex
                                 secondIndex:(NSInteger)secondIndex
                              withFirstDelta:(CGFloat)firstDelta
                                 secondDelta:(CGFloat)secondDelta;
// Returns the length of |size| in the current layout direction.
- (CGFloat)layoutLength:(CGSize)size;
// Returns the offset of |position| in the current layout direction.
- (CGFloat)layoutOffset:(LayoutRectPosition)position;
// Returns the offset of |card| in the current layout direction.
- (CGFloat)cardOffsetOnLayoutAxis:(StackCard*)card;
// Returns the pixel offset relative to the first/last card in a fully
// compressed stack to show a card that is |countFromEdge| fram the start/end.
- (CGFloat)staggerOffsetForIndexFromEdge:(NSInteger)countFromEdge;
// Returns the pixel offset relative to the first/last card in a fully
// compressed stack where a card being pushed onto the stack should start
// moving the existing cards.
- (CGFloat)pushThresholdForIndexFromEdge:(NSInteger)countFromEdge;
// Controls whether the cards keep their views synchronized when updates are
// made to their frame/bounds/center.
- (void)setSynchronizeCardViews:(BOOL)synchronizeViews;
// Returns YES if |index| is in the start stack.
- (BOOL)isInStartStack:(NSUInteger)index;
// Returns YES if |index| is in the end stack.
- (BOOL)isInEndStack:(NSUInteger)index;
// Returns YES if |index| is in the start or end stack.
- (BOOL)isInEdgeStack:(NSUInteger)index;

@end

#pragma mark -

@implementation CardStackLayoutManager

@synthesize cardSize = cardSize_;
@synthesize maxStagger = maxStagger_;
@synthesize maximumOverextensionAmount = maximumOverextensionAmount_;
@synthesize endLimit = endLimit_;
@synthesize layoutAxisPosition = layoutAxisPosition_;
@synthesize startLimit = startLimit_;
@synthesize layoutIsVertical = layoutIsVertical_;
@synthesize lastStartStackCardIndex = lastStartStackCardIndex_;
@synthesize firstEndStackCardIndex = firstEndStackCardIndex_;

- (id)init {
  if ((self = [super init])) {
    cards_ = [[NSMutableArray alloc] init];
    layoutIsVertical_ = YES;
    lastStartStackCardIndex_ = -1;
    firstEndStackCardIndex_ = -1;
  }
  return self;
}

- (CGFloat)minStackStaggerAmount {
  return kMinStackStaggerAmount;
}

- (CGFloat)scrollCardAwayFromNeighborAmount {
  return kScrollAwayFromNeighborAmount;
}

- (void)setEndLimit:(CGFloat)endLimit {
  endLimit_ = endLimit;
  [self recomputeEndStack];
}

- (void)addCard:(StackCard*)card {
  [self insertCard:card atIndex:[cards_ count]];
}

- (void)insertCard:(StackCard*)card atIndex:(NSUInteger)index {
  card.size = cardSize_;
  [cards_ insertObject:card atIndex:index];
}

- (void)removeCard:(StackCard*)card {
  // Update edge stack boundary indices if necessary.
  NSInteger cardIndex = [cards_ indexOfObject:card];
  DCHECK(cardIndex != NSNotFound);
  if (cardIndex <= lastStartStackCardIndex_)
    lastStartStackCardIndex_ -= 1;
  if (cardIndex < firstEndStackCardIndex_)
    firstEndStackCardIndex_ -= 1;

  [cards_ removeObject:card];
}

- (void)removeAllCards {
  lastStartStackCardIndex_ = -1;
  firstEndStackCardIndex_ = -1;
  [cards_ removeAllObjects];
}

- (void)setCardSize:(CGSize)size {
  cardSize_ = size;
  NSUInteger i = 0;
  CGFloat previousFirstCardOffset = 0;
  CGFloat newFirstCardOffset = 0;
  for (StackCard* card in cards_) {
    CGFloat offset = [self cardOffsetOnLayoutAxis:card];
    card.size = cardSize_;
    CGFloat newOffset = offset;

    // Attempt to preserve card positions, but ensure that the deck starts
    // within overextension limits and that all cards not in the start stack are
    // within minimum/maximum separation limits of their preceding neighbors.
    if (i == 0) {
      newOffset = std::max(newOffset, [self limitOfOverextensionTowardStart]);
      newOffset = std::min(newOffset, [self limitOfOverscrollTowardEnd]);
      previousFirstCardOffset = offset;
      newFirstCardOffset = newOffset;
    } else if ((NSInteger)i <= lastStartStackCardIndex_) {
      // Preserve the layout of the start stack.
      newOffset = newFirstCardOffset + (offset - previousFirstCardOffset);
    } else {
      newOffset = [self constrainedOffset:newOffset
                           forCardAtIndex:i
           constrainingNeighborIsPrevious:YES];
    }

    [self moveOriginOfCardAtIndex:i toOffset:newOffset];
    i++;
  }
}

- (void)setLayoutIsVertical:(BOOL)layoutIsVertical {
  if (layoutIsVertical_ == layoutIsVertical)
    return;
  layoutIsVertical_ = layoutIsVertical;
  // Restore the cards' positions along the new layout axis.
  for (NSUInteger i = 0; i < [cards_ count]; i++) {
    LayoutRectPosition position = [[cards_ objectAtIndex:i] layout].position;
    CGFloat prevLayoutAxisOffset =
        layoutIsVertical_ ? position.leading : position.originY;
    [self moveOriginOfCardAtIndex:i toOffset:prevLayoutAxisOffset];
  }
}

- (void)setLayoutAxisPosition:(CGFloat)position {
  layoutAxisPosition_ = position;
  for (StackCard* card in cards_) {
    LayoutRect layout = card.layout;
    if (layoutIsVertical_)
      layout.position.leading = position - 0.5 * layout.size.width;
    else
      layout.position.originY = position - 0.5 * layout.size.height;
    card.layout = layout;
  }
}

- (NSArray*)cards {
  return cards_;
}

- (void)fanOutCardsWithStartIndex:(NSUInteger)startIndex {
  NSUInteger numCards = [cards_ count];
  if (numCards == 0)
    return;
  DCHECK(startIndex < numCards);

  // Temporarily turn off updates to the cards' views as this method might be
  // being called from within an animation, and updating the coordinates of a
  // |UIView| multiple times while it is animating can cause undesired
  // behavior.
  [self setSynchronizeCardViews:NO];

  // Move the cards starting at |startIndex| into place.
  for (NSUInteger i = 0; i < numCards - startIndex; ++i) {
    // The start cap for this card, accounting for visual stacking.
    CGFloat uncappedPosition = i * maxStagger_ + startLimit_;
    [self moveOriginOfCardAtIndex:(startIndex + i) toOffset:uncappedPosition];
  }

  // Fan out the cards behind the one at |startIndex|.
  for (NSInteger i = (startIndex - 1); i >= 0; --i) {
    CGFloat uncappedPosition = startLimit_ - (startIndex - i) * maxStagger_;
    [self moveOriginOfCardAtIndex:i toOffset:uncappedPosition];
  }

  [self layOutEdgeStacksWithStartLimit:startLimit_];
  [self setSynchronizeCardViews:YES];
}

- (void)recomputeEndStack {
  [self setSynchronizeCardViews:NO];
  if (firstEndStackCardIndex_ != -1)
    [self fanOutCardsInEdgeStack:NO];
  [self layOutEndStack];
  [self setSynchronizeCardViews:YES];
}

// Starts the fan at the stack boundary if the neighboring non-collapsed card
// is at least |maxStagger_| away from the stack (note that due to pinching,
// the neighboring card can be an arbitrary distance away from the stack);
// otherwise, starts the fan at |maxStagger_| away from that neighboring
// non-collapsed card.
- (void)fanOutCardsInEdgeStack:(BOOL)startStack {
  NSUInteger numCards = [cards_ count];
  if (numCards == 0)
    return;
  NSUInteger numCardsToMove;
  if (startStack)
    numCardsToMove = lastStartStackCardIndex_ + 1;
  else
    numCardsToMove = numCards - firstEndStackCardIndex_;

  if (numCardsToMove == 0)
    return;

  // Find the offset at which to start.
  NSUInteger stackBoundaryIndex =
      startStack ? lastStartStackCardIndex_ : firstEndStackCardIndex_;
  CGFloat startOffset =
      [self cardOffsetOnLayoutAxis:[cards_ objectAtIndex:stackBoundaryIndex]];
  if ((startStack && stackBoundaryIndex < numCards - 1) ||
      (!startStack && stackBoundaryIndex > 0)) {
    // Ensure that the stack is laid out starting at least |maxStagger_|
    // separation from the neighboring non-collapsed card.
    NSUInteger nonCollapsedLimitIndex =
        startStack ? stackBoundaryIndex + 1 : stackBoundaryIndex - 1;
    CGFloat nonCollapsedLimitOffset = [self
        cardOffsetOnLayoutAxis:[cards_ objectAtIndex:nonCollapsedLimitIndex]];
    CGFloat distance = fabs(nonCollapsedLimitOffset - startOffset);
    if (distance < maxStagger_) {
      startOffset = startStack ? nonCollapsedLimitOffset - maxStagger_
                               : nonCollapsedLimitOffset + maxStagger_;
    }
  }

  NSUInteger currentIndex = stackBoundaryIndex;
  for (NSUInteger i = 0; i < numCardsToMove; i++) {
    DCHECK(currentIndex < numCards);
    CGFloat delta = startStack ? i * -maxStagger_ : i * maxStagger_;
    CGFloat newOrigin = startOffset + delta;
    [self moveOriginOfCardAtIndex:currentIndex toOffset:newOrigin];
    currentIndex = startStack ? currentIndex - 1 : currentIndex + 1;
  }
}

- (CGFloat)distanceBetweenCardAtIndex:(NSUInteger)firstIndex
                       andCardAtIndex:(NSUInteger)secondIndex {
  DCHECK(firstIndex < [cards_ count]);
  DCHECK(secondIndex < [cards_ count]);
  CGFloat firstOrigin =
      [self cardOffsetOnLayoutAxis:[cards_ objectAtIndex:firstIndex]];
  CGFloat secondOrigin =
      [self cardOffsetOnLayoutAxis:[cards_ objectAtIndex:secondIndex]];
  return std::abs(secondOrigin - firstOrigin);
}

- (BOOL)overextensionTowardStartOnCardAtIndex:(NSUInteger)index {
  DCHECK(index < [cards_ count]);
  CGFloat offset = [self cardOffsetOnLayoutAxis:[cards_ objectAtIndex:index]];
  CGFloat collapsedOffset =
      startLimit_ + [self staggerOffsetForIndexFromEdge:index];
  // Uses an epsilon to allow for floating-point imprecision.
  return (offset < collapsedOffset - kDistanceBeforeOverextension);
}

- (BOOL)overextensionTowardEndOnFirstCard {
  if ([cards_ count] == 0)
    return NO;
  CGFloat offset = [self cardOffsetOnLayoutAxis:[cards_ firstObject]];
  // Uses an epsilon to allow for floating-point imprecision.
  return (offset > startLimit_ + kDistanceBeforeOverextension);
}

- (CGFloat)limitOfOverextensionTowardStart {
  return startLimit_ - maximumOverextensionAmount_;
}

- (CGFloat)limitOfOverscrollTowardEnd {
  return startLimit_ + maximumOverextensionAmount_;
}

- (void)capOverscrollWithScrolledIndex:(NSUInteger)scrolledIndex
                  allowEarlyOverscroll:(BOOL)allowEarlyOverscroll {
  DCHECK(scrolledIndex < [cards_ count]);
  [self capOverscrollTowardEnd];
  // Allow for overscroll as appropriate when laying out the start stack.
  NSUInteger allowedStartOverscrollIndex =
      allowEarlyOverscroll ? scrolledIndex : [cards_ count] - 1;
  CGFloat startLimit =
      [self startStackLimitAllowingForOverextensionOnCardAtIndex:
                allowedStartOverscrollIndex];
  [self layOutEdgeStacksWithStartLimit:startLimit];
}

// Reduces overscroll on the first card to its maximum allowed amount, and
// undoes the effect of the extra overscroll on the rest of the cards. NOTE: In
// the current implementation of scroll, undoing the effect of the extra
// overscroll on the rest of the cards is as simple as moving them the reverse
// of the extra overscroll amount. If the implementation of scroll becomes more
// complex, undoing the effect of the extra overscroll may have to become more
// complex to correspond.
- (void)capOverscrollTowardEnd {
  if ([cards_ count] == 0)
    return;
  CGFloat firstCardOffset = [self cardOffsetOnLayoutAxis:[cards_ firstObject]];
  CGFloat distance = firstCardOffset - [self limitOfOverscrollTowardEnd];
  if (distance > 0)
    [self moveCardsFromIndex:0 toIndex:[cards_ count] - 1 byAmount:-distance];
}

- (void)eliminateOverextension {
  if (treatOverExtensionAsScroll_)
    [self eliminateOverscroll];
  else
    [self eliminateOverpinch];
}

// If eliminating overscroll that was toward the end (where cards have
// overscrolled into the end stack), the cards scroll so that cards fan out
// from the end stack properly. If eliminating overscroll from the start, the
// overscrolled cards simply move back into place.
- (void)eliminateOverscroll {
  if ([cards_ count] == 0)
    return;
  [self setSynchronizeCardViews:NO];
  CGFloat firstCardOffset = [self cardOffsetOnLayoutAxis:[cards_ firstObject]];
  CGFloat overscrollEliminationAmount = startLimit_ - firstCardOffset;
  if (overscrollEliminationAmount <= 0) {
    [self scrollCardAtIndex:0
                     byDelta:overscrollEliminationAmount
        allowEarlyOverscroll:YES
           decayOnOverscroll:NO
          scrollLeadingCards:YES];
  }
  [self layOutEdgeStacksWithStartLimit:startLimit_];
  [self setSynchronizeCardViews:YES];
}

- (void)eliminateOverpinch {
  if ([cards_ count] == 0)
    return;
  DCHECK(previousFirstPinchCardIndex_ != NSNotFound);
  DCHECK(previousSecondPinchCardIndex_ != NSNotFound);
  DCHECK(previousFirstPinchCardIndex_ < [cards_ count]);
  DCHECK(previousSecondPinchCardIndex_ < [cards_ count]);
  CGFloat firstCardOffset = [self cardOffsetOnLayoutAxis:[cards_ firstObject]];
  CGFloat overpinchReductionAmount = startLimit_ - firstCardOffset;
  if (overpinchReductionAmount >= 0) {
    // Overpinching was toward the start stack. The overpinched cards simply
    // move back into place.
    [self layOutStartStackWithLimit:startLimit_];
  } else {
    // Overpinching was toward the end stack. The effect of the overpinch is
    // undone by a corresponding negating pinch.
    [self handleMultitouchWithFirstDelta:overpinchReductionAmount
                             secondDelta:0
                          firstCardIndex:previousFirstPinchCardIndex_
                         secondCardIndex:previousSecondPinchCardIndex_
                        decayOnOverpinch:NO];
  }
  [self setSynchronizeCardViews:NO];
  [self setSynchronizeCardViews:YES];
}

- (void)scrollCardAtIndex:(NSUInteger)index
                  byDelta:(CGFloat)delta
     allowEarlyOverscroll:(BOOL)allowEarlyOverscroll
        decayOnOverscroll:(BOOL)decayOnOverscroll
       scrollLeadingCards:(BOOL)scrollLeadingCards {
  NSUInteger numCards = [cards_ count];
  if (numCards == 0)
    return;
  DCHECK(index < [cards_ count]);

  treatOverExtensionAsScroll_ = YES;

  // Temporarily turn off updates to the cards' views as this method might be
  // being called from within an animation, and updating the coordinates of a
  // |UIView| multiple times while it is animating can cause undesired
  // behavior.
  [self setSynchronizeCardViews:NO];
  BOOL scrollIsTowardsEnd = (delta > 0);

  if (decayOnOverscroll) {
    // NOTE: This calculation is imprecise around the boundary case of a scroll
    // that moves the stack from not being overscrolled to being overscrolled.
    // This imprecision does not present a problem in practice, and eliminates
    // the need to compute the distance until the stack becomes overscrolled,
    // which is an unfortunately fiddly computation.
    if ([self overextensionTowardStartOnCardAtIndex:0] ||
        [self overextensionTowardEndOnFirstCard])
      delta = delta / kOverextensionDecayFactor;
  }

  NSUInteger leadingIndex = index;
  if (scrollLeadingCards)
    leadingIndex = scrollIsTowardsEnd ? numCards - 1 : 0;

  // Move the scrolled card and those further on in the direction being
  // scrolled by |delta|.
  if (scrollIsTowardsEnd)
    [self moveCardsFromIndex:index toIndex:leadingIndex byAmount:delta];
  else
    [self moveCardsFromIndex:leadingIndex toIndex:index byAmount:delta];

  // Move the cards trailing the scrolled card, but restore fan out in the
  // process as necessary.
  [self moveCardsrestoringFanoutFromIndex:index
                                    toEnd:!scrollIsTowardsEnd
                                 byAmount:delta
                restoreFanOutInStartStack:allowEarlyOverscroll];

  [self capOverscrollWithScrolledIndex:index
                  allowEarlyOverscroll:allowEarlyOverscroll];
  [self setSynchronizeCardViews:YES];
}

- (void)scrollCardAtIndex:(NSUInteger)index awayFromNeighbor:(BOOL)preceding {
  DCHECK(index < [cards_ count]);
  if (index == 0)
    return;

  CGFloat currentOffset =
      [self cardOffsetOnLayoutAxis:[cards_ objectAtIndex:index]];
  CGFloat offsetToScrollTo =
      preceding ? currentOffset + kScrollAwayFromNeighborAmount
                : currentOffset - kScrollAwayFromNeighborAmount;

  CGFloat limitOffsetToScrollTo;
  if (index == [cards_ count] - 1 && !preceding) {
    limitOffsetToScrollTo = endLimit_ - [self maximumCardSeparation];
  } else {
    CGFloat neighborOffset =
        preceding
            ? [self cardOffsetOnLayoutAxis:[cards_ objectAtIndex:index - 1]]
            : [self cardOffsetOnLayoutAxis:[cards_ objectAtIndex:index + 1]];
    limitOffsetToScrollTo = preceding
                                ? neighborOffset + [self maximumCardSeparation]
                                : neighborOffset - [self maximumCardSeparation];
  }
  offsetToScrollTo = preceding
                         ? std::min(offsetToScrollTo, limitOffsetToScrollTo)
                         : std::max(offsetToScrollTo, limitOffsetToScrollTo);

  CGFloat distanceToScroll = offsetToScrollTo - currentOffset;

  [self setSynchronizeCardViews:NO];
  if (preceding) {
    [self moveCardsFromIndex:index
                     toIndex:[cards_ count] - 1
                    byAmount:distanceToScroll];
  } else {
    [self moveCardsFromIndex:0 toIndex:index byAmount:distanceToScroll];
  }
  [self layOutEdgeStacksWithStartLimit:startLimit_];
  [self setSynchronizeCardViews:YES];
}

- (void)moveCardsrestoringFanoutFromIndex:(NSUInteger)index
                                    toEnd:(BOOL)toEnd
                                 byAmount:(CGFloat)amount
                restoreFanOutInStartStack:(BOOL)restoreFanOutInStartStack {
  DCHECK(index < [cards_ count]);

  // This method assumes that the cards are being moved toward the card at
  // |index|.
  if (toEnd)
    DCHECK(amount <= 0);
  else
    DCHECK(amount >= 0);

  CGFloat currentAmount = amount;
  // The index of the card against which separation will be checked for the
  // card currently being moved.
  NSUInteger precedingIndex = index;
  // The index of the card currently being moved.
  NSUInteger currentIndex = toEnd ? precedingIndex + 1 : precedingIndex - 1;
  NSInteger step = toEnd ? 1 : -1;

  // Move all the cards after/before the one at |index| as indicated by |toEnd|.
  NSInteger numCardsToMove = toEnd ? ([cards_ count] - index - 1) : index;
  for (int i = 0; i < numCardsToMove; i++) {
    BOOL restoreFanout = YES;
    // Do not restore fanout when cards are moving into an edge stack unless
    // directed to.
    if (toEnd) {
      if (!restoreFanOutInStartStack) {
        restoreFanout = (![self isInStartStack:currentIndex] &&
                         ![self isInStartStack:precedingIndex]);
      }
    } else {
      restoreFanout = (![self isInEndStack:currentIndex] &&
                       ![self isInEndStack:precedingIndex]);
    }

    if (restoreFanout) {
      CGFloat distance = [self distanceBetweenCardAtIndex:currentIndex
                                           andCardAtIndex:precedingIndex];
      // Account for the fact that the card at |precedingIndex| has already
      // been moved.
      distance -= std::abs(currentAmount);
      // Calculate how much of the move (if any) should be eliminated in order
      // to restore fan out between this card and the preceding card.
      CGFloat amountToRestoreFanOut =
          std::max<CGFloat>(0, maxStagger_ - distance);
      if (amountToRestoreFanOut > std::abs(currentAmount))
        currentAmount = 0;
      else if (currentAmount > 0)
        currentAmount -= amountToRestoreFanOut;
      else
        currentAmount += amountToRestoreFanOut;
    }
    [self moveCardAtIndex:currentIndex byAmount:currentAmount];
    precedingIndex = currentIndex;
    currentIndex += step;
  }
}

- (CGFloat)clipDelta:(CGFloat)delta forCardAtIndex:(NSInteger)index {
  DCHECK(index < (NSInteger)[cards_ count]);
  StackCard* card = [cards_ objectAtIndex:index];
  CGFloat startingOffset = [self cardOffsetOnLayoutAxis:card];
  if (delta < 0) {
    // |delta| is towards start stack.
    CGFloat collapsedPosition =
        startLimit_ + [self staggerOffsetForIndexFromEdge:index];
    delta = std::max(delta, collapsedPosition - startingOffset);
  } else {
    // |delta| is towards end stack.
    NSInteger indexFromEnd = [cards_ count] - 1 - index;
    CGFloat collapsedPosition =
        endLimit_ - kMinStackStaggerAmount -
        [self staggerOffsetForIndexFromEdge:indexFromEnd];
    delta = std::min(delta, collapsedPosition - startingOffset);
  }
  return delta;
}

- (CGFloat)maximumCardSeparation {
  return [self layoutLength:self.cardSize] - kFullyExtendedCardOverlap;
}

- (CGFloat)maximumOffsetForCardAtIndex:(NSInteger)index {
  DCHECK(index < (NSInteger)[cards_ count]);
  // Account for the fact that the first card may be overextended toward the
  // start or the end.
  CGFloat firstCardOffset = [self cardOffsetOnLayoutAxis:[cards_ firstObject]];
  return firstCardOffset + index * [self maximumCardSeparation];
}

- (CGFloat)cappedFanoutOffsetForCardAtIndex:(NSInteger)index {
  CGFloat fannedOutPosition = startLimit_ + index * maxStagger_;
  NSInteger indexFromEnd = [cards_ count] - 1 - index;
  CGFloat endStackPosition = endLimit_ - kMinStackStaggerAmount -
                             [self staggerOffsetForIndexFromEdge:indexFromEnd];
  return std::min(fannedOutPosition, endStackPosition);
}

- (void)moveCardAtIndex:(NSUInteger)index byAmount:(CGFloat)amount {
  DCHECK(index < [cards_ count]);
  [self moveCard:cards_[index] byAmount:amount];
}

- (void)moveCard:(StackCard*)card byAmount:(CGFloat)amount {
  DCHECK(card);
  LayoutRect layout = card.layout;
  if (layoutIsVertical_) {
    layout.position.leading = layoutAxisPosition_ - 0.5 * card.size.width;
    layout.position.originY += amount;
  } else {
    layout.position.leading += amount;
    layout.position.originY = layoutAxisPosition_ - 0.5 * card.size.height;
  }
  card.layout = layout;
}

- (void)moveCardsFromIndex:(NSUInteger)startIndex
                   toIndex:(NSUInteger)endIndex
                  byAmount:(CGFloat)amount {
  DCHECK(startIndex <= endIndex);
  DCHECK(endIndex < [cards_ count]);
  for (NSUInteger i = startIndex; i <= endIndex; ++i) {
    [self moveCardAtIndex:i byAmount:amount];
  }
}

- (void)moveOriginOfCardAtIndex:(NSUInteger)index toOffset:(CGFloat)offset {
  DCHECK(index < [cards_ count]);
  StackCard* card = [cards_ objectAtIndex:index];
  CGFloat startingOffset = [self cardOffsetOnLayoutAxis:card];
  [self moveCard:card byAmount:offset - startingOffset];
}

// Constrains offset to satisfy the following constraints:
// - >= |kMinStackStaggerAmount| away from origin of constraining neighbor.
// - <= |maximumCardSeparation:| away from origin of constraining neighbor.
// - <= |maximumOffsetForCardAtIndex:index|.
- (CGFloat)constrainedOffset:(CGFloat)offset
                    forCardAtIndex:(NSInteger)index
    constrainingNeighborIsPrevious:(BOOL)isPrevious {
  DCHECK(index < (NSInteger)[cards_ count]);
  if (isPrevious)
    DCHECK(index > 0);
  else
    DCHECK(index < (NSInteger)[cards_ count] - 1);

  CGFloat constrainingIndex = isPrevious ? index - 1 : index + 1;
  StackCard* constrainingCard = [cards_ objectAtIndex:constrainingIndex];
  CGFloat constrainingCardOffset =
      [self cardOffsetOnLayoutAxis:constrainingCard];
  // Ensures that the above constraints are mutually satisfiable.
  DCHECK(constrainingCardOffset <=
         [self maximumOffsetForCardAtIndex:constrainingIndex]);

  CGFloat minOffset, maxOffset;
  if (isPrevious) {
    minOffset = constrainingCardOffset + kMinStackStaggerAmount;
    maxOffset = constrainingCardOffset + [self maximumCardSeparation];
    maxOffset = std::min(maxOffset, [self maximumOffsetForCardAtIndex:index]);
  } else {
    minOffset = constrainingCardOffset - [self maximumCardSeparation];
    maxOffset = constrainingCardOffset - kMinStackStaggerAmount;
    maxOffset = std::min(maxOffset, [self maximumOffsetForCardAtIndex:index]);
  }
  DCHECK(minOffset <= maxOffset);
  offset = std::max(offset, minOffset);
  offset = std::min(offset, maxOffset);
  return offset;
}

// If |towardsEnd|, then all cards up to and including the last card are moved,
// with each card being constrained by the position of its previous neighbor.
// Otherwise, all cards down to but *not* including the first card are moved,
// with each card being constrained by the position of its following neighbor.
// NOTE: It is assumed that at the time of calling this method that the
// boundary card for the movement (i.e., the card before |index| if
// |towardsEnd|, the card after |index| otherwise), if it exists, is in its
// desired position, as constraining is performed in this method with respect
// to the position of that boundary card.
- (void)moveCardsStartingAtIndex:(NSInteger)index
                      towardsEnd:(BOOL)towardsEnd
                withDrivingDelta:(CGFloat)drivingDelta {
  const CGFloat kDecayFactor = 2.0;
  DCHECK(index < (NSInteger)[cards_ count]);
  DCHECK(index >= 0);

  NSInteger numCardsToMove;
  if (towardsEnd)
    numCardsToMove = [cards_ count] - index;
  else
    numCardsToMove = index;

  NSInteger currentIndex = index;
  CGFloat currentDelta = drivingDelta / kDecayFactor;
  for (int i = 0; i < numCardsToMove; i++) {
    StackCard* card = [cards_ objectAtIndex:currentIndex];
    CGFloat cardStartingOffset = [self cardOffsetOnLayoutAxis:card];
    CGFloat cardEndingOffset =
        [self constrainedOffset:cardStartingOffset + currentDelta
                            forCardAtIndex:currentIndex
            constrainingNeighborIsPrevious:towardsEnd];
    [self moveOriginOfCardAtIndex:currentIndex toOffset:cardEndingOffset];

    currentIndex = towardsEnd ? currentIndex + 1 : currentIndex - 1;
    currentDelta = (cardEndingOffset - cardStartingOffset) / kDecayFactor;
  }
}

// Moves cards as follows:
// - the card at |firstIndex| moves by |firstDelta|.
// - the card at |secondIndex| moves by |secondDelta|.
// - the cards in-between move by a combination of |firstDelta| and
//   |secondDelta|, with the contribution of each being weighted by the
//   closeness of the card's starting position to the starting positions of the
//   cards at |firstIndex| and |secondIndex| respectively.
// Each card is constrained to be within its maximum offset, and each card
// other than the first is constrained by the position of its previous
// neighbor.
// NOTE: It is assumed that at the time of calling this method the card before
// |firstIndex| and the card after |secondIndex|, if they exist, are not
// necessarily in their desired positions. Hence, no constraining is performed
// in this method with respect to the positions of those boundary cards.
- (void)blendOffsetsOfCardsBetweenFirstIndex:(NSInteger)firstIndex
                                 secondIndex:(NSInteger)secondIndex
                              withFirstDelta:(CGFloat)firstDelta
                                 secondDelta:(CGFloat)secondDelta {
  DCHECK(firstIndex < secondIndex);
  DCHECK(secondIndex < (NSInteger)[cards_ count]);
  StackCard* firstCard = [cards_ objectAtIndex:firstIndex];
  CGFloat firstStartingOffset = [self cardOffsetOnLayoutAxis:firstCard];
  StackCard* secondCard = [cards_ objectAtIndex:secondIndex];
  CGFloat secondStartingOffset = [self cardOffsetOnLayoutAxis:secondCard];
  CGFloat firstEndingOffset = firstStartingOffset + firstDelta;
  CGFloat secondEndingOffset = secondStartingOffset + secondDelta;

  // Move each card by a combination of |firstDelta| and |secondDelta|, with
  // the contribution of each being weighted by the card's closeness
  // to |firstStartingOffset| and |secondStartingOffset| respectively.
  for (NSInteger i = firstIndex; i <= secondIndex; i++) {
    StackCard* card = [cards_ objectAtIndex:i];
    CGFloat cardStartingOffset = [self cardOffsetOnLayoutAxis:card];
    CGFloat weightOfSecondDelta = (cardStartingOffset - firstStartingOffset) /
                                  (secondStartingOffset - firstStartingOffset);
    CGFloat weightOfFirstDelta = 1 - weightOfSecondDelta;
    CGFloat cardEndingOffset = weightOfFirstDelta * firstEndingOffset +
                               weightOfSecondDelta * secondEndingOffset;
    // First card being moved is not constrained to previous neighbor but is
    // constrained to be within its maximum offset unless it is the first card
    // of the deck, which is allowed to move off its maximum offset for an
    // overpinch effect.
    if (i == firstIndex) {
      if (i > 0) {
        cardEndingOffset = std::min(
            cardEndingOffset, [self maximumOffsetForCardAtIndex:firstIndex]);
      }
    } else {
      cardEndingOffset = [self constrainedOffset:cardEndingOffset
                                  forCardAtIndex:i
                  constrainingNeighborIsPrevious:YES];
    }
    [self moveOriginOfCardAtIndex:i toOffset:cardEndingOffset];
  }
}

// - The cards at indices between |firstCardIndex| and |secondCardIndex|
//   inclusive are blended proportionally between the ending positions of those
//   two cards.
// - The cards at indices < |firstCardIndex| are adjusted based on |firstDelta|
//   with an exponential decay.
// - The cards at indices > |secondCardIndex| are adjusted based on
//   |secondDelta| with an exponential decay.
- (void)handleMultitouchWithFirstDelta:(CGFloat)firstDelta
                           secondDelta:(CGFloat)secondDelta
                        firstCardIndex:(NSInteger)firstCardIndex
                       secondCardIndex:(NSInteger)secondCardIndex
                      decayOnOverpinch:(BOOL)decayOnOverpinch {
  DCHECK(firstCardIndex < secondCardIndex);
  NSInteger numCards = (NSInteger)[cards_ count];
  DCHECK(secondCardIndex < numCards);

  treatOverExtensionAsScroll_ = NO;
  previousFirstPinchCardIndex_ = firstCardIndex;
  previousSecondPinchCardIndex_ = secondCardIndex;

  // Temporarily turn off updates to the cards' views as this method might be
  // being called from within an animation, and updating the coordinates of a
  // |UIView| multiple times while it is animating can cause undesired
  // behavior.
  [self setSynchronizeCardViews:NO];

  if (decayOnOverpinch) {
    if ([self overextensionTowardStartOnCardAtIndex:firstCardIndex] ||
        (firstCardIndex == 0 && [self overextensionTowardEndOnFirstCard]))
      firstDelta /= kOverextensionDecayFactor;
    if ([self overextensionTowardStartOnCardAtIndex:secondCardIndex] ||
        (secondCardIndex == 0 && [self overextensionTowardEndOnFirstCard]))
      secondDelta /= kOverextensionDecayFactor;
  }

  // Blend the positions of the cards between the two touched cards (inclusive).
  // This step must be performed first, as the following two calls assume that
  // |firstCardIndex| and |secondCardIndex| are in their correct positions when
  // calculating constraints for positions of other cards.
  [self blendOffsetsOfCardsBetweenFirstIndex:firstCardIndex
                                 secondIndex:secondCardIndex
                              withFirstDelta:firstDelta
                                 secondDelta:secondDelta];

  // Adjust the cards after |secondCardIndex| and before |firstCardIndex|.
  if (secondCardIndex < numCards - 1) {
    [self moveCardsStartingAtIndex:secondCardIndex + 1
                        towardsEnd:YES
                  withDrivingDelta:secondDelta];
  }
  if (firstCardIndex > 0) {
    [self moveCardsStartingAtIndex:firstCardIndex - 1
                        towardsEnd:NO
                  withDrivingDelta:firstDelta];
  }

  // Perform start and end capping, allowing overextension on the start stack as
  // determined by the offset of the first pinched card.
  CGFloat startLimit = [self
      startStackLimitAllowingForOverextensionOnCardAtIndex:firstCardIndex];
  [self layOutEdgeStacksWithStartLimit:startLimit];
  [self setSynchronizeCardViews:YES];
}

- (CGFloat)startStackLimitAllowingForOverextensionOnCardAtIndex:
    (NSUInteger)index {
  DCHECK(index < [cards_ count]);
  if (![self overextensionTowardStartOnCardAtIndex:index])
    return startLimit_;
  // Calculate the start limit that will lay the start stack into place around
  // the card at |index|.
  CGFloat startLimit =
      [self cardOffsetOnLayoutAxis:[cards_ objectAtIndex:index]] -
      [self staggerOffsetForIndexFromEdge:index];
  return std::max(startLimit, [self limitOfOverextensionTowardStart]);
}

- (void)layOutEdgeStacksWithStartLimit:(CGFloat)startLimit {
  [self layOutStartStackWithLimit:startLimit];
  [self layOutEndStack];
}

- (void)layOutStartStack {
  [self layOutStartStackWithLimit:startLimit_];
}

- (void)layOutStartStackWithLimit:(CGFloat)limit {
  lastStartStackCardIndex_ =
      [self computeEdgeStackBoundaryIndex:YES withVisualStackLimit:limit];
  if (lastStartStackCardIndex_ == -1)
    return;

  // Position the cards. Cards up to the last card of the start stack are
  // staggered backwards from the start stack's inner edge.
  CGFloat stackInnerEdge =
      [self computeEdgeStackInnerEdge:YES withVisualStackLimit:limit];
  for (NSInteger i = 0; i <= lastStartStackCardIndex_; i++) {
    CGFloat distanceFromInnerEdge =
        (lastStartStackCardIndex_ - i) * kMinStackStaggerAmount;
    CGFloat offset = std::max(limit, stackInnerEdge - distanceFromInnerEdge);
    [self moveOriginOfCardAtIndex:i toOffset:offset];
  }
}

- (void)layOutEndStack {
  NSInteger numCards = [cards_ count];
  // When laying out the stack, leave enough room so that the last card is
  // visible.
  CGFloat visualLimit = endLimit_ - kMinStackStaggerAmount;
  firstEndStackCardIndex_ =
      [self computeEdgeStackBoundaryIndex:NO withVisualStackLimit:visualLimit];
  if (firstEndStackCardIndex_ == numCards)
    return;

  // Position the cards. Cards from the first card of the end stack are
  // staggered forwards from the end stack's inner edge.
  CGFloat stackInnerEdge =
      [self computeEdgeStackInnerEdge:NO withVisualStackLimit:visualLimit];
  for (NSInteger i = firstEndStackCardIndex_; i < numCards; i++) {
    CGFloat distanceFromInnerEdge =
        (i - firstEndStackCardIndex_) * kMinStackStaggerAmount;
    CGFloat offset =
        std::min(visualLimit, stackInnerEdge + distanceFromInnerEdge);
    [self moveOriginOfCardAtIndex:i toOffset:offset];
  }
}

- (NSInteger)computeEdgeStackBoundaryIndex:(BOOL)startStack
                      withVisualStackLimit:(CGFloat)visualStackLimit {
  NSInteger numCards = [cards_ count];
  NSInteger boundaryIndex = startStack ? -1 : numCards;
  for (NSInteger i = 0; i < numCards; ++i) {
    StackCard* card = [cards_ objectAtIndex:i];
    CGFloat uncappedPosition = [self cardOffsetOnLayoutAxis:card];
    if (startStack) {
      CGFloat pushThreshold =
          visualStackLimit + [self pushThresholdForIndexFromEdge:i];
      if (uncappedPosition <= pushThreshold)
        boundaryIndex = i;
    } else {
      NSInteger indexFromEnd = numCards - 1 - i;
      CGFloat pushThreshold =
          visualStackLimit - [self pushThresholdForIndexFromEdge:indexFromEnd];
      if (uncappedPosition >= pushThreshold) {
        boundaryIndex = i;
        break;
      }
    }
  }
  return boundaryIndex;
}

- (CGFloat)computeEdgeStackInnerEdge:(BOOL)startStack
                withVisualStackLimit:(CGFloat)visualStackLimit {
  NSInteger boundaryIndex =
      startStack ? lastStartStackCardIndex_ : firstEndStackCardIndex_;
  DCHECK(boundaryIndex >= 0);
  DCHECK(boundaryIndex < (NSInteger)[cards_ count]);
  StackCard* card = [cards_ objectAtIndex:boundaryIndex];
  CGFloat offset = [self cardOffsetOnLayoutAxis:card];
  NSUInteger indexFromEnd = [cards_ count] - 1 - boundaryIndex;
  CGFloat cap = startStack
                    ? visualStackLimit +
                          [self staggerOffsetForIndexFromEdge:boundaryIndex]
                    : visualStackLimit -
                          [self staggerOffsetForIndexFromEdge:indexFromEnd];
  return startStack ? std::max(cap, offset) : std::min(cap, offset);
}

- (CGFloat)fannedStackLength {
  if ([cards_ count] == 0)
    return 0;
  CGFloat cardLength = [self layoutLength:cardSize_];
  return maxStagger_ * ([cards_ count] - 1) + cardLength;
}

- (CGFloat)maximumStackLength {
  if ([cards_ count] == 0)
    return 0;
  CGFloat cardLength = [self layoutLength:cardSize_];
  return [self maximumCardSeparation] * ([cards_ count] - 1) + cardLength;
}

- (CGFloat)fullyCollapsedStackLength {
  CGFloat staggerLength =
      kMinStackStaggerAmount * (kMaxVisibleStaggerCount - 1);
  return [self layoutLength:cardSize_] + staggerLength;
}

- (CGFloat)layoutLength:(CGSize)size {
  return layoutIsVertical_ ? size.height : size.width;
}

- (CGFloat)layoutOffset:(LayoutRectPosition)position {
  return layoutIsVertical_ ? position.originY : position.leading;
}

- (CGFloat)cardOffsetOnLayoutAxis:(StackCard*)card {
  return [self layoutOffset:card.layout.position];
}

- (CGFloat)staggerOffsetForIndexFromEdge:(NSInteger)countFromEdge {
  return std::min(countFromEdge, kMaxVisibleStaggerCount - 1) *
         kMinStackStaggerAmount;
}

- (CGFloat)pushThresholdForIndexFromEdge:(NSInteger)countFromEdge {
  return std::min(countFromEdge, kMaxVisibleStaggerCount) *
         kMinStackStaggerAmount;
}

- (BOOL)cardIsCovered:(StackCard*)card {
  NSUInteger index = [cards_ indexOfObject:card];
  DCHECK(index != NSNotFound);
  DCHECK(index < [cards_ count]);

  if (index == [cards_ count] - 1)
    return NO;

  // Card positions are non-decreasing, and cards are all the same size, so a
  // card is completely covered iff the next card is in exactly the same
  // position (in terms of screen coordinates).
  StackCard* nextCard = [cards_ objectAtIndex:(index + 1)];
  LayoutRectPosition position =
      AlignLayoutRectPositionToPixel(card.layout.position);
  LayoutRectPosition nextPosition =
      AlignLayoutRectPositionToPixel(nextCard.layout.position);
  return LayoutRectPositionEqualToPosition(position, nextPosition);
}

- (BOOL)cardIsCollapsed:(StackCard*)card {
  NSUInteger index = [cards_ indexOfObject:card];
  DCHECK(index != NSNotFound);
  DCHECK(index < [cards_ count]);

  // Last card is collapsed if close enough to edge that title isn't visible.
  if (index == [cards_ count] - 1) {
    CGFloat cardOffset = [self cardOffsetOnLayoutAxis:card];
    CGFloat edgeOffset = endLimit_ - kMinStackStaggerAmount;
    return cardOffset >= edgeOffset;
  }
  CGFloat separation =
      [self distanceBetweenCardAtIndex:index andCardAtIndex:(index + 1)];
  return separation <= kMinStackStaggerAmount;
}

- (BOOL)cardLabelCovered:(StackCard*)card {
  NSUInteger index = [cards_ indexOfObject:card];
  CGFloat labelOffset = [card.view titleLabel].frame.size.height;
  if (index == [cards_ count] - 1) {
    CGFloat cardOffset = [self cardOffsetOnLayoutAxis:card];
    CGFloat edgeOffset = endLimit_ - labelOffset;
    return cardOffset >= edgeOffset;
  } else {
    CGFloat separation =
        [self distanceBetweenCardAtIndex:index andCardAtIndex:(index + 1)];
    return separation <= labelOffset;
  }
}

- (void)setSynchronizeCardViews:(BOOL)synchronizeViews {
  for (StackCard* card in cards_) {
    card.synchronizeView = synchronizeViews;
  }
}

- (BOOL)isInStartStack:(NSUInteger)index {
  DCHECK(index < [cards_ count]);
  return ((NSInteger)index <= lastStartStackCardIndex_);
}

- (BOOL)isInEndStack:(NSUInteger)index {
  DCHECK(index < [cards_ count]);
  return ((NSInteger)index >= firstEndStackCardIndex_);
}

- (BOOL)isInEdgeStack:(NSUInteger)index {
  return ([self isInStartStack:index] || [self isInEndStack:index]);
}

- (BOOL)stackIsFullyCollapsed {
  NSInteger numCards = [cards_ count];
  if (numCards == 0)
    return YES;
  return (lastStartStackCardIndex_ == (numCards - 1));
}

- (BOOL)stackIsFullyFannedOut {
  for (NSUInteger i = 0; i < [cards_ count]; i++) {
    CGFloat offset = [self cardOffsetOnLayoutAxis:[cards_ objectAtIndex:i]];
    if (offset < [self cappedFanoutOffsetForCardAtIndex:i])
      return NO;
  }
  return YES;
}

- (BOOL)stackIsFullyOverextended {
  NSInteger numCards = [cards_ count];
  if (numCards == 0)
    return YES;

  // Test for being fully overextended toward the start.
  StackCard* lastCard = [cards_ objectAtIndex:numCards - 1];
  CGFloat lastCardOrigin = [self cardOffsetOnLayoutAxis:lastCard];
  // Note that -limitOfOverextensionTowardStart is defined with respect to the
  // *start* of the stack.
  if ((lastCardOrigin - [self staggerOffsetForIndexFromEdge:numCards - 1]) <=
      [self limitOfOverextensionTowardStart])
    return YES;

  // Test for being fully overextended toward the end.
  StackCard* firstCard = [cards_ firstObject];
  return ([self cardOffsetOnLayoutAxis:firstCard] >=
          [self limitOfOverscrollTowardEnd]);
}

- (CGFloat)overextensionAmount {
  if ([cards_ count] == 0)
    return 0;
  return std::abs([self cardOffsetOnLayoutAxis:[cards_ firstObject]] -
                  startLimit_);
}

- (NSUInteger)fannedStackCount {
  return floor((endLimit_ - startLimit_) / maxStagger_);
}

@end
