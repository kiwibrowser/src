// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/stack_view/stack_view_controller.h"

#import <QuartzCore/QuartzCore.h>

#include <algorithm>
#include <cmath>
#include <limits>

#include "base/format_macros.h"
#import "base/ios/block_types.h"
#include "base/ios/ios_util.h"
#include "base/logging.h"
#import "base/mac/bundle_locations.h"
#import "base/mac/foundation_util.h"
#include "base/mac/scoped_block.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "base/strings/sys_string_conversions.h"
#include "components/feature_engagement/public/event_constants.h"
#include "components/feature_engagement/public/tracker.h"
#include "ios/chrome/browser/chrome_url_constants.h"
#include "ios/chrome/browser/experimental_flags.h"
#include "ios/chrome/browser/feature_engagement/tracker_factory.h"
#include "ios/chrome/browser/feature_engagement/tracker_util.h"
#import "ios/chrome/browser/snapshots/snapshot_tab_helper.h"
#import "ios/chrome/browser/tabs/tab.h"
#import "ios/chrome/browser/tabs/tab_model.h"
#import "ios/chrome/browser/tabs/tab_model_observer.h"
#import "ios/chrome/browser/ui/animation_util.h"
#import "ios/chrome/browser/ui/background_generator.h"
#import "ios/chrome/browser/ui/commands/browser_commands.h"
#import "ios/chrome/browser/ui/commands/command_dispatcher.h"
#import "ios/chrome/browser/ui/commands/open_new_tab_command.h"
#import "ios/chrome/browser/ui/keyboard/UIKeyCommand+Chrome.h"
#include "ios/chrome/browser/ui/main/main_feature_flags.h"
#import "ios/chrome/browser/ui/ntp/new_tab_page_toolbar_controller.h"
#import "ios/chrome/browser/ui/reversed_animation.h"
#import "ios/chrome/browser/ui/rtl_geometry.h"
#import "ios/chrome/browser/ui/stack_view/card_stack_layout_manager.h"
#import "ios/chrome/browser/ui/stack_view/card_stack_pinch_gesture_recognizer.h"
#import "ios/chrome/browser/ui/stack_view/card_view.h"
#import "ios/chrome/browser/ui/stack_view/close_button.h"
#import "ios/chrome/browser/ui/stack_view/new_tab_button.h"
#import "ios/chrome/browser/ui/stack_view/page_animation_util.h"
#import "ios/chrome/browser/ui/stack_view/stack_card.h"
#import "ios/chrome/browser/ui/stack_view/stack_view_controller_private.h"
#import "ios/chrome/browser/ui/stack_view/stack_view_toolbar_controller.h"
#import "ios/chrome/browser/ui/stack_view/title_label.h"
#import "ios/chrome/browser/ui/toolbar/legacy/toolbar_controller_constants.h"
#import "ios/chrome/browser/ui/toolbar/legacy/toolbar_utils.h"
#import "ios/chrome/browser/ui/toolbar/public/toolbar_controller_base_feature.h"
#import "ios/chrome/browser/ui/toolbar/toolbar_owner.h"
#import "ios/chrome/browser/ui/toolbar/toolbar_snapshot_providing.h"
#import "ios/chrome/browser/ui/tools_menu/public/tools_menu_configuration_provider.h"
#import "ios/chrome/browser/ui/tools_menu/tools_menu_configuration.h"
#import "ios/chrome/browser/ui/tools_menu/tools_menu_coordinator.h"
#import "ios/chrome/browser/ui/ui_util.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/common/material_timing.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ios/web/public/referrer.h"
#import "net/base/mac/url_conversions.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using base::UserMetricsAction;

// To obtain scroll behavior, places the card stacks' display views within a
// UIScrollView container. The container is used only as a driver of scroll
// events. To avoid the finite size of the container from impacting scrolling,
// (1) the container is made large enough that it cannot be scrolled to a
// boundary point without the user having first fully scrolled the card stack
// in that direction, and (2) after scroll events, the container's scroll
// offset is recentered if necessary.

namespace {
// The fraction of the display to use for the active deck if both decks are
// being displayed.
const CGFloat kActiveDeckDisplayFraction = 0.85;
// Animation durations.
const NSTimeInterval kCascadingCardCloseDelay = 0.1;
const NSTimeInterval kDefaultAnimationDuration = 0.25;
// The length of the animation that eliminates overextension after a
// scroll/pinch.
const NSTimeInterval kOverextensionEliminationAnimationDuration = .4;
// Fraction of the screen that must be swiped for a stack to switch or a card
// to dismiss.
const CGFloat kSwipeCardScreenFraction = 0.35;
// The velocity (in points / millisecond) below which a scroll's deceleration
// will be killed once the stack is overscrolled. Determined by experimentation
// to see what resulted in a good feel.
const CGFloat kMinFlingVelocityInOverscroll = 0.3;
// The velocity (in points / millisecond) at/above which a scroll will be
// treated as a fling even if it is not for the purposes of determining
// overscroll behavior. Used to handle the corner case where the user flings
// the cards toward the start stack, but at the moment that the scrolled card
// would become overscrolled, the finger is still registered as tracking. Has
// the tradeoff that if user is legimately scrolling above this velocity near
// the start stack, the card will not track the user's finger. Value determined
// by experimentation to see what resulted in a good handling of this tradeoff.
const CGFloat kThresholdVelocityForTreatingScrollAsFling = 1.0;
// The factor by which scroll velocity is decayed once a fling becomes
// overscrolled.
const CGFloat kDecayFactorInBounce = .75;
// The duration (in seconds) that the user must press on a card before
// beginning an ambiguous swipe (i.e., a swipe that could result in either
// dismissing the card or changing decks) for that swipe to trigger card
// dismissal instead of changing decks.
const CGFloat kPressDurationForAmbiguousSwipeToTriggerDismissal = .2;
// The vertical overlap between the scroll view and the toolbar (chosen to match
// aspect ratio of snapshotted webview).
const CGFloat kVerticalToolbarOverlap = 0.0;
// The delay into the dismissal transition animation at which to update the
// status bar.  The value was provided by UX, and corresponds to approximately
// when the selected card's frame image crosses the status bar in the animation.
const NSTimeInterval kDismissalStatusBarUpdateDelay = 0.15;
// When choosing the size of the cards, ensure that the bottom of the card
// is at most |kCardBottomPadding| from the bottom of the scroll view.
const CGFloat kCardBottomPadding = 29.0;
// Animation key used for the dummy toolbar background view animation.
NSString* const kDummyToolbarBackgroundViewAnimationKey =
    @"DummyToolbarBackgroundViewAnimationKey";
// Animation key used for the transition toolbar snapshot animation.
NSString* const kTransitionToolbarAnimationKey =
    @"TransitionToolbarAnimationKey";
}  // anonymous namespace

// Container for the state associated with gesture-related events.
@interface GestureStateTracker : NSObject

@property(nonatomic, assign) NSUInteger scrollCardIndex;
@property(nonatomic, assign) CGFloat previousScrollOffset;
@property(nonatomic, assign) base::TimeTicks previousScrollTime;
// The current scroll velocity, in points / millisecond.
@property(nonatomic, readonly) CGFloat scrollVelocity;
@property(nonatomic, assign) BOOL resetScrollCardOnNextDrag;
@property(nonatomic, assign) NSUInteger firstPinchCardIndex;
@property(nonatomic, assign) NSUInteger secondPinchCardIndex;
@property(nonatomic, assign) CGFloat previousFirstPinchOffset;
@property(nonatomic, assign) CGFloat previousSecondPinchOffset;
// YES when a pinch gesture is currently being recognized.
@property(nonatomic, assign) BOOL pinching;
// YES when a 1-fingered pinch gesture is being interpreted by
// StackViewController's |handlePinchFrom:| as a scroll.
@property(nonatomic, assign) BOOL scrollingInPinch;
// Swipe gesture starting position. In portrait, this is the x position of the
// beginning touch. In landscape this is the y position.
@property(nonatomic, assign) CGFloat swipeStartingPosition;
// If YES, a swipe gesture is interpreted as being a swipe to change decks.
// Otherwise, a swipe gesture is interpreted as being a swipe to close a card.
@property(nonatomic, assign) BOOL swipeChangesDecks;
// The index of the card being swiped. Undefined if |swipeChangesDecks| is YES.
@property(nonatomic, assign) NSUInteger swipedCardIndex;
@property(nonatomic, assign) BOOL resetSwipedCardOnNextSwipe;
@property(nonatomic, assign) BOOL swipeIsBeginning;
// Whether a swipe whose intent is ambiguous should change decks (as opposed to
// dismiss a card). Relevant only when multiple stacks are present.
@property(nonatomic, assign) BOOL ambiguousSwipeChangesDecks;

// Given |distance|, which should be the distance scrolled since
// |previousScrollTime|, updates |scrollVelocity|.
- (void)updateScrollVelocityWithScrollDistance:(CGFloat)distance;

@end

@implementation GestureStateTracker

@synthesize ambiguousSwipeChangesDecks = _ambiguousSwipeChangesDecks;
@synthesize firstPinchCardIndex = _firstPinchCardIndex;
@synthesize pinching = _pinching;
@synthesize previousFirstPinchOffset = _previousFirstPinchOffset;
@synthesize previousScrollOffset = _previousScrollOffset;
@synthesize previousScrollTime = _previousScrollTime;
@synthesize previousSecondPinchOffset = _previousSecondPinchOffset;
@synthesize resetScrollCardOnNextDrag = _resetScrollCardOnNextDrag;
@synthesize resetSwipedCardOnNextSwipe = _resetSwipedCardOnNextSwipe;
@synthesize scrollCardIndex = _scrollCardIndex;
@synthesize scrollingInPinch = _scrollingInPinch;
@synthesize scrollVelocity = _scrollVelocity;
@synthesize secondPinchCardIndex = _secondPinchCardIndex;
@synthesize swipeChangesDecks = _swipeChangesDecks;
@synthesize swipedCardIndex = _swipedCardIndex;
@synthesize swipeIsBeginning = _swipeIsBeginning;
@synthesize swipeStartingPosition = _swipeStartingPosition;

- (instancetype)init {
  if ((self = [super init])) {
    _resetScrollCardOnNextDrag = YES;
  }
  return self;
}

- (void)updateScrollVelocityWithScrollDistance:(CGFloat)distance {
  base::TimeDelta elapsedTime = base::TimeTicks::Now() - _previousScrollTime;
  if (elapsedTime == base::TimeDelta::FromMicroseconds(0))
    return;
  _scrollVelocity =
      fabs(distance / CGFloat(elapsedTime.InMillisecondsRoundedUp()));
}

@end

@interface StackViewController ()<StackViewToolbarControllerDelegate,
                                  ToolsMenuConfigurationProvider>

// Clears the internal state of the object. Should only be called when the
// object is not being shown. After this method is called, a call to
// |restoreInternalState| must be made before the object is reshown.
- (void)clearInternalState;
// Updates the layout parameters of the scroll view and the display views based
// on the viewport size. Should be called any time that the viewport size is
// changed.
- (void)viewportSizeWasChanged;
// Configures the scroll view to be large enough so that the user could not
// scroll to one of its boundaries without also having reached the
// corresponding boundary of the stack being scrolled.
- (void)updateScrollViewContentSize;
// Eliminates the ability for the user to perform any further interactions
// with the stack view. Should be called when the stack view starts being
// dismissed.
- (void)prepareForDismissal;

// Asynchronously adds the remaining cards to the display view to pre-load them.
// This is safe to call multiple times.
- (void)preloadCardViewsAsynchronously;
// Adds the next not-yet-loaded card to the display view.
- (void)preloadNextCardView;
// Animates the removal of |cardView| from the superview, with the start of the
// animation being delayed by |delay|. Performs |completion| on animation
// finish (may be NULL). Card will dismiss clockwise when |clockwise| is YES and
// counter-clockwise when |clockwise| is NO.
- (void)animateOutCardView:(CardView*)cardView
                     delay:(NSTimeInterval)delay
                 clockwise:(BOOL)clockwise
                completion:(ProceduralBlock)completion;
// Removes all cards in |cardSet| from the underlying model and their superview.
- (void)removeAllCardsFromSet:(CardSet*)cardSet;
// Disable all the gesture handlers. Must be called before removing cards from
// the active set.
- (void)disableGestureHandlers;
// Enable all the gesture handlers.
- (void)enableGestureHandlers;
// Should be called whenever the number of cards in the active set changes
// (including the active set itself changing).
- (void)activeCardCountChanged;
// Computes and stores the initial card size information that will decide how
// layout is done for the remainder of the stack view's lifetime.
- (void)setInitialCardSizingForSize:(CGSize)size;
// Configures the card sets based on the initial card size and the current size
// of the scroll view.
- (void)configureCardSets;
// Returns |YES| if |viewWillAppear| should perform steps to display cards for
// the first time.
- (BOOL)needsInitialDisplay;
// Updates the card sizing and layout for the current device orientation.
// If |animates| is true, the size change will be animated, otherwise it will be
// done synchronously.
- (void)updateDeckOrientationWithAnimation:(BOOL)animates;
// Updates the card sizing for the current deck states and device orientation.
// If |animates| is true, the size change will be animated, otherwise it will be
// done synchronously.
- (void)updateCardSizesWithAnimation:(BOOL)animates;
// Animates setting the opacity of the card tabs of the current deck to
// |opacity|.
- (void)animateActiveSetCardTabsToOpacity:(CGFloat)opacity
                             withDuration:(CGFloat)duration
                               completion:(ProceduralBlock)completion;
// Updates the positions of the decks in the non-layout direction. Should be
// called if the layout direction, card size, or active set changes.
- (void)updateDeckAxisPositions;
// Updates the positions of the decks in the non-layout direction and then
// shifts them from the standard positions by |amount|.
- (void)updateDeckAxisPositionsWithShiftAmount:(CGFloat)amount;
// Updates the position of |cardSet| in the non-layout direction and then
// shifts it from its standard position by |amount|.
- (void)updateDeckAxisPositionForCardSet:(CardSet*)cardSet
                         withShiftAmount:(CGFloat)shiftAmount;
// The amount by which |cardSet| should be shifted from its default layout axis
// position to be positioned offscreen.
- (CGFloat)shiftOffscreenAmountForCardSet:(CardSet*)cardSet;
// Refreshes the card display, using the current orientation. Should be called
// any time the orientation has changed or the display views have been rebuilt.
- (void)refreshCardDisplayWithAnimation:(BOOL)animates;
// Updates the UI to reflect the current card set. Called automatically by
// setActiveCardSet:, but also called during setup.
- (void)displayActiveCardSet;
// Switches to showing only the main card set, with no room for showing the
// incognito set. Should be called when the last incognito card is closed.
- (void)displayMainCardSetOnly;
// Updates the appearance of the toolbar for the current state of the card
// stack.
- (void)updateToolbarAppearanceWithAnimation:(BOOL)animate;

// Returns the size of a single card (at normal zoom).
- (CGSize)cardSize;
// Returns the size that should be used for cards being displayed in a viewport
// with breadth |breadth| and a scrollview of size |scrollViewSize|, taking
// margins into account and preserving the content area aspect ratio.
- (CGSize)cardSizeForBreadth:(CGFloat)breadth
              scrollViewSize:(CGSize)scrollViewSize;
// Returns the amount that |point| is offset on the current scroll axis.
- (CGFloat)scrollOffsetAmountForPoint:(CGPoint)point;
// Returns the amount that |position| is offset on the current scroll axis.
- (CGFloat)scrollOffsetAmountForPosition:(LayoutRectPosition)position;
// Returns the CGPoint offset |offset| in the current scroll direction, with
// 0 as the other component of the point.
- (CGPoint)scrollOffsetPointWithAmount:(CGFloat)offset;
// Returns the length of |size| in the current scroll direction.
- (CGFloat)scrollLength:(CGSize)size;
// Returns the length of |size| in the non-scroll direction.
- (CGFloat)scrollBreadth:(CGSize)size;
// Returns a size for the current scroll direction with the given scroll length
// and breadth.
- (CGSize)sizeForScrollLength:(CGFloat)length breadth:(CGFloat)breadth;
// Returns the index of the card that |point| is contained within, or
// |NSNotFound| if |point| is not contained in any card. This is not an
// efficient lookup, so this should *not* be called frequently.
- (NSUInteger)indexOfCardAtPoint:(CGPoint)point;
// Will reverse the current transition animations if the tab switcher button is
// pressed before the animation can finish.
- (void)cancelTransitionAnimation;
// Called within the completion block for the transition animations.
- (void)finishTransitionAnimation;
// Called within the completion block for the transition animations.  Notifies
// the delegates that the current transition has finished.
- (void)notifyDelegatesTransitionFinished;
// Sets up the view hierarchy for transitioning and calls the stack view and
// toolbar animation selectors below, wrapping the animations in a single
// CATransaction.  Use |transitionStyle| = StackTransitionStylePresenting for
// presentation and |transitionStyle| = StackTransitionStyleDismissing for
// dismissal.
- (void)animateTransitionWithStyle:(StackTransitionStyle)transitionStyle;
// Updates the view hierarchy for the transition based on the current transition
// style.  If the style is STACK_TRANSITION_STYLE_PRESENTING or
// STACK_TRANSITION_STYLE_DISMISSING, the display views are added to the root
// view and the toolbar is inserted between them.  If the style is
// STACK_TRANSITION_STYLE_NONE, the display views are reparented into the scroll
// view and aligned to the view port.
- (void)reorderSubviewsForTransition;
// Animates the cards in |cardSet| from |beginLayouts| to |endLayouts| with the
// transition style specified by |self.transitionStyle|.
- (void)animateCardSet:(CardSet*)cardSet
      fromBeginLayouts:(std::vector<LayoutRect>)beginLayouts
          toEndLayouts:(std::vector<LayoutRect>)endLayouts;
// Reverses the cancelled transition animations added to the card set.
- (void)reverseTransitionAnimationsForCardSet:(CardSet*)cardSet;
// Adds transition animations to the active card set.  If |self.transitionStyle|
// = StackTransitionStylePresenting, this will fan out the cards in the active
// set from the frames returned by |-cardTransitionFrames| to the fanned out
// frames. If |transitionStyle| = StackTransitionStyleDismissing, the cards will
// animate from their current frames to |-cardTransitionFrames|.
- (void)animateActiveSetTransition;
// Adds transition animations to the inactive set.  The inactive card set will
// be laid out in the fanned frames and will animate in from offscreen if
// |self.transitionStyle| = StackTransitionStylePresenting, and will animate off
// the screen if |transitionStyle| = StackTransitionStyleDismissing.
- (void)animateInactiveSetTransition;
// Adds the dummy toolbar background to the active card set's current card and
// animates alongside the toolbar, creating a cross-fade effect from the
// toolbar's frame at the top of the screen to the tab portion of |cardFrame|
// and vise versa.
- (void)animateDummyToolbarForCardFrame:(CGRect)cardFrame
                        transitionStyle:(StackTransitionStyle)transitionStyle;
// Reverses the dummy toolbar background animations for cancelled transitions.
- (void)reverseDummyToolbarBackgroundViewAnimation;
// Adds a snapshot of the toolbar, provided by the |snapshotProvider| of the
// |transitionToolbarOwner| to the stack's view hierarchy and animates the frame
// alongside the active card set's current card (i.e. from its position at the
// top of the screen to the tab portion of the provided |cardFrame| or vise
// versa).
- (void)animateTransitionToolbarSnapshotWithCardFrame:(CGRect)cardFrame
                                      transitionStyle:
                                          (StackTransitionStyle)transitionStyle;
// Returns an vector of LayoutRects corresponding to the active card set's
// frames at the moment of switching to or from the stack view.  The cards below
// the current card in the z axis will have top-aligned unscaled frames, the
// current card will have a top-aligned frame scaled such that the web view
// snapshot takes the entire screen below the toolbar, and cards above the the
// current card will have the scaled frame translated offscreen.
- (std::vector<LayoutRect>)cardTransitionLayouts;
// Returns the index of what should be the first visible card in the initial
// fanout of |cardSet|.
- (NSUInteger)startIndexOfInitialFanoutForCardSet:(CardSet*)cardSet;
// Perform the animation for switching out of the stack view while
// simultaneously opening a tab with |url|, at |position| with the given
// |transition|. The tab where |url| is opened is returned.
- (Tab*)dismissWithNewTabAnimation:(const GURL&)url
                           atIndex:(NSUInteger)position
                        transition:(ui::PageTransition)transition;
- (void)closeTab:(id)sender;
- (void)handleLongPressFrom:(UIPinchGestureRecognizer*)recognizer;
- (void)handlePinchFrom:(UIPinchGestureRecognizer*)recognizer;
- (void)handleTapFrom:(UITapGestureRecognizer*)recognizer;
// Returns the card corresponding to |view|. This is not an efficient lookup,
// so this should *not* be called frequently.
- (StackCard*)cardForView:(CardView*)view;

// Updates the display views so that they are aligned to the scroll view's
// viewport. Should be called any time the scroll view's content offset
// changes.
- (void)alignDisplayViewsToViewport;

// Handles swipe gestures between card sets and swipes to remove cards.
- (void)handlePanFrom:(UIPanGestureRecognizer*)gesture;
// Determines whether the current swipe should be treated as a swipe to dismiss
// a card or a swipe to change decks.
- (void)determineSwipeType:(UIPanGestureRecognizer*)gesture;
// Returns the distance that a swipe needs to travel in order to trigger an
// action (close card/change deck).
- (CGFloat)distanceForSwipeToTriggerAction;
// Returns |YES| if the current swipe should trigger an action (close
// card/change deck) based on the swipe's ending position and its starting
// position.
- (BOOL)swipeShouldTriggerAction:(CGFloat)endingPosition;
// Moves between card sets, potentially changing the active card set at the end
// of the gesture.
- (void)swipeDeck:(UIPanGestureRecognizer*)gesture;
// Moves the card being swiped, potentially dismissing the card at the end of
// the gesture.
- (void)swipeCard:(UIPanGestureRecognizer*)gesture;

// Returns whether the current scroll should be ended.
- (BOOL)shouldEndScroll;
// Performs any necessary cleanup actions after a scroll is completed.
- (void)scrollEnded;
// Performs any necessary cleanup actions after a pinch is completed.
- (void)pinchEnded;
// Animates overextension elimination from the active card set. Performs
// |completion| on animation finish (may be |NULL|).
- (void)animateOverextensionEliminationWithCompletion:
    (ProceduralBlock)completion;
// Cancels a scroll that is in its deceleration phase.
// NOTE: Will not have the desired behavior if invoked on a scroll that is not
// in the deceleration phase, i.e., the user is still dragging on the screen.
- (void)killScrollDeceleration;
// Adjusts the amount that the stack is allowed to overextend depending on
// whether the current scroll is a fling.
- (void)adjustMaximumOverextensionAmount:(BOOL)isFling;

// Responds to voice over focusing on TitleLabel or CloseButton. If the element
// label is covered, scroll it toward the start stack, or the next card toward
// the end stack as appropriate. If the element is in the middle of a stack, fan
// outcards from its card's index.
- (void)accessibilityFocusedOnElement:(id)element;

// Determine the center of |sender| if it's a view or a toolbar item and store.
- (void)setLastTapPoint:(id)sender;

@end

@implementation StackViewController {
  UIScrollView* _scrollView;
  // The view containing the stack view's background.
  UIView* _backgroundView;
  // The main card set.
  CardSet* _mainCardSet;
  // The off-the-record card set.
  CardSet* _otrCardSet;
  // The currently active card set; one of _mainCardSet or _otrCardSet.
  __weak CardSet* _activeCardSet;
  __weak id<StackViewControllerTestDelegate> _testDelegate;
  // Controller for the stack view toolbar.
  StackViewToolbarController* _toolbarController;
  // The size of a card at the time the stack was first shown.
  CGSize _initialCardSize;
  // The previous orientation of the interface.
  UIInterfaceOrientation _lastInterfaceOrientation;
  // Gesture recognizer to catch taps on the inactive stack.
  UITapGestureRecognizer* _modeSwitchRecognizer;
  // Gesture recognizer to catch pinches in the active scroll view.
  UIGestureRecognizer* _pinchRecognizer;
  // Gesture recognizer to catch swipes to switch decks/dismiss cards.
  UIGestureRecognizer* _swipeGestureRecognizer;
  // Gesture recognizer that determines whether an ambiguous swipe action
  // (i.e., a swipe on an active card in the direction that would cause a deck
  // change) should trigger a change of decks or a card dismissal.
  UILongPressGestureRecognizer* _swipeDismissesCardRecognizer;
  // Tracks the parameters of gesture-related events.
  GestureStateTracker* _gestureStateTracker;
  // If |YES|, callbacks to |scrollViewDidScroll:| do not trigger scrolling.
  // Default is |NO|.
  BOOL _ignoreScrollCallbacks;
  // The scroll view's pan gesture recognizer.
  __weak UIPanGestureRecognizer* _scrollGestureRecognizer;
  // Because the removal of the StackCard during a swipe happens in a callback,
  // track which direction the animation should dismiss with.
  // |_reverseDismissCard| is only set when the dismissal happens in reverse.
  StackCard* _reverseDismissCard;
  // |YES| if the stack view is in the process of being dismissed.
  BOOL _isBeingDismissed;
  // |YES| if the stack view is currently active.
  BOOL _isActive;
  // |YES| if the stack view has been told to restore internal state, but has
  // not yet become active.
  BOOL _preparingForActive;
  // |YES| if initial card sizes have been computed but cards have not yet been
  // laid out for the first time.  This is only used when the
  // TabSwitcherPresentsBVC experiment is enabled.
  BOOL _needsInitialDisplay;
  // Records whether a memory warning occurred in the current session.
  BOOL _receivedMemoryWarningInSession;
  // |YES| if there is card set animation being processed. For testing only.
  // Save last touch point used by new tab animation.
  CGPoint _lastTapPoint;
  // The dispacther instance used when this view controller is active.
  CommandDispatcher* _dispatcher;
}

@synthesize activeCardSet = _activeCardSet;
@synthesize animationDelegate = _animationDelegate;
@synthesize delegate = _delegate;
@synthesize dummyToolbarBackgroundView = _dummyToolbarBackgroundView;
@synthesize inActiveDeckChangeAnimation = _inActiveDeckChangeAnimation;
@synthesize testDelegate = _testDelegate;
@synthesize toolsMenuCoordinator = _toolsMenuCoordinator;
@synthesize transitionStyle = _transitionStyle;
@synthesize transitionTappedCard = _transitionTappedCard;
@synthesize transitionToolbarOwner = _transitionToolbarOwner;
@synthesize transitionToolbarSnapshot = _transitionToolbarSnapshot;
@synthesize transitionWasCancelled = _transitionWasCancelled;

- (instancetype)initWithMainCardSet:(CardSet*)mainCardSet
                         otrCardSet:(CardSet*)otrCardSet
                      activeCardSet:(CardSet*)activeCardSet
         applicationCommandEndpoint:
             (id<ApplicationCommands>)applicationCommandEndpoint {
  DCHECK(mainCardSet);
  DCHECK(otrCardSet);
  DCHECK(activeCardSet == otrCardSet || activeCardSet == mainCardSet);
  self = [super initWithNibName:nil bundle:nil];
  if (self) {
    [self setUpWithMainCardSet:mainCardSet
                    otrCardSet:otrCardSet
                 activeCardSet:activeCardSet];
    _swipeDismissesCardRecognizer = [[UILongPressGestureRecognizer alloc]
        initWithTarget:self
                action:@selector(handleLongPressFrom:)];
    [_swipeDismissesCardRecognizer
        setMinimumPressDuration:
            kPressDurationForAmbiguousSwipeToTriggerDismissal];
    [_swipeDismissesCardRecognizer setDelegate:self];
    _pinchRecognizer = [[CardStackPinchGestureRecognizer alloc]
        initWithTarget:self
                action:@selector(handlePinchFrom:)];
    [_pinchRecognizer setDelegate:self];
    _modeSwitchRecognizer = [[UITapGestureRecognizer alloc]
        initWithTarget:self
                action:@selector(handleTapFrom:)];
    [_modeSwitchRecognizer setDelegate:self];
    _dispatcher = [[CommandDispatcher alloc] init];
    [_dispatcher startDispatchingToTarget:self
                              forProtocol:@protocol(BrowserCommands)];
    [_dispatcher startDispatchingToTarget:applicationCommandEndpoint
                              forProtocol:@protocol(ApplicationCommands)];

    _toolsMenuCoordinator = [[ToolsMenuCoordinator alloc] init];
    _toolsMenuCoordinator.dispatcher = _dispatcher;
    _toolsMenuCoordinator.configurationProvider = self;
    [_toolsMenuCoordinator start];
  }
  return self;
}

- (instancetype)initWithMainTabModel:(TabModel*)mainModel
                         otrTabModel:(TabModel*)otrModel
                      activeTabModel:(TabModel*)activeModel
          applicationCommandEndpoint:
              (id<ApplicationCommands>)applicationCommandEndpoint {
  DCHECK(mainModel);
  DCHECK(otrModel);
  DCHECK(activeModel == otrModel || activeModel == mainModel);
  CardSet* mainCardSet = [[CardSet alloc] initWithModel:mainModel];
  CardSet* otrCardSet = [[CardSet alloc] initWithModel:otrModel];
  CardSet* activeCardSet =
      (activeModel == mainModel) ? mainCardSet : otrCardSet;
  return [self initWithMainCardSet:mainCardSet
                        otrCardSet:otrCardSet
                     activeCardSet:activeCardSet
        applicationCommandEndpoint:applicationCommandEndpoint];
}

- (instancetype)initWithNibName:(NSString*)nibNameOrNil
                         bundle:(NSBundle*)nibBundleOrNil {
  NOTREACHED();
  return nil;
}

- (instancetype)initWithCoder:(NSCoder*)aDecoder {
  NOTREACHED();
  return nil;
}

- (id<ApplicationCommands, BrowserCommands, ToolbarCommands>)dispatcher {
  return static_cast<id<ApplicationCommands, BrowserCommands, ToolbarCommands>>(
      _dispatcher);
}

- (void)setUpWithMainCardSet:(CardSet*)mainCardSet
                  otrCardSet:(CardSet*)otrCardSet
               activeCardSet:(CardSet*)activeCardSet {
  _mainCardSet = mainCardSet;
  _otrCardSet = otrCardSet;
  if (experimental_flags::IsLRUSnapshotCacheEnabled()) {
    [_mainCardSet setKeepOnlyVisibleCardViewsAlive:YES];
    [_otrCardSet setKeepOnlyVisibleCardViewsAlive:YES];
  }
  _activeCardSet = (activeCardSet == mainCardSet) ? mainCardSet : otrCardSet;
  _gestureStateTracker = [[GestureStateTracker alloc] init];
  _pinchRecognizer = [[CardStackPinchGestureRecognizer alloc]
      initWithTarget:self
              action:@selector(handlePinchFrom:)];
  [_pinchRecognizer setDelegate:self];
  _modeSwitchRecognizer =
      [[UITapGestureRecognizer alloc] initWithTarget:self
                                              action:@selector(handleTapFrom:)];
  [_modeSwitchRecognizer setDelegate:self];
}

- (UIViewController*)viewController {
  return self;
}

- (void)restoreInternalStateWithMainTabModel:(TabModel*)mainModel
                                 otrTabModel:(TabModel*)otrModel
                              activeTabModel:(TabModel*)activeModel {
  DCHECK(mainModel);
  DCHECK(otrModel);
  DCHECK(activeModel == otrModel || activeModel == mainModel);
  DCHECK(!_isActive);
  DCHECK(!_preparingForActive);
  _preparingForActive = YES;

  CardSet* mainCardSet = [[CardSet alloc] initWithModel:mainModel];
  CardSet* otrCardSet = [[CardSet alloc] initWithModel:otrModel];
  CardSet* activeCardSet =
      (activeModel == mainModel) ? mainCardSet : otrCardSet;
  [self setUpWithMainCardSet:mainCardSet
                  otrCardSet:otrCardSet
               activeCardSet:activeCardSet];

  // If the view is not currently loaded, do not adjust its size or add
  // gesture recognizers.  That work will be done in |viewDidLoad|.
  if ([self isViewLoaded]) {
    [self prepareForDisplay];
    // The delegate is set to nil when the stack view is dismissed.
    [_scrollView setDelegate:self];
  }
}

- (void)setOtrTabModel:(TabModel*)otrModel {
  DCHECK(_isActive || _preparingForActive);
  DCHECK(_mainCardSet == _activeCardSet);
  DCHECK([otrModel count] == 0);
  DCHECK([[_otrCardSet tabModel] count] == 0);
  [_otrCardSet setTabModel:otrModel];
}

- (void)clearInternalState {
  DCHECK(!_isActive);
  [[_mainCardSet displayView] removeFromSuperview];
  [[_otrCardSet displayView] removeFromSuperview];

  [_mainCardSet disconnect];
  _mainCardSet = nil;

  [_otrCardSet disconnect];
  _otrCardSet = nil;

  _activeCardSet = nil;

  // Remove gesture recognizers and notifications.
  [self prepareForDismissal];
  _gestureStateTracker = nil;
  _pinchRecognizer = nil;
  _modeSwitchRecognizer = nil;
  _swipeGestureRecognizer = nil;

  // The cards need to recompute their sizes the next time they are shown.
  _initialCardSize.height = _initialCardSize.width = 0.0f;
  // The scroll view will need to recenter itself relative to its viewport.
  [_scrollView setContentOffset:CGPointZero];
  _isBeingDismissed = NO;
}

- (void)viewportSizeWasChanged {
  [self updateScrollViewContentSize];
  [_mainCardSet displayViewSizeWasChanged];
  [_otrCardSet displayViewSizeWasChanged];
}

- (void)updateScrollViewContentSize {
  // Configure the scroll view to be large enough so that the user could not
  // scroll to one of its boundaries from the center without also having
  // reached the corresponding boundary of the stack being scrolled: the
  // maximum size of the larger of the two stacks plus padding.
  CGFloat scrollLength = std::max([_mainCardSet maximumStackLength],
                                  [_otrCardSet maximumStackLength]);
  scrollLength += [self scrollLength:[self cardSize]];
  scrollLength *= 2.0;
  CGFloat scrollBreadth = [self scrollBreadth:[_scrollView bounds].size];
  // Changing the scroll view's content size will result in a callback to
  // |scrollViewDidScroll|.
  _ignoreScrollCallbacks = YES;
  [_scrollView setContentSize:[self sizeForScrollLength:scrollLength
                                                breadth:scrollBreadth]];
  _ignoreScrollCallbacks = NO;
  [self recenterScrollViewIfNecessary];
}

- (void)setUpDisplayViews {
  CGRect displayViewFrame = CGRectMake(0, 0, [_scrollView frame].size.width,
                                       [_scrollView frame].size.height);
  UIView* mainDisplayView = [[UIView alloc] initWithFrame:displayViewFrame];
  [mainDisplayView setAutoresizingMask:UIViewAutoresizingFlexibleWidth |
                                       UIViewAutoresizingFlexibleHeight];
  UIView* otrDisplayView = [[UIView alloc] initWithFrame:displayViewFrame];
  [otrDisplayView setAutoresizingMask:UIViewAutoresizingFlexibleWidth |
                                      UIViewAutoresizingFlexibleHeight];

  [_scrollView addSubview:mainDisplayView];
  [_scrollView addSubview:otrDisplayView];
  [_mainCardSet setDisplayView:mainDisplayView];
  [_otrCardSet setDisplayView:otrDisplayView];
}

- (void)prepareForDisplay {
  [self setUpDisplayViews];

  // Now that the toolbar and the display views are set up, configure the
  // initial display state.
  [self displayActiveCardSet];

  _lastInterfaceOrientation = GetInterfaceOrientation();
  if (_lastInterfaceOrientation == UIInterfaceOrientationUnknown) {
    CGRect screenBounds = [[UIScreen mainScreen] bounds];
    _lastInterfaceOrientation =
        CGRectGetHeight(screenBounds) > CGRectGetWidth(screenBounds)
            ? UIInterfaceOrientationPortrait
            : UIInterfaceOrientationLandscapeRight;
  }
  // TODO(blundell): Why isn't this recognizer initialized with the
  // pinch and mode switch recognizers?
  UIPanGestureRecognizer* panGestureRecognizer =
      [[UIPanGestureRecognizer alloc] initWithTarget:self
                                              action:@selector(handlePanFrom:)];
  [panGestureRecognizer setMaximumNumberOfTouches:1];
  _swipeGestureRecognizer = panGestureRecognizer;
  [[self view] addGestureRecognizer:_swipeGestureRecognizer];
  [_swipeGestureRecognizer setDelegate:self];
}

- (void)viewDidLoad {
  _backgroundView = [[UIView alloc] initWithFrame:self.view.bounds];
  [_backgroundView setAutoresizingMask:(UIViewAutoresizingFlexibleHeight |
                                        UIViewAutoresizingFlexibleWidth)];
  [self.view addSubview:_backgroundView];

  _toolbarController =
      [[StackViewToolbarController alloc] initWithDispatcher:self.dispatcher];
  _toolbarController.delegate = self;
  [self addChildViewController:_toolbarController];
  self.toolsMenuCoordinator.presentationProvider = _toolbarController;
  CGRect toolbarFrame = [self.view bounds];
  toolbarFrame.origin.y = CGRectGetMinY([[_toolbarController view] frame]);
  toolbarFrame.size.height = CGRectGetHeight([[_toolbarController view] frame]);
  [[_toolbarController view] setFrame:toolbarFrame];
  [self.view addSubview:[_toolbarController view]];
  [_toolbarController didMoveToParentViewController:self];

  [[_toolbarController view].leadingAnchor
      constraintEqualToAnchor:self.view.leadingAnchor]
      .active = YES;
  [[_toolbarController view].trailingAnchor
      constraintEqualToAnchor:self.view.trailingAnchor]
      .active = YES;
  [[_toolbarController view].topAnchor
      constraintEqualToAnchor:self.view.topAnchor]
      .active = YES;
  [_toolbarController heightConstraint].constant =
      ToolbarHeightWithTopOfScreenOffset([_toolbarController statusBarOffset]);
  [_toolbarController heightConstraint].active = YES;

  [self updateToolbarAppearanceWithAnimation:NO];

  InstallBackgroundInView(_backgroundView);

  UIEdgeInsets contentInsets = UIEdgeInsetsMake(
      toolbarFrame.size.height - kVerticalToolbarOverlap, 0.0, 0.0, 0.0);
  CGRect scrollViewFrame =
      UIEdgeInsetsInsetRect(self.view.bounds, contentInsets);
  _scrollView = [[UIScrollView alloc] initWithFrame:scrollViewFrame];

  if (IsIPhoneX()) {
    if (@available(iOS 11, *)) {
      // The safe area adds a content inset which negatively impacts the stack
      // view opening animation after a rotation.
      // TODO(crbug.com/768868): Figure out what is going on, and whether
      // changing the contentInsetAdjustmentBehavior is the right fix.
      _scrollView.contentInsetAdjustmentBehavior =
          UIScrollViewContentInsetAdjustmentNever;
    }
  }
  [self.view addSubview:_scrollView];

  [_scrollView setTranslatesAutoresizingMaskIntoConstraints:NO];
  [NSLayoutConstraint activateConstraints:@[
    [_scrollView.topAnchor
        constraintEqualToAnchor:[_toolbarController view].bottomAnchor],
    [_scrollView.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor],
    [_scrollView.trailingAnchor
        constraintEqualToAnchor:self.view.trailingAnchor],
    [_scrollView.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor]
  ]];

  [_scrollView setBounces:NO];
  [_scrollView setScrollsToTop:NO];
  [_scrollView setClipsToBounds:NO];
  [_scrollView setShowsVerticalScrollIndicator:NO];
  [_scrollView setShowsHorizontalScrollIndicator:NO];
  [_scrollView setDelegate:self];

  _scrollGestureRecognizer = [_scrollView panGestureRecognizer];

  [self prepareForDisplay];
}

- (void)viewSafeAreaInsetsDidChange {
  [super viewSafeAreaInsetsDidChange];
  [_toolbarController heightConstraint].constant =
      ToolbarHeightWithTopOfScreenOffset([_toolbarController statusBarOffset]);
  [[_toolbarController view] setNeedsLayout];
}

- (void)viewWillAppear:(BOOL)animated {
  _isActive = YES;
  _preparingForActive = NO;
  // Sizing steps need to be done here rather than viewDidLoad since they
  // depend on the view bounds being correct. Setting initial card size should
  // be done only once, however, and viewWillAppear: can be called more than
  // once. For initial display, the transition animation will handle initial
  // layout. Avoid doing it here since that will potentially cause more views
  // to be added to the hierarchy synchronously, slowing down initial load.  The
  // rest of the time refreshing is necessary because the card views may have
  // been purged and recreated or the orientation might have changed while in
  // a modal view.
  if ([self needsInitialDisplay]) {
    _needsInitialDisplay = NO;

    // Calls like -viewportSizeWasChanged should instead be called in
    // viewDidLayoutSubviews, but since stack_view_controller is going away in
    // the near future, it's easier to put this here instead of refactoring.
    [self.view layoutIfNeeded];

    // If cards haven't been sized yet, size them now.
    if (_initialCardSize.height == 0.0) {
      DCHECK(!TabSwitcherPresentsBVCEnabled());
      [self setInitialCardSizingForSize:_scrollView.bounds.size];
    } else {
      DCHECK(TabSwitcherPresentsBVCEnabled());
    }

    [_mainCardSet setObserver:self];
    [_otrCardSet setObserver:self];
    [self configureCardSets];
    [self viewportSizeWasChanged];
  } else {
    [self refreshCardDisplayWithAnimation:NO];
    [self updateToolbarAppearanceWithAnimation:NO];
  }
  [self preloadCardViewsAsynchronously];

  // Reset the gesture state tracker to clear gesture-related information from
  // the last time the stack view was shown.
  _gestureStateTracker = [[GestureStateTracker alloc] init];

  [super viewWillAppear:animated];
}

- (void)refreshCardDisplayWithAnimation:(BOOL)animates {
  _lastInterfaceOrientation = GetInterfaceOrientation();
  [self updateDeckOrientationWithAnimation:animates];
  [self viewportSizeWasChanged];
  [_mainCardSet updateCardVisibilities];
  [_otrCardSet updateCardVisibilities];
}

- (void)viewDidDisappear:(BOOL)animated {
  if (![self presentedViewController]) {
    // Stop pre-loading card views if the stack view has been dismissed.
    [NSObject cancelPreviousPerformRequestsWithTarget:self];
    _isActive = NO;
    [self clearInternalState];
  }
  [self.dispatcher dismissToolsMenu];
  [super viewDidDisappear:animated];
}

- (void)dealloc {
  [_mainCardSet clearGestureRecognizerTargetAndDelegateFromCards:self];
  [_otrCardSet clearGestureRecognizerTargetAndDelegateFromCards:self];
  // Card sets shouldn't have any other references, but nil the observer just
  // in case one somehow does end up with another ref.
  [_mainCardSet setObserver:nil];
  [_otrCardSet setObserver:nil];
  [self cleanUpViewsAndNotifications];
}

// Overridden to always return NO, ensuring that the status bar shows in
// landscape on iOS8.
- (BOOL)prefersStatusBarHidden {
  return NO;
}

// Called when in the foreground and the OS needs more memory. Release as much
// as possible.
- (void)didReceiveMemoryWarning {
  // Releases the view if it doesn't have a superview.
  [super didReceiveMemoryWarning];
  _receivedMemoryWarningInSession = YES;
  [_mainCardSet setKeepOnlyVisibleCardViewsAlive:YES];
  [_otrCardSet setKeepOnlyVisibleCardViewsAlive:YES];

  if (![self isViewLoaded]) {
    [self cleanUpViewsAndNotifications];
  }
}

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orient
                                duration:(NSTimeInterval)duration {
  [super willRotateToInterfaceOrientation:orient duration:duration];
  // No animation is performed on rotation if the view is not on screen.
  if (!_isActive)
    return;

  // Hide the inactive set. NOTE: Ideally this hiding would be done as a
  // sliding-off-the-screen animation during the first half of the rotation
  // animation. However, integrating that custom animation with the default
  // animation that is being done to the cards on rotation has proved
  // challenging. For now, the inactive set is invisible during the rotation
  // itself.
  [[[self inactiveCardSet] displayView] setHidden:YES];
}

- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)orient
                                         duration:(NSTimeInterval)duration {
  [super willAnimateRotationToInterfaceOrientation:orient duration:duration];

  if (orient == _lastInterfaceOrientation)
    return;

  // If the stack view controller is not actually active, internal state will
  // not necessarily be consistent, and the animations could crash as a result.
  // Short-circuit out in this case (which should happen only in rare race
  // conditions involving the device being rotated as stack view is
  // entered/exited).
  if (!_isActive) {
    _lastInterfaceOrientation = orient;
    return;
  }

  [self updateToolbarAppearanceWithAnimation:YES];
  [self.dispatcher dismissToolsMenu];
  [self refreshCardDisplayWithAnimation:YES];

  // Animate the update of the card tabs.
  CGFloat halfOfTotalDuration = duration / 2.0;
  void (^cardTabFadeIn)(void) = ^{
    // Update the card tabs to their new positions instantaneously and then
    // fade them back in.
    [self animateActiveSetCardTabsToOpacity:1.0
                               withDuration:halfOfTotalDuration
                                 completion:nil];
  };
  [self animateActiveSetCardTabsToOpacity:0.0
                             withDuration:halfOfTotalDuration
                               completion:cardTabFadeIn];

  [_gestureStateTracker setResetScrollCardOnNextDrag:YES];
  [_gestureStateTracker setFirstPinchCardIndex:NSNotFound];
  [_gestureStateTracker setSecondPinchCardIndex:NSNotFound];
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)orient {
  [super didRotateFromInterfaceOrientation:orient];
  // No animation is performed on rotation if the view is not on screen.
  if (!_isActive)
    return;

  // Animate the inactive card set sliding in. NOTE: Ideally this animation
  // would be done during the second half of the rotation animation. However,
  // integrating this animation and the default animation that is being done to
  // the cards on rotation has proved challenging. For now, the inactive set is
  // invisible during the rotation itself.
  CardSet* inactiveSet = [self inactiveCardSet];
  [self updateDeckAxisPositionForCardSet:inactiveSet
                         withShiftAmount:
                             [self shiftOffscreenAmountForCardSet:inactiveSet]];
  [[inactiveSet displayView] setHidden:NO];
  [UIView animateWithDuration:kDefaultAnimationDuration
                        delay:0
                      options:0
                   animations:^{
                     [self updateDeckAxisPositionForCardSet:inactiveSet
                                            withShiftAmount:0];
                   }
                   completion:nil];
}

- (void)prepareForDismissal {
  UIView* activeView = [_activeCardSet displayView];
  [activeView removeGestureRecognizer:_pinchRecognizer];
  [activeView removeGestureRecognizer:_modeSwitchRecognizer];
  [activeView removeGestureRecognizer:_swipeDismissesCardRecognizer];
  [[self view] removeGestureRecognizer:_swipeGestureRecognizer];
  [_mainCardSet clearGestureRecognizerTargetAndDelegateFromCards:self];
  [_otrCardSet clearGestureRecognizerTargetAndDelegateFromCards:self];
  [_scrollView setDelegate:nil];
  [_scrollView setScrollEnabled:YES];
  _ignoreScrollCallbacks = NO;

  // Record per-session metrics.
  UMA_HISTOGRAM_BOOLEAN("MemoryWarning.OccurredDuringCardStackSession",
                        _receivedMemoryWarningInSession);
}

- (void)cleanUpViewsAndNotifications {
  [_mainCardSet setDisplayView:nil];
  [_otrCardSet setDisplayView:nil];
  // Stop pre-loading cards.
  [NSObject cancelPreviousPerformRequestsWithTarget:self];
  [_scrollView setDelegate:nil];
  _scrollView = nil;
  _backgroundView = nil;
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (UIStatusBarStyle)preferredStatusBarStyle {
  // When dismissing the stack view, the status bar's style is updated when this
  // view controller is still responsible.  If the stack view is dismissing into
  // a non-incognito BVC, the status bar needs to use the default style.
  BOOL useDefaultStyle = _isBeingDismissed && ![self isCurrentSetIncognito];
  return useDefaultStyle ? UIStatusBarStyleDefault
                         : UIStatusBarStyleLightContent;
}

- (void)prepareForDisplayAtSize:(CGSize)size {
  DCHECK(TabSwitcherPresentsBVCEnabled());
  _needsInitialDisplay = YES;
  [self setInitialCardSizingForSize:size];
}

#pragma mark -
#pragma mark Card and Stack Construction

- (void)preloadCardViewsAsynchronously {
  // Start the deferred loading of card views. Defers the pre-loading slightly
  // in order to give the initially visible cards a head start.
  [NSObject cancelPreviousPerformRequestsWithTarget:self];
  [self performSelector:@selector(preloadNextCardView)
             withObject:nil
             afterDelay:0.01];
}

- (void)preloadNextCardView {
  if (_isBeingDismissed)
    return;
  // Preload one card from the active set, or if that's already loaded, from
  // the other set.
  BOOL preloadedCard = [_activeCardSet preloadNextCard];
  if (!preloadedCard)
    preloadedCard = [[self inactiveCardSet] preloadNextCard];
  // If there was a card to preload, queue the next round.
  if (preloadedCard) {
    [self performSelector:@selector(preloadNextCardView)
               withObject:nil
               afterDelay:0];
  } else {
    [_testDelegate stackViewControllerPreloadCardViewsDidEnd];
  }
}

- (void)animateOutCardView:(CardView*)cardView
                     delay:(NSTimeInterval)delay
                 clockwise:(BOOL)clockwise
                completion:(ProceduralBlock)completion {
  DCHECK(cardView);
  void (^toDoWhenDone)(void) = ^{
    [cardView removeFromSuperview];
    if (completion)
      completion();
  };
  BOOL isPortrait = UIInterfaceOrientationIsPortrait(_lastInterfaceOrientation);
  page_animation_util::AnimateOutWithCompletion(cardView, delay, clockwise,
                                                isPortrait, toDoWhenDone);
}

- (void)removeAllCardsFromSet:(CardSet*)cardSet {
  // Ignore model updates while the cards are closing, to batch all the
  // re-laying-out work.
  [cardSet setIgnoresTabModelChanges:YES];

  NSTimeInterval delay = 0;
  NSArray* cards = [cardSet cards];
  BOOL isPortrait = UIInterfaceOrientationIsPortrait(_lastInterfaceOrientation);

  // Find the last visible card.
  StackCard* lastVisibleCard = nil;
  for (StackCard* card in [cards reverseObjectEnumerator]) {
    if ([card viewIsLive]) {
      lastVisibleCard = card;
      break;
    }
  }
  if (lastVisibleCard == nil) {
    [cardSet.tabModel closeAllTabs];
    return;
  }

  for (StackCard* card in cards) {
    NSInteger cardIndex = [cards indexOfObject:card];
    DCHECK(cardIndex != NSNotFound);
    BOOL cardWasCollapsed = [cardSet cardIsCollapsed:card];

    if ([card viewIsLive]) {
      void (^toDoWhenDone)(void) = NULL;
      if (card == lastVisibleCard) {
        toDoWhenDone = ^{
          [cardSet setIgnoresTabModelChanges:NO];
          [cardSet.tabModel closeAllTabs];
        };
      }
      [self animateOutCardView:card.view
                         delay:delay
                     clockwise:isPortrait
                    completion:toDoWhenDone];
    } else {
      // It's too late to create a view for this card now. This case should only
      // occur if the card was covered, meaning that its animation out would
      // have been invisible anyway.
      DCHECK(cardWasCollapsed);
    }

    // Add a delay before the next card's animation if this card was not
    // collapsed into the next card.
    if (!cardWasCollapsed)
      delay += kCascadingCardCloseDelay;
  }
}

- (void)disableGestureHandlers {
  // Disable gesture handlers before modifying the stack. Don't call this too
  // late or a gesture callback could occur while still in the old state of the
  // world.
  // (see the comment in -cardSet:willRemoveCard:atIndex for details).
  [_scrollView setScrollEnabled:NO];
  _pinchRecognizer.enabled = NO;
  _swipeGestureRecognizer.enabled = NO;
}

- (void)enableGestureHandlers {
  // Reenable gesture handlers after modifying the stack. Don't call this too
  // early or a gesture callback could occur while still in the old state of the
  // world.
  // (see the comment in -cardSet:willRemoveCard:atIndex for details).
  [_scrollView setScrollEnabled:YES];
  _pinchRecognizer.enabled = YES;
  _swipeGestureRecognizer.enabled = YES;
}

- (void)activeCardCountChanged {
  // Cancel any outstanding gestures (see the comment in
  // -cardSet:willRemoveCard:atIndex).
  [self disableGestureHandlers];
  [self enableGestureHandlers];
}

- (void)setInitialCardSizingForSize:(CGSize)size {
  DCHECK(_initialCardSize.height == 0.0);
  CGFloat viewportBreadth = [self scrollBreadth:size];
  _initialCardSize =
      [self cardSizeForBreadth:viewportBreadth scrollViewSize:size];
}

- (void)configureCardSets {
  // Configure the stack layout behaviors. This is done only once because the
  // fan-out, margins, etc. should stay the same even if the cards change size
  // due to rotation.
  [self updateDeckOrientationWithAnimation:NO];
  [_mainCardSet
      configureLayoutParametersWithMargin:page_animation_util::kCardMargin];
  [_otrCardSet
      configureLayoutParametersWithMargin:page_animation_util::kCardMargin];
}

- (BOOL)needsInitialDisplay {
  if (TabSwitcherPresentsBVCEnabled()) {
    return _needsInitialDisplay;
  } else {
    return _initialCardSize.height == 0.0;
  }
}

- (void)updateDeckOrientationWithAnimation:(BOOL)animates {
  [self updateDeckAxisPositions];
  [self updateCardSizesWithAnimation:animates];
}

- (void)updateCardSizesWithAnimation:(BOOL)animates {
  CGSize cardSize = [self cardSize];
  NSTimeInterval animationDuration = animates ? kDefaultAnimationDuration : 0;
  [UIView animateWithDuration:animationDuration
                        delay:0
                      options:UIViewAnimationOptionBeginFromCurrentState
                   animations:^{
                     [_mainCardSet setCardSize:cardSize];
                     [_otrCardSet setCardSize:cardSize];
                   }
                   completion:nil];
}

- (void)animateActiveSetCardTabsToOpacity:(CGFloat)opacity
                             withDuration:(CGFloat)duration
                               completion:(ProceduralBlock)completion {
  [UIView animateWithDuration:duration
      delay:0
      options:(UIViewAnimationOptionBeginFromCurrentState |
               UIViewAnimationOptionOverrideInheritedDuration)
      animations:^{
        for (StackCard* card in [_activeCardSet cards]) {
          if (card.viewIsLive)
            [card.view setTabOpacity:opacity];
        }
      }
      completion:^(BOOL) {
        if (completion)
          completion();
      }];
}

- (void)updateDeckAxisPositions {
  [self updateDeckAxisPositionsWithShiftAmount:0];
}

- (void)updateDeckAxisPositionsWithShiftAmount:(CGFloat)shiftAmount {
  [self updateDeckAxisPositionForCardSet:_activeCardSet
                         withShiftAmount:shiftAmount];
  [self updateDeckAxisPositionForCardSet:[self inactiveCardSet]
                         withShiftAmount:shiftAmount];
}

- (void)updateDeckAxisPositionForCardSet:(CardSet*)cardSet
                         withShiftAmount:(CGFloat)shiftAmount {
  // Skip axis layout if card size hasn't been set up yet; it will be handled
  // when card size is.
  if (_initialCardSize.height == 0.0)
    return;

  if (!cardSet)
    return;

  CGFloat viewportBreadth =
      [self scrollBreadth:[_mainCardSet displayView].bounds.size];
  CGFloat fullDisplayBreadth =
      [self bothDecksShouldBeDisplayed]
          ? (viewportBreadth * kActiveDeckDisplayFraction)
          : viewportBreadth;

  CGFloat center = (fullDisplayBreadth / 2.0);
  if ([self isCurrentSetIncognito])
    center += viewportBreadth - fullDisplayBreadth;
  // Adjust the set's center if it's not the active card set.
  if (cardSet != _activeCardSet) {
    CGFloat inactiveSetDelta =
        fullDisplayBreadth - page_animation_util::kCardMargin + kCardFrameInset;
    center = [self isCurrentSetIncognito] ? center - inactiveSetDelta
                                          : center + inactiveSetDelta;
  }
  center += shiftAmount;

  BOOL isPortrait = UIInterfaceOrientationIsPortrait(_lastInterfaceOrientation);
  [cardSet setLayoutAxisPosition:center isVertical:isPortrait];
}

- (CGFloat)shiftOffscreenAmountForCardSet:(CardSet*)cardSet {
  if (!cardSet)
    return 0;

  CGFloat viewportBreadth =
      [self scrollBreadth:[_activeCardSet displayView].bounds.size];
  // The incognito card set moves offscreen to the right; the main card set
  // moves offscreen to the left.
  CGFloat offset = (1 - kActiveDeckDisplayFraction) * viewportBreadth;
  if (cardSet == _mainCardSet)
    offset = -offset;
  return offset;
}

- (BOOL)bothDecksShouldBeDisplayed {
  return [[_otrCardSet cards] count] > 0;
}

#pragma mark -
#pragma mark Current Set Handling

- (BOOL)isCurrentSetIncognito {
  return _activeCardSet == _otrCardSet;
}

- (CardSet*)inactiveCardSet {
  return [self isCurrentSetIncognito] ? _mainCardSet : _otrCardSet;
}

- (void)setActiveCardSet:(CardSet*)cardSet {
  DCHECK(cardSet);
  if (cardSet == _activeCardSet)
    return;
  [self activeCardCountChanged];
  _activeCardSet = cardSet;

  [self displayActiveCardSet];
}

- (void)displayActiveCardSet {
  UIView* activeView = [_activeCardSet displayView];
  DCHECK(activeView);
  UIView* inactiveView = [[self inactiveCardSet] displayView];

  [_scrollView bringSubviewToFront:activeView];

  // |_swipeGestureRecognizer| is added to the main SVC view, so don't add or
  // remove that here.
  // TODO(blundell): Figure out which recognizers need to be associated with the
  // active display view and which can just be on the superview.
  [inactiveView removeGestureRecognizer:_pinchRecognizer];
  [inactiveView removeGestureRecognizer:_modeSwitchRecognizer];
  [inactiveView removeGestureRecognizer:_swipeDismissesCardRecognizer];
  [activeView addGestureRecognizer:_pinchRecognizer];
  [activeView addGestureRecognizer:_modeSwitchRecognizer];
  [activeView addGestureRecognizer:_swipeDismissesCardRecognizer];

  activeView.accessibilityElementsHidden = NO;
  inactiveView.accessibilityElementsHidden = YES;

  _inActiveDeckChangeAnimation = YES;  // This flag is used for testing.
  [UIView animateWithDuration:kDefaultAnimationDuration
      delay:0
      options:UIViewAnimationOptionBeginFromCurrentState
      animations:^{
        [self updateDeckAxisPositions];
      }
      completion:^(BOOL finished) {
        _inActiveDeckChangeAnimation = NO;
      }];

  [self updateToolbarAppearanceWithAnimation:YES];
}

- (void)displayMainCardSetOnly {
  [self updateCardSizesWithAnimation:YES];
  if ([self isCurrentSetIncognito]) {
    [self setActiveCardSet:[self inactiveCardSet]];
  } else {
    // Ensure that layout axis position is up to date.
    [self displayActiveCardSet];
  }
}

- (void)updateToolbarAppearanceWithAnimation:(BOOL)animate {
  [_toolbarController setTabCount:[_activeCardSet.cards count]];
  [[_toolbarController openNewTabButton]
      setIncognito:[self isCurrentSetIncognito]
          animated:animate];

  // Position the toolbar above/below the cards depending on the current state
  // of the cards and the device. In landscape with multiple stacks, the cards
  // must be behind the toolbar to avoid covering it. In all other cases, the
  // cards are positioned to go in front of the toolbar.
  BOOL toolbarShouldHaveBackground = NO;
  if (([[_otrCardSet cards] count] > 0) && IsLandscape())
    toolbarShouldHaveBackground = YES;

  NSUInteger scrollViewIndex =
      [[[self view] subviews] indexOfObject:_scrollView];
  NSUInteger toolbarViewIndex =
      [[[self view] subviews] indexOfObject:[_toolbarController view]];
  BOOL toolbarInFrontOfScrollView = (toolbarViewIndex > scrollViewIndex);

  // If moving the toolbar to the front, have it cover the cards before any
  // animation of the background starts to occur, as this looks cleanest.
  if (toolbarShouldHaveBackground && !toolbarInFrontOfScrollView) {
    [[self view] exchangeSubviewAtIndex:scrollViewIndex
                     withSubviewAtIndex:toolbarViewIndex];
  }

  void (^updateToolbar)(void) = ^{
    CGFloat alpha = toolbarShouldHaveBackground ? 1.0 : 0.0;
    [_toolbarController backgroundView].alpha = alpha;
    [_toolbarController shadowView].alpha = alpha;
  };

  // If moving the toolbar to the back, have the cards move forward only after
  // the toolbar background finishes disappearing, as this looks cleanest.
  void (^toDoWhenDone)(void) = ^{
    if (!toolbarShouldHaveBackground && toolbarInFrontOfScrollView) {
      [[self view] exchangeSubviewAtIndex:scrollViewIndex
                       withSubviewAtIndex:toolbarViewIndex];
      [_scrollView setClipsToBounds:NO];
    }
  };

  if (animate) {
    [UIView animateWithDuration:kDefaultAnimationDuration
        delay:0
        options:UIViewAnimationOptionBeginFromCurrentState
        animations:^{
          updateToolbar();
        }
        completion:^(BOOL finished) {
          toDoWhenDone();
        }];
  } else {
    updateToolbar();
    toDoWhenDone();
  }
}

#pragma mark -
#pragma mark Sizing/Measuring Helpers

- (CGSize)cardSize {
  DCHECK(_initialCardSize.height != 0.0);
  CGFloat availableBreadth = [self scrollBreadth:[_scrollView bounds].size];
  if ([self bothDecksShouldBeDisplayed])
    availableBreadth *= kActiveDeckDisplayFraction;
  CGSize idealCardSize = [self cardSizeForBreadth:availableBreadth
                                   scrollViewSize:[_scrollView bounds].size];

  // Crop the ideal size so that it's no bigger than the initial size.
  return CGSizeMake(std::min(idealCardSize.width, _initialCardSize.width),
                    std::min(idealCardSize.height, _initialCardSize.height));
}

- (CGSize)cardSizeForBreadth:(CGFloat)breadth scrollViewSize:(CGSize)viewSize {
  BOOL isPortrait = IsPortrait();
  CGFloat cardBreadth = breadth - 2 * page_animation_util::kCardMargin;
  CGFloat contentBreadthInset =
      isPortrait ? kCardImageInsets.left + kCardImageInsets.right
                 : kCardImageInsets.top + kCardImageInsets.bottom;
  CGFloat contentBreadth = cardBreadth - contentBreadthInset;
  CGFloat aspectRatio =
      [self scrollLength:viewSize] / [self scrollBreadth:viewSize];
  CGFloat contentLength = std::floor(aspectRatio * contentBreadth);
  CGFloat contentLengthInset =
      isPortrait ? kCardImageInsets.top + kCardImageInsets.bottom
                 : kCardImageInsets.left + kCardImageInsets.right;
  CGFloat cardLength = contentLength + contentLengthInset;
  // Truncate the card length so that the entire card can be visible at once.
  CGFloat viewLength = isPortrait ? viewSize.height : viewSize.width;
  CGFloat truncatedCardLength =
      viewLength - page_animation_util::kCardMargin - kCardBottomPadding;
  cardLength = std::min(cardLength, truncatedCardLength);
  return [self sizeForScrollLength:cardLength breadth:cardBreadth];
}

- (CGFloat)scrollOffsetAmountForPoint:(CGPoint)point {
  return IsPortrait() ? point.y : point.x;
}

- (CGFloat)scrollOffsetAmountForPosition:(LayoutRectPosition)position {
  return IsPortrait() ? position.originY : position.leading;
}

- (CGPoint)scrollOffsetPointWithAmount:(CGFloat)offset {
  return IsPortrait() ? CGPointMake(0, offset) : CGPointMake(offset, 0);
}

- (CGFloat)scrollLength:(CGSize)size {
  return IsPortrait() ? size.height : size.width;
}

- (CGFloat)scrollBreadth:(CGSize)size {
  return IsPortrait() ? size.width : size.height;
}

- (CGSize)sizeForScrollLength:(CGFloat)length breadth:(CGFloat)breadth {
  return IsPortrait() ? CGSizeMake(breadth, length)
                      : CGSizeMake(length, breadth);
}

- (CGRect)inactiveDeckRegion {
  // If only one deck is showing, there's no inactive deck region.
  if (![self bothDecksShouldBeDisplayed])
    return CGRectZero;

  CGSize viewportSize = [_activeCardSet displayView].frame.size;
  CGFloat viewportBreadth = [self scrollBreadth:viewportSize];
  CGFloat inactiveBreadth = (1 - kActiveDeckDisplayFraction) * viewportBreadth;
  CGSize regionSize = [self sizeForScrollLength:[self scrollLength:viewportSize]
                                        breadth:inactiveBreadth];
  CGPoint regionOrigin = [_scrollView contentOffset];
  if (IsPortrait()) {
    BOOL inactiveOnRight = UseRTLLayout() == [self isCurrentSetIncognito];
    if (inactiveOnRight)
      regionOrigin.x = viewportBreadth - regionSize.width;
  } else {
    BOOL inactiveOnBottom = ![self isCurrentSetIncognito];
    if (inactiveOnBottom)
      regionOrigin.y = viewportBreadth - regionSize.height;
  }
  return {regionOrigin, regionSize};
}

- (NSUInteger)indexOfCardAtPoint:(CGPoint)point {
  UIView* view = [_activeCardSet.displayView hitTest:point withEvent:nil];
  while (view && ![view isKindOfClass:[CardView class]]) {
    view = [view superview];
  }
  if (!view)
    return NSNotFound;
  StackCard* card = [self cardForView:(CardView*)view];
  return [_activeCardSet.cards indexOfObject:card];
}

#pragma mark -
#pragma mark Stack View Transition Helpers

// Determine what should be the first visible card. Preference is to start one
// card before the current card so that the current card ends up in the middle
// of the visible cards. However, if the current card is the last in a stack of
// > 2 cards, start two cards before so that the screen is fully populated (and
// if the current card is the first card, the only option is to start with the
// first card).
- (NSUInteger)startIndexOfInitialFanoutForCardSet:(CardSet*)cardSet {
  if ([[cardSet cards] count] == 0)
    return 0;
  NSUInteger currentCardIndex =
      [cardSet.tabModel indexOfTab:cardSet.tabModel.currentTab];
  NSUInteger startingCardIndex =
      (currentCardIndex == 0) ? 0 : currentCardIndex - 1;
  if ((currentCardIndex > 1) &&
      (currentCardIndex == ([cardSet.tabModel count] - 1)))
    startingCardIndex -= 1;
  return startingCardIndex;
}

- (void)showWithSelectedTabAnimation {
  [self animateTransitionWithStyle:STACK_TRANSITION_STYLE_PRESENTING];

  [_testDelegate stackViewControllerShowWithSelectedTabAnimationDidStart];

  [_activeCardSet.currentCard setIsActiveTab:YES];

  // When in accessbility mode, fan out cards from the start, announce open tabs
  // and move the VoiceOver cursor to the New Tab button. Fanning out the cards
  // from the start eliminates the screen change that would otherwise occur when
  // moving the VoiceOver cursor from the Show Tabs button to the card stack.
  if (UIAccessibilityIsVoiceOverRunning()) {
    [_activeCardSet fanOutCardsWithStartIndex:0];
    [self postOpenTabsAccessibilityNotification];
    UIAccessibilityPostNotification(UIAccessibilityScreenChangedNotification,
                                    _toolbarController.view);
  }
}

- (void)cancelTransitionAnimation {
  // Set up transaction.
  [CATransaction begin];
  [CATransaction setCompletionBlock:^{
    [self finishTransitionAnimation];
  }];
  self.transitionWasCancelled = YES;
  // Reverse all the animations.
  [self reverseDummyToolbarBackgroundViewAnimation];
  [self reverseTransitionAnimationsForCardSet:_activeCardSet];
  [self reverseTransitionAnimationsForCardSet:[self inactiveCardSet]];
    if (self.transitionToolbarSnapshot) {
      ReverseAnimationsForKeyForLayers(kTransitionToolbarAnimationKey, @[
        self.transitionToolbarSnapshot.layer,
        self.transitionToolbarSnapshot.layer.mask
      ]);
    }
  // Commit the transaction.  Since the animations added for the previous
  // transition are all removed, this commit will call the previous
  // animation's completion block.
  [CATransaction commit];
}

- (void)finishTransitionAnimation {
  // Early return if cancelled.
  if (self.transitionWasCancelled) {
    // Notify the delegates.
    [self notifyDelegatesTransitionFinished];
    // When transitions are cancelled, reverse the transition style so that the
    // new completion block sends the correct delegate methods.
    self.transitionStyle =
        self.transitionStyle == STACK_TRANSITION_STYLE_PRESENTING
            ? STACK_TRANSITION_STYLE_DISMISSING
            : STACK_TRANSITION_STYLE_PRESENTING;
    self.transitionWasCancelled = NO;
    return;
  }
  // Clean up card view animations.
  for (StackCard* card in _activeCardSet.cards) {
    if ([card viewIsLive])
      [card.view cleanUpAnimations];
  }
  for (StackCard* card in [self inactiveCardSet].cards) {
    if ([card viewIsLive])
      [card.view cleanUpAnimations];
  }
  // Clean up toolbar animations.
    [self.transitionToolbarSnapshot removeFromSuperview];
    self.transitionToolbarSnapshot = nil;
  self.transitionToolbarOwner = nil;
  // Clean up dummy toolbar background.
  [self.dummyToolbarBackgroundView removeFromSuperview];
  self.dummyToolbarBackgroundView = nil;
  // Notify the delegates.
  [self notifyDelegatesTransitionFinished];
  // Reset the current transition style.
  StackTransitionStyle transitionStyleAtFinish = self.transitionStyle;
  self.transitionStyle = STACK_TRANSITION_STYLE_NONE;
  // Restore the original subview ordering.
  [self reorderSubviewsForTransition];
  // Dismiss immediately if a card was selected mid-presentation.
  if (self.transitionTappedCard) {
    _activeCardSet.currentCard = self.transitionTappedCard;
    self.transitionTappedCard = nil;
    [self dismissWithSelectedTabAnimation];
  }

  if (transitionStyleAtFinish == STACK_TRANSITION_STYLE_DISMISSING) {
    // Dismissal is complete and delegate was told that stack view has been
    // dismissed. Make sure that internal state reflects that.
    _isActive = NO;
    [self clearInternalState];
  }
}

- (void)notifyDelegatesTransitionFinished {
  // Notify delegates.
  DCHECK_NE(self.transitionStyle, STACK_TRANSITION_STYLE_NONE);
  if (self.transitionStyle == STACK_TRANSITION_STYLE_PRESENTING) {
    [_testDelegate stackViewControllerShowWithSelectedTabAnimationDidEnd];
    [_animationDelegate tabSwitcherPresentationAnimationDidEnd:self];
  } else {
    [_animationDelegate tabSwitcherDismissalAnimationDidEnd:self];
    [_delegate tabSwitcherDismissTransitionDidEnd:self];
  }
}

- (void)animateTransitionWithStyle:(StackTransitionStyle)transitionStyle {
  // If the dummy toolbar background view is instantiated, reverse the current
  // transition animations.
  if (self.dummyToolbarBackgroundView) {
    [self cancelTransitionAnimation];
    return;
  }

  // The transition style must be specified.
  DCHECK_NE(transitionStyle, STACK_TRANSITION_STYLE_NONE);
  self.transitionStyle = transitionStyle;
  BOOL isPresenting = self.transitionStyle == STACK_TRANSITION_STYLE_PRESENTING;

  // Get reference to toolbar for transition.
  self.transitionToolbarOwner = [_delegate tabSwitcherTransitionToolbarOwner];

  // Create dummy toolbar background view.
  self.dummyToolbarBackgroundView = [[UIView alloc] initWithFrame:CGRectZero];
  [self.dummyToolbarBackgroundView setClipsToBounds:YES];

  if (TabSwitcherPresentsBVCEnabled()) {
    if (!CGSizeEqualToSize(self.view.frame.size,
                           self.view.superview.bounds.size)) {
      // Forcibly mark the view as needing layout if it is a different size from
      // its superview.
      [self.view setNeedsLayout];
    }

    // Forces a layout because the views may not yet be sized and positioned
    // correctly for their initial layout.
    [self.view layoutIfNeeded];
    [self refreshCardDisplayWithAnimation:NO];
    [self updateToolbarAppearanceWithAnimation:NO];
  }

  // Set the transition completion block.
  [CATransaction begin];
  [CATransaction setCompletionBlock:^{
    [self finishTransitionAnimation];
  }];

  // Slide in/out the inactive card set.
  [self animateInactiveSetTransition];

  // The current card's frame is necessary for the toolbar animation below.  For
  // dismissals, the toolbar animates from the card's current frame (i.e. the
  // frame before the animation is added).  For presentation, the toolbar
  // animates to the final frame (i.e. the frame after the animation is added).
  LayoutRect currentCardLayout = _activeCardSet.currentCard.layout;
  [self animateActiveSetTransition];
  if (isPresenting)
    currentCardLayout = _activeCardSet.currentCard.layout;
  CGRect currentCardFrame =
      AlignRectOriginAndSizeToPixels(LayoutRectGetRect(currentCardLayout));

  // Forces a layout because the views may not yet be positioned correctly
  // due to a screen rotation.
  [self.view layoutIfNeeded];

  // Animate the dummy toolbar background view.
  [self animateDummyToolbarForCardFrame:currentCardFrame
                        transitionStyle:transitionStyle];

  //  Animate the transition toolbar.
    [self animateTransitionToolbarSnapshotWithCardFrame:currentCardFrame
                                        transitionStyle:transitionStyle];


  // Update the order of the view hierarchy.
  [self reorderSubviewsForTransition];

  [CATransaction commit];
}

- (void)reorderSubviewsForTransition {
  if (self.transitionStyle != STACK_TRANSITION_STYLE_NONE) {
    // Add the card set display views to the main view and insert the toolbar
    // between them.
    [self.view addSubview:[self inactiveCardSet].displayView];
    [self inactiveCardSet].displayView.frame = [_scrollView frame];
    [self.view addSubview:_activeCardSet.displayView];
    _activeCardSet.displayView.frame = [_scrollView frame];
    [self.view insertSubview:[_toolbarController view]
                belowSubview:_activeCardSet.displayView];
  } else {
    // Add the display views back into the scroll view.
    [_scrollView addSubview:[self inactiveCardSet].displayView];
    [_scrollView addSubview:_activeCardSet.displayView];
    [self updateToolbarAppearanceWithAnimation:NO];
    [self alignDisplayViewsToViewport];
  }
}

- (void)animateCardSet:(CardSet*)cardSet
      fromBeginLayouts:(std::vector<LayoutRect>)beginLayouts
          toEndLayouts:(std::vector<LayoutRect>)endLayouts {
  NSUInteger cardCount = [cardSet.cards count];
  DCHECK_EQ(cardCount, beginLayouts.size());
  DCHECK_EQ(cardCount, endLayouts.size());

  [CATransaction begin];
  [CATransaction setDisableActions:YES];
  // Place cards into final position.
  for (NSUInteger i = 0; i < cardCount; ++i)
    [cardSet.cards[i] setLayout:endLayouts[i]];
  // For presentation, update visibilty so only cards that will ultimately be
  // shown are live.
  BOOL isPresenting = self.transitionStyle == STACK_TRANSITION_STYLE_PRESENTING;
  if (isPresenting)
    [cardSet updateCardVisibilities];
  [CATransaction commit];

  // Animate each card to its final frame.
  StackCard* currentCard = cardSet.currentCard;
  BOOL isActiveCardSet = (cardSet == _activeCardSet);
  for (NSUInteger i = 0; i < cardCount; ++i) {
    StackCard* card = cardSet.cards[i];
    if ([card viewIsLive]) {
      CardTabAnimationStyle tabAnimationStyle = CARD_TAB_ANIMATION_STYLE_NONE;
      if (isActiveCardSet && card == currentCard) {
        tabAnimationStyle = isPresenting ? CARD_TAB_ANIMATION_STYLE_FADE_IN
                                         : CARD_TAB_ANIMATION_STYLE_FADE_OUT;
      }
      [card.view animateFromBeginFrame:LayoutRectGetRect(beginLayouts[i])
                            toEndFrame:LayoutRectGetRect(endLayouts[i])
                     tabAnimationStyle:tabAnimationStyle];
    }
  }
}

- (void)reverseTransitionAnimationsForCardSet:(CardSet*)cardSet {
  for (StackCard* card in cardSet.cards) {
    if ([card viewIsLive])
      [card.view reverseAnimations];
  }
}

- (void)animateActiveSetTransition {
  // Early return for an empty active card set.
  if (![_activeCardSet.cards count])
    return;

  std::vector<LayoutRect> beginLayouts;
  std::vector<LayoutRect> endLayouts;
  BOOL isPresenting = self.transitionStyle == STACK_TRANSITION_STYLE_PRESENTING;
  if (isPresenting) {
    // For presentation, animate from transition frames to fan frames.
    NSUInteger activeSetStartIndex =
        [self startIndexOfInitialFanoutForCardSet:_activeCardSet];
    beginLayouts = [self cardTransitionLayouts];
    [_activeCardSet fanOutCardsWithStartIndex:activeSetStartIndex];
    endLayouts = [_activeCardSet cardLayouts];
  } else {
    // For dismissal, animate from the cards' current frames to the transition
    // frames.
    beginLayouts = [_activeCardSet cardLayouts];
    endLayouts = [self cardTransitionLayouts];
    // For dismissals, the status bar needs to be updated early.
    [self performSelector:@selector(setNeedsStatusBarAppearanceUpdate)
               withObject:nil
               afterDelay:kDismissalStatusBarUpdateDelay];
    // Ensure that the current card view is visible.
    _activeCardSet.currentCard.view.hidden = NO;
  }

  // Add animations.
  [self animateCardSet:_activeCardSet
      fromBeginLayouts:beginLayouts
          toEndLayouts:endLayouts];
}

- (void)animateInactiveSetTransition {
  // Early return for an emtpy inactive card set.
  CardSet* inactiveCardSet = [self inactiveCardSet];
  if (![[inactiveCardSet cards] count])
    return;

  BOOL isPresenting = self.transitionStyle == STACK_TRANSITION_STYLE_PRESENTING;

  // Calculate transition animation card frames
  if (isPresenting) {
    // For presentation, fan out the cards for the transition.  Otherwise, use
    // the current frames of the cards.
    NSUInteger inactiveSetStartIndex =
        [self startIndexOfInitialFanoutForCardSet:inactiveCardSet];
    [inactiveCardSet fanOutCardsWithStartIndex:inactiveSetStartIndex];
  }
  std::vector<LayoutRect> cardStackLayouts = [inactiveCardSet cardLayouts];
  BOOL isPortrait = UIInterfaceOrientationIsPortrait(_lastInterfaceOrientation);
  CGFloat shiftAmount = [self shiftOffscreenAmountForCardSet:inactiveCardSet];
  std::vector<LayoutRect> shiftedStackLayouts;
  for (const auto& cardLayout : cardStackLayouts) {
    LayoutRect shiftedLayout = cardLayout;
    if (isPortrait)
      shiftedLayout.position.leading += shiftAmount;
    else
      shiftedLayout.position.originY += shiftAmount;
    shiftedStackLayouts.push_back(shiftedLayout);
  }

  std::vector<LayoutRect> beginLayouts =
      isPresenting ? shiftedStackLayouts : cardStackLayouts;
  std::vector<LayoutRect> endLayouts =
      isPresenting ? cardStackLayouts : shiftedStackLayouts;

  // Add animations.
  [self animateCardSet:inactiveCardSet
      fromBeginLayouts:beginLayouts
          toEndLayouts:endLayouts];
}

- (void)animateDummyToolbarForCardFrame:(CGRect)cardFrame
                        transitionStyle:(StackTransitionStyle)transitionStyle {
  // Install the dummy toolbar background view into the card tab.
  CardView* cardView = _activeCardSet.currentCard.view;
  [cardView installDummyToolbarBackgroundView:self.dummyToolbarBackgroundView];

  // When calculating the frames below, convert them into the card's tab's
  // coordinate system, whose origin is at |cardTabOriginOffset| from the card's
  // frame origin.
  CGVector cardTabOriginOffset =
      CGVectorMake(kCardFrameInset, kCardTabTopInset);

  // The card's frame image extends beyond the edges of the screen when the
  // current card is scaled to the full content area, so extend the toolbar
  // background to match the card's width.
  UIView* toolbarView = [_toolbarController view];
  UIView* displayView = _activeCardSet.displayView;
  CGRect screenToolbarFrame = [displayView convertRect:toolbarView.frame
                                              fromView:toolbarView.superview];
  UIEdgeInsets screenToolbarFrameOutsets =
      UIEdgeInsetsMake(0.0, kCardFrameInset - kCardImageInsets.left, 0.0,
                       kCardFrameInset - kCardImageInsets.right);
  screenToolbarFrame =
      UIEdgeInsetsInsetRect(screenToolbarFrame, screenToolbarFrameOutsets);
  CGPoint screenCardOrigin =
      CGPointMake(displayView.bounds.origin.x - kCardImageInsets.left,
                  displayView.bounds.origin.y - kCardImageInsets.top);
  screenToolbarFrame = CGRectOffset(
      screenToolbarFrame, -(screenCardOrigin.x + cardTabOriginOffset.dx),
      -(screenCardOrigin.y + cardTabOriginOffset.dy));

  // The frame should interpolate to the frame of the card's tab view.
  CGRect cardToolbarFrame =
      CGRectInset(cardFrame, kCardFrameInset, kCardFrameInset);
  cardToolbarFrame.size.height = kCardImageInsets.top - kCardFrameInset;
  cardToolbarFrame = CGRectOffset(
      cardToolbarFrame, -(cardFrame.origin.x + cardTabOriginOffset.dx),
      -(cardFrame.origin.y + cardTabOriginOffset.dy));

  // Calculate colors for the crossfade.
  UIColor* cardBackgroundColor =
      [self isCurrentSetIncognito]
          ? [UIColor colorWithWhite:kCardFrameBackgroundBrightnessIncognito
                              alpha:1.0]
          : [UIColor colorWithWhite:kCardFrameBackgroundBrightness alpha:1.0];
  UIColor* toolbarBackgroundColor = cardBackgroundColor;
    UIColor* backgroundColor =
        [self.transitionToolbarOwner
                .toolbarSnapshotProvider toolbarBackgroundColor];
    if (backgroundColor) {
      toolbarBackgroundColor = backgroundColor;
    }

  // Create frame animation.
  CFTimeInterval duration = ios::material::kDuration1;
  CAMediaTimingFunction* timingFunction =
      ios::material::TimingFunction(ios::material::CurveEaseInOut);
  BOOL isPresentingStackView =
      (transitionStyle == STACK_TRANSITION_STYLE_PRESENTING);
  CGRect beginFrame =
      isPresentingStackView ? screenToolbarFrame : cardToolbarFrame;
  CGRect endFrame =
      isPresentingStackView ? cardToolbarFrame : screenToolbarFrame;
  CAAnimation* frameAnimation = FrameAnimationMake(
      self.dummyToolbarBackgroundView.layer, beginFrame, endFrame);
  frameAnimation.duration = duration;
  frameAnimation.timingFunction = timingFunction;

  // Create color animation.
  UIColor* beginColor =
      isPresentingStackView ? toolbarBackgroundColor : cardBackgroundColor;
  UIColor* endColor =
      isPresentingStackView ? cardBackgroundColor : toolbarBackgroundColor;
  CABasicAnimation* colorAnimation =
      [CABasicAnimation animationWithKeyPath:@"backgroundColor"];
  colorAnimation.fromValue = reinterpret_cast<id>(beginColor.CGColor);
  colorAnimation.toValue = reinterpret_cast<id>(endColor.CGColor);
  colorAnimation.fillMode = kCAFillModeBoth;
  colorAnimation.removedOnCompletion = NO;
  colorAnimation.duration = duration;
  colorAnimation.timingFunction = timingFunction;

  // Create corner radius animation.
  CGFloat toolbarCornerRadius = toolbarView.layer.cornerRadius;
  CGFloat beginCornerRadius =
      isPresentingStackView ? toolbarCornerRadius : kCardFrameCornerRadius;
  CGFloat endCornerRadius =
      isPresentingStackView ? kCardFrameCornerRadius : toolbarCornerRadius;
  CABasicAnimation* cornerRadiusAnimation =
      [CABasicAnimation animationWithKeyPath:@"cornerRadius"];
  cornerRadiusAnimation.fromValue = @(beginCornerRadius);
  cornerRadiusAnimation.toValue = @(endCornerRadius);
  cornerRadiusAnimation.fillMode = kCAFillModeBoth;
  cornerRadiusAnimation.removedOnCompletion = NO;
  cornerRadiusAnimation.duration = duration;
  cornerRadiusAnimation.timingFunction = timingFunction;

  // Add animations.
  CAAnimation* animation = AnimationGroupMake(
      @[ frameAnimation, colorAnimation, cornerRadiusAnimation ]);
  [self.dummyToolbarBackgroundView.layer
      addAnimation:animation
            forKey:kDummyToolbarBackgroundViewAnimationKey];
}

- (void)reverseDummyToolbarBackgroundViewAnimation {
  ReverseAnimationsForKeyForLayers(kDummyToolbarBackgroundViewAnimationKey,
                                   @[ self.dummyToolbarBackgroundView.layer ]);
}

- (void)animateTransitionToolbarSnapshotWithCardFrame:(CGRect)cardFrame
                                      transitionStyle:(StackTransitionStyle)
                                                          transitionStyle {
  // Add the snapshot and update its frame.
  self.transitionToolbarSnapshot =
      [self.transitionToolbarOwner.toolbarSnapshotProvider
          snapshotForStackViewWithWidth:CGRectGetWidth(self.view.frame)
                         safeAreaInsets:SafeAreaInsetsForView(self.view)];
  CGFloat toolbarHeight = self.transitionToolbarSnapshot.frame.size.height;

  CALayer* mask = [CALayer layer];
  mask.backgroundColor = [UIColor blackColor].CGColor;

  self.transitionToolbarSnapshot.layer.mask = mask;
  [_activeCardSet.displayView insertSubview:self.transitionToolbarSnapshot
                               aboveSubview:_activeCardSet.currentCard.view];
  CGRect toolbarFrame = [_activeCardSet.displayView
      convertRect:_toolbarController.view.frame
         fromView:_toolbarController.view.superview];
  CGFloat heightDifference = toolbarFrame.size.height - toolbarHeight;
  toolbarFrame.origin.y += heightDifference;
  toolbarFrame.size.height -= heightDifference;
  self.transitionToolbarSnapshot.frame = toolbarFrame;

  // The toolbar should animate such that its frame interpolates between the
  // normal toolbar frame at the top of the screen and the frame of the current
  // card's tab view.
  CGRect screenToolbarFrame = self.transitionToolbarSnapshot.frame;
  CGFloat cardTabHeight = kCardImageInsets.top - kCardFrameInset;
  CGRect cardToolbarFrame =
      CGRectInset(cardFrame, kCardFrameInset, kCardFrameInset);
  cardToolbarFrame.size.height = cardTabHeight;

  // Add animations.
  BOOL isPresentingStackView =
      (transitionStyle == STACK_TRANSITION_STYLE_PRESENTING);
  CGRect beginFrame =
      isPresentingStackView ? screenToolbarFrame : cardToolbarFrame;
  CGRect endFrame =
      isPresentingStackView ? cardToolbarFrame : screenToolbarFrame;

  // Difference between the height of the toolbar at the beginning of the
  // animation and at the end.
  CGFloat additionalHeight =
      screenToolbarFrame.size.height - endFrame.size.height;

  // Frame letting the toolbar be displayed such as the mask is croping the
  // additional height and let the part with the content be displayed in the
  // |endFrame|.
  CGRect endFrameWithMask =
      CGRectMake(endFrame.origin.x, endFrame.origin.y - additionalHeight,
                 endFrame.size.width, screenToolbarFrame.size.height);

  // Animation moving and resizing the snapshot between its card position and
  // the toolbar position, depending if it is a presentation or a dismissal.
  // The toolbar is not resized in height, it is croped using the mask.
  CAAnimation* frameAnimation = FrameAnimationMake(
      self.transitionToolbarSnapshot.layer, beginFrame, endFrameWithMask);
  frameAnimation.duration = ios::material::kDuration1;
  frameAnimation.timingFunction = TimingFunction(ios::material::CurveEaseInOut);

  // The snapshot is faded out/in to do a cross fade with the title of the card.
  CGFloat beginOpacity = isPresentingStackView ? 1 : 0;
  CGFloat endOpacity = isPresentingStackView ? 0 : 1;
  CAAnimation* fadeAnimation = OpacityAnimationMake(beginOpacity, endOpacity);
  fadeAnimation.timingFunction = TimingFunction(ios::material::CurveEaseIn);
  fadeAnimation.duration = ios::material::kDuration6;

  [self.transitionToolbarSnapshot.layer
      addAnimation:AnimationGroupMake(@[ frameAnimation, fadeAnimation ])
            forKey:kTransitionToolbarAnimationKey];

  // The mask animation is used to crop the additional height of the toolbar to
  // have the snapshot display only the content part of the toolbar.
  // The last pixel is croped as it is displaying the shadow below the toolbar.
  CGRect toolbarMaskFrame = CGRectMake(0, 0, screenToolbarFrame.size.width,
                                       screenToolbarFrame.size.height - 1);
  CGRect cardMaskFrame = CGRectMake(0, additionalHeight, endFrame.size.width,
                                    endFrame.size.height - 1);

  CGRect beginMaskFrame =
      isPresentingStackView ? toolbarMaskFrame : cardMaskFrame;
  CGRect endMaskFrame =
      isPresentingStackView ? cardMaskFrame : toolbarMaskFrame;

  CAAnimation* cropAnimation =
      FrameAnimationMake(mask, beginMaskFrame, endMaskFrame);
  cropAnimation.duration = ios::material::kDuration1;
  cropAnimation.timingFunction = TimingFunction(ios::material::CurveEaseInOut);
  mask.frame = endFrame;
  [mask addAnimation:cropAnimation forKey:kTransitionToolbarAnimationKey];
}

- (void)dismissWithSelectedTabAnimation {
  if (_isBeingDismissed || _activeCardSet.closingCard ||
      !_activeCardSet.cards.count) {
    return;
  }
  DCHECK(_isActive);
  [self prepareForDismissal];
  _isBeingDismissed = YES;
  // Once the stack view is starting to be dismissed, stop loading cards in the
  // background.
  [NSObject cancelPreviousPerformRequestsWithTarget:self];

  [_delegate tabSwitcher:self
      shouldFinishWithActiveModel:_activeCardSet.tabModel];

  [self animateTransitionWithStyle:STACK_TRANSITION_STYLE_DISMISSING];
}

- (std::vector<LayoutRect>)cardTransitionLayouts {
  std::vector<LayoutRect> cardLayouts;
  UIView* activeSetView = _activeCardSet.displayView;

  // Setting a card's layout to |fullscreenLayout| will scale the content
  // snapshot such that it will fill the entire portion of the screen below the
  // toolbar.  Used for the current card.
  LayoutRect fullscreenLayout = LayoutRectZero;
  fullscreenLayout.boundingWidth = CGRectGetWidth(activeSetView.bounds);
  fullscreenLayout.position.leading = -UIEdgeInsetsGetLeading(kCardImageInsets);
  fullscreenLayout.position.originY = -kCardImageInsets.top;
  fullscreenLayout.size.width = fullscreenLayout.boundingWidth +
                                kCardImageInsets.left + kCardImageInsets.right;
  fullscreenLayout.size.height = CGRectGetHeight(activeSetView.bounds) +
                                 kCardImageInsets.top + kCardImageInsets.bottom;

  // Cards above the current card (in z-index terms) should start/end offscreen.
  // Also account for the shadow so that the shadows cast by offscreen cards are
  // not visible at the beginning/end of the animation.
  CGFloat viewportLength =
      [self scrollLength:activeSetView.bounds.size] + kCardShadowThickness;
  LayoutRect offscreenLayout = fullscreenLayout;
  if (IsPortrait()) {
    offscreenLayout.position.originY += viewportLength + kCardImageInsets.top;
  } else {
    offscreenLayout.position.leading +=
        viewportLength + UIEdgeInsetsGetLeading(kCardImageInsets);
  }

  // Cards below the current card (in z-index terms) should be top-aligned with
  // the toolbar and at the final card size.
  LayoutRect cardLayout = LayoutRectZero;
  cardLayout.boundingWidth = CGRectGetWidth(activeSetView.bounds);
  cardLayout.size = [self cardSize];
  cardLayout.position.leading = page_animation_util::kCardMargin;
  cardLayout.position.originY = -kCardImageInsets.top;

  for (StackCard* card in _activeCardSet.cards) {
    if (card == _activeCardSet.currentCard) {
      // Current card takes the full screen.
      cardLayout = fullscreenLayout;
    } else if (LayoutRectEqualToRect(cardLayout, fullscreenLayout)) {
      // The card after the current card animates from off screen.
      cardLayout = offscreenLayout;
    }
    cardLayouts.push_back(cardLayout);
  }

  return cardLayouts;
}

- (Tab*)dismissWithNewTabAnimationToModel:(TabModel*)targetModel
                                  withURL:(const GURL&)url
                                  atIndex:(NSUInteger)position
                               transition:(ui::PageTransition)transition {
  if (_isBeingDismissed)
    return NULL;
  if ([_activeCardSet tabModel] != targetModel)
    [self setActiveCardSet:[self inactiveCardSet]];
  return [self dismissWithNewTabAnimation:url
                                  atIndex:position
                               transition:transition];
}

- (void)setLastTapPoint:(OpenNewTabCommand*)command {
  if (CGPointEqualToPoint(command.originPoint, CGPointZero)) {
    _lastTapPoint = CGPointZero;
  } else {
    _lastTapPoint =
        [self.view.window convertPoint:command.originPoint toView:self.view];
  }
}

- (Tab*)dismissWithNewTabAnimation:(const GURL&)URL
                           atIndex:(NSUInteger)position
                        transition:(ui::PageTransition)transition {
  // Record the start time for this operation so it may be reported as a metric
  // in the animation completion block.
  NSTimeInterval startTime = [NSDate timeIntervalSinceReferenceDate];

  // This helps smooth out the animation.
  [[_scrollView layer] setShouldRasterize:YES];
  if (_isBeingDismissed)
    return NULL;
  DCHECK(_isActive);
  [self prepareForDismissal];
  _isBeingDismissed = YES;
  [self setNeedsStatusBarAppearanceUpdate];
  DCHECK(URL.is_valid());
  // Stop pre-loading cards.
  [NSObject cancelPreviousPerformRequestsWithTarget:self];

  // This uses a custom animation, so ignore the change that would be triggered
  // by adding a new tab to the model. This is left on since the stack view is
  // going away at this point, so staying in sync doesn't matter any more.
  [_activeCardSet setIgnoresTabModelChanges:YES];
  if (position == NSNotFound)
    position = [_activeCardSet.tabModel count];
  DCHECK(position <= [_activeCardSet.tabModel count]);

  Tab* tab = [_activeCardSet.tabModel insertTabWithURL:URL
                                              referrer:web::Referrer()
                                            transition:transition
                                                opener:nil
                                           openedByDOM:NO
                                               atIndex:position
                                          inBackground:NO];
  [_activeCardSet.tabModel setCurrentTab:tab];

  [_delegate tabSwitcher:self
      shouldFinishWithActiveModel:_activeCardSet.tabModel];

  CGFloat statusBarHeight = StatusBarHeight();
  CGRect viewBounds, remainder;
  CGRectDivide([self.view bounds], &remainder, &viewBounds, statusBarHeight,
               CGRectMinYEdge);
  UIImageView* newCard = [[UIImageView alloc] initWithFrame:viewBounds];
  // Temporarily resize the tab's view to ensure it matches the card while
  // generating a snapshot, but then restore the original frame.
  CGRect originalTabFrame = [tab view].frame;
  [tab view].frame = viewBounds;
  newCard.image =
      SnapshotTabHelper::FromWebState(tab.webState)
          ->UpdateSnapshot(/*with_overlays=*/true, /*visible_frame_only=*/true);
  [tab view].frame = originalTabFrame;
  newCard.center =
      CGPointMake(CGRectGetMidX(viewBounds), CGRectGetMidY(viewBounds));
  [self.view addSubview:newCard];

  void (^completionBlock)(void) = ^{
    [newCard removeFromSuperview];
    [[_scrollView layer] setShouldRasterize:NO];
    [_animationDelegate tabSwitcherDismissalAnimationDidEnd:self];
    [_delegate tabSwitcherDismissTransitionDidEnd:self];
    double duration = [NSDate timeIntervalSinceReferenceDate] - startTime;
    if (_activeCardSet.tabModel.isOffTheRecord) {
      UMA_HISTOGRAM_TIMES(
          "Toolbar.TabSwitcher.NewIncognitoTabPresentationDuration",
          base::TimeDelta::FromSecondsD(duration));
    } else {
      UMA_HISTOGRAM_TIMES("Toolbar.TabSwitcher.NewTabPresentationDuration",
                          base::TimeDelta::FromSecondsD(duration));
    }

    // The following must come after logging metrics, because
    // |clearInternalState| resets _activeCardSet.
    _isActive = NO;
    [self clearInternalState];
  };

  CGPoint origin = _lastTapPoint;
  _lastTapPoint = CGPointZero;
  page_animation_util::AnimateInPaperWithAnimationAndCompletion(
      newCard, -statusBarHeight,
      newCard.frame.size.height - newCard.image.size.height, origin,
      [self isCurrentSetIncognito], nil, completionBlock);
  // TODO(stuartmorgan): Animate the other set off to the side.

  return tab;
}

#pragma mark UIGestureRecognizerDelegate methods

- (BOOL)gestureRecognizer:(UIGestureRecognizer*)recognizer
       shouldReceiveTouch:(UITouch*)touch {
  // Don't swallow any touches while the tools menu is open.
  if ([self.toolsMenuCoordinator isShowingToolsMenu])
    return NO;

  if ((recognizer == _pinchRecognizer) ||
      (recognizer == _swipeGestureRecognizer))
    return YES;

  // Only the mode switch recognizer should be triggered in the inactive deck
  // region (and it should only be triggered there).
  CGPoint touchLocation = [touch locationInView:_scrollView];
  BOOL inInactiveDeckRegion =
      CGRectContainsPoint([self inactiveDeckRegion], touchLocation);
  if (recognizer == _modeSwitchRecognizer)
    return inInactiveDeckRegion;
  else if (inInactiveDeckRegion)
    return NO;

  // Extract the card on which the touch is occurring.
  CardView* cardView = nil;
  StackCard* card = nil;
  if (recognizer == _swipeDismissesCardRecognizer) {
    UIView* activeView = _activeCardSet.displayView;
    CGPoint locationInActiveView = [touch locationInView:activeView];
    NSUInteger cardIndex = [self indexOfCardAtPoint:locationInActiveView];
    // |_swipeDismissesCardRecognizer| is interested only in touches that are
    // on cards in the active set.
    if (cardIndex == NSNotFound)
      return NO;
    DCHECK(cardIndex < [[_activeCardSet cards] count]);
    card = [[_activeCardSet cards] objectAtIndex:cardIndex];
    // This case seems like it should never happen, but it can be easily
    // handled anyway.
    if (![card viewIsLive])
      return YES;
    cardView = card.view;
  } else {
    // The recognizer is one of those attached to the card.
    // See https://crbug.com/393230 where recognizer.view may not be a CardView
    // type. In that case, early return with a NO to avoid unnecessary crash.
    cardView = base::mac::ObjCCastStrict<CardView>(recognizer.view);
    if (!cardView)
      return NO;
    card = [self cardForView:cardView];
  }

  // Prevent taps/presses in an uncollapsed card's close button from being
  // swallowed by the swipe-triggers-dismissal long press recognizer or
  // the card's tap/long press recognizer.
  if (CGRectContainsPoint([cardView closeButtonFrame],
                          [touch locationInView:cardView]) &&
      card && ![_activeCardSet cardIsCollapsed:card])
    return NO;

  return YES;
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer*)gestureRecognizer
    shouldRecognizeSimultaneouslyWithGestureRecognizer:
        (UIGestureRecognizer*)otherGestureRecognizer {
  // Pinch and scroll must be allowed to recognize simultaneously to enable
  // smooth transitioning between scrolling and pinching.
  BOOL pinchRecognizerInvolved = (gestureRecognizer == _pinchRecognizer ||
                                  otherGestureRecognizer == _pinchRecognizer);
  BOOL scrollRecognizerInvolved =
      (gestureRecognizer == _scrollGestureRecognizer ||
       otherGestureRecognizer == _scrollGestureRecognizer);
  if (pinchRecognizerInvolved && scrollRecognizerInvolved)
    return YES;

  // Swiping must be allowed to recognize simultaneously with the recognizer of
  // long presses that turn ambiguous swipes into card dismissals.
  BOOL swipeRecognizerInvolved =
      (gestureRecognizer == _swipeGestureRecognizer ||
       otherGestureRecognizer == _swipeGestureRecognizer);
  BOOL swipeDismissesCardRecognizerInvolved =
      (gestureRecognizer == _swipeDismissesCardRecognizer ||
       otherGestureRecognizer == _swipeDismissesCardRecognizer);
  if (swipeRecognizerInvolved && swipeDismissesCardRecognizerInvolved)
    return YES;

  // The swipe-triggers-card-dismissal long press recognizer must be allowed to
  // recognize simultaneously with the cards' long press recognizers that
  // trigger show-more-of-card.
  BOOL longPressRecognizerInvolved =
      ([gestureRecognizer isKindOfClass:[UILongPressGestureRecognizer class]] ||
       [otherGestureRecognizer
           isKindOfClass:[UILongPressGestureRecognizer class]]);
  if (swipeDismissesCardRecognizerInvolved && longPressRecognizerInvolved)
    return YES;

  return NO;
}

#pragma mark Action Handlers

- (void)closeTab:(id)sender {
  // Don't close any tabs mid-dismissal.
  if (_isBeingDismissed)
    return;

  // Remove the frame animation before adding the fade out animation.
  DCHECK([sender isKindOfClass:[CardView class]]);
  CardView* cardView = static_cast<CardView*>(sender);
  [cardView removeFrameAnimation];

  base::RecordAction(UserMetricsAction("MobileStackViewCloseTab"));
  StackCard* card = [self cardForView:cardView];
  DCHECK(card);
  NSUInteger tabIndex = [_activeCardSet.cards indexOfObject:card];
  if (tabIndex == NSNotFound)
    return;

  // TODO(blundell): Crashes have been seen wherein |tabIndex| is out of bounds
  // of the TabModel's array. It is not currently understood how this case
  // occurs. To work around these crashes, close the tab only if it is indeed
  // the tab that corresponds to this card; otherwise, remove the card directly
  // without modifying the tab model. b/8321162
  BOOL cardCorrespondsToTab = NO;
  if (tabIndex < [_activeCardSet.tabModel count]) {
    Tab* tab = [_activeCardSet.tabModel tabAtIndex:tabIndex];
    cardCorrespondsToTab = (card.tabID == reinterpret_cast<NSUInteger>(tab));
  }

  _activeCardSet.closingCard = card;
  if (cardCorrespondsToTab) {
    [_activeCardSet.tabModel closeTabAtIndex:tabIndex];
  } else {
    if (tabIndex < [_activeCardSet.tabModel count])
      DLOG(ERROR) << "Closed a card that didn't match the tab at its index";
    else
      DLOG(ERROR) << "Closed card at an index out of range of the tab model";
    [_activeCardSet removeCardAtIndex:tabIndex];
  }
}

- (void)handleLongPressFrom:(UIGestureRecognizer*)recognizer {
  DCHECK(!_isBeingDismissed);
  DCHECK(_isActive);

  if (recognizer == _swipeDismissesCardRecognizer)
    return;

  UIGestureRecognizerState state = [recognizer state];
  if (state != UIGestureRecognizerStateBegan)
    return;
  if ([recognizer numberOfTouches] == 0)
    return;

  // Don't take action on a card that is in the inactive stack, collapsed, or
  // the last card.
  CardView* cardView = (CardView*)recognizer.view;
  StackCard* card = [self cardForView:cardView];
  DCHECK(card);
  NSUInteger cardIndex = [[_activeCardSet cards] indexOfObject:card];
  DCHECK(cardIndex != NSNotFound);
  NSUInteger numCards = [[_activeCardSet cards] count];
  UIView* activeView = _activeCardSet.displayView;

  if ([cardView superview] != activeView ||
      [_activeCardSet cardIsCollapsed:card] || cardIndex == (numCards - 1))
    return;

  // Defer hiding the views of any cards that will be covered after the scroll
  // until the animation completes, as otherwise these cards immediately
  // disappear at the start of the animation.
  _activeCardSet.defersCardHiding = YES;
  [UIView animateWithDuration:kDefaultAnimationDuration
      animations:^{
        [_activeCardSet scrollCardAtIndex:cardIndex + 1 awayFromNeighbor:YES];
      }
      completion:^(BOOL finished) {
        _activeCardSet.defersCardHiding = NO;
      }];
}

- (void)handlePinchFrom:(UIPinchGestureRecognizer*)recognizer {
  DCHECK(!_isBeingDismissed);
  DCHECK(_isActive);
  UIView* currentView = _activeCardSet.displayView;
  DCHECK(recognizer.view == currentView);

  [_gestureStateTracker setPinching:YES];
  // Disable scrollView scrolling while a pinch is occurring. If the user lifts
  // a finger while pinching, callbacks to |handlePinchFrom:| will continue to
  // be made, and the code below will ensure that the cards get scrolled
  // properly.
  // TODO(blundell): Try to figure out how to re-enable deceleration for
  // such scrolls. b/5976932
  if ([_scrollView isScrollEnabled]) {
    [_scrollView setScrollEnabled:NO];
    _ignoreScrollCallbacks = YES;
    [self recenterScrollViewIfNecessary];
  }

  UIGestureRecognizerState state = [recognizer state];
  if ((state == UIGestureRecognizerStateCancelled) ||
      (state == UIGestureRecognizerStateEnded)) {
    [_gestureStateTracker setScrollingInPinch:NO];
    [self pinchEnded];
    _ignoreScrollCallbacks = NO;
    [_scrollView setScrollEnabled:YES];
    [_gestureStateTracker setPinching:NO];
    return;
  }

  DCHECK((state == UIGestureRecognizerStateBegan) ||
         (state == UIGestureRecognizerStateChanged));

  CardStackPinchGestureRecognizer* pinchGestureRecognizer =
      base::mac::ObjCCastStrict<CardStackPinchGestureRecognizer>(recognizer);
  if ([pinchGestureRecognizer numberOfActiveTouches] < 2) {
    // Clear the pinch card indices so that they are refetched if the user puts
    // a second finger back down.
    [_gestureStateTracker setFirstPinchCardIndex:NSNotFound];
    [_gestureStateTracker setSecondPinchCardIndex:NSNotFound];

    // The recognizer may continue to register two touches for a short period
    // after one of the touches is no longer active. Wait until there is only
    // one touch to be sure of accessing the information for the right touch.
    if ([recognizer numberOfTouches] != 1) {
      return;
    }

    CGPoint fingerLocation =
        [_pinchRecognizer locationOfTouch:0 inView:currentView];
    CGFloat fingerOffset = [self scrollOffsetAmountForPoint:fingerLocation];
    if (![_gestureStateTracker scrollingInPinch]) {
      NSUInteger scrolledIndex = [self indexOfCardAtPoint:fingerLocation];
      if (scrolledIndex != NSNotFound) {
        // Begin the scroll.
        [_gestureStateTracker setScrollCardIndex:scrolledIndex];
        [_gestureStateTracker setPreviousFirstPinchOffset:fingerOffset];
        [_gestureStateTracker setPreviousScrollTime:(base::TimeTicks::Now())];
        [_gestureStateTracker setScrollingInPinch:YES];
        // Animate back overpinch as necessary.
        [self pinchEnded];
      }
      return;
    }

    // Perform the scroll.
    CGFloat delta =
        fingerOffset - [_gestureStateTracker previousFirstPinchOffset];
    NSInteger scrolledIndex = [_gestureStateTracker scrollCardIndex];
    DCHECK(scrolledIndex != NSNotFound);
    [_activeCardSet scrollCardAtIndex:scrolledIndex
                              byDelta:delta
                 allowEarlyOverscroll:YES
                    decayOnOverscroll:YES
                   scrollLeadingCards:YES];
    [_gestureStateTracker setPreviousFirstPinchOffset:fingerOffset];
    [_gestureStateTracker updateScrollVelocityWithScrollDistance:delta];
    [_gestureStateTracker setPreviousScrollTime:(base::TimeTicks::Now())];
    return;
  }

  [_gestureStateTracker setScrollingInPinch:NO];

  DCHECK([recognizer numberOfTouches] >= 2);
  // Extract first and second offsets of the pinch.
  CGPoint firstPinchPoint = [recognizer locationOfTouch:0 inView:currentView];
  CGPoint secondPinchPoint = [recognizer locationOfTouch:1 inView:currentView];
  if ([self scrollOffsetAmountForPoint:firstPinchPoint] >
      [self scrollOffsetAmountForPoint:secondPinchPoint]) {
    CGPoint temp = firstPinchPoint;
    firstPinchPoint = secondPinchPoint;
    secondPinchPoint = temp;
  }
  CGFloat firstOffset = [self scrollOffsetAmountForPoint:firstPinchPoint];
  CGFloat secondOffset = [self scrollOffsetAmountForPoint:secondPinchPoint];
  NSInteger firstPinchCardIndex = [_gestureStateTracker firstPinchCardIndex];
  NSInteger secondPinchCardIndex = [_gestureStateTracker secondPinchCardIndex];

  // Pinch does not actually cause cards to move until user has started moving
  // fingers with each finger on a distinct card.
  if ((state == UIGestureRecognizerStateBegan) ||
      (firstPinchCardIndex == NSNotFound) ||
      (secondPinchCardIndex == NSNotFound) ||
      (firstPinchCardIndex == secondPinchCardIndex)) {
    [_gestureStateTracker
        setFirstPinchCardIndex:[self indexOfCardAtPoint:firstPinchPoint]];
    [_gestureStateTracker
        setSecondPinchCardIndex:[self indexOfCardAtPoint:secondPinchPoint]];
    [_gestureStateTracker setPreviousFirstPinchOffset:firstOffset];
    [_gestureStateTracker setPreviousSecondPinchOffset:secondOffset];
    return;
  }

  DCHECK(firstPinchCardIndex != NSNotFound);
  DCHECK(secondPinchCardIndex != NSNotFound);
  DCHECK(firstPinchCardIndex < secondPinchCardIndex);

  CGFloat firstDelta =
      firstOffset - [_gestureStateTracker previousFirstPinchOffset];
  CGFloat secondDelta =
      secondOffset - [_gestureStateTracker previousSecondPinchOffset];
  [_activeCardSet.stackModel handleMultitouchWithFirstDelta:firstDelta
                                                secondDelta:secondDelta
                                             firstCardIndex:firstPinchCardIndex
                                            secondCardIndex:secondPinchCardIndex
                                           decayOnOverpinch:YES];

  [_activeCardSet updateCardVisibilities];

  [_gestureStateTracker setPreviousFirstPinchOffset:firstOffset];
  [_gestureStateTracker setPreviousSecondPinchOffset:secondOffset];
}

- (void)handleTapFrom:(UITapGestureRecognizer*)recognizer {
  DCHECK(!_isBeingDismissed);
  DCHECK(_isActive);
  if (recognizer.state != UIGestureRecognizerStateEnded)
    return;

  if (recognizer == _modeSwitchRecognizer) {
    DCHECK(CGRectContainsPoint([self inactiveDeckRegion],
                               [recognizer locationInView:_scrollView]));
    [self setActiveCardSet:[self inactiveCardSet]];
    return;
  }

  CardView* cardView = (CardView*)recognizer.view;
  UIView* activeView = _activeCardSet.displayView;
  if ([cardView superview] != activeView)
    return;

  // Don't open the card if it's collapsed behind its succeeding neighbor, as
  // this was likely just a misplaced tap in that case.
  StackCard* card = [self cardForView:cardView];
  // TODO(blundell) : The card should not be nil here, see b/6759862.
  if (!card || [_activeCardSet cardIsCollapsed:card])
    return;

  DCHECK(!CGRectContainsPoint([cardView closeButtonFrame],
                              [recognizer locationInView:cardView]));

  if (self.transitionStyle == STACK_TRANSITION_STYLE_NONE) {
    [_activeCardSet setCurrentCard:card];
    [self dismissWithSelectedTabAnimation];
  } else if ([card isEqual:_activeCardSet.currentCard]) {
    // If the currently selected card is tapped mid-presentation animation,
    // simply reverse the animation immediately if it hasn't already been
    // reversed.
    if (self.transitionStyle != STACK_TRANSITION_STYLE_DISMISSING)
      [self cancelTransitionAnimation];
  } else {
    // If a new card is tapped mid-presentation, store a reference to the card
    // so that it can be selected upon dismissal in the presentation's
    // completion block.
    self.transitionTappedCard = card;
  }
}

- (void)handlePanFrom:(UIPanGestureRecognizer*)gesture {
  DCHECK(!_isBeingDismissed);
  DCHECK(_isActive);
  // Check if the gesture's initial state needs to be set.
  if (gesture.state == UIGestureRecognizerStateBegan ||
      [_gestureStateTracker resetSwipedCardOnNextSwipe]) {
    // Save first position to be able to calculate swipe distances later.
    BOOL isPortrait =
        UIInterfaceOrientationIsPortrait(_lastInterfaceOrientation);
    // TODO(crbug.com/228456): All of the swipe code should be operating on the
    // swipe's first touch in order to avoid the "dancing swipe" bug. Note that
    // this might involve adding some checks to handle the case where the number
    // of touches in the swipe is 0.
    CGPoint point = [gesture locationInView:_scrollView];
    [_gestureStateTracker
        setSwipeStartingPosition:(isPortrait ? point.x : point.y)];

    // Save the index of the card on which the swipe started (if any).
    CGPoint activePoint = [gesture locationInView:_activeCardSet.displayView];
    NSUInteger cardIndex = [self indexOfCardAtPoint:activePoint];
    [_gestureStateTracker setSwipedCardIndex:cardIndex];

    [_gestureStateTracker setResetSwipedCardOnNextSwipe:NO];
    // Signal that the swipe is beginning so that the type of the swipe will be
    // calculated on the next callback (note that it is too early to calculate
    // it here as the direction of the swipe, which is a component used in the
    // calculation, is currently unknown).
    [_gestureStateTracker setSwipeIsBeginning:YES];
    // Determine whether a swipe of ambiguous intent should change decks or
    // dismiss a card.
    UIGestureRecognizerState state = [_swipeDismissesCardRecognizer state];
    BOOL ambiguousSwipeChangesDecks =
        (state != UIGestureRecognizerStateBegan &&
         state != UIGestureRecognizerStateChanged);
    [_gestureStateTracker
        setAmbiguousSwipeChangesDecks:ambiguousSwipeChangesDecks];
    return;
  }

  if ([_gestureStateTracker swipeIsBeginning]) {
    [self determineSwipeType:gesture];
    [_gestureStateTracker setSwipeIsBeginning:NO];
  }

  if ([_gestureStateTracker swipeChangesDecks]) {
    [self swipeDeck:gesture];
    return;
  }

  // Check whether the swipe is actually on a card.
  NSUInteger cardIndex = [_gestureStateTracker swipedCardIndex];
  if (cardIndex == NSNotFound) {
    [_gestureStateTracker setResetSwipedCardOnNextSwipe:YES];
    return;
  }
  // Take action only if the card being swiped is not collapsed.
  DCHECK(cardIndex < [[_activeCardSet cards] count]);
  StackCard* card = [[_activeCardSet cards] objectAtIndex:cardIndex];
  if ([_activeCardSet cardIsCollapsed:card]) {
    [_gestureStateTracker setResetSwipedCardOnNextSwipe:YES];
    return;
  }
  [self swipeCard:gesture];
}

- (void)determineSwipeType:(UIPanGestureRecognizer*)gesture {
  if (![self bothDecksShouldBeDisplayed]) {
    [_gestureStateTracker setSwipeChangesDecks:NO];
    return;
  }

  if ([_gestureStateTracker swipedCardIndex] == NSNotFound) {
    [_gestureStateTracker setSwipeChangesDecks:YES];
    return;
  }

  // The swipe is on a card. Check whether the intent of the swipe is
  // ambiguous.
  BOOL isPortrait = UIInterfaceOrientationIsPortrait(_lastInterfaceOrientation);
  CGPoint swipePoint = [gesture locationInView:_scrollView];
  CGFloat swipePosition = isPortrait ? swipePoint.x : swipePoint.y;
  CGFloat swipeStartingPosition = [_gestureStateTracker swipeStartingPosition];
  CGFloat swipeDistance = swipePosition - swipeStartingPosition;
  if (UseRTLLayout() && isPortrait)
    swipeDistance *= -1;
  BOOL mainSetActive = (_activeCardSet == _mainCardSet);
  BOOL swipeIntentIsAmbiguous = (mainSetActive && swipeDistance < 0.0) ||
                                (!mainSetActive && swipeDistance > 0.0);

  BOOL swipeChangesDecks =
      swipeIntentIsAmbiguous ? [_gestureStateTracker ambiguousSwipeChangesDecks]
                             : NO;
  [_gestureStateTracker setSwipeChangesDecks:swipeChangesDecks];
}

- (CGFloat)distanceForSwipeToTriggerAction {
  return ([self scrollBreadth:[self view].bounds.size] *
          kSwipeCardScreenFraction);
}

- (BOOL)swipeShouldTriggerAction:(CGFloat)endingPosition {
  CGFloat swipeStartingPosition = [_gestureStateTracker swipeStartingPosition];
  CGFloat threshold = [self distanceForSwipeToTriggerAction];
  return std::abs(endingPosition - swipeStartingPosition) > threshold;
}

- (void)swipeDeck:(UIPanGestureRecognizer*)gesture {
  // StateBegan is handled by the caller handlePanFrom.
  DCHECK(gesture.state != UIGestureRecognizerStateBegan);
  // Swiping between decks should only be invoked when more than one deck is
  // being displayed.
  DCHECK([self bothDecksShouldBeDisplayed]);

  CGPoint point = [gesture locationInView:_scrollView];
  BOOL isPortrait = UIInterfaceOrientationIsPortrait(_lastInterfaceOrientation);
  CGFloat position = isPortrait ? point.x : point.y;
  CGFloat swipeStartingPosition = [_gestureStateTracker swipeStartingPosition];

  // In portrait on RTL, the incognito card stack is laid out to the left of
  // the maint stack, so invert the pan direction.
  BOOL shouldInvert = UseRTLLayout() && isPortrait;

  if (gesture.state == UIGestureRecognizerStateChanged) {
    CGFloat offset = position - swipeStartingPosition;
    // Decay drag if going off screen to mimic UIScrollView's bounce.
    BOOL isIncognito = [self isCurrentSetIncognito];
    if ((isIncognito && swipeStartingPosition > position) ||
        (!isIncognito && swipeStartingPosition < position))
      offset /= 2;
    if (shouldInvert)
      offset *= -1;
    [self updateDeckAxisPositionsWithShiftAmount:offset];

  } else if (gesture.state == UIGestureRecognizerStateEnded) {
    if ([self swipeShouldTriggerAction:position]) {
      // |topLeftCardSet| is the card set on the left in portrait and on top in
      // landscape, and |bottomRightCardSet| is the card set on the right in
      // portrait and the bottom in landscape.  If |position| is greater than
      // |swipeStartingPosition|, the gesture is dragging |topLeftCardSet| into
      // view. Otherwise, |bottomRightCardSet| is being dragged into view. Can't
      // just flip the active card set because this swipe might be a bounce that
      // is leaving the active card set unchanged.
      CardSet* topLeftCardSet = shouldInvert ? _otrCardSet : _mainCardSet;
      CardSet* bottomRightCardSet = shouldInvert ? _mainCardSet : _otrCardSet;
      _activeCardSet = position > swipeStartingPosition ? topLeftCardSet
                                                        : bottomRightCardSet;
    }
    [self displayActiveCardSet];
  }
}

- (void)swipeCard:(UIPanGestureRecognizer*)gesture {
  // StateBegan is handled by the caller handlePanFrom.
  DCHECK(gesture.state != UIGestureRecognizerStateBegan);

  CGPoint point = [gesture locationInView:_scrollView];
  BOOL isPortrait = UIInterfaceOrientationIsPortrait(_lastInterfaceOrientation);
  CGFloat position = isPortrait ? point.x : point.y;
  CGFloat swipeStartingPosition = [_gestureStateTracker swipeStartingPosition];
  CGFloat distanceMoved = fabs(position - swipeStartingPosition);

  // Calculate fractions of animation breadth to trigger dismissal that have
  // been covered so far.
  CGFloat fractionOfAnimationBreadth =
      distanceMoved / page_animation_util::AnimateOutTransformBreadth();
  // User can potentially move their finger further than animation breath/
  // dismissal threshold distance. Ensure that these corner cases don't cause
  // any unexpected behavior.
  fractionOfAnimationBreadth =
      std::min(fractionOfAnimationBreadth, CGFloat(1.0));

  // Calculate direction of the swipe.
  BOOL clockwise = position - swipeStartingPosition > 0;
  if (!isPortrait)
    clockwise = !clockwise;

  NSUInteger swipedCardIndex = [_gestureStateTracker swipedCardIndex];
  StackCard* card = [_activeCardSet.cards objectAtIndex:swipedCardIndex];
  _activeCardSet.closingCard = card;

  if (gesture.state == UIGestureRecognizerStateChanged) {
    // Transform card along |AnimateOutTransform| by the fraction of the
    // animation breadth that has been covered so far.
    [card view].transform = page_animation_util::AnimateOutTransform(
        fractionOfAnimationBreadth, clockwise, isPortrait);
    // Fade the card to become transparent at the conclusion of the animation,
    // and the card's tab to become transparent at the time that the card
    // reaches the threshold for being dismissed.
    [card view].alpha = 1 - fractionOfAnimationBreadth;
  } else {
    if (gesture.state == UIGestureRecognizerStateEnded &&
        [self swipeShouldTriggerAction:position]) {
      // Track card if animation should dismiss in reverse from the norm of
      // clockwise in portrait, counter-clockwise in landscape.
      if ((isPortrait && !clockwise) || (!isPortrait && clockwise))
        _reverseDismissCard = card;
      // This will trigger the completion of the close card animation.
      [self closeTab:card.view];
    } else {
      // Animate back to starting position.
      [UIView animateWithDuration:kDefaultAnimationDuration
          delay:0
          options:UIViewAnimationCurveEaseOut
          animations:^{
            [card view].alpha = 1;
            [[card view] setTabOpacity:1];
            [card view].transform = CGAffineTransformIdentity;
          }
          completion:^(BOOL finished) {
            _activeCardSet.closingCard = nil;
          }];
    }
  }
}

- (StackCard*)cardForView:(CardView*)view {
  // This isn't terribly efficient, but since it is only intended for use in
  // response to a user action it's not worth the bookkeeping of a reverse
  // mapping to make it constant time.
  for (StackCard* card in _activeCardSet.cards) {
    if (card.viewIsLive && card.view == view) {
      return card;
    }
  }
  return nil;
}

#pragma mark - StackViewToolbarControllerDelegate

- (void)stackViewToolbarControllerShouldDismiss:
    (StackViewToolbarController*)stackViewToolbarController {
  [self dismissWithSelectedTabAnimation];
}

#pragma mark - ToolsMenuCoordinator Configuration

- (ToolsMenuConfiguration*)menuConfigurationForToolsMenuCoordinator:
    (ToolsMenuCoordinator*)coordinator {
  ToolsMenuConfiguration* configuration =
      [[ToolsMenuConfiguration alloc] initWithDisplayView:[self view]
                                       baseViewController:self];
  [configuration setInTabSwitcher:YES];
  // When checking for the existence of tabs, catch the case where the main set
  // is both active and empty, but the incognito set has some cards.
  if (([[_activeCardSet cards] count] == 0) &&
      (_activeCardSet == _otrCardSet || [[_otrCardSet cards] count] == 0))
    [configuration setNoOpenedTabs:YES];
  if (_activeCardSet == _otrCardSet)
    [configuration setInIncognito:YES];
  return configuration;
}

#pragma mark - BrowserCommands

- (void)openNewTab:(OpenNewTabCommand*)command {
  // Ensure that the right mode is showing.
  if ([self isCurrentSetIncognito] != command.incognito)
    [self setActiveCardSet:[self inactiveCardSet]];

  // Either send or don't send the "New Tab Opened" or "Incognito Tab Opened" to
  // the feature_engagement::Tracker based on |command.userInitiated| and
  // |command.incognito|.
  feature_engagement::NotifyNewTabEventForCommand(
      _activeCardSet.tabModel.browserState, command);

  [self setLastTapPoint:command];
  [self dismissWithNewTabAnimation:GURL(kChromeUINewTabURL)
                           atIndex:NSNotFound
                        transition:ui::PAGE_TRANSITION_TYPED];
}

// Closing all while the main set is active closes everything, but closing
// all while incognito is active only closes incognito tabs.
- (void)closeAllTabs {
  DCHECK(![self isCurrentSetIncognito]);
  [self removeAllCardsFromSet:_mainCardSet];
  [self removeAllCardsFromSet:_otrCardSet];
}

- (void)closeAllIncognitoTabs {
  DCHECK([self isCurrentSetIncognito]);
  [self removeAllCardsFromSet:_activeCardSet];
}

#pragma mark Notification Handlers

- (void)cardSetDidCloseAllTabs:(CardSet*)cardSet {
  // Early return if the stack view is not active.  This can sometimes occur if
  // |clearInternalState| triggers the deletion of a tab model.
  if (!_isActive)
    return;

  if (cardSet == _activeCardSet)
    [self activeCardCountChanged];
  for (UIView* card in [cardSet.displayView subviews]) {
    [card removeFromSuperview];
  }
  // No need to re-sync with the card set here, since the tab model (and thus
  // the card set) is known to be empty.

  // The animation of closing all the main set's cards interacts badly with the
  // animation of switching to main-card-set-only mode, so if the incognito set
  // finishes closing while the main set is still animating (in the case of
  // closing all cards at once) wait until the main set finishes before updating
  // the display (necessary so the state is right if a new tab is opened).
  if ((cardSet == _otrCardSet && ![_mainCardSet ignoresTabModelChanges]) ||
      (cardSet == _mainCardSet && [[_otrCardSet cards] count] == 0)) {
    [self displayMainCardSetOnly];
  }

  [_toolbarController setTabCount:[_activeCardSet.cards count]];
}

#pragma mark CardSetObserver Methods

- (void)cardSet:(CardSet*)cardSet didAddCard:(StackCard*)newCard {
  [self updateScrollViewContentSize];

  if (cardSet == _activeCardSet) {
    [self activeCardCountChanged];
    [_toolbarController setTabCount:[_activeCardSet.cards count]];
  }

  // Place the card at the right destination point to animate in: staggered
  // from its previous neighbor if it is the last card, or in the location of
  // its successive neighbor (which will slide down to make room) otherwise.
  NSArray* cards = [cardSet cards];
  NSUInteger cardIndex = [cards indexOfObject:newCard];
  CGFloat maxStagger = [[cardSet stackModel] maxStagger];

  if (newCard == [cards lastObject]) {
    if ([cards count] == 1) {
      // Simply lay out the card.
      [cardSet fanOutCards];
    } else {
      StackCard* previousCard = [cards objectAtIndex:cardIndex - 1];
      BOOL isPortrait =
          UIInterfaceOrientationIsPortrait(_lastInterfaceOrientation);
      LayoutRect layout = previousCard.layout;
      if (isPortrait)
        layout.position.originY += maxStagger;
      else
        layout.position.leading += maxStagger;
      newCard.layout = layout;
    }

  } else {
    DCHECK(cardIndex != NSNotFound);
    DCHECK(cardIndex + 1 < [cards count]);
    newCard.layout = [[cards objectAtIndex:cardIndex + 1] layout];
  }

  // Animate the new card in at its destination location.
  page_animation_util::AnimateInCardWithAnimationAndCompletion(newCard.view,
                                                               NULL, NULL);

  // Set up the animation of the existing cards.
  NSUInteger indexToScroll = cardIndex + 1;
  CGFloat scrollAmount = maxStagger;
  if (newCard == [cards lastObject] ||
      [cardSet isCardInEndStaggerRegion:newCard]) {
    // No scrolling actually needs to be done, although |scrollCardAtIndex:|
    // still has to be called if the new card is not in the start stack in
    // order to ensure that the end stack is re-laid out if necessary.
    indexToScroll = cardIndex;
    scrollAmount = 0;
  }

  // If the new card is in the start stack, just re-lay out the start stack.
  // Otherwise, slide down the successive cards to make room and/or re-lay out
  // the end stack. TODO(blundell): The animation is behaving incorrectly when
  // the card being inserted is near the end stack: sometimes the slide down
  // doesn't occur, and sometimes it overscrolls, causing a visible bounce.
  void (^stackAnimation)(void) = ^{
    if ([cardSet isCardInStartStaggerRegion:newCard]) {
      [cardSet layOutStartStack];
    } else {
      [cardSet scrollCardAtIndex:indexToScroll
                         byDelta:scrollAmount
            allowEarlyOverscroll:NO
               decayOnOverscroll:NO
              scrollLeadingCards:YES];
    }
  };

  cardSet.defersCardHiding = YES;
  [UIView animateWithDuration:kDefaultAnimationDuration
                        delay:0
                      options:UIViewAnimationOptionBeginFromCurrentState
                   animations:stackAnimation
                   completion:^(BOOL) {
                     cardSet.defersCardHiding = NO;
                     if (cardSet == _activeCardSet) {
                       // Ensure that state is properly reset if there was a
                       // scroll/pinch occurring.
                       [self scrollEnded];
                     }
                   }];
}

- (void)cardSet:(CardSet*)cardSet
    willRemoveCard:(StackCard*)cardBeingRemoved
           atIndex:(NSUInteger)index {
  // All handlers working on that card must be stopped to prevent concurrency
  // and/or UI inconcistencies.
  if (cardSet == _activeCardSet) {
    // Cancel any outstanding gestures that were tracking a card index, as they
    // might have been operating on cards that no longer exist. Ideally, these
    // events would allow the gestures to continue and just reset the cards on
    // which they are operating. However, doing that correctly in all cases
    // proves near-impossible: if the call to -disableGestureHandlers happens
    // slightly too late, then a gesture callback could occur in the new state
    // of the world with the gesture still operating on the old state of the
    // world. Meanwhile if the call to -enableGestureHandlers happens
    // slightly too early, then a gesture callback could occur while still in
    // the old state of the world, meaning that the card being tracked would
    // revert to the old (problematic) card.
    [self disableGestureHandlers];
  }
}

- (void)cardSet:(CardSet*)cardSet
    didRemoveCard:(StackCard*)removedCard
          atIndex:(NSUInteger)index {
  // If card was tapped during a transition, but its WebState was closed before
  // the animation finishes, reset |transitionTappedCard|, as attempting to
  // show that Tab upon finishing the animation will result in a crash since its
  // WebState is destroyed.
  if (removedCard == self.transitionTappedCard)
    self.transitionTappedCard = nil;

  if (cardSet == _activeCardSet) {
    // Reenable the gesture handlers (disabled in
    // -cardSet:willRemoveCard:atIndex). It is now safe to do so as the card
    // that was being removed has been removed at this point.
    [self enableGestureHandlers];
    [_toolbarController setTabCount:[_activeCardSet.cards count]];
  }

  // If no view was ever created for this card, it's too late to make one. This
  // can only happen if a tab is closed by something other than user action,
  // and even then only if the card hasn't been pre-loaded yet, so not doing th
  // animation isn't a problem.
  if (removedCard.viewIsLive) {
    // Determine what direction animation should rotate in. The norm is
    // clockwise in portrait, counter-clockwise in landscape; however, it needs
    // to be reversed if this animation is occurring as the conclusion of a
    // swipe that went in the opposite direction from the norm.
    BOOL isPortrait =
        UIInterfaceOrientationIsPortrait(_lastInterfaceOrientation);
    BOOL clockwise = isPortrait ? _reverseDismissCard != removedCard
                                : _reverseDismissCard == removedCard;
    [self animateOutCardView:removedCard.view
                       delay:0
                   clockwise:clockwise
                  completion:nil];
    // Reset |reverseDismissCard| if that card was the one dismissed.
    if ((isPortrait && !clockwise) || (!isPortrait && clockwise))
      _reverseDismissCard = nil;
  }
  // Nil out the the closing card after all closing animations have finished.
  [CATransaction begin];
  [CATransaction setCompletionBlock:^{
    cardSet.closingCard = nil;
  }];
  // If the last incognito card closes, switch back to just the main set.
  if ([cardSet.cards count] == 0 && cardSet == _otrCardSet) {
    [self displayMainCardSetOnly];
  } else {
    NSUInteger numCards = [[cardSet cards] count];
    if (numCards == 0) {
      // Commit the transaction before early return.
      [CATransaction commit];
      return;
    }
    if (index == numCards) {
      // If the card that was closed was the last card and was in the start
      // stack, the start stack might need to be re-laid out to show a
      // previously hidden card.
      if ([cardSet overextensionTowardStartOnCardAtIndex:numCards - 1]) {
        [UIView animateWithDuration:kDefaultAnimationDuration
                         animations:^{
                           [cardSet layOutStartStack];
                         }];
      }
    } else {
      // Scroll up the card following the removed card to be placed where the
      // removed card was.
      LayoutRectPosition removedCardPosition = removedCard.layout.position;
      LayoutRectPosition followingCardPosition =
          [[cardSet cards][index] layout].position;
      CGFloat scrollAmount =
          [self scrollOffsetAmountForPosition:removedCardPosition] -
          [self scrollOffsetAmountForPosition:followingCardPosition];
      [cardSet updateShadowLayout];
      [UIView animateWithDuration:kDefaultAnimationDuration
                       animations:^{
                         [cardSet scrollCardAtIndex:index
                                            byDelta:scrollAmount
                               allowEarlyOverscroll:YES
                                  decayOnOverscroll:NO
                                 scrollLeadingCards:NO];
                         [cardSet updateShadowLayout];
                       }];
    }
  }
  [CATransaction commit];
}

- (void)cardSet:(CardSet*)cardSet displayedCard:(StackCard*)card {
  // Add gesture recognizers to the card.
  [card.view addCardCloseTarget:self action:@selector(closeTab:)];
  [card.view addAccessibilityTarget:self
                             action:@selector(accessibilityFocusedOnElement:)];

  UIGestureRecognizer* tapRecognizer =
      [[UITapGestureRecognizer alloc] initWithTarget:self
                                              action:@selector(handleTapFrom:)];
  tapRecognizer.delegate = self;
  [card.view addGestureRecognizer:tapRecognizer];

  UIGestureRecognizer* longPressRecognizer =
      [[UILongPressGestureRecognizer alloc]
          initWithTarget:self
                  action:@selector(handleLongPressFrom:)];
  longPressRecognizer.delegate = self;
  [card.view addGestureRecognizer:longPressRecognizer];
}

- (void)cardSetRecreatedCards:(CardSet*)cardSet {
  // Remove the old card views, if any, then start loading the new ones.
  for (UIView* card in [cardSet.displayView subviews]) {
    [card removeFromSuperview];
  }
  [self preloadCardViewsAsynchronously];
}

#pragma mark -

// The following method is based on Apple sample code available at
// http://developer.apple.com/library/ios/samplecode/StreetScroller/
// Introduction/Intro.html.
- (void)recenterScrollViewIfNecessary {
  CGFloat contentOffset =
      [self scrollOffsetAmountForPoint:[_scrollView contentOffset]];
  CGFloat contentLength = [self scrollLength:[_scrollView contentSize]];
  CGFloat viewportLength = [self scrollLength:[_scrollView bounds].size];
  DCHECK(contentLength > viewportLength || [_activeCardSet.cards count] == 0);

  CGFloat centerOffset = (contentLength - viewportLength) / 2.0;
  CGFloat distanceFromCenter = fabs(contentOffset - centerOffset);

  if (distanceFromCenter > centerOffset / 2.0) {
    _ignoreScrollCallbacks = YES;
    [_scrollView
        setContentOffset:[self scrollOffsetPointWithAmount:centerOffset]];
    [self alignDisplayViewsToViewport];
    _ignoreScrollCallbacks = NO;
  }
}

- (void)alignDisplayViewsToViewport {
  // TODO(crbug.com/789975): The iPhoneX iOS 11.0.0 simulator was a beta release
  // and has a bug that causes this DCHECK to fire incorrectly.  Disable the
  // DCHECK on iPhoneX 11.0.0, until the bots are updated to a newer simulator
  // version.
  if (!IsIPhoneX() || base::ios::IsRunningOnOrLater(11, 0, 1)) {
    DCHECK(CGSizeEqualToSize(
        AlignRectOriginAndSizeToPixels([_mainCardSet displayView].frame).size,
        [_scrollView frame].size));
    DCHECK(CGSizeEqualToSize(
        AlignRectOriginAndSizeToPixels([_otrCardSet displayView].frame).size,
        [_scrollView frame].size));
  }
  CGRect newDisplayViewFrame = CGRectMake(
      [_scrollView contentOffset].x, [_scrollView contentOffset].y,
      [_scrollView frame].size.width, [_scrollView frame].size.height);
  [_mainCardSet displayView].frame = newDisplayViewFrame;
  [_otrCardSet displayView].frame = newDisplayViewFrame;
}

// Caps overscroll once the stack becomes fully overextended or deceleration
// slows below a given velocity to achieve a nice-looking bounce effect.
- (BOOL)shouldEndScroll {
  if ([[_activeCardSet cards] count] == 0 ||
      ![_activeCardSet stackIsOverextended] || ![_scrollView isDecelerating])
    return NO;
  if ([_activeCardSet stackIsFullyOverextended])
    return YES;
  NSUInteger lastCardIndex = [[_activeCardSet cards] count] - 1;
  // Kill the scroll in the case where a fling wasn't detected early enough,
  // resulting in part of the stack being overscrolled toward the start without
  // the whole stack being overscrolled toward the start.
  if ([_activeCardSet overextensionTowardStartOnCardAtIndex:0] &&
      ![_activeCardSet overextensionTowardStartOnCardAtIndex:lastCardIndex])
    return YES;
  return [_gestureStateTracker scrollVelocity] < kMinFlingVelocityInOverscroll;
}

- (void)scrollEnded {
  if ([_activeCardSet stackIsOverextended]) {
    void (^toDoWhenDone)(void) = ^{
      [self recenterScrollViewIfNecessary];
    };
    [self animateOverextensionEliminationWithCompletion:toDoWhenDone];
  } else {
    [self recenterScrollViewIfNecessary];
  }
}

- (void)pinchEnded {
  BOOL scrollingInPinch = [_gestureStateTracker scrollingInPinch];

  if (![_activeCardSet stackIsOverextended]) {
    if (!scrollingInPinch)
      [self recenterScrollViewIfNecessary];
    return;
  }

  if (scrollingInPinch) {
    NSUInteger scrollCardIndex = [_gestureStateTracker scrollCardIndex];
    DCHECK(scrollCardIndex != NSNotFound);
    if ([_activeCardSet overextensionOnCardAtIndex:scrollCardIndex])
      return;
  }

  void (^toDoWhenDone)(void) = NULL;
  if (!scrollingInPinch) {
    toDoWhenDone = ^{
      [self recenterScrollViewIfNecessary];
    };
  }
  [self animateOverextensionEliminationWithCompletion:toDoWhenDone];
}

- (void)animateOverextensionEliminationWithCompletion:
    (ProceduralBlock)completion {
  _activeCardSet.defersCardHiding = YES;
  [UIView animateWithDuration:kOverextensionEliminationAnimationDuration
      delay:0
      options:(UIViewAnimationOptionAllowUserInteraction |
               UIViewAnimationOptionOverrideInheritedCurve |
               UIViewAnimationOptionCurveEaseOut)
      animations:^{
        [_activeCardSet eliminateOverextension];
      }
      completion:^(BOOL finished) {
        _activeCardSet.defersCardHiding = NO;
        if (completion)
          completion();
      }];
}

- (void)killScrollDeceleration {
  _ignoreScrollCallbacks = YES;
  [_scrollView setContentOffset:[_scrollView contentOffset] animated:NO];
  // The above call does not always generate a callback to
  // |scrollViewDidScroll:|, so it is necessary to update the gesture state
  // tracker's previous scroll offset explicitly here.
  [_gestureStateTracker
      setPreviousScrollOffset:
          [self scrollOffsetAmountForPoint:[_scrollView contentOffset]]];
  _ignoreScrollCallbacks = NO;
}

// To mimic standard iOS behavior on overscroll, the stack is allowed to
// overscroll approximately half of the screen length on drag and a lesser
// amount on fling.
- (void)adjustMaximumOverextensionAmount:(BOOL)scrollIsFling {
  CGFloat screenLength = [self scrollLength:self.view.bounds.size];
  CGFloat maximumOverextensionAmount =
      scrollIsFling ? [_activeCardSet overextensionAmount] + screenLength / 4.0
                    : screenLength / 2.0;

  // The overextension amount is not allowed to grow after a fling begins, as
  // otherwise the fling would just keep overextending further and further.
  if (scrollIsFling &&
      maximumOverextensionAmount > [_activeCardSet maximumOverextensionAmount])
    return;
  [_activeCardSet setMaximumOverextensionAmount:maximumOverextensionAmount];
}

#pragma mark UIScrollViewDelegate methods

- (void)scrollViewDidScroll:(UIScrollView*)scrollView {
  DCHECK(!_isBeingDismissed);
  DCHECK(_isActive);
  DCHECK(scrollView == _scrollView);

  // During rotation on iPhone X, UIScrollView's internal handling of the status
  // bar triggers a |-scrollViewDidScroll:| callback before any rotation-
  // related callbacks have been received.  When this occurs, the bounds of the
  // UIScrollView have been updated to its new rotated value, but the content
  // size has not yet been updated via |-updateScrollViewContentSize|.  When
  // this occurs, early return before peforming subsequent scrolling
  // calculations.  The layout logic will eventually be triggered when the
  // UIScrollView's contentSize is reset.
  if (_lastInterfaceOrientation != GetInterfaceOrientation())
    return;

  // Whether this callback will trigger a scroll or not, have to ensure that
  // the display views' positions are updated after any change in the scroll
  // view's content offset.
  [self alignDisplayViewsToViewport];

  if (_ignoreScrollCallbacks) {
    [_gestureStateTracker
        setPreviousScrollOffset:
            [self scrollOffsetAmountForPoint:[_scrollView contentOffset]]];
    return;
  }

  if ([[_activeCardSet cards] count] == 0)
    return;

  // First check if the scrolled card needs to be reset. Have to be careful to
  // do this only when the user is actually starting a new scroll.
  if ([_scrollView isTracking] &&
      [_gestureStateTracker resetScrollCardOnNextDrag]) {
    CGPoint fingerLocation =
        [_scrollGestureRecognizer locationOfTouch:0
                                           inView:_activeCardSet.displayView];
    [_gestureStateTracker
        setScrollCardIndex:[self indexOfCardAtPoint:fingerLocation]];
    // In certain corner cases the previous scroll offset is not up-to-date
    // when the scrolled card needs to be reset (most notably, when rotating
    // the device while scrolling). Below provides a fix for these cases
    // without harming other cases.
    [_gestureStateTracker
        setPreviousScrollOffset:
            [self scrollOffsetAmountForPoint:[_scrollView contentOffset]]];
    [_gestureStateTracker setPreviousScrollTime:(base::TimeTicks::Now())];
    [_gestureStateTracker setResetScrollCardOnNextDrag:NO];
  }

  CGFloat contentOffset =
      [self scrollOffsetAmountForPoint:[_scrollView contentOffset]];
  CGFloat delta = contentOffset - [_gestureStateTracker previousScrollOffset];

  // If overscrolled and in a fling, compute the delta to apply manually to
  // achieve a nice-looking deceleration effect.
  if ([_activeCardSet stackIsOverextended] && ![_scrollView isTracking]) {
    CGFloat currentVelocity = [_gestureStateTracker scrollVelocity];
    CGFloat elapsedTime = CGFloat(
        (base::TimeTicks::Now() - [_gestureStateTracker previousScrollTime])
            .InMilliseconds());
    if (elapsedTime > 0.0) {
      CGFloat sign = (delta >= 0) ? 1.0 : -1.0;
      CGFloat distanceAtCurrentVelocity = sign * currentVelocity * elapsedTime;
      delta = distanceAtCurrentVelocity * kDecayFactorInBounce;
    }
  }
  [_gestureStateTracker updateScrollVelocityWithScrollDistance:delta];

  if ([self shouldEndScroll]) {
    [self killScrollDeceleration];
    return;
  }

  // Perform the scroll.
  NSInteger scrolledIndex = [_gestureStateTracker scrollCardIndex];
  if (scrolledIndex == NSNotFound) {
    // User is scrolling outside the active card stack. Ideally, this scroll
    // would be ignored; however, that is challenging to implement properly
    // (in particular, continuing to ensure that scroll view is recentered
    // when it needs to be). For now, pick a reasonable index to do the
    // scrolling on. TODO(blundell): Figure out how to ignore these scrolls
    // while ensuring that scroll view is recentered as necessary. b/5858386
    scrolledIndex = 0;
    if (delta > 0)
      scrolledIndex = [[_activeCardSet cards] count] - 1;
    // On the next scroll, check again so that if the user starts scrolling on
    // a card, the scroll moves to be on that card.
    [_gestureStateTracker setResetScrollCardOnNextDrag:YES];
  }

  DCHECK(scrolledIndex != NSNotFound);

  // Scrolls that are greater than a given velocity are assumed to be flings
  // even if the user's finger is still registered as being down, as it is
  // extremely likely that the user is actually in the middle of doing a fling
  // motion (and if the scrolled card is allowed to visibly overscroll before
  // the stack is fully collapsed, the ability to handle the fling as a fling
  // from a UI perspective is lost). The latter heuristic has the cost of
  // sometimes ending up in the scrolled card not tracking the user's finger if
  // the user is scrolling very fast near the start stack.
  BOOL isFling =
      ![_scrollView isTracking] || ([_gestureStateTracker scrollVelocity] >
                                    kThresholdVelocityForTreatingScrollAsFling);
  [self adjustMaximumOverextensionAmount:isFling];

  // The scroll view's content offset increases with scrolling toward the start
  // stack. These semantics are inverted from those of
  // |scrollCardAtIndex:byDelta:|.  If the scroll is a drag, then overscroll
  // occurs with the scrolled card and the scroll decays once overscrolling
  // begins to mimic the native iOS behavior on overscroll. In the case of
  // fling, overscroll does not occur until the scroll is fully
  // collapsed/expanded and no decay occurs on overscroll as the delta has
  // already been manually adjusted in this case (see above).
  BOOL inverseFanDirection = UseRTLLayout() && !IsPortrait();
  if (inverseFanDirection) {
    // In landscape RTL layouts, StackCard's application of its layout values to
    // its underlying CardView is flipped across the midpoint Y axis, but the
    // scroll view maintains its non-RTL scrolling behavior.  In this
    // situation, reverse the scrolling direction before applying it to the
    // CardSet.
    delta *= -1;
  }
  [_activeCardSet scrollCardAtIndex:scrolledIndex
                            byDelta:-delta
               allowEarlyOverscroll:!isFling
                  decayOnOverscroll:!isFling
                 scrollLeadingCards:YES];

  // Verify that if scroll view's content offset has hit a boundary point, the
  // active card stack is fully scrolled in the corresponding direction. Note
  // that this check intentionally doesn't take into account overextension: due
  // to the fact that overextension is a transient state, the stack is not
  // guaranteed to be fully overextended when these checks are performed (and
  // that is OK).
  DCHECK(contentOffset >= 0);
  CGFloat epsilon = std::numeric_limits<CGFloat>::epsilon();
  if (contentOffset < epsilon) {
    if (inverseFanDirection)
      DCHECK([_activeCardSet stackIsFullyCollapsed]);
    else
      DCHECK([_activeCardSet stackIsFullyFannedOut]);
  }
  CGFloat viewportLength = [self scrollLength:[_scrollView bounds].size];
  CGFloat contentSizeUpperLimit =
      [self scrollLength:[_scrollView contentSize]] - viewportLength;
  DCHECK(contentOffset <= contentSizeUpperLimit);
  if (std::abs(contentSizeUpperLimit - contentOffset) < epsilon) {
    if (inverseFanDirection)
      DCHECK([_activeCardSet stackIsFullyFannedOut]);
    else
      DCHECK([_activeCardSet stackIsFullyCollapsed]);
  }

  [_gestureStateTracker setPreviousScrollTime:(base::TimeTicks::Now())];
  [_gestureStateTracker
      setPreviousScrollOffset:
          [self scrollOffsetAmountForPoint:[_scrollView contentOffset]]];
}

- (void)scrollViewDidEndDragging:(UIScrollView*)scrollView
                  willDecelerate:(BOOL)willDecelerate {
  DCHECK(scrollView == _scrollView);
  [_gestureStateTracker setResetScrollCardOnNextDrag:YES];
  // Recenter the scroll view's content offset after making sure that there is
  // no scrolling currently occurring.
  if (willDecelerate || [_scrollView isDragging] ||
      [_scrollView isDecelerating] || [_gestureStateTracker pinching])
    return;
  [self scrollEnded];
}

- (void)scrollViewDidEndDecelerating:(UIScrollView*)scrollView {
  DCHECK(scrollView == _scrollView);
  // Recenter the scroll view's content offset after making sure that there is
  // no scrolling currently occurring (this deceleration might have been ended
  // by the user starting a new scroll).
  if ([_scrollView isDragging] || [_scrollView isDecelerating] ||
      [_gestureStateTracker pinching])
    return;
  [self scrollEnded];
}

#pragma mark - Accessibility methods

// Handles scrolling through the card stack and scrolling between main and
// incognito card stacks while in voiceover mode. Three finger scroll toward
// an edge stack displays the next few collapsed tabs from the appropriate edge
// stack. Three finger scroll toward the inactive stack switches between the
// main and incognito card stacks, as appropriate.
- (BOOL)accessibilityScroll:(UIAccessibilityScrollDirection)direction {
  BOOL isPortrait = IsPortrait();
  BOOL shouldScrollTowardEnd = NO;
  BOOL shouldScrollTowardStart = NO;
  BOOL shouldSwitchToMain = NO;
  BOOL shouldSwitchToIncognito = NO;

  switch (direction) {
    case UIAccessibilityScrollDirectionDown:
      if (isPortrait) {
        shouldScrollTowardEnd = YES;
      } else {
        shouldSwitchToIncognito = YES;
      }
      break;
    case UIAccessibilityScrollDirectionUp:
      if (isPortrait) {
        shouldScrollTowardStart = YES;
      } else {
        shouldSwitchToMain = YES;
      }
      break;
    case UIAccessibilityScrollDirectionLeft:
      if (isPortrait) {
        shouldSwitchToIncognito = YES;
      } else {
        shouldScrollTowardEnd = YES;
      }
      break;
    case UIAccessibilityScrollDirectionRight:
      if (isPortrait) {
        shouldSwitchToMain = YES;
      } else {
        shouldScrollTowardStart = YES;
      }
      break;
    default:
      break;
  }

  if (shouldScrollTowardEnd) {
    DCHECK([_activeCardSet.stackModel firstEndStackCardIndex] > -1);
    [_activeCardSet
        fanOutCardsWithStartIndex:
            std::min(
                (NSInteger)[_activeCardSet.cards count] - 1,
                (NSInteger)[_activeCardSet.stackModel firstEndStackCardIndex])];
  } else if (shouldScrollTowardStart) {
    DCHECK([_activeCardSet.stackModel lastStartStackCardIndex] > -1);
    [_activeCardSet
        fanOutCardsWithStartIndex:
            std::max((NSInteger)0,
                     (NSInteger)(
                         [_activeCardSet.stackModel lastStartStackCardIndex] -
                         [_activeCardSet.stackModel fannedStackCount]))];
  } else if ([self bothDecksShouldBeDisplayed]) {
    if (shouldSwitchToMain) {
      [self setActiveCardSet:_mainCardSet];
    } else if (shouldSwitchToIncognito) {
      [self setActiveCardSet:_otrCardSet];
    }
  } else {
    return NO;
  }

  NSUInteger firstCardIndex = std::max(
      [_activeCardSet.stackModel lastStartStackCardIndex], (NSInteger)0);
  StackCard* card = [_activeCardSet.cards objectAtIndex:firstCardIndex];
  [[card view] postAccessibilityNotification];
  [self postOpenTabsAccessibilityNotification];

  return YES;
}

// Posts accessibility notification that announces to the user which tabs are
// currently visible on the screen.
- (void)postOpenTabsAccessibilityNotification {
  if ([_activeCardSet.cards count] == 0) {
    return;
  }

  NSInteger count = [_activeCardSet.cards count];
  NSInteger lastStartIndex =
      [_activeCardSet.stackModel lastStartStackCardIndex];
  DCHECK(lastStartIndex < (int)[_activeCardSet.cards count]);
  if (lastStartIndex < 0) {
    lastStartIndex = 0;
  }
  NSInteger firstEndIndex = [_activeCardSet.stackModel firstEndStackCardIndex];
  NSInteger first = lastStartIndex < 0 ? 1 : lastStartIndex + 1;
  NSInteger last = firstEndIndex < 0 ? count : firstEndIndex;

  // Post notification to voiceover to read which tabs are currently visible.
  NSString* incognito = [self isCurrentSetIncognito] ? @"Incognito" : @"";
  NSString* firstVisible = [NSString stringWithFormat:@"%" PRIuNS, first];
  NSString* lastVisible = [NSString stringWithFormat:@"%" PRIuNS, last];
  NSString* numCards = [NSString stringWithFormat:@"%" PRIuNS, count];
  UIAccessibilityPostNotification(
      UIAccessibilityPageScrolledNotification,
      l10n_util::GetNSStringF(IDS_IOS_CARD_STACK_SCROLLED_NOTIFICATION,
                              base::SysNSStringToUTF16(incognito),
                              base::SysNSStringToUTF16(firstVisible),
                              base::SysNSStringToUTF16(lastVisible),
                              base::SysNSStringToUTF16(numCards)));
}

// Returns the StackCard with |element| in its view hierarchy.
// Handles CardView, TitleLabel, and CloseButton elements.
- (StackCard*)cardForAccessibilityElement:(UIView*)element {
  DCHECK([element isKindOfClass:[CardView class]] ||
         [element isKindOfClass:[TitleLabel class]] ||
         [element isKindOfClass:[CloseButton class]]);

  if ([element isKindOfClass:[CardView class]]) {
    for (StackCard* card in _activeCardSet.cards) {
      if (card.view == element) {
        return card;
      }
    }
  } else {
    for (StackCard* card in _activeCardSet.cards) {
      if (card.view == element.superview.superview) {
        return card;
      }
    }
  }
  return nil;
}

- (void)accessibilityFocusedOnElement:(id)element {
  StackCard* card = [self cardForAccessibilityElement:element];
  DCHECK(card);
  NSInteger index = [_activeCardSet.cards indexOfObject:card];

  if (![_activeCardSet.stackModel cardLabelCovered:card]) {
    return;
  }

  if (index >= [_activeCardSet.stackModel firstEndStackCardIndex] - 1) {
    // If card is in the end stack, scroll it toward the start.
    [_activeCardSet scrollCardAtIndex:index awayFromNeighbor:NO];
  } else if (index == [_activeCardSet.stackModel lastStartStackCardIndex] - 1) {
    // If card is the last covered card in the start stack, scroll the last
    // start stack card away from the start stack to reveal it.
    [_activeCardSet scrollCardAtIndex:index + 1 awayFromNeighbor:YES];
  } else {
    // If the card is in the middle of a stack that is not the end stack, fan
    // the cards out starting with that card.
    [_activeCardSet fanOutCardsWithStartIndex:index];
    [card.view postAccessibilityNotification];
    [self postOpenTabsAccessibilityNotification];
  }
}

#pragma mark - UIResponder

- (NSArray*)keyCommands {
  __weak StackViewController* weakSelf = self;

  // New tab blocks.
  void (^newTab)() = ^{
    [weakSelf.dispatcher openNewTab:[OpenNewTabCommand command]];
  };

  void (^newIncognitoTab)() = ^{
    [weakSelf.dispatcher openNewTab:[OpenNewTabCommand incognitoTabCommand]];
  };

  return @[
    [UIKeyCommand cr_keyCommandWithInput:@"t"
                           modifierFlags:UIKeyModifierCommand
                                   title:l10n_util::GetNSStringWithFixup(
                                             IDS_IOS_TOOLS_MENU_NEW_TAB)
                                  action:newTab],
    [UIKeyCommand
        cr_keyCommandWithInput:@"n"
                 modifierFlags:UIKeyModifierCommand | UIKeyModifierShift
                         title:l10n_util::GetNSStringWithFixup(
                                   IDS_IOS_TOOLS_MENU_NEW_INCOGNITO_TAB)
                        action:newIncognitoTab],
    [UIKeyCommand cr_keyCommandWithInput:@"n"
                           modifierFlags:UIKeyModifierCommand
                                   title:nil
                                  action:newTab],
  ];
}

@end

@implementation StackViewController (Testing)

- (UIScrollView*)scrollView {
  return _scrollView;
}

@end
