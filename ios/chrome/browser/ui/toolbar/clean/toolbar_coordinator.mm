// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/toolbar/clean/toolbar_coordinator.h"

#include <memory>

#include "base/mac/foundation_util.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "base/strings/sys_string_conversions.h"
#include "components/strings/grit/components_strings.h"
#include "ios/chrome/browser/bookmarks/bookmark_model_factory.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/reading_list/reading_list_model_factory.h"
#import "ios/chrome/browser/ui/commands/toolbar_commands.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_controller.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_controller_factory.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_ui_updater.h"
#import "ios/chrome/browser/ui/location_bar/location_bar_legacy_coordinator.h"
#import "ios/chrome/browser/ui/ntp/ntp_util.h"
#import "ios/chrome/browser/ui/omnibox/omnibox_text_field_ios.h"
#import "ios/chrome/browser/ui/omnibox/popup/omnibox_popup_positioner.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_button_factory.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_button_visibility_configuration.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_style.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_tools_menu_button.h"
#import "ios/chrome/browser/ui/toolbar/buttons/tools_menu_button_observer_bridge.h"
#import "ios/chrome/browser/ui/toolbar/clean/toolbar_button_updater.h"
#import "ios/chrome/browser/ui/toolbar/clean/toolbar_coordinator_delegate.h"
#import "ios/chrome/browser/ui/toolbar/clean/toolbar_mediator.h"
#import "ios/chrome/browser/ui/toolbar/clean/toolbar_view_controller.h"
#import "ios/chrome/browser/ui/toolbar/public/fakebox_focuser.h"
#import "ios/chrome/browser/ui/toolbar/public/omnibox_focuser.h"
#import "ios/chrome/browser/ui/tools_menu/public/tools_menu_constants.h"
#import "ios/chrome/browser/ui/tools_menu/public/tools_menu_presentation_provider.h"
#import "ios/chrome/browser/ui/tools_menu/tools_menu_coordinator.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/browser/ui/url_loader.h"
#import "ios/chrome/browser/ui/voice/text_to_speech_player.h"
#import "ios/chrome/browser/ui/voice/voice_search_notification_names.h"
#import "ios/chrome/browser/web_state_list/web_state_list.h"
#import "ios/chrome/common/material_timing.h"
#import "ios/public/provider/chrome/browser/chrome_browser_provider.h"
#import "ios/web/public/web_state/web_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface ToolbarCoordinator ()<OmniboxPopupPositioner,
                                 ToolbarCommands,
                                 ToolsMenuPresentationProvider> {
  // Observer that updates |toolbarViewController| for fullscreen events.
  std::unique_ptr<FullscreenControllerObserver> _fullscreenObserver;
}

// The View Controller managed by this coordinator.
@property(nonatomic, strong) ToolbarViewController* toolbarViewController;
// The mediator owned by this coordinator.
@property(nonatomic, strong) ToolbarMediator* mediator;
// Whether this coordinator has been started.
@property(nonatomic, assign) BOOL started;
// Button observer for the ToolsMenu button.
@property(nonatomic, strong)
    ToolsMenuButtonObserverBridge* toolsMenuButtonObserverBridge;
// The coordinator for the location bar in the toolbar.
@property(nonatomic, strong)
    LocationBarLegacyCoordinator* locationBarCoordinator;
// Weak reference to ChromeBrowserState;
@property(nonatomic, assign) ios::ChromeBrowserState* browserState;
// The dispatcher for this view controller.
@property(nonatomic, weak) CommandDispatcher* dispatcher;
// The commands endpoint for this view controller.
@property(nonatomic, weak)
    id<ApplicationCommands, BrowserCommands, OmniboxFocuser>
        commandsEndpoint;
// Coordinator for the tools menu UI.
@property(nonatomic, strong) ToolsMenuCoordinator* toolsMenuCoordinator;
// Button updater for the toolbar.
@property(nonatomic, strong) ToolbarButtonUpdater* buttonUpdater;

@end

@implementation ToolbarCoordinator
@synthesize delegate = _delegate;
@synthesize browserState = _browserState;
@synthesize buttonUpdater = _buttonUpdater;
@synthesize dispatcher = _dispatcher;
@synthesize mediator = _mediator;
@synthesize started = _started;
@synthesize toolbarViewController = _toolbarViewController;
@synthesize toolsMenuButtonObserverBridge = _toolsMenuButtonObserverBridge;
@synthesize toolsMenuCoordinator = _toolsMenuCoordinator;
@synthesize URLLoader = _URLLoader;
@synthesize webStateList = _webStateList;
@synthesize locationBarCoordinator = _locationBarCoordinator;
@synthesize commandsEndpoint = _commandsEndpoint;

- (instancetype)
initWithToolsMenuConfigurationProvider:
    (id<ToolsMenuConfigurationProvider>)configurationProvider
                            dispatcher:(CommandDispatcher*)dispatcher
                          browserState:(ios::ChromeBrowserState*)browserState {
  self = [super init];
  if (self) {
    DCHECK(browserState);
    _mediator = [[ToolbarMediator alloc] init];
    _dispatcher = dispatcher;
    _commandsEndpoint =
        static_cast<CommandDispatcher<ApplicationCommands, BrowserCommands,
                                      OmniboxFocuser>*>(dispatcher);
    _browserState = browserState;

    _toolsMenuCoordinator = [[ToolsMenuCoordinator alloc] init];
    _toolsMenuCoordinator.dispatcher = dispatcher;
    _toolsMenuCoordinator.configurationProvider = configurationProvider;
    _toolsMenuCoordinator.presentationProvider = self;
    [_toolsMenuCoordinator start];

    [dispatcher startDispatchingToTarget:self
                             forProtocol:@protocol(ToolbarCommands)];

    NSNotificationCenter* defaultCenter = [NSNotificationCenter defaultCenter];
    [defaultCenter addObserver:self
                      selector:@selector(toolsMenuWillShowNotification:)
                          name:kToolsMenuWillShowNotification
                        object:_toolsMenuCoordinator];
    [defaultCenter addObserver:self
                      selector:@selector(toolsMenuWillHideNotification:)
                          name:kToolsMenuWillHideNotification
                        object:_toolsMenuCoordinator];
  }
  return self;
}

- (instancetype)init {
  if ((self = [super init])) {
  }
  return self;
}

#pragma mark - Properties

- (UIViewController*)viewController {
  return self.toolbarViewController;
}

- (void)setDelegate:(id<ToolbarCoordinatorDelegate>)delegate {
  if (_delegate == delegate)
    return;

  // TTS notifications cannot be handled without a delegate.
  if (_delegate && self.started)
    [self stopObservingTTSNotifications];
  _delegate = delegate;
  if (_delegate && self.started)
    [self startObservingTTSNotifications];
}

#pragma mark - BrowserCoordinator

- (void)start {
  if (self.started)
    return;

  self.started = YES;
  BOOL isIncognito = self.browserState->IsOffTheRecord();

  self.locationBarCoordinator = [[LocationBarLegacyCoordinator alloc] init];
  self.locationBarCoordinator.browserState = self.browserState;
  self.locationBarCoordinator.dispatcher = self.dispatcher;
  self.locationBarCoordinator.URLLoader = self.URLLoader;
  self.locationBarCoordinator.delegate = self.delegate;
  self.locationBarCoordinator.webStateList = self.webStateList;
  self.locationBarCoordinator.popupPositioner = self;
  [self.locationBarCoordinator start];

  ToolbarStyle style = isIncognito ? INCOGNITO : NORMAL;
  ToolbarButtonFactory* factory =
      [[ToolbarButtonFactory alloc] initWithStyle:style];
  factory.dispatcher = self.commandsEndpoint;
  factory.visibilityConfiguration =
      [[ToolbarButtonVisibilityConfiguration alloc] initWithType:LEGACY];

  self.buttonUpdater = [[ToolbarButtonUpdater alloc] init];
  self.buttonUpdater.factory = factory;
  self.toolbarViewController = [[ToolbarViewController alloc]
      initWithDispatcher:self.commandsEndpoint
           buttonFactory:factory
           buttonUpdater:self.buttonUpdater
          omniboxFocuser:self.locationBarCoordinator];
  self.toolbarViewController.locationBarView = self.locationBarCoordinator.view;
  self.toolbarViewController.dispatcher = self.commandsEndpoint;

  _fullscreenObserver =
      std::make_unique<FullscreenUIUpdater>(self.toolbarViewController);
  FullscreenControllerFactory::GetInstance()
      ->GetForBrowserState(self.browserState)
      ->AddObserver(_fullscreenObserver.get());

  DCHECK(self.toolbarViewController.toolsMenuButton);
  self.toolsMenuButtonObserverBridge = [[ToolsMenuButtonObserverBridge alloc]
      initWithModel:ReadingListModelFactory::GetForBrowserState(
                        self.browserState)
      toolbarButton:self.toolbarViewController.toolsMenuButton];

  self.mediator.consumer = self.toolbarViewController;
  self.mediator.webStateList = self.webStateList;
  self.mediator.bookmarkModel =
      ios::BookmarkModelFactory::GetForBrowserState(self.browserState);

  if (self.delegate)
    [self startObservingTTSNotifications];
}

- (void)stop {
  if (!self.started)
    return;

  [self setToolbarBackgroundToIncognitoNTPColorWithAlpha:0];
  self.started = NO;
  self.delegate = nil;
  [self.mediator disconnect];
  [self.locationBarCoordinator stop];
  [self stopObservingTTSNotifications];

  FullscreenControllerFactory::GetInstance()
      ->GetForBrowserState(self.browserState)
      ->RemoveObserver(_fullscreenObserver.get());
  _fullscreenObserver = nullptr;
}

#pragma mark - PrimaryToolbarCoordinator

- (id<VoiceSearchControllerDelegate>)voiceSearchDelegate {
  return self.locationBarCoordinator;
}

- (id<QRScannerResultLoading>)QRScannerResultLoader {
  return self.locationBarCoordinator;
}

- (id<TabHistoryUIUpdater>)tabHistoryUIUpdater {
  return self.buttonUpdater;
}

- (id<ActivityServicePositioner>)activityServicePositioner {
  return self.toolbarViewController;
}

- (id<OmniboxFocuser>)omniboxFocuser {
  return self.locationBarCoordinator;
}

- (void)showPrerenderingAnimation {
  [self.toolbarViewController showPrerenderingAnimation];
}

- (BOOL)isOmniboxFirstResponder {
  return [self.locationBarCoordinator isOmniboxFirstResponder];
}

- (BOOL)showingOmniboxPopup {
  return [self.locationBarCoordinator showingOmniboxPopup];
}

- (void)transitionToLocationBarFocusedState:(BOOL)focused {
  if (IsIPadIdiom()) {
    [self.toolbarViewController locationBarIsFirstResonderOnIPad:focused];
    return;
  }

  DCHECK(!IsIPadIdiom());
  if (focused == self.toolbarViewController.expanded) {
    // The view controller is already in the correct state.
    return;
  }

  if (focused) {
    [self expandOmniboxAnimated:YES];
  } else {
    [self contractOmnibox];
  }
}

// TODO(crbug.com/786940): This protocol should move to the ViewController
// owning the Toolbar. This can wait until the omnibox and toolbar refactoring
// is more advanced.
#pragma mark OmniboxPopupPositioner methods.

- (UIView*)popupParentView {
  return self.toolbarViewController.view.superview;
}

#pragma mark - ToolsMenuPresentationStateProvider

- (BOOL)isShowingToolsMenu {
  return [_toolsMenuCoordinator isShowingToolsMenu];
}

#pragma mark - ToolbarCoordinating

- (void)updateToolsMenu {
  [_toolsMenuCoordinator updateConfiguration];
}

#pragma mark - ToolbarCommands

- (void)triggerToolsMenuButtonAnimation {
  [self.toolbarViewController.toolsMenuButton triggerAnimation];
}

#pragma mark - NewTabPageControllerDelegate

- (void)setToolbarBackgroundToIncognitoNTPColorWithAlpha:(CGFloat)alpha {
  [self.toolbarViewController setBackgroundToIncognitoNTPColorWithAlpha:alpha];
}

- (void)setScrollProgressForTabletOmnibox:(CGFloat)progress {
  NOTREACHED();
}

#pragma mark - ToolbarSnapshotProviding

- (UIView*)snapshotForTabSwitcher {
  UIView* toolbarSnapshotView;
  if ([self.viewController.view window]) {
    toolbarSnapshotView =
        [self.viewController.view snapshotViewAfterScreenUpdates:NO];
  } else {
    toolbarSnapshotView =
        [[UIView alloc] initWithFrame:self.viewController.view.frame];
    [toolbarSnapshotView layer].contents = static_cast<id>(
        CaptureViewWithOption(self.viewController.view, 0, kClientSideRendering)
            .CGImage);
  }
  return toolbarSnapshotView;
}

- (UIView*)snapshotForStackViewWithWidth:(CGFloat)width
                          safeAreaInsets:(UIEdgeInsets)safeAreaInsets {
  CGRect oldFrame = self.viewController.view.superview.frame;
  CGRect newFrame = oldFrame;
  newFrame.size.width = width;

  if (self.webStateList->GetActiveWebState())
    [self updateToolbarForSnapshot:self.webStateList->GetActiveWebState()];

  self.viewController.view.superview.frame = newFrame;
  [self.toolbarViewController activateFakeSafeAreaInsets:safeAreaInsets];
  [self.viewController.view.superview layoutIfNeeded];

  UIView* toolbarSnapshotView = [self snapshotForTabSwitcher];

  self.viewController.view.superview.frame = oldFrame;
  [self.toolbarViewController deactivateFakeSafeAreaInsets];

  if (self.webStateList->GetActiveWebState())
    [self resetToolbarAfterSnapshot];

  return toolbarSnapshotView;
}

- (UIColor*)toolbarBackgroundColor {
  if (self.webStateList && self.webStateList->GetActiveWebState() &&
      IsVisibleUrlNewTabPage(self.webStateList->GetActiveWebState()))
    return self.toolbarViewController.backgroundColorNTP;
  return nil;
}

#pragma mark - FakeboxFocuser

- (void)focusFakebox {
  if (IsIPadIdiom()) {
    // On iPhone there is no visible omnibox, so there's no need to indicate
    // interaction was initiated from the fakebox.
    [self.locationBarCoordinator focusOmniboxFromFakebox];
  } else {
    [self expandOmniboxAnimated:NO];
    [self.locationBarCoordinator focusOmnibox];
  }

  if ([self.locationBarCoordinator omniboxPopupHasAutocompleteResults]) {
    [self onFakeboxAnimationComplete];
  }
}

- (void)onFakeboxBlur {
  DCHECK(!IsIPadIdiom());
  // Hide the toolbar if the NTP is currently displayed.
  web::WebState* webState = self.webStateList->GetActiveWebState();
  if (webState && IsVisibleUrlNewTabPage(webState)) {
    self.viewController.view.hidden = YES;
  }
}

- (void)onFakeboxAnimationComplete {
  DCHECK(!IsIPadIdiom());
  self.viewController.view.hidden = NO;
}

#pragma mark - SideSwipeToolbarInteracting

- (BOOL)isInsideToolbar:(CGPoint)point {
  // The toolbar frame is inset by -1 because CGRectContainsPoint does include
  // points on the max X and Y edges, which will happen frequently with edge
  // swipes from the right side.
  CGRect toolbarFrame = CGRectInset(self.viewController.view.frame, -1, -1);
  return CGRectContainsPoint(toolbarFrame, point);
}

#pragma mark - SideSwipeToolbarSnapshotProviding

- (UIImage*)toolbarSideSwipeSnapshotForWebState:(web::WebState*)webState {
  [self updateToolbarForSnapshot:webState];
  UIImage* toolbarSnapshot = CaptureViewWithOption(
      [self.viewController view], [[UIScreen mainScreen] scale],
      kClientSideRendering);

  [self resetToolbarAfterSnapshot];
  return toolbarSnapshot;
}

#pragma mark - ToolsMenuPresentationProvider

- (UIButton*)presentingButtonForToolsMenuCoordinator:
    (ToolsMenuCoordinator*)coordinator {
  return self.toolbarViewController.toolsMenuButton;
}

#pragma mark - TTS

// Starts observing the NSNotifications from the Text-To-Speech player.
- (void)startObservingTTSNotifications {
  // The toolbar is only used to play text-to-speech search results on iPads.
  if (IsIPadIdiom()) {
    NSNotificationCenter* defaultCenter = [NSNotificationCenter defaultCenter];
    [defaultCenter addObserver:self
                      selector:@selector(audioReadyForPlayback:)
                          name:kTTSAudioReadyForPlaybackNotification
                        object:nil];
    const auto& selectorsForTTSNotifications =
        [[self class] selectorsForTTSNotificationNames];
    for (const auto& selectorForNotification : selectorsForTTSNotifications) {
      [defaultCenter addObserver:self.buttonUpdater
                        selector:selectorForNotification.second
                            name:selectorForNotification.first
                          object:nil];
    }
  }
}

// Stops observing the NSNotifications from the Text-To-Speech player.
- (void)stopObservingTTSNotifications {
  NSNotificationCenter* defaultCenter = [NSNotificationCenter defaultCenter];
  [defaultCenter removeObserver:self
                           name:kTTSAudioReadyForPlaybackNotification
                         object:nil];
  const auto& selectorsForTTSNotifications =
      [[self class] selectorsForTTSNotificationNames];
  for (const auto& selectorForNotification : selectorsForTTSNotifications) {
    [defaultCenter removeObserver:self.buttonUpdater
                             name:selectorForNotification.first
                           object:nil];
  }
}

// Returns a map where the keys are names of text-to-speech notifications and
// the values are the selectors to use for these notifications.
+ (const std::map<__strong NSString*, SEL>&)selectorsForTTSNotificationNames {
  static std::map<__strong NSString*, SEL> selectorsForNotifications;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    selectorsForNotifications[kTTSWillStartPlayingNotification] =
        @selector(updateIsTTSPlaying:);
    selectorsForNotifications[kTTSDidStopPlayingNotification] =
        @selector(updateIsTTSPlaying:);
    selectorsForNotifications[kVoiceSearchWillHideNotification] =
        @selector(moveVoiceOverToVoiceSearchButton);
  });
  return selectorsForNotifications;
}

// Received when a TTS player has received audio data.
- (void)audioReadyForPlayback:(NSNotification*)notification {
  if ([self.buttonUpdater canStartPlayingTTS]) {
    // Only trigger TTS playback when the voice search button is visible.
    TextToSpeechPlayer* TTSPlayer =
        base::mac::ObjCCastStrict<TextToSpeechPlayer>(notification.object);
    [TTSPlayer beginPlayback];
  }
}

#pragma mark - Private

// Updates the toolbar so it is in a state where a snapshot for |webState| can
// be taken.
- (void)updateToolbarForSnapshot:(web::WebState*)webState {
  BOOL isNTP = IsVisibleUrlNewTabPage(webState);

  self.viewController.view.hidden = NO;
  // Don't do anything for the current tab if it is not a non-incognito NTP.
  if (webState == self.webStateList->GetActiveWebState() &&
      !(isNTP && !self.browserState->IsOffTheRecord())) {
    [self.locationBarCoordinator.view setHidden:NO];
    return;
  }

  [self.locationBarCoordinator.view setHidden:YES];
  [self.mediator updateConsumerForWebState:webState];
  [self.toolbarViewController updateForSnapshotOnNTP:isNTP];
}

// Resets the toolbar after taking a snapshot. After calling this method the
// toolbar is adapted to the current webState.
- (void)resetToolbarAfterSnapshot {
  [self.mediator
      updateConsumerForWebState:self.webStateList->GetActiveWebState()];
  [self.locationBarCoordinator.view setHidden:NO];
  [self.toolbarViewController resetAfterSnapshot];
}

// Animates |_toolbar| and |_locationBarView| for omnibox expansion. If
// |animated| is NO the animation will happen instantly.
- (void)expandOmniboxAnimated:(BOOL)animated {
  // iPad should never try to expand.
  DCHECK(!IsIPadIdiom());

  UIViewPropertyAnimator* animator = [[UIViewPropertyAnimator alloc]
      initWithDuration:ios::material::kDuration1
                 curve:UIViewAnimationCurveEaseInOut
            animations:nil];
  UIViewPropertyAnimator* completionAnimator = [[UIViewPropertyAnimator alloc]
      initWithDuration:ios::material::kDuration1
                 curve:UIViewAnimationCurveEaseOut
            animations:nil];
  [animator addCompletion:^(UIViewAnimatingPosition finalPosition) {
    [completionAnimator startAnimationAfterDelay:ios::material::kDuration4];
  }];

  [self.locationBarCoordinator addExpandOmniboxAnimations:animator
                                       completionAnimator:completionAnimator];
  [self.toolbarViewController addToolbarExpansionAnimations:animator
                                         completionAnimator:completionAnimator];
  [animator startAnimation];

  if (!animated) {
    [animator stopAnimation:NO];
    [animator finishAnimationAtPosition:UIViewAnimatingPositionEnd];
  }
}

// Animates |_toolbar| and |_locationBarView| for omnibox contraction.
- (void)contractOmnibox {
  // iPad should never try to contract.
  DCHECK(!IsIPadIdiom());

  UIViewPropertyAnimator* animator = [[UIViewPropertyAnimator alloc]
      initWithDuration:ios::material::kDuration1
                 curve:UIViewAnimationCurveEaseInOut
            animations:^{
            }];
  [self.locationBarCoordinator addContractOmniboxAnimations:animator];
  [self.toolbarViewController addToolbarContractionAnimations:animator];
  [animator startAnimation];
}

// Called when the tools menu will show.
- (void)toolsMenuWillShowNotification:(NSNotification*)note {
  [self.toolbarViewController.toolsMenuButton setToolsMenuIsVisible:YES];
}

// Called when the tools menu will hide.
- (void)toolsMenuWillHideNotification:(NSNotification*)note {
  [self.toolbarViewController.toolsMenuButton setToolsMenuIsVisible:NO];
}

@end
