// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/stack_view/card_set.h"

#import <QuartzCore/QuartzCore.h>

#include "base/logging.h"
#include "components/favicon/ios/web_favicon_driver.h"
#import "ios/chrome/browser/snapshots/snapshot_tab_helper.h"
#import "ios/chrome/browser/tabs/legacy_tab_helper.h"
#import "ios/chrome/browser/tabs/tab.h"
#import "ios/chrome/browser/tabs/tab_model.h"
#include "ios/chrome/browser/ui/rtl_geometry.h"
#import "ios/chrome/browser/ui/stack_view/card_stack_layout_manager.h"
#import "ios/chrome/browser/ui/stack_view/page_animation_util.h"
#import "ios/chrome/browser/ui/stack_view/stack_card.h"
#include "ios/chrome/browser/ui/ui_util.h"
#import "ios/chrome/browser/web_state_list/web_state_list.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const CGFloat kMaxCardStaggerPercentage = 0.35;
}

@interface CardSet ()<StackCardViewProvider, TabModelObserver> {
  TabModel* tabModel_;
  UIView* displayView_;
  CardStackLayoutManager* stackModel_;
  UIImageView* stackShadow_;
}

// Set to |YES| when the card set should draw a shadow around the entire stack.
@property(nonatomic, assign) BOOL shouldShowShadow;

// Creates and returns an autoreleased StackCard from the given |tab| (which
// must not be nil).
- (StackCard*)buildCardFromTab:(Tab*)tab;

// Rebuilds the set of cards from the current state of the tab model.
- (void)rebuildCards;

// Makes |card| visible (or in the view hierarchy but hidden if it's covered
// by other cards) in the current display view at the right z-order relative
// to any other cards from the set that are already displayed.
- (void)displayCard:(StackCard*)card;

// Updates the tab display side of the cards in the set based on the current
// layout orientation.
- (void)updateCardTabs;

@end

#pragma mark -

@implementation CardSet

@synthesize displayView = displayView_;
@synthesize observer = observer_;
@synthesize ignoresTabModelChanges = ignoresTabModelChanges_;
@synthesize defersCardHiding = defersCardHiding_;
@synthesize keepOnlyVisibleCardViewsAlive = keepOnlyVisibleCardViewsAlive_;
@synthesize shouldShowShadow = shouldShowShadow_;
@synthesize closingCard = closingCard_;

- (CardStackLayoutManager*)stackModel {
  return stackModel_;
}

- (id)initWithModel:(TabModel*)model {
  if ((self = [super init])) {
    tabModel_ = model;
    [tabModel_ addObserver:self];
    stackModel_ = [[CardStackLayoutManager alloc] init];
    [self rebuildCards];
    self.shouldShowShadow = YES;
  }
  return self;
}

- (void)disconnect {
  [tabModel_ removeObserver:self];
}

- (void)dealloc {
  [self disconnect];
}

#pragma mark Properties

- (TabModel*)tabModel {
  return tabModel_;
}

- (void)setTabModel:(TabModel*)tabModel {
  DCHECK([tabModel count] == 0);
  DCHECK([tabModel_ count] == 0);
  [tabModel_ removeObserver:self];
  tabModel_ = tabModel;
  if (!ignoresTabModelChanges_)
    [tabModel_ addObserver:self];
}

- (NSArray*)cards {
  return [stackModel_ cards];
}

- (StackCard*)currentCard {
  DCHECK(!ignoresTabModelChanges_);
  Tab* currentTab = [tabModel_ currentTab];
  if (!currentTab)
    return nil;
  NSUInteger currentTabIndex = [tabModel_ indexOfTab:currentTab];
  // There is a period of time during closing the current tab where currentTab
  // is still the closed tab, but that tab is no longer *in* the model.
  // TODO(stuartmorgan): Fix this in TabModel; this is dumb.
  if (currentTabIndex == NSNotFound)
    return nil;
  return currentTabIndex < self.cards.count ? self.cards[currentTabIndex] : nil;
}

- (void)setCurrentCard:(StackCard*)card {
  DCHECK(!ignoresTabModelChanges_);
  NSInteger cardIndex = [self.cards indexOfObject:card];
  DCHECK(cardIndex != NSNotFound);
  [tabModel_ setCurrentTab:[tabModel_ tabAtIndex:cardIndex]];
}
- (void)setDisplayView:(UIView*)displayView {
  if (displayView == displayView_)
    return;
  for (StackCard* card in self.cards) {
    if (card.viewIsLive) {
      [card.view removeFromSuperview];
      [card releaseView];
    }
  }
  [stackShadow_ removeFromSuperview];
  displayView_ = displayView;
  // Add the stack shadow view to the new display view.
  if (!stackShadow_) {
    UIImage* shadowImage = [UIImage imageNamed:kCardShadowImageName];
    shadowImage = [shadowImage
        resizableImageWithCapInsets:UIEdgeInsetsMake(
                                        shadowImage.size.height / 2.0,
                                        shadowImage.size.width / 2.0,
                                        shadowImage.size.height / 2.0,
                                        shadowImage.size.width / 2.0)];
    stackShadow_ = [[UIImageView alloc] initWithImage:shadowImage];
    [stackShadow_ setHidden:!self.cards.count];
  }
  [self.displayView addSubview:stackShadow_];
  // Don't set the stack's end limit when the view is set to nil in order to
  // avoid losing existing card positions; these positions will be needed
  // if/when the view is restored (e.g., if the view was purged due to a memory
  // warning while in a modal view and then restored when exiting the modal
  // view).
  if (self.displayView)
    [self displayViewSizeWasChanged];
}

- (CardCloseButtonSide)closeButtonSide {
  return [stackModel_ layoutIsVertical] ? CardCloseButtonSide::TRAILING
                                        : CardCloseButtonSide::LEADING;
}

- (void)setIgnoresTabModelChanges:(BOOL)ignoresTabModelChanges {
  if (ignoresTabModelChanges_ == ignoresTabModelChanges)
    return;
  ignoresTabModelChanges_ = ignoresTabModelChanges;
  if (ignoresTabModelChanges) {
    [tabModel_ removeObserver:self];
  } else {
    [self rebuildCards];
    [tabModel_ addObserver:self];
  }
}

- (void)setDefersCardHiding:(BOOL)defersCardHiding {
  if (defersCardHiding_ == defersCardHiding)
    return;
  defersCardHiding_ = defersCardHiding;
  if (!defersCardHiding_)
    [self updateCardVisibilities];
}

- (CGFloat)maximumOverextensionAmount {
  return [stackModel_ maximumOverextensionAmount];
}

- (void)setMaximumOverextensionAmount:(CGFloat)amount {
  [stackModel_ setMaximumOverextensionAmount:amount];
}

- (void)setKeepOnlyVisibleCardViewsAlive:(BOOL)keepAlive {
  if (keepOnlyVisibleCardViewsAlive_ == keepAlive)
    return;
  keepOnlyVisibleCardViewsAlive_ = keepAlive;
  if (!keepOnlyVisibleCardViewsAlive_)
    [self updateCardVisibilities];
}

- (void)setShouldShowShadow:(BOOL)shouldShowShadow {
  if (shouldShowShadow_ != shouldShowShadow) {
    shouldShowShadow_ = shouldShowShadow;
    [stackShadow_ setHidden:!shouldShowShadow_];
  }
}

- (void)setClosingCard:(StackCard*)closingCard {
  if (closingCard_ != closingCard) {
    closingCard_ = closingCard;
    if (closingCard) {
      self.shouldShowShadow = self.cards.count > 1;
      [self updateShadowLayout];
      closingCard.view.shouldShowShadow = YES;
      closingCard.view.shouldMaskShadow = NO;
      StackCard* nextVisibleCard = nil;
      NSUInteger nextVisibleCardIdx = [self.cards indexOfObject:closingCard];
      while (++nextVisibleCardIdx < self.cards.count) {
        nextVisibleCard = self.cards[nextVisibleCardIdx];
        if ([nextVisibleCard viewIsLive] && !nextVisibleCard.view.hidden)
          break;
      }
      nextVisibleCard.view.shouldShowShadow = YES;
      nextVisibleCard.view.shouldMaskShadow = NO;
    } else {
      self.shouldShowShadow = YES;
      [self updateCardVisibilities];
    }
  }
}

#pragma mark Public Methods

- (void)configureLayoutParametersWithMargin:(CGFloat)margin {
  DCHECK(self.displayView);

  [stackModel_ setStartLimit:margin];

  BOOL isVertical = [stackModel_ layoutIsVertical];
  CGSize cardSize = [stackModel_ cardSize];
  CGFloat cardLength = isVertical ? cardSize.height : cardSize.width;
  [stackModel_ setMaxStagger:(kMaxCardStaggerPercentage * cardLength)];
}

- (void)displayViewSizeWasChanged {
  CGRect displayBounds = self.displayView.bounds;
  CGFloat displayWidth = CGRectGetWidth(displayBounds);
  for (StackCard* card in self.cards) {
    LayoutRect layout = card.layout;
    layout.boundingWidth = displayWidth;
    card.layout = layout;
  }
  CGFloat endLimit = [stackModel_ layoutIsVertical]
                         ? CGRectGetHeight(displayBounds)
                         : displayWidth;
  [stackModel_ setEndLimit:endLimit];
}

- (void)setCardSize:(CGSize)cardSize {
  [stackModel_ setCardSize:cardSize];
}

- (void)setLayoutAxisPosition:(CGFloat)position
                   isVertical:(BOOL)layoutIsVertical {
  [stackModel_ setLayoutIsVertical:layoutIsVertical];
  [stackModel_ setLayoutAxisPosition:position];
  [self updateCardTabs];
  [self updateShadowLayout];
}

- (void)layOutStartStack {
  [stackModel_ layOutStartStack];
  [self updateCardVisibilities];
}

- (void)fanOutCards {
  if ([self.cards count] == 0)
    return;
  [self fanOutCardsWithStartIndex:0];
}

- (void)fanOutCardsWithStartIndex:(NSUInteger)startIndex {
  if (![self.cards count])
    return;
  DCHECK(startIndex < [self.cards count]);
  [stackModel_ fanOutCardsWithStartIndex:startIndex];
  [self updateCardVisibilities];
}

- (std::vector<LayoutRect>)cardLayouts {
  std::vector<LayoutRect> cardLayouts;
  for (StackCard* card in self.cards)
    cardLayouts.push_back(card.layout);
  return cardLayouts;
}

- (void)scrollCardAtIndex:(NSUInteger)index
                  byDelta:(CGFloat)delta
     allowEarlyOverscroll:(BOOL)allowEarlyOverscroll
        decayOnOverscroll:(BOOL)decayOnOverscroll
       scrollLeadingCards:(BOOL)scrollLeadingCards {
  DCHECK(index < [self.cards count]);
  [stackModel_ scrollCardAtIndex:index
                         byDelta:delta
            allowEarlyOverscroll:allowEarlyOverscroll
               decayOnOverscroll:decayOnOverscroll
              scrollLeadingCards:scrollLeadingCards];
  [self updateCardVisibilities];
}

- (BOOL)stackIsOverextended {
  if ([self.cards count] == 0)
    return NO;
  return ([self overextensionOnCardAtIndex:0]);
}

- (BOOL)overextensionOnCardAtIndex:(NSUInteger)index {
  DCHECK(index < [self.cards count]);
  if ([self overextensionTowardStartOnCardAtIndex:index])
    return YES;
  if ((index == 0) && [stackModel_ overextensionTowardEndOnFirstCard])
    return YES;
  return NO;
}

- (BOOL)overextensionTowardStartOnCardAtIndex:(NSUInteger)index {
  DCHECK(index < [self.cards count]);
  return [stackModel_ overextensionTowardStartOnCardAtIndex:index];
}

- (void)eliminateOverextension {
  [stackModel_ eliminateOverextension];
  [self updateCardVisibilities];
}

- (void)scrollCardAtIndex:(NSUInteger)index awayFromNeighbor:(BOOL)preceding {
  DCHECK(index < [self.cards count]);
  [stackModel_ scrollCardAtIndex:index awayFromNeighbor:preceding];
  [self updateCardVisibilities];
}

- (void)updateCardVisibilities {
  BOOL shouldHideNextVisibleCardShadow = YES;
  for (StackCard* card in self.cards) {
    if ([stackModel_ cardIsCovered:card]) {
      if (card.viewIsLive && !defersCardHiding_) {
        if (keepOnlyVisibleCardViewsAlive_) {
          [card.view removeFromSuperview];
          [card releaseView];
        } else {
          card.view.hidden = YES;
        }
      }
    } else {
      [self displayCard:card];
      // Hide the first visible card's shadow.
      card.view.shouldShowShadow = !shouldHideNextVisibleCardShadow;
      if (shouldHideNextVisibleCardShadow)
        shouldHideNextVisibleCardShadow = NO;
      card.view.shouldMaskShadow = card.view.shouldShowShadow;
    }
  }
  if (self.shouldShowShadow)
    [self updateShadowLayout];
  // Updates VoiceOver with currently accessible elements.
  UIAccessibilityPostNotification(UIAccessibilityLayoutChangedNotification,
                                  nil);
}

- (BOOL)preloadNextCard {
  if (keepOnlyVisibleCardViewsAlive_)
    return NO;
  // Find the next card to preload.
  StackCard* nextCard = nil;
  for (nextCard in self.cards) {
    // TODO(stuartmorgan): Change the selection here to favor the cards that
    // are closest to becoming visible.
    if (!nextCard.viewIsLive)
      break;
  }
  // If there was one, preload it.
  if (nextCard) {
    // Visible card views should have already been synchronously loaded.
    DCHECK([stackModel_ cardIsCovered:nextCard]);
    [self displayCard:nextCard];
  }
  return nextCard != nil;
}

- (void)clearGestureRecognizerTargetAndDelegateFromCards:(id)object {
  for (StackCard* card in self.cards) {
    // Ensure that views aren't created just to remove their recognizers.
    if (!card.viewIsLive)
      continue;
    for (UIGestureRecognizer* recognizer in card.view.gestureRecognizers) {
      if (recognizer.delegate == object)
        recognizer.delegate = nil;
      // Passing NULL as the value of the |action| parameter causes all actions
      // associated with this target to be removed. Note that if |object| is
      // not a target of |recognizer| this method is a no-op.
      [recognizer removeTarget:object action:NULL];
    }
  }
}

- (void)removeCardAtIndex:(NSUInteger)index {
  DCHECK(index < [self.cards count]);
  StackCard* card = [self.cards objectAtIndex:index];
  [self.observer cardSet:self willRemoveCard:card atIndex:index];
  [stackModel_ removeCard:card];

  [self.observer cardSet:self didRemoveCard:card atIndex:index];
}

#pragma mark Card Construction/Display

- (StackCard*)buildCardFromTab:(Tab*)tab {
  DCHECK(tab);
  StackCard* card = [[StackCard alloc] initWithViewProvider:self];
  card.size = [stackModel_ cardSize];
  card.tabID = reinterpret_cast<NSUInteger>(tab);

  return card;
}

- (void)rebuildCards {
  [stackModel_ removeAllCards];

  WebStateList* webStateList = tabModel_.webStateList;
  for (int index = 0; index < webStateList->count(); ++index) {
    web::WebState* webState = webStateList->GetWebStateAt(index);
    StackCard* card =
        [self buildCardFromTab:LegacyTabHelper::GetTabForWebState(webState)];
    [stackModel_ addCard:card];
  }

  [self.observer cardSetRecreatedCards:self];
}

- (void)displayCard:(StackCard*)card {
  DCHECK(self.displayView);
  card.view.hidden = [stackModel_ cardIsCovered:card];

  if (card.view.superview)
    return;
  // Find the card view (if any) that's above the card to add and already in the
  // view.
  StackCard* cardAboveNewCard = nil;
  NSUInteger indexOfCard = [self.cards indexOfObject:card];
  DCHECK(indexOfCard != NSNotFound);
  for (NSUInteger i = indexOfCard + 1; i < [self.cards count]; ++i) {
    StackCard* nextCard = [self.cards objectAtIndex:i];
    if (nextCard.viewIsLive && nextCard.view.superview) {
      cardAboveNewCard = nextCard;
      break;
    }
  }
  if (cardAboveNewCard) {
    [self.displayView insertSubview:card.view
                       belowSubview:cardAboveNewCard.view];
  } else {
    [self.displayView addSubview:card.view];
  }

  LayoutRect layout = card.layout;
  layout.boundingWidth = CGRectGetWidth(self.displayView.bounds);
  card.layout = layout;

  [self.observer cardSet:self displayedCard:card];
}

#pragma mark Deck Management

- (void)updateCardTabs {
  CardCloseButtonSide closeButtonSide = self.closeButtonSide;
  for (StackCard* card in self.cards) {
    if (card.viewIsLive)
      card.view.closeButtonSide = closeButtonSide;
  }
}

#pragma mark -
#pragma mark StackCardViewProvider Methods

- (CardView*)cardViewWithFrame:(CGRect)frame forStackCard:(StackCard*)card {
  DCHECK(!ignoresTabModelChanges_);
  NSUInteger cardIndex = [self.cards indexOfObject:card];
  DCHECK(cardIndex != NSNotFound);
  Tab* tab = [tabModel_ tabAtIndex:cardIndex];
  DCHECK(tab);
  NSString* title = tab.title;
  if (![title length])
    title = tab.urlDisplayString;
  CardView* view = [[CardView alloc] initWithFrame:frame
                                       isIncognito:[tabModel_ isOffTheRecord]];
  [view setTitle:title];
  [view setFavicon:nil];

  favicon::FaviconDriver* faviconDriver =
      favicon::WebFaviconDriver::FromWebState(tab.webState);
  if (faviconDriver && faviconDriver->FaviconIsValid()) {
    gfx::Image favicon = faviconDriver->GetFavicon();
    if (!favicon.IsEmpty())
      [view setFavicon:favicon.ToUIImage()];
  }

  SnapshotTabHelper::FromWebState(tab.webState)
      ->RetrieveColorSnapshot(^(UIImage* image) {
        [view setImage:image];
      });
  if (!view.image)
    [view setImage:SnapshotTabHelper::GetDefaultSnapshotImage()];
  view.closeButtonSide = self.closeButtonSide;

  return view;
}

#pragma mark TabModelObserver Methods

- (void)tabModel:(TabModel*)model
    didInsertTab:(Tab*)tab
         atIndex:(NSUInteger)index
    inForeground:(BOOL)fg {
  DCHECK(model == tabModel_);
  StackCard* newCard = [self buildCardFromTab:tab];
  [stackModel_ insertCard:newCard atIndex:index];
  [self.observer cardSet:self didAddCard:newCard];
}

// A tab was removed at the given index.
- (void)tabModel:(TabModel*)model
    didRemoveTab:(Tab*)tab
         atIndex:(NSUInteger)index {
  [self removeCardAtIndex:index];
}

// All tabs were closed in the model
- (void)tabModelClosedAllTabs:(TabModel*)model {
  [self.observer cardSetDidCloseAllTabs:self];
}

- (CGFloat)maximumStackLength {
  return [stackModel_ maximumStackLength];
}

- (BOOL)cardIsCollapsed:(StackCard*)card {
  return [stackModel_ cardIsCollapsed:card];
}

- (BOOL)stackIsFullyCollapsed {
  return [stackModel_ stackIsFullyCollapsed];
}

- (BOOL)stackIsFullyFannedOut {
  return [stackModel_ stackIsFullyFannedOut];
}

- (BOOL)stackIsFullyOverextended {
  return [stackModel_ stackIsFullyOverextended];
}

- (CGFloat)overextensionAmount {
  return [stackModel_ overextensionAmount];
}

- (BOOL)isCardInStartStaggerRegion:(StackCard*)card {
  NSInteger cardIndex = [self.cards indexOfObject:card];
  DCHECK(cardIndex != NSNotFound);
  // The last card in the start stack is not actually collapsed, thus the -1.
  NSInteger indexOfLastCardInStartStaggerRegion =
      [stackModel_ lastStartStackCardIndex] - 1;
  return (cardIndex <= indexOfLastCardInStartStaggerRegion);
}

- (BOOL)isCardInEndStaggerRegion:(StackCard*)card {
  NSInteger cardIndex = [self.cards indexOfObject:card];
  DCHECK(cardIndex != NSNotFound);
  NSInteger indexOfFirstCardInEndStaggerRegion =
      [stackModel_ firstEndStackCardIndex];
  return (indexOfFirstCardInEndStaggerRegion != -1 &&
          cardIndex >= indexOfFirstCardInEndStaggerRegion);
}

- (void)updateShadowLayout {
  CGRect stackFrame = CGRectNull;
  for (StackCard* card in self.cards) {
    if (![card isEqual:self.closingCard]) {
      CGRect cardFrame =
          AlignRectOriginAndSizeToPixels(LayoutRectGetRect(card.layout));
      stackFrame = CGRectIsNull(stackFrame)
                       ? cardFrame
                       : CGRectUnion(stackFrame, cardFrame);
    }
  }
  [stackShadow_
      setHidden:CGRectIsNull(stackFrame) || CGRectIsEmpty(stackFrame)];
  if (![stackShadow_ isHidden]) {
    [stackShadow_
        setFrame:UIEdgeInsetsInsetRect(stackFrame, kCardShadowLayoutOutsets)];
  }
}

@end

@implementation CardSet (Testing)

- (StackCard*)cardForTab:(Tab*)tab {
  NSUInteger tabIndex = [tabModel_ indexOfTab:tab];
  if (tabIndex == NSNotFound)
    return nil;
  return [self.cards objectAtIndex:tabIndex];
}

- (void)setStackModelForTesting:(CardStackLayoutManager*)stackModel {
  stackModel_ = stackModel;
}

@end
