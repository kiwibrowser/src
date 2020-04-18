// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/stack_view/card_stack_layout_manager.h"
#include "base/macros.h"
#include "ios/chrome/browser/ui/rtl_geometry.h"
#import "ios/chrome/browser/ui/stack_view/stack_card.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface CardStackLayoutManager (Private)
- (CGFloat)minStackStaggerAmount;
- (CGFloat)scrollCardAwayFromNeighborAmount;
- (CGFloat)staggerOffsetForIndexFromEdge:(NSInteger)countFromEdge;
- (CGFloat)maximumCardSeparation;
@end

// A mock version of StackCard.
@interface MockStackCard : NSObject

@property(nonatomic, readwrite, assign) BOOL synchronizeView;
@property(nonatomic, readwrite, assign) LayoutRect layout;
@property(nonatomic, readwrite, assign) CGSize size;

@end

@implementation MockStackCard

@synthesize synchronizeView = _synchronizeView;
@synthesize layout = _layout;
@synthesize size = _size;

- (void)setSize:(CGSize)size {
  _layout.position.leading += (_layout.size.width - size.width) / 2.0;
  _layout.position.originY += (_layout.size.height - size.height) / 2.0;
  _layout.size = size;
  _size = size;
}

@end

namespace {

using CardStackLayoutManagerTest = PlatformTest;

const float kMargin = 5;
const float kMaxStagger = 40;
const float kAxisPosition = 55;
const float kCardWidth = 300;
const float kCardHeight = 400;
const float kDefaultEndLimitFraction = 0.4;

// Returns the offset of |point| in the current layout direction.
CGFloat LayoutOffset(CardStackLayoutManager* stack,
                     LayoutRectPosition position) {
  return [stack layoutIsVertical] ? position.originY : position.leading;
}

// Returns the distance along the layout axis between the cards at |firstIndex|
// and |secondIndex|.
CGFloat SeparationOnLayoutAxis(CardStackLayoutManager* stack,
                               NSUInteger firstIndex,
                               NSUInteger secondIndex) {
  StackCard* firstCard = [[stack cards] objectAtIndex:firstIndex];
  StackCard* secondCard = [[stack cards] objectAtIndex:secondIndex];
  CGFloat firstCardOffset = LayoutOffset(stack, firstCard.layout.position);
  CGFloat secondCardOffset = LayoutOffset(stack, secondCard.layout.position);
  return secondCardOffset - firstCardOffset;
}

// Validates basic constraints:
// - All cards should be centered at kAxisPosition along the non-layout axis.
// - If not overscrolled toward start, all start edges should be at or past
//   kMargin.
// - All start edges should be visibly before endLimit.
// - No card should start before a previous card.
// - No card should start after the end of a previous card.
// If |shouldBeWithinMaxStagger|:
// - Consecutive cards should be no more than kMaxStagger apart.
void ValidateCardPositioningConstraints(CardStackLayoutManager* stack,
                                        CGFloat endLimit,
                                        bool shouldBeWithinMaxStagger) {
  BOOL isVertical = [stack layoutIsVertical];
  StackCard* previousCard = nil;
  for (StackCard* card in [stack cards]) {
    CGRect cardFrame = LayoutRectGetRect(card.layout);
    CGFloat nonLayoutAxisCenter =
        isVertical ? CGRectGetMidX(cardFrame) : CGRectGetMidY(cardFrame);
    EXPECT_FLOAT_EQ(kAxisPosition, nonLayoutAxisCenter);
    CGFloat startEdge = LayoutOffset(stack, card.layout.position);
    if (![stack overextensionTowardStartOnCardAtIndex:0])
      EXPECT_LE(kMargin, startEdge);
    EXPECT_GT(endLimit, startEdge);
    if (previousCard != nil) {
      CGFloat previousTopEdge =
          LayoutOffset(stack, previousCard.layout.position);
      EXPECT_LE(previousTopEdge, startEdge);
      CGFloat cardSize = isVertical ? kCardHeight : kCardWidth;
      EXPECT_GE(cardSize, startEdge - previousTopEdge);
      if (shouldBeWithinMaxStagger)
        EXPECT_GE(kMaxStagger, startEdge - previousTopEdge);
    }
    previousCard = card;
  }
}

// Creates a new |CardStackLayoutManager|, adds |n| cards to it, and sets
// dimensional/positioning parameters to the constants defined above.
CardStackLayoutManager* newStackOfNCards(unsigned int n, BOOL layoutIsVertical)
    NS_RETURNS_RETAINED {
  CardStackLayoutManager* stack = [[CardStackLayoutManager alloc] init];
  stack.layoutIsVertical = layoutIsVertical;
  for (unsigned int i = 0; i < n; ++i) {
    StackCard* card = static_cast<StackCard*>([[MockStackCard alloc] init]);
    [stack addCard:card];
  }

  CGSize cardSize = CGSizeMake(kCardWidth, kCardHeight);
  [stack setCardSize:cardSize];
  [stack setStartLimit:kMargin];
  [stack setMaxStagger:kMaxStagger];
  [stack setLayoutAxisPosition:kAxisPosition];
  CGFloat cardLength = layoutIsVertical ? cardSize.height : cardSize.width;
  [stack setMaximumOverextensionAmount:cardLength / 2.0];

  return stack;
}

TEST_F(CardStackLayoutManagerTest, CardSizing) {
  BOOL boolValues[2] = {NO, YES};
  for (unsigned long i = 0; i < arraysize(boolValues); i++) {
    CardStackLayoutManager* stack = [[CardStackLayoutManager alloc] init];
    stack.layoutIsVertical = boolValues[i];

    StackCard* view1 = static_cast<StackCard*>([[MockStackCard alloc] init]);
    StackCard* view2 = static_cast<StackCard*>([[MockStackCard alloc] init]);
    StackCard* view3 = static_cast<StackCard*>([[MockStackCard alloc] init]);
    [stack addCard:view1];
    [stack addCard:view2];
    [stack addCard:view3];
    // Ensure that removed cards are not altered.
    [stack removeCard:view2];

    CGSize cardSize = CGSizeMake(111, 222);
    [stack setCardSize:cardSize];

    EXPECT_FLOAT_EQ(cardSize.width, [view1 size].width);
    EXPECT_FLOAT_EQ(cardSize.height, [view1 size].height);
    EXPECT_FLOAT_EQ(0.0, [view2 size].width);
    EXPECT_FLOAT_EQ(0.0, [view2 size].height);
    EXPECT_FLOAT_EQ(cardSize.width, [view3 size].width);
    EXPECT_FLOAT_EQ(cardSize.height, [view3 size].height);

    // But it should be automatically updated when it's added again.
    [stack addCard:view2];
    EXPECT_FLOAT_EQ(cardSize.width, [view2 size].width);
    EXPECT_FLOAT_EQ(cardSize.height, [view2 size].height);
  }
}

TEST_F(CardStackLayoutManagerTest, StackSizes) {
  BOOL boolValues[2] = {NO, YES};
  for (unsigned long i = 0; i < arraysize(boolValues); i++) {
    CardStackLayoutManager* stack = [[CardStackLayoutManager alloc] init];
    stack.layoutIsVertical = boolValues[i];
    CGRect cardFrame = CGRectMake(0, 0, 100, 200);
    [stack setCardSize:cardFrame.size];
    [stack setMaxStagger:30];

    // Asking the size for a collapsed stack should give the same result.
    CGFloat emptyCollapsedSize = [stack fullyCollapsedStackLength];
    for (int i = 0; i < 10; ++i) {
      StackCard* card =
          static_cast<StackCard*>([[UIView alloc] initWithFrame:cardFrame]);
      [stack addCard:card];
    }
    CGFloat largeCollapsedSize = [stack fullyCollapsedStackLength];
    EXPECT_FLOAT_EQ(emptyCollapsedSize, largeCollapsedSize);

    // But a fanned-out stack should get bigger, and the maximum stack size
    // should be larger still.
    CGFloat largeExpandedSize = [stack fannedStackLength];
    EXPECT_GT(largeExpandedSize, largeCollapsedSize);
    CGFloat largeMaximumSize = [stack maximumStackLength];
    EXPECT_GT(largeMaximumSize, largeExpandedSize);
    StackCard* card = static_cast<StackCard*>([[MockStackCard alloc] init]);
    [stack addCard:card];
    CGFloat evenLargerExpandedSize = [stack fannedStackLength];
    EXPECT_LT(largeExpandedSize, evenLargerExpandedSize);
    CGFloat evenLargerMaximumSize = [stack maximumStackLength];
    EXPECT_LT(largeMaximumSize, evenLargerMaximumSize);

    // And start limit shouldn't matter.
    [stack setStartLimit:10];
    EXPECT_FLOAT_EQ(emptyCollapsedSize, [stack fullyCollapsedStackLength]);
    EXPECT_FLOAT_EQ(evenLargerExpandedSize, [stack fannedStackLength]);
    EXPECT_FLOAT_EQ(evenLargerMaximumSize, [stack maximumStackLength]);
  }
}

TEST_F(CardStackLayoutManagerTest, StackLayout) {
  BOOL boolValues[2] = {NO, YES};
  for (unsigned long i = 0; i < arraysize(boolValues); i++) {
    const unsigned int kCardCount = 30;
    CardStackLayoutManager* stack = newStackOfNCards(kCardCount, boolValues[i]);

    const float kEndLimit =
        kDefaultEndLimitFraction * [stack fannedStackLength];
    [stack setEndLimit:kEndLimit];
    [stack fanOutCardsWithStartIndex:0];

    EXPECT_EQ(kCardCount, [[stack cards] count]);
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_EQ(0, [stack lastStartStackCardIndex]);
    EXPECT_LT(0, [stack firstEndStackCardIndex]);
    for (NSInteger i = 0; i < [stack firstEndStackCardIndex]; i++) {
      StackCard* card = [[stack cards] objectAtIndex:i];
      EXPECT_FLOAT_EQ(kMargin + i * kMaxStagger,
                      LayoutOffset(stack, card.layout.position));
    }
  }
}

TEST_F(CardStackLayoutManagerTest, PreservingPositionsOnCardSizeChange) {
  BOOL boolValues[2] = {NO, YES};
  for (unsigned long i = 0; i < arraysize(boolValues); i++) {
    const unsigned int kCardCount = 3;
    CardStackLayoutManager* stack = newStackOfNCards(kCardCount, boolValues[i]);

    const float kEndLimit =
        kDefaultEndLimitFraction * [stack fannedStackLength];
    [stack setEndLimit:kEndLimit];
    [stack fanOutCardsWithStartIndex:0];

    EXPECT_EQ(kCardCount, [[stack cards] count]);
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_EQ(0, [stack lastStartStackCardIndex]);
    EXPECT_LT(0, [stack firstEndStackCardIndex]);
    for (NSInteger i = 0; i < [stack firstEndStackCardIndex]; i++) {
      StackCard* card = [[stack cards] objectAtIndex:i];
      EXPECT_FLOAT_EQ(kMargin + i * kMaxStagger,
                      LayoutOffset(stack, card.layout.position));
    }

    // Cards should retain their positions after changing the card size.
    CGSize cardSize = CGSizeMake(kCardWidth + 10, kCardHeight + 10);
    [stack setCardSize:cardSize];
    EXPECT_EQ(kCardCount, [[stack cards] count]);
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_EQ(0, [stack lastStartStackCardIndex]);
    EXPECT_LT(0, [stack firstEndStackCardIndex]);
    for (NSInteger i = 0; i < [stack firstEndStackCardIndex]; i++) {
      StackCard* card = [[stack cards] objectAtIndex:i];
      EXPECT_FLOAT_EQ(kMargin + i * kMaxStagger,
                      LayoutOffset(stack, card.layout.position));
    }
  }
}

TEST_F(CardStackLayoutManagerTest, SwappingPositionsOnOrientationChange) {
  BOOL boolValues[2] = {NO, YES};
  for (unsigned long i = 0; i < arraysize(boolValues); i++) {
    const unsigned int kCardCount = 3;
    CardStackLayoutManager* stack = newStackOfNCards(kCardCount, boolValues[i]);

    const float kEndLimit =
        kDefaultEndLimitFraction * [stack fannedStackLength];
    [stack setEndLimit:kEndLimit];
    [stack fanOutCardsWithStartIndex:0];

    EXPECT_EQ(kCardCount, [[stack cards] count]);
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_EQ(0, [stack lastStartStackCardIndex]);
    EXPECT_LT(0, [stack firstEndStackCardIndex]);
    for (NSInteger i = 0; i < [stack firstEndStackCardIndex]; i++) {
      StackCard* card = [[stack cards] objectAtIndex:i];
      EXPECT_FLOAT_EQ(kMargin + i * kMaxStagger,
                      LayoutOffset(stack, card.layout.position));
    }

    // After changing orientation, cards' layout offsets should be preserved on
    // the new layout axis.
    [stack setLayoutIsVertical:![stack layoutIsVertical]];
    EXPECT_EQ(kCardCount, [[stack cards] count]);
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_EQ(0, [stack lastStartStackCardIndex]);
    EXPECT_LT(0, [stack firstEndStackCardIndex]);
    for (NSInteger i = 0; i < [stack firstEndStackCardIndex]; i++) {
      StackCard* card = [[stack cards] objectAtIndex:i];
      EXPECT_FLOAT_EQ(kMargin + i * kMaxStagger,
                      LayoutOffset(stack, card.layout.position));
    }
  }
}
TEST_F(CardStackLayoutManagerTest, EndStackRecomputationOnEndLimitChange) {
  BOOL boolValues[2] = {NO, YES};
  for (unsigned long i = 0; i < arraysize(boolValues); i++) {
    const unsigned int kCardCount = 3;
    CardStackLayoutManager* stack = newStackOfNCards(kCardCount, boolValues[i]);

    CGFloat endLimit = [stack maximumStackLength];
    [stack setEndLimit:endLimit];
    [stack fanOutCardsWithStartIndex:0];

    EXPECT_EQ(kCardCount, [[stack cards] count]);
    ValidateCardPositioningConstraints(stack, endLimit, true);
    EXPECT_EQ(0, [stack lastStartStackCardIndex]);
    EXPECT_EQ((int)kCardCount, [stack firstEndStackCardIndex]);
    for (NSInteger i = 0; i < [stack firstEndStackCardIndex]; i++) {
      StackCard* card = [[stack cards] objectAtIndex:i];
      EXPECT_FLOAT_EQ(kMargin + i * kMaxStagger,
                      LayoutOffset(stack, card.layout.position));
    }

    // Setting smaller end limit should push third card into end stack.
    endLimit = 2 * kMaxStagger;
    [stack setEndLimit:endLimit];
    ValidateCardPositioningConstraints(stack, endLimit, true);
    EXPECT_EQ(2, [stack firstEndStackCardIndex]);

    // Making it smaller still should push second card into end stack.
    endLimit = kMaxStagger;
    [stack setEndLimit:endLimit];
    ValidateCardPositioningConstraints(stack, endLimit, true);
    EXPECT_EQ(1, [stack firstEndStackCardIndex]);

    // Making it larger again should re-fanout the end stack cards.
    endLimit = [stack maximumStackLength];
    [stack setEndLimit:endLimit];
    ValidateCardPositioningConstraints(stack, endLimit, true);
    EXPECT_EQ(0, [stack lastStartStackCardIndex]);
    EXPECT_EQ((int)kCardCount, [stack firstEndStackCardIndex]);
    for (NSInteger i = 0; i < [stack firstEndStackCardIndex]; i++) {
      StackCard* card = [[stack cards] objectAtIndex:i];
      EXPECT_FLOAT_EQ(kMargin + i * kMaxStagger,
                      LayoutOffset(stack, card.layout.position));
    }
  }
}

TEST_F(CardStackLayoutManagerTest, StackLayoutAtSpecificIndex) {
  BOOL boolValues[2] = {NO, YES};
  for (unsigned long i = 0; i < arraysize(boolValues); i++) {
    const unsigned int kCardCount = 30;
    CardStackLayoutManager* stack = newStackOfNCards(kCardCount, boolValues[i]);

    NSInteger startIndex = 10;

    const float kEndLimit =
        kDefaultEndLimitFraction * [stack fannedStackLength];
    [stack setEndLimit:kEndLimit];
    [stack fanOutCardsWithStartIndex:startIndex];

    EXPECT_EQ(kCardCount, [[stack cards] count]);
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_EQ(startIndex, [stack lastStartStackCardIndex]);
    // Take into account start stack when verifying position of card at
    // |startIndex|.
    StackCard* startCard = [[stack cards] objectAtIndex:startIndex];
    EXPECT_FLOAT_EQ(kMargin + [stack staggerOffsetForIndexFromEdge:startIndex],
                    LayoutOffset(stack, startCard.layout.position));
    NSInteger firstEndStackCardIndex = [stack firstEndStackCardIndex];
    for (NSInteger i = startIndex + 1; i < firstEndStackCardIndex; i++) {
      StackCard* card = [[stack cards] objectAtIndex:i];
      EXPECT_FLOAT_EQ(kMargin + (i - startIndex) * kMaxStagger,
                      LayoutOffset(stack, card.layout.position));
    }
  }
}

TEST_F(CardStackLayoutManagerTest, CardIsCovered) {
  BOOL boolValues[2] = {NO, YES};
  for (unsigned long i = 0; i < arraysize(boolValues); i++) {
    const unsigned int kCardCount = 3;
    CardStackLayoutManager* stack = newStackOfNCards(kCardCount, boolValues[i]);

    const float kEndLimit =
        kDefaultEndLimitFraction * [stack fannedStackLength];
    [stack setEndLimit:kEndLimit];
    [stack fanOutCardsWithStartIndex:0];

    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_EQ(0, [stack lastStartStackCardIndex]);
    EXPECT_EQ((int)kCardCount, [stack firstEndStackCardIndex]);
    // Since no cards are hidden in the start or end stack, all cards should
    // be visible (i.e., not covered).
    for (NSUInteger i = 0; i < kCardCount; i++) {
      StackCard* card = [[stack cards] objectAtIndex:i];
      EXPECT_FALSE([stack cardIsCovered:card]);
    }
    // Moving the second card to the same location as the third card should
    // result in the third card covering the second card.
    StackCard* secondCard = [[stack cards] objectAtIndex:1];
    StackCard* thirdCard = [[stack cards] objectAtIndex:2];
    secondCard.layout = thirdCard.layout;
    EXPECT_TRUE([stack cardIsCovered:secondCard]);
    EXPECT_FALSE([stack cardIsCovered:thirdCard]);
  }
}

TEST_F(CardStackLayoutManagerTest, CardIsCollapsed) {
  BOOL boolValues[2] = {NO, YES};
  for (unsigned long i = 0; i < arraysize(boolValues); i++) {
    const unsigned int kCardCount = 3;
    CardStackLayoutManager* stack = newStackOfNCards(kCardCount, boolValues[i]);

    const float kEndLimit =
        kDefaultEndLimitFraction * [stack fannedStackLength];
    [stack setEndLimit:kEndLimit];
    [stack fanOutCardsWithStartIndex:0];

    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_EQ(0, [stack lastStartStackCardIndex]);
    EXPECT_EQ((int)kCardCount, [stack firstEndStackCardIndex]);
    // Since the cards are fully fanned out, no cards should be collapsed.
    for (NSUInteger i = 0; i < kCardCount; i++) {
      StackCard* card = [[stack cards] objectAtIndex:i];
      EXPECT_FALSE([stack cardIsCollapsed:card]);
    }
    // Moving the second card to be |minStackStaggerAmount| away from the
    // third card should result in the second card being collapsed.
    StackCard* secondCard = [[stack cards] objectAtIndex:1];
    StackCard* thirdCard = [[stack cards] objectAtIndex:2];
    LayoutRect collapsedLayout = thirdCard.layout;
    if ([stack layoutIsVertical])
      collapsedLayout.position.originY -= [stack minStackStaggerAmount];
    else
      collapsedLayout.position.leading -= [stack minStackStaggerAmount];
    secondCard.layout = collapsedLayout;
    EXPECT_TRUE([stack cardIsCollapsed:secondCard]);
  }
}

TEST_F(CardStackLayoutManagerTest, BasicScroll) {
  BOOL boolValues[2] = {NO, YES};
  for (unsigned long i = 0; i < arraysize(boolValues); i++) {
    const unsigned int kCardCount = 3;
    CardStackLayoutManager* stack = newStackOfNCards(kCardCount, boolValues[i]);
    StackCard* firstCard = [[stack cards] objectAtIndex:0];

    const float kEndLimit =
        kDefaultEndLimitFraction * [stack fannedStackLength];
    [stack setEndLimit:kEndLimit];
    [stack fanOutCardsWithStartIndex:0];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_FLOAT_EQ(kMargin, LayoutOffset(stack, firstCard.layout.position));
    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 0, 1));
    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 1, 2));
    // Scrolling towards start stack should keep first card anchored, and move
    // the other two.
    [stack scrollCardAtIndex:kCardCount - 1
                     byDelta:-10
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_FLOAT_EQ(kMargin, LayoutOffset(stack, firstCard.layout.position));
    EXPECT_FLOAT_EQ(kMaxStagger - 10, SeparationOnLayoutAxis(stack, 0, 1));
    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 1, 2));
    // Scrolling back towards end stack should reverse the reverse scroll.
    [stack scrollCardAtIndex:kCardCount - 1
                     byDelta:10
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_FLOAT_EQ(kMargin, LayoutOffset(stack, firstCard.layout.position));
    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 0, 1));
    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 1, 2));
  }
}

TEST_F(CardStackLayoutManagerTest, ScrollCardAwayFromNeighbor) {
  BOOL boolValues[2] = {NO, YES};
  for (unsigned long i = 0; i < arraysize(boolValues); i++) {
    const unsigned int kCardCount = 3;
    CardStackLayoutManager* stack = newStackOfNCards(kCardCount, boolValues[i]);
    StackCard* firstCard = [[stack cards] objectAtIndex:0];

    // Configure the stack so that the first card is > the scroll-away distance
    // from the end stack, but the second card is not.
    const float kEndLimit =
        [stack scrollCardAwayFromNeighborAmount] + 2 * [stack maxStagger];
    [stack setEndLimit:kEndLimit];
    [stack fanOutCardsWithStartIndex:0];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_FLOAT_EQ(kMargin, LayoutOffset(stack, firstCard.layout.position));
    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 0, 1));
    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 1, 2));

    // Scrolling the third card away from the second card should result in it
    // being placed in the end stack.
    [stack scrollCardAtIndex:2 awayFromNeighbor:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, false);
    EXPECT_FLOAT_EQ(kMargin, LayoutOffset(stack, firstCard.layout.position));
    EXPECT_EQ(2, [stack firstEndStackCardIndex]);

    // Scrolling the second card away from the first card should result in it
    // being the min of |maxStagger + scrollAwayAmount, maximumCardSeparation|
    // away from the first card.
    [stack scrollCardAtIndex:1 awayFromNeighbor:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, false);
    EXPECT_FLOAT_EQ(kMargin, LayoutOffset(stack, firstCard.layout.position));
    CGFloat separation =
        std::min([stack maxStagger] + [stack scrollCardAwayFromNeighborAmount],
                 [stack maximumCardSeparation]);
    EXPECT_FLOAT_EQ(separation, SeparationOnLayoutAxis(stack, 0, 1));
    EXPECT_EQ(2, [stack firstEndStackCardIndex]);

    // Scrolling the second card away from the third card should result in it
    // being the min of |maxStagger + scrollAwayAmount, maximumCardSeparation|
    // away from the third card.
    separation = std::min([stack maximumCardSeparation],
                          SeparationOnLayoutAxis(stack, 1, 2) +
                              [stack scrollCardAwayFromNeighborAmount]);
    [stack scrollCardAtIndex:1 awayFromNeighbor:NO];
    ValidateCardPositioningConstraints(stack, kEndLimit, false);
    EXPECT_FLOAT_EQ(kMargin, LayoutOffset(stack, firstCard.layout.position));
    EXPECT_FLOAT_EQ(separation, SeparationOnLayoutAxis(stack, 1, 2));
    EXPECT_EQ(2, [stack firstEndStackCardIndex]);

    // Scrolling the third card away from the end stack should result in it
    // being |scrollAwayAmount| away from the endLimit and not being in the
    // end stack.
    StackCard* thirdCard = [[stack cards] objectAtIndex:2];
    separation =
        std::min([stack maximumCardSeparation],
                 kEndLimit - LayoutOffset(stack, thirdCard.layout.position) +
                     [stack scrollCardAwayFromNeighborAmount]);
    [stack scrollCardAtIndex:2 awayFromNeighbor:NO];
    ValidateCardPositioningConstraints(stack, kEndLimit, false);
    EXPECT_FLOAT_EQ(kMargin, LayoutOffset(stack, firstCard.layout.position));
    EXPECT_FLOAT_EQ(separation,
                    kEndLimit - LayoutOffset(stack, thirdCard.layout.position));
    EXPECT_EQ(3, [stack firstEndStackCardIndex]);
  }
}

TEST_F(CardStackLayoutManagerTest, ScrollNotScrollingLeadingCards) {
  BOOL boolValues[2] = {NO, YES};
  for (unsigned long i = 0; i < arraysize(boolValues); i++) {
    const unsigned int kCardCount = 4;
    CardStackLayoutManager* stack = newStackOfNCards(kCardCount, boolValues[i]);
    StackCard* firstCard = [[stack cards] objectAtIndex:0];

    // Make the stack large enough to fan out all its cards to avoid having to
    // worry about the end stack below.
    const float kEndLimit = [stack fannedStackLength];
    [stack setEndLimit:kEndLimit];
    [stack fanOutCardsWithStartIndex:0];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_FLOAT_EQ(kMargin, LayoutOffset(stack, firstCard.layout.position));
    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 0, 1));
    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 1, 2));
    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 2, 3));

    // Scrolling third card toward start stack without scrolling leading cards
    // should result in third and fourth cards scrolling, but second card not
    // scrolling.
    [stack scrollCardAtIndex:2
                     byDelta:-10
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:NO];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 0, 1));
    EXPECT_FLOAT_EQ(kMaxStagger - 10, SeparationOnLayoutAxis(stack, 1, 2));
    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 2, 3));

    // Doing the same toward the end stack should have the opposite effect.
    [stack fanOutCardsWithStartIndex:0];
    // First give the cards some room to scroll away from the start stack.
    [stack scrollCardAtIndex:3
                     byDelta:-10
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    [stack scrollCardAtIndex:2
                     byDelta:10
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:NO];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 0, 1));
    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 1, 2));
    EXPECT_FLOAT_EQ(kMaxStagger - 10, SeparationOnLayoutAxis(stack, 2, 3));
  }
}

TEST_F(CardStackLayoutManagerTest, ScrollCollapseExpansionOfLargeStack) {
  BOOL boolValues[2] = {NO, YES};
  for (unsigned long i = 0; i < arraysize(boolValues); i++) {
    const unsigned int kCardCount = 10;
    CardStackLayoutManager* stack = newStackOfNCards(kCardCount, boolValues[i]);

    const float kEndLimit =
        kDefaultEndLimitFraction * [stack fannedStackLength];
    [stack setEndLimit:kEndLimit];

    [stack fanOutCardsWithStartIndex:0];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_TRUE([stack stackIsFullyFannedOut]);
    EXPECT_FALSE([stack stackIsFullyCollapsed]);
    EXPECT_FALSE([stack stackIsFullyOverextended]);

    // Test fanning out/overextension toward end stack.
    [stack scrollCardAtIndex:0
                     byDelta:-10
        allowEarlyOverscroll:NO
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_FALSE([stack stackIsFullyFannedOut]);
    EXPECT_FALSE([stack stackIsFullyCollapsed]);
    EXPECT_FALSE([stack stackIsFullyOverextended]);

    [stack scrollCardAtIndex:0
                     byDelta:[stack maximumStackLength]
        allowEarlyOverscroll:NO
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_TRUE([stack stackIsFullyFannedOut]);
    EXPECT_FALSE([stack stackIsFullyCollapsed]);
    EXPECT_TRUE([stack stackIsFullyOverextended]);

    // Test collapsing/overextension toward start stack.
    [stack fanOutCardsWithStartIndex:0];
    [stack scrollCardAtIndex:0
                     byDelta:-2.0 * [stack maximumStackLength]
        allowEarlyOverscroll:NO
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_FALSE([stack stackIsFullyFannedOut]);
    EXPECT_TRUE([stack stackIsFullyCollapsed]);
    EXPECT_TRUE([stack stackIsFullyOverextended]);
  }
}

TEST_F(CardStackLayoutManagerTest, ScrollCollapseExpansionOfStackCornerCases) {
  BOOL boolValues[2] = {NO, YES};
  const unsigned int kCardCount = 1;
  for (unsigned long i = 0; i < arraysize(boolValues); i++) {
    CardStackLayoutManager* stack = newStackOfNCards(kCardCount, boolValues[i]);

    // A laid-out stack with one card is fully collapsed and fully fanned out,
    // but not fully overextended.
    [stack fanOutCardsWithStartIndex:0];
    EXPECT_TRUE([stack stackIsFullyFannedOut]);
    EXPECT_TRUE([stack stackIsFullyCollapsed]);
    EXPECT_FALSE([stack stackIsFullyOverextended]);

    // A stack with no cards is fully collapsed, fully fanned out, and fully
    // overextended.
    [stack removeCard:[[stack cards] objectAtIndex:0]];
    EXPECT_TRUE([stack stackIsFullyFannedOut]);
    EXPECT_TRUE([stack stackIsFullyCollapsed]);
    EXPECT_TRUE([stack stackIsFullyOverextended]);
  }
}

TEST_F(CardStackLayoutManagerTest, OneCardOverscroll) {
  BOOL boolValues[2] = {NO, YES};
  for (unsigned long i = 0; i < arraysize(boolValues); i++) {
    const unsigned int kCardCount = 1;
    const float kScrollAwayAmount = 20.0;
    CardStackLayoutManager* stack = newStackOfNCards(kCardCount, boolValues[i]);
    StackCard* firstCard = [[stack cards] objectAtIndex:0];

    const float kEndLimit =
        kDefaultEndLimitFraction * [stack fannedStackLength];
    [stack setEndLimit:kEndLimit];
    [stack fanOutCardsWithStartIndex:0];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_FLOAT_EQ(kMargin, LayoutOffset(stack, firstCard.layout.position));

    // Scrolling toward end should result in overscroll.
    [stack scrollCardAtIndex:0
                     byDelta:kScrollAwayAmount
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_TRUE([stack overextensionTowardEndOnFirstCard]);
    EXPECT_FALSE([stack overextensionTowardStartOnCardAtIndex:0]);
    EXPECT_FLOAT_EQ(kMargin + kScrollAwayAmount,
                    LayoutOffset(stack, firstCard.layout.position));

    // Eliminate the overscroll to test scrolling toward start.
    [stack eliminateOverextension];
    EXPECT_FALSE([stack overextensionTowardEndOnFirstCard]);
    EXPECT_FALSE([stack overextensionTowardStartOnCardAtIndex:0]);
    EXPECT_FLOAT_EQ(kMargin, LayoutOffset(stack, firstCard.layout.position));

    // Scrolling toward start should result in overscroll.
    [stack scrollCardAtIndex:0
                     byDelta:-kScrollAwayAmount
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_FALSE([stack overextensionTowardEndOnFirstCard]);
    EXPECT_TRUE([stack overextensionTowardStartOnCardAtIndex:0]);
    EXPECT_FLOAT_EQ(kMargin - kScrollAwayAmount,
                    LayoutOffset(stack, firstCard.layout.position));
  }
}

TEST_F(CardStackLayoutManagerTest, MaximumOverextensionAmount) {
  BOOL boolValues[2] = {NO, YES};
  for (unsigned long i = 0; i < arraysize(boolValues); i++) {
    const unsigned int kCardCount = 1;
    const float kScrollAwayAmount = 20.0;
    CardStackLayoutManager* stack = newStackOfNCards(kCardCount, boolValues[i]);
    StackCard* firstCard = [[stack cards] objectAtIndex:0];

    const float kEndLimit =
        kDefaultEndLimitFraction * [stack fannedStackLength];
    [stack setEndLimit:kEndLimit];
    [stack fanOutCardsWithStartIndex:0];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_FLOAT_EQ(kMargin, LayoutOffset(stack, firstCard.layout.position));

    // Scrolling toward start/end should have no impact.
    [stack setMaximumOverextensionAmount:0];
    [stack scrollCardAtIndex:0
                     byDelta:kScrollAwayAmount
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_FALSE([stack overextensionTowardEndOnFirstCard]);
    EXPECT_FALSE([stack overextensionTowardStartOnCardAtIndex:0]);
    EXPECT_FLOAT_EQ(kMargin, LayoutOffset(stack, firstCard.layout.position));
    EXPECT_FLOAT_EQ(0, [stack overextensionAmount]);

    [stack scrollCardAtIndex:0
                     byDelta:-kScrollAwayAmount
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_FALSE([stack overextensionTowardEndOnFirstCard]);
    EXPECT_FALSE([stack overextensionTowardStartOnCardAtIndex:0]);
    EXPECT_FLOAT_EQ(kMargin, LayoutOffset(stack, firstCard.layout.position));
    EXPECT_FLOAT_EQ(0, [stack overextensionAmount]);

    // Setting a maximum overextension amount > 0 should allow overscrolling to
    // that limit.
    CGFloat maxOverextensionAmount = kScrollAwayAmount / 2.0;
    [stack setMaximumOverextensionAmount:maxOverextensionAmount];
    [stack scrollCardAtIndex:0
                     byDelta:kScrollAwayAmount
        allowEarlyOverscroll:YES
           decayOnOverscroll:NO
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_TRUE([stack overextensionTowardEndOnFirstCard]);
    EXPECT_FALSE([stack overextensionTowardStartOnCardAtIndex:0]);
    EXPECT_FLOAT_EQ(kMargin + maxOverextensionAmount,
                    LayoutOffset(stack, firstCard.layout.position));
    EXPECT_FLOAT_EQ(maxOverextensionAmount, [stack overextensionAmount]);

    // Eliminate the overscroll to test scrolling toward start.
    [stack eliminateOverextension];

    [stack scrollCardAtIndex:0
                     byDelta:-kScrollAwayAmount
        allowEarlyOverscroll:YES
           decayOnOverscroll:NO
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_FALSE([stack overextensionTowardEndOnFirstCard]);
    EXPECT_TRUE([stack overextensionTowardStartOnCardAtIndex:0]);
    EXPECT_FLOAT_EQ(kMargin - maxOverextensionAmount,
                    LayoutOffset(stack, firstCard.layout.position));
    EXPECT_FLOAT_EQ(maxOverextensionAmount, [stack overextensionAmount]);
  }
}

TEST_F(CardStackLayoutManagerTest, DecayOnOverscroll) {
  BOOL boolValues[2] = {NO, YES};
  for (unsigned long i = 0; i < arraysize(boolValues); i++) {
    const unsigned int kCardCount = 1;
    const float kScrollAwayAmount = 10.0;
    CardStackLayoutManager* stack = newStackOfNCards(kCardCount, boolValues[i]);
    StackCard* firstCard = [[stack cards] objectAtIndex:0];

    const float kEndLimit =
        kDefaultEndLimitFraction * [stack fannedStackLength];
    [stack setEndLimit:kEndLimit];
    [stack fanOutCardsWithStartIndex:0];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_FLOAT_EQ(kMargin, LayoutOffset(stack, firstCard.layout.position));

    // Scrolling toward end by |kScrollAwayAmount| should result in overscroll.
    [stack scrollCardAtIndex:0
                     byDelta:kScrollAwayAmount
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_TRUE([stack overextensionTowardEndOnFirstCard]);
    EXPECT_FALSE([stack overextensionTowardStartOnCardAtIndex:0]);
    EXPECT_FLOAT_EQ(kMargin + kScrollAwayAmount,
                    LayoutOffset(stack, firstCard.layout.position));

    // Scrolling again by |kScrollAwayAmount| with no decay on overscroll
    // should result in another move of |kScrollAwayAmount|.
    [stack scrollCardAtIndex:0
                     byDelta:kScrollAwayAmount
        allowEarlyOverscroll:YES
           decayOnOverscroll:NO
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_TRUE([stack overextensionTowardEndOnFirstCard]);
    EXPECT_FALSE([stack overextensionTowardStartOnCardAtIndex:0]);
    EXPECT_FLOAT_EQ(kMargin + 2 * kScrollAwayAmount,
                    LayoutOffset(stack, firstCard.layout.position));

    // Scrolling by |kScrollAwayAmount| a third time *with* decay on overscroll
    // should result in a move of less than |kScrollAwayAmount|.
    [stack scrollCardAtIndex:0
                     byDelta:kScrollAwayAmount
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_TRUE([stack overextensionTowardEndOnFirstCard]);
    EXPECT_FALSE([stack overextensionTowardStartOnCardAtIndex:0]);
    EXPECT_GT(kMargin + 3 * kScrollAwayAmount,
              LayoutOffset(stack, firstCard.layout.position));
  }
}

TEST_F(CardStackLayoutManagerTest, EliminateOverextension) {
  BOOL boolValues[2] = {NO, YES};
  for (unsigned long i = 0; i < arraysize(boolValues); i++) {
    const unsigned int kCardCount = 2;
    const float kScrollAwayAmount = 20.0;
    CardStackLayoutManager* stack = newStackOfNCards(kCardCount, boolValues[i]);
    StackCard* firstCard = [[stack cards] objectAtIndex:0];
    StackCard* secondCard = [[stack cards] objectAtIndex:1];

    const float kEndLimit =
        kDefaultEndLimitFraction * [stack fannedStackLength];
    [stack setEndLimit:kEndLimit];
    [stack fanOutCardsWithStartIndex:0];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_FLOAT_EQ(kMargin, LayoutOffset(stack, firstCard.layout.position));

    CGFloat firstCardInitialOrigin =
        LayoutOffset(stack, firstCard.layout.position);
    CGFloat secondCardInitialOrigin =
        LayoutOffset(stack, secondCard.layout.position);

    // Scrolling toward end should result in overscroll.
    [stack scrollCardAtIndex:0
                     byDelta:kScrollAwayAmount
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_TRUE([stack overextensionTowardEndOnFirstCard]);
    EXPECT_FALSE([stack overextensionTowardStartOnCardAtIndex:0]);
    EXPECT_FLOAT_EQ(firstCardInitialOrigin + kScrollAwayAmount,
                    LayoutOffset(stack, firstCard.layout.position));
    EXPECT_FLOAT_EQ(secondCardInitialOrigin + kScrollAwayAmount,
                    LayoutOffset(stack, secondCard.layout.position));

    // Calling |eliminateOverextension| should undo the overscroll on the first
    // and second card.
    [stack eliminateOverextension];
    EXPECT_FLOAT_EQ(firstCardInitialOrigin,
                    LayoutOffset(stack, firstCard.layout.position));
    EXPECT_FLOAT_EQ(secondCardInitialOrigin,
                    LayoutOffset(stack, secondCard.layout.position));

    // Scrolling toward start should result in overscroll.
    [stack scrollCardAtIndex:0
                     byDelta:-kScrollAwayAmount
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_FALSE([stack overextensionTowardEndOnFirstCard]);
    EXPECT_TRUE([stack overextensionTowardStartOnCardAtIndex:0]);
    EXPECT_FLOAT_EQ(firstCardInitialOrigin - kScrollAwayAmount,
                    LayoutOffset(stack, firstCard.layout.position));
    EXPECT_FLOAT_EQ(secondCardInitialOrigin - kScrollAwayAmount,
                    LayoutOffset(stack, secondCard.layout.position));

    // Calling |eliminateOverextension| should undo the overscroll on the first
    // card but leave the second card as-is, since it's not overscrolled.
    [stack eliminateOverextension];
    EXPECT_FLOAT_EQ(firstCardInitialOrigin,
                    LayoutOffset(stack, firstCard.layout.position));
    EXPECT_FLOAT_EQ(secondCardInitialOrigin - kScrollAwayAmount,
                    LayoutOffset(stack, secondCard.layout.position));

    // Reset state to test pinch.
    [stack fanOutCardsWithStartIndex:0];

    // Pinching first card toward end should result in it being overextended.
    [stack handleMultitouchWithFirstDelta:kScrollAwayAmount
                              secondDelta:kScrollAwayAmount
                           firstCardIndex:0
                          secondCardIndex:1
                         decayOnOverpinch:YES];
    EXPECT_FLOAT_EQ(firstCardInitialOrigin + kScrollAwayAmount,
                    LayoutOffset(stack, firstCard.layout.position));
    EXPECT_FLOAT_EQ(secondCardInitialOrigin + kScrollAwayAmount,
                    LayoutOffset(stack, secondCard.layout.position));
    EXPECT_TRUE([stack overextensionTowardEndOnFirstCard]);
    EXPECT_FALSE([stack overextensionTowardStartOnCardAtIndex:0]);

    // ELiminating the overextension, which is now an overpinch, should restore
    // the first card to its initial position but not alter the offset of the
    // second card.
    [stack eliminateOverextension];
    EXPECT_FLOAT_EQ(firstCardInitialOrigin,
                    LayoutOffset(stack, firstCard.layout.position));
    EXPECT_FLOAT_EQ(secondCardInitialOrigin + kScrollAwayAmount,
                    LayoutOffset(stack, secondCard.layout.position));
  }
}

TEST_F(CardStackLayoutManagerTest, MultiCardOverscroll) {
  BOOL boolValues[2] = {NO, YES};
  for (unsigned long i = 0; i < arraysize(boolValues); i++) {
    const unsigned int kCardCount = 3;
    const float kScrollAwayAmount = 100.0;
    CardStackLayoutManager* stack = newStackOfNCards(kCardCount, boolValues[i]);
    StackCard* firstCard = [[stack cards] objectAtIndex:0];

    const float kEndLimit =
        kDefaultEndLimitFraction * [stack fannedStackLength];
    [stack setEndLimit:kEndLimit];
    [stack fanOutCardsWithStartIndex:0];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_FLOAT_EQ(kMargin, LayoutOffset(stack, firstCard.layout.position));
    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 0, 1));
    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 1, 2));
    // Scrolling away from the start stack should result in overscroll.
    [stack scrollCardAtIndex:0
                     byDelta:kScrollAwayAmount
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_TRUE([stack overextensionTowardEndOnFirstCard]);
    EXPECT_FALSE([stack overextensionTowardStartOnCardAtIndex:0]);
    EXPECT_FLOAT_EQ(kMargin + kScrollAwayAmount,
                    LayoutOffset(stack, firstCard.layout.position));

    // Calling |eliminateOverextension| should restore the stack to its previous
    // fanned-out state.
    [stack eliminateOverextension];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_FLOAT_EQ(kMargin, LayoutOffset(stack, firstCard.layout.position));
    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 0, 1));
    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 1, 2));

    // Scrolling toward the start more than is necessary to fully collapse the
    // stack should result in overscroll toward the start.
    CGFloat lastCardCollapsedPosition =
        kMargin + [stack staggerOffsetForIndexFromEdge:kCardCount - 1];
    StackCard* lastCard = [[stack cards] objectAtIndex:kCardCount - 1];
    CGFloat distanceToCollapsedStack =
        lastCardCollapsedPosition -
        LayoutOffset(stack, lastCard.layout.position);
    [stack scrollCardAtIndex:kCardCount - 1
                     byDelta:distanceToCollapsedStack - 10
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_FALSE([stack overextensionTowardEndOnFirstCard]);
    EXPECT_TRUE([stack overextensionTowardStartOnCardAtIndex:0]);
    EXPECT_FLOAT_EQ(kMargin - 10,
                    LayoutOffset(stack, firstCard.layout.position));
  }
}

TEST_F(CardStackLayoutManagerTest, Fling) {
  BOOL boolValues[2] = {NO, YES};
  for (unsigned long i = 0; i < arraysize(boolValues); i++) {
    const unsigned int kCardCount = 3;
    CardStackLayoutManager* stack = newStackOfNCards(kCardCount, boolValues[i]);
    StackCard* firstCard = [[stack cards] objectAtIndex:0];

    const float kEndLimit =
        kDefaultEndLimitFraction * [stack fannedStackLength];
    [stack setEndLimit:kEndLimit];
    [stack fanOutCardsWithStartIndex:0];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_FLOAT_EQ(kMargin, LayoutOffset(stack, firstCard.layout.position));
    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 0, 1));
    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 1, 2));

    // Flinging on the first card should not result in overscroll...
    [stack scrollCardAtIndex:0
                     byDelta:-20.0
        allowEarlyOverscroll:NO
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_FALSE([stack overextensionTowardStartOnCardAtIndex:0]);
    EXPECT_FALSE([stack overextensionTowardEndOnFirstCard]);
    // ... until the last card becomes overscrolled.
    [stack scrollCardAtIndex:0
                     byDelta:-[stack maximumStackLength]
        allowEarlyOverscroll:NO
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_TRUE([stack overextensionTowardStartOnCardAtIndex:0]);
    EXPECT_FALSE([stack overextensionTowardEndOnFirstCard]);
  }
}

TEST_F(CardStackLayoutManagerTest, ScrollAroundStartStack) {
  BOOL boolValues[2] = {NO, YES};
  for (unsigned long i = 0; i < arraysize(boolValues); i++) {
    const unsigned int kCardCount = 3;
    CardStackLayoutManager* stack = newStackOfNCards(kCardCount, boolValues[i]);

    const float kEndLimit =
        kDefaultEndLimitFraction * [stack fannedStackLength];
    [stack setEndLimit:kEndLimit];
    [stack fanOutCardsWithStartIndex:0];

    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 0, 1));
    EXPECT_EQ(0, [stack lastStartStackCardIndex]);
    ValidateCardPositioningConstraints(stack, kEndLimit, true);

    // Scroll second card into start stack.
    CGFloat cardTwoStartStackOffset =
        kMargin + [stack staggerOffsetForIndexFromEdge:1];
    LayoutRectPosition cardTwoPosition =
        ((StackCard*)[[stack cards] objectAtIndex:1]).layout.position;
    [stack scrollCardAtIndex:1
                     byDelta:cardTwoStartStackOffset -
                             LayoutOffset(stack, cardTwoPosition)
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_EQ(1, [stack lastStartStackCardIndex]);
    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 1, 2));

    // Scroll third card toward start stack, and check that everything is as
    // expected.
    [stack scrollCardAtIndex:2
                     byDelta:kMaxStagger / -2.0
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_EQ(1, [stack lastStartStackCardIndex]);
    EXPECT_FLOAT_EQ(kMaxStagger / 2.0, SeparationOnLayoutAxis(stack, 1, 2));

    // Scroll third card away from stack stack, and check that second card
    // doesn't come out before it should.
    [stack scrollCardAtIndex:2
                     byDelta:kMaxStagger / 4.0
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_EQ(1, [stack lastStartStackCardIndex]);
    EXPECT_FLOAT_EQ(kMaxStagger * .75, SeparationOnLayoutAxis(stack, 1, 2));
    [stack scrollCardAtIndex:2
                     byDelta:kMaxStagger / 4.0 + 1
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_EQ(0, [stack lastStartStackCardIndex]);
    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 1, 2));
  }
}

TEST_F(CardStackLayoutManagerTest, ScrollAroundEndStack) {
  BOOL boolValues[2] = {NO, YES};
  for (unsigned long i = 0; i < arraysize(boolValues); i++) {
    const unsigned int kCardCount = 7;
    CardStackLayoutManager* stack = newStackOfNCards(kCardCount, boolValues[i]);

    const float kEndLimit = 0.2 * [stack fannedStackLength];
    [stack setEndLimit:kEndLimit];
    [stack fanOutCardsWithStartIndex:4];

    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 5, 6));
    EXPECT_EQ(4, [stack lastStartStackCardIndex]);
    EXPECT_EQ(7, [stack firstEndStackCardIndex]);
    ValidateCardPositioningConstraints(stack, kEndLimit, true);

    // Scroll seventh card into end stack.
    CGFloat cardSevenEndStackOffset =
        kEndLimit - ([stack staggerOffsetForIndexFromEdge:0] +
                     [stack minStackStaggerAmount]);
    LayoutRectPosition cardSevenPosition =
        ((StackCard*)[[stack cards] objectAtIndex:6]).layout.position;
    [stack scrollCardAtIndex:6
                     byDelta:cardSevenEndStackOffset -
                             LayoutOffset(stack, cardSevenPosition)
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_EQ([stack firstEndStackCardIndex], 6);
    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 5, 6));

    // Scroll sixth card toward end stack, and check that everything is as
    // expected.
    [stack scrollCardAtIndex:5
                     byDelta:kMaxStagger / 2.0
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_EQ([stack firstEndStackCardIndex], 6);
    EXPECT_FLOAT_EQ(kMaxStagger / 2.0, SeparationOnLayoutAxis(stack, 5, 6));

    // Scroll sixth card away from end stack, and check that seventh card
    // doesn't come out before it should.
    [stack scrollCardAtIndex:5
                     byDelta:kMaxStagger / -4.0
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_EQ([stack firstEndStackCardIndex], 6);
    EXPECT_FLOAT_EQ(SeparationOnLayoutAxis(stack, 5, 6), kMaxStagger * .75);
    [stack scrollCardAtIndex:5
                     byDelta:kMaxStagger / -4.0 - 1
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_EQ([stack firstEndStackCardIndex], 7);
    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 5, 6));
  }
}

TEST_F(CardStackLayoutManagerTest, BasicMultitouch) {
  BOOL boolValues[2] = {NO, YES};
  for (unsigned long i = 0; i < arraysize(boolValues); i++) {
    const unsigned int kCardCount = 3;
    CardStackLayoutManager* stack = newStackOfNCards(kCardCount, boolValues[i]);

    const float kEndLimit =
        kDefaultEndLimitFraction * [stack fannedStackLength];
    [stack setEndLimit:kEndLimit];
    [stack fanOutCardsWithStartIndex:0];

    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 0, 1));
    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 1, 2));

    [stack handleMultitouchWithFirstDelta:-10
                              secondDelta:50
                           firstCardIndex:1
                          secondCardIndex:2
                         decayOnOverpinch:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, false);
    EXPECT_FLOAT_EQ(kMaxStagger - 10, SeparationOnLayoutAxis(stack, 0, 1));
    EXPECT_FLOAT_EQ(kMaxStagger + 60, SeparationOnLayoutAxis(stack, 1, 2));
  }
}

TEST_F(CardStackLayoutManagerTest, MultitouchBoundedByNeighbor) {
  BOOL boolValues[2] = {NO, YES};
  for (unsigned long i = 0; i < arraysize(boolValues); i++) {
    const unsigned int kCardCount = 3;
    CardStackLayoutManager* stack = newStackOfNCards(kCardCount, boolValues[i]);

    // Make sure that the stack end limit isn't hit in this test.
    const float kEndLimit = 2.0 * [stack maximumStackLength];
    [stack setEndLimit:kEndLimit];
    [stack fanOutCardsWithStartIndex:0];

    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 0, 1));
    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 1, 2));

    CGFloat cardSize = (i > 0) ? kCardHeight : kCardWidth;

    // Verify that it's not possible to pinch the third card entirely off the
    // second card.
    [stack handleMultitouchWithFirstDelta:0
                              secondDelta:2.0 * cardSize
                           firstCardIndex:1
                          secondCardIndex:2
                         decayOnOverpinch:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, false);
    // Verify that it's not possible to pinch the second card entirely off the
    // third card.
    [stack handleMultitouchWithFirstDelta:-2.0 * cardSize
                              secondDelta:0
                           firstCardIndex:1
                          secondCardIndex:2
                         decayOnOverpinch:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, false);
    // Verify that it's not possible to pinch the two cards off each other.
    [stack handleMultitouchWithFirstDelta:-cardSize
                              secondDelta:cardSize
                           firstCardIndex:1
                          secondCardIndex:2
                         decayOnOverpinch:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, false);
  }
}

TEST_F(CardStackLayoutManagerTest, OverpinchTowardStart) {
  BOOL boolValues[2] = {NO, YES};
  for (unsigned long i = 0; i < arraysize(boolValues); i++) {
    const unsigned int kCardCount = 2;
    CardStackLayoutManager* stack = newStackOfNCards(kCardCount, boolValues[i]);

    const float kEndLimit =
        kDefaultEndLimitFraction * [stack fannedStackLength];
    [stack setEndLimit:kEndLimit];
    [stack fanOutCardsWithStartIndex:0];

    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    StackCard* firstCard = [[stack cards] objectAtIndex:0];
    LayoutRectPosition firstCardPosition = firstCard.layout.position;
    StackCard* secondCard = [[stack cards] objectAtIndex:1];
    LayoutRectPosition secondCardPosition = secondCard.layout.position;

    [stack handleMultitouchWithFirstDelta:-20
                              secondDelta:10
                           firstCardIndex:0
                          secondCardIndex:1
                         decayOnOverpinch:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, false);
    // First card should have been overpinched.
    EXPECT_TRUE([stack overextensionTowardStartOnCardAtIndex:0]);
    EXPECT_FALSE([stack overextensionTowardEndOnFirstCard]);
    EXPECT_FLOAT_EQ(LayoutOffset(stack, firstCardPosition) - 20,
                    LayoutOffset(stack, firstCard.layout.position));
    EXPECT_FLOAT_EQ(LayoutOffset(stack, secondCardPosition) + 10,
                    LayoutOffset(stack, secondCard.layout.position));
  }
}

TEST_F(CardStackLayoutManagerTest, OverpinchTowardEnd) {
  BOOL boolValues[2] = {NO, YES};
  for (unsigned long i = 0; i < arraysize(boolValues); i++) {
    const unsigned int kCardCount = 2;
    CardStackLayoutManager* stack = newStackOfNCards(kCardCount, boolValues[i]);

    const float kEndLimit =
        kDefaultEndLimitFraction * [stack fannedStackLength];
    [stack setEndLimit:kEndLimit];
    [stack fanOutCardsWithStartIndex:0];

    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    StackCard* firstCard = [[stack cards] objectAtIndex:0];
    LayoutRectPosition firstCardPosition = firstCard.layout.position;
    StackCard* secondCard = [[stack cards] objectAtIndex:1];
    LayoutRectPosition secondCardPosition = secondCard.layout.position;

    [stack handleMultitouchWithFirstDelta:20
                              secondDelta:10
                           firstCardIndex:0
                          secondCardIndex:1
                         decayOnOverpinch:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, false);
    // Both first and second card should have moved.
    EXPECT_FLOAT_EQ(LayoutOffset(stack, firstCardPosition) + 20,
                    LayoutOffset(stack, firstCard.layout.position));
    EXPECT_FLOAT_EQ(LayoutOffset(stack, secondCardPosition) + 10,
                    LayoutOffset(stack, secondCard.layout.position));
  }
}

TEST_F(CardStackLayoutManagerTest, StressMultitouch) {
  BOOL boolValues[2] = {NO, YES};
  for (unsigned long i = 0; i < arraysize(boolValues); i++) {
    const unsigned int kCardCount = 30;
    CardStackLayoutManager* stack = newStackOfNCards(kCardCount, boolValues[i]);

    const float kEndLimit =
        kDefaultEndLimitFraction * [stack fannedStackLength];
    [stack setEndLimit:kEndLimit];
    [stack fanOutCardsWithStartIndex:0];

    ValidateCardPositioningConstraints(stack, kEndLimit, true);

    [stack handleMultitouchWithFirstDelta:-10
                              secondDelta:50
                           firstCardIndex:5
                          secondCardIndex:10
                         decayOnOverpinch:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, false);

    [stack handleMultitouchWithFirstDelta:20
                              secondDelta:-10
                           firstCardIndex:3
                          secondCardIndex:15
                         decayOnOverpinch:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, false);

    [stack handleMultitouchWithFirstDelta:-20
                              secondDelta:-10
                           firstCardIndex:0
                          secondCardIndex:4
                         decayOnOverpinch:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, false);
  }
}

TEST_F(CardStackLayoutManagerTest, ScrollAfterMultitouch) {
  BOOL boolValues[2] = {NO, YES};
  for (unsigned long i = 0; i < arraysize(boolValues); i++) {
    const unsigned int kCardCount = 3;
    CardStackLayoutManager* stack = newStackOfNCards(kCardCount, boolValues[i]);

    const float kEndLimit =
        kDefaultEndLimitFraction * [stack fannedStackLength];
    const float kPinchDistance = 50;
    [stack setEndLimit:kEndLimit];
    [stack fanOutCardsWithStartIndex:0];

    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 1, 2));

    [stack handleMultitouchWithFirstDelta:0
                              secondDelta:kPinchDistance
                           firstCardIndex:1
                          secondCardIndex:2
                         decayOnOverpinch:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, false);
    EXPECT_FLOAT_EQ(kMaxStagger + kPinchDistance,
                    SeparationOnLayoutAxis(stack, 1, 2));

    [stack scrollCardAtIndex:2
                     byDelta:10
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, false);
    // Separation between cards should be maintained.
    EXPECT_FLOAT_EQ(kMaxStagger + kPinchDistance,
                    SeparationOnLayoutAxis(stack, 1, 2));
    [stack scrollCardAtIndex:2
                     byDelta:-20
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, false);
    // Separation between cards should be maintained.
    EXPECT_FLOAT_EQ(kMaxStagger + kPinchDistance,
                    SeparationOnLayoutAxis(stack, 1, 2));
  }
}

TEST_F(CardStackLayoutManagerTest, ScrollEveningOutAfterMultitouch) {
  BOOL boolValues[2] = {NO, YES};
  for (unsigned long i = 0; i < arraysize(boolValues); i++) {
    const unsigned int kCardCount = 3;
    CardStackLayoutManager* stack = newStackOfNCards(kCardCount, boolValues[i]);

    const float kEndLimit =
        kDefaultEndLimitFraction * [stack fannedStackLength];
    const float kPinchDistance = kMaxStagger / 2.0;
    [stack setEndLimit:kEndLimit];
    [stack fanOutCardsWithStartIndex:0];

    ValidateCardPositioningConstraints(stack, kEndLimit, true);
    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 1, 2));

    // Scroll cards toward start stack to give some room to scroll second card
    // toward end stack (see below).
    [stack scrollCardAtIndex:1
                     byDelta:-10
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, false);
    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 1, 2));

    // Pinch the third card closer to the second card.
    [stack handleMultitouchWithFirstDelta:0
                              secondDelta:-kPinchDistance
                           firstCardIndex:1
                          secondCardIndex:2
                         decayOnOverpinch:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, false);
    EXPECT_FLOAT_EQ(kMaxStagger - kPinchDistance,
                    SeparationOnLayoutAxis(stack, 1, 2));

    // Separation between cards should be maintained when second card is
    // scrolled towards third card.
    [stack scrollCardAtIndex:1
                     byDelta:10
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, false);
    EXPECT_FLOAT_EQ(kMaxStagger - kPinchDistance,
                    SeparationOnLayoutAxis(stack, 1, 2));

    // Scrolling second card away from third card by the distance that the
    // third card was pinched should restore separation of |kMaxStagger|
    // between the second and third card.
    [stack scrollCardAtIndex:1
                     byDelta:-kPinchDistance
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, false);
    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 1, 2));
  }
}

TEST_F(CardStackLayoutManagerTest, ScrollAroundStartStackAfterMultitouch) {
  BOOL boolValues[2] = {NO, YES};
  for (unsigned long i = 0; i < arraysize(boolValues); i++) {
    const unsigned int kCardCount = 3;
    CardStackLayoutManager* stack = newStackOfNCards(kCardCount, boolValues[i]);

    const float kEndLimit =
        kDefaultEndLimitFraction * [stack fannedStackLength];
    const float kPinchDistance = 50;
    [stack setEndLimit:kEndLimit];
    [stack fanOutCardsWithStartIndex:0];

    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 0, 1));
    ValidateCardPositioningConstraints(stack, kEndLimit, true);

    [stack handleMultitouchWithFirstDelta:0
                              secondDelta:kPinchDistance
                           firstCardIndex:1
                          secondCardIndex:2
                         decayOnOverpinch:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, false);
    EXPECT_FLOAT_EQ(kMaxStagger + kPinchDistance,
                    SeparationOnLayoutAxis(stack, 1, 2));

    [stack scrollCardAtIndex:2
                     byDelta:-20
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, false);
    // Separation between cards should be maintained.
    EXPECT_FLOAT_EQ(kMaxStagger + kPinchDistance,
                    SeparationOnLayoutAxis(stack, 1, 2));

    // Scroll the cards completely into the start stack.
    CGFloat lastCardCollapsedPosition =
        kMargin + [stack staggerOffsetForIndexFromEdge:kCardCount - 1];
    StackCard* lastCard = [[stack cards] objectAtIndex:kCardCount - 1];
    CGFloat distanceToCollapsedStack =
        lastCardCollapsedPosition -
        LayoutOffset(stack, lastCard.layout.position);
    [stack scrollCardAtIndex:2
                     byDelta:distanceToCollapsedStack
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    EXPECT_EQ((NSInteger)(kCardCount - 1), [stack lastStartStackCardIndex]);
    // Scroll the cards out of the start stack: they should now be separated by
    // |kMaxStagger|.
    [stack scrollCardAtIndex:2
                     byDelta:2 * kMaxStagger
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 1, 2));
  }
}

TEST_F(CardStackLayoutManagerTest, ScrollAroundEndStackAfterMultitouch) {
  BOOL boolValues[2] = {NO, YES};
  for (unsigned long i = 0; i < arraysize(boolValues); i++) {
    const unsigned int kCardCount = 7;
    CardStackLayoutManager* stack = newStackOfNCards(kCardCount, boolValues[i]);

    const float kEndLimit = 0.3 * [stack fannedStackLength];
    const float kPinchDistance = 20;
    [stack setEndLimit:kEndLimit];
    // Start in the middle of the stack to be able to scroll cards into the end
    // stack.
    [stack fanOutCardsWithStartIndex:4];

    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 5, 6));
    ValidateCardPositioningConstraints(stack, kEndLimit, true);

    [stack handleMultitouchWithFirstDelta:0
                              secondDelta:kPinchDistance
                           firstCardIndex:5
                          secondCardIndex:6
                         decayOnOverpinch:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, false);
    EXPECT_FLOAT_EQ(kMaxStagger + kPinchDistance,
                    SeparationOnLayoutAxis(stack, 5, 6));

    [stack scrollCardAtIndex:5
                     byDelta:-10
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, false);
    // Separation between cards should be maintained.
    EXPECT_FLOAT_EQ(kMaxStagger + kPinchDistance,
                    SeparationOnLayoutAxis(stack, 5, 6));
    // Scroll the two cards in question into the end stack.
    CGFloat cardSixEndStackPosition =
        kEndLimit - [stack staggerOffsetForIndexFromEdge:1];
    LayoutRectPosition cardSixPosition =
        ((StackCard*)[[stack cards] objectAtIndex:5]).layout.position;
    [stack scrollCardAtIndex:5
                     byDelta:cardSixEndStackPosition -
                             LayoutOffset(stack, cardSixPosition)
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    cardSixPosition =
        ((StackCard*)[[stack cards] objectAtIndex:5]).layout.position;
    ValidateCardPositioningConstraints(stack, kEndLimit, false);
    EXPECT_EQ(5, [stack firstEndStackCardIndex]);
    // Scroll the cards out of the end stack: cards should now be
    // separated by |kMaxStagger|.
    [stack scrollCardAtIndex:5
                     byDelta:-2.0 * kMaxStagger
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    EXPECT_FLOAT_EQ(SeparationOnLayoutAxis(stack, 5, 6), kMaxStagger);
  }
}

TEST_F(CardStackLayoutManagerTest, ScrollAfterPinchOutOfStartStack) {
  BOOL boolValues[2] = {NO, YES};
  for (unsigned long i = 0; i < arraysize(boolValues); i++) {
    const unsigned int kCardCount = 3;
    CardStackLayoutManager* stack = newStackOfNCards(kCardCount, boolValues[i]);

    const float kEndLimit =
        kDefaultEndLimitFraction * [stack fannedStackLength];
    const float kPinchDistance = 50;
    [stack setEndLimit:kEndLimit];
    [stack fanOutCardsWithStartIndex:0];

    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 0, 1));
    ValidateCardPositioningConstraints(stack, kEndLimit, true);

    [stack handleMultitouchWithFirstDelta:0
                              secondDelta:kPinchDistance
                           firstCardIndex:1
                          secondCardIndex:2
                         decayOnOverpinch:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, false);
    EXPECT_FLOAT_EQ(kMaxStagger + kPinchDistance,
                    SeparationOnLayoutAxis(stack, 1, 2));

    [stack scrollCardAtIndex:1
                     byDelta:-20
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, false);
    // Separation between cards should be maintained.
    EXPECT_FLOAT_EQ(kMaxStagger + kPinchDistance,
                    SeparationOnLayoutAxis(stack, 1, 2));
    // Scroll the cards completely into the start stack.
    CGFloat lastCardCollapsedPosition =
        kMargin + [stack staggerOffsetForIndexFromEdge:kCardCount - 1];
    StackCard* lastCard = [[stack cards] objectAtIndex:kCardCount - 1];
    CGFloat distanceToCollapsedStack =
        lastCardCollapsedPosition -
        LayoutOffset(stack, lastCard.layout.position);
    [stack scrollCardAtIndex:kCardCount - 1
                     byDelta:distanceToCollapsedStack
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    EXPECT_EQ((NSInteger)(kCardCount - 1), [stack lastStartStackCardIndex]);
    CGFloat inStackSeparation = SeparationOnLayoutAxis(stack, 1, 2);
    // Pinch the third card far out of the start stack.
    [stack handleMultitouchWithFirstDelta:0
                              secondDelta:kMaxStagger
                           firstCardIndex:1
                          secondCardIndex:2
                         decayOnOverpinch:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, false);
    EXPECT_FLOAT_EQ(inStackSeparation + kMaxStagger,
                    SeparationOnLayoutAxis(stack, 1, 2));
    // A scroll should immediately bring the second card out of the start
    // stack, without affecting the distance between the second and third cards.
    [stack scrollCardAtIndex:2
                     byDelta:kMaxStagger / 2.0
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, false);
    EXPECT_FLOAT_EQ(inStackSeparation + kMaxStagger,
                    SeparationOnLayoutAxis(stack, 1, 2));
  }
}

TEST_F(CardStackLayoutManagerTest, ScrollAfterPinchOutOfEndStack) {
  BOOL boolValues[2] = {NO, YES};
  for (unsigned long i = 0; i < arraysize(boolValues); i++) {
    const unsigned int kCardCount = 7;
    CardStackLayoutManager* stack = newStackOfNCards(kCardCount, boolValues[i]);

    const float kEndLimit = 0.3 * [stack fannedStackLength];
    const float kPinchDistance = 20;
    [stack setEndLimit:kEndLimit];
    // Start in the middle of the stack to be able to scroll cards into the end
    // stack.
    [stack fanOutCardsWithStartIndex:4];

    EXPECT_FLOAT_EQ(kMaxStagger, SeparationOnLayoutAxis(stack, 5, 6));
    ValidateCardPositioningConstraints(stack, kEndLimit, true);

    [stack handleMultitouchWithFirstDelta:0
                              secondDelta:kPinchDistance
                           firstCardIndex:5
                          secondCardIndex:6
                         decayOnOverpinch:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, false);
    EXPECT_FLOAT_EQ(kMaxStagger + kPinchDistance,
                    SeparationOnLayoutAxis(stack, 5, 6));

    [stack scrollCardAtIndex:5
                     byDelta:-10
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, false);
    // Separation between cards should be maintained.
    EXPECT_FLOAT_EQ(kMaxStagger + kPinchDistance,
                    SeparationOnLayoutAxis(stack, 5, 6));

    // Scroll the two cards in question into the end stack.
    CGFloat cardSixEndStackOffset =
        kEndLimit - ([stack staggerOffsetForIndexFromEdge:1] +
                     [stack minStackStaggerAmount]);
    LayoutRectPosition cardSixPosition =
        ((StackCard*)[[stack cards] objectAtIndex:5]).layout.position;
    [stack scrollCardAtIndex:5
                     byDelta:cardSixEndStackOffset -
                             LayoutOffset(stack, cardSixPosition)
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    cardSixPosition =
        ((StackCard*)[[stack cards] objectAtIndex:5]).layout.position;
    ValidateCardPositioningConstraints(stack, kEndLimit, false);
    EXPECT_EQ(5, [stack firstEndStackCardIndex]);
    CGFloat inStackSeparation = SeparationOnLayoutAxis(stack, 5, 6);
    // Pinch the sixth card far out of the start stack.
    [stack handleMultitouchWithFirstDelta:-2.0 * kMaxStagger
                              secondDelta:0
                           firstCardIndex:5
                          secondCardIndex:6
                         decayOnOverpinch:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, false);
    EXPECT_EQ(6, [stack firstEndStackCardIndex]);
    EXPECT_FLOAT_EQ(inStackSeparation + 2 * kMaxStagger,
                    SeparationOnLayoutAxis(stack, 5, 6));
    // A scroll should immediately bring the seventh card out of the start
    // stack, without affecting the distance between the sixth and seventh
    // cards.
    [stack scrollCardAtIndex:5
                     byDelta:kMaxStagger / -2.0
        allowEarlyOverscroll:YES
           decayOnOverscroll:YES
          scrollLeadingCards:YES];
    ValidateCardPositioningConstraints(stack, kEndLimit, false);
    EXPECT_EQ(7, [stack firstEndStackCardIndex]);
    EXPECT_FLOAT_EQ(inStackSeparation + 2 * kMaxStagger,
                    SeparationOnLayoutAxis(stack, 5, 6));
  }
}

}  // namespace
