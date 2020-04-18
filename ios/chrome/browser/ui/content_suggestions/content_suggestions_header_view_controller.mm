// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/content_suggestions/content_suggestions_header_view_controller.h"

#include "base/logging.h"
#include "base/mac/foundation_util.h"
#include "base/metrics/user_metrics.h"
#include "components/strings/grit/components_strings.h"
#import "ios/chrome/browser/ui/UIView+SizeClassSupport.h"
#import "ios/chrome/browser/ui/commands/application_commands.h"
#import "ios/chrome/browser/ui/commands/browser_commands.h"
#import "ios/chrome/browser/ui/content_suggestions/content_suggestions_collection_synchronizing.h"
#import "ios/chrome/browser/ui/content_suggestions/content_suggestions_collection_utils.h"
#import "ios/chrome/browser/ui/content_suggestions/content_suggestions_commands.h"
#import "ios/chrome/browser/ui/content_suggestions/content_suggestions_header_view.h"
#import "ios/chrome/browser/ui/content_suggestions/content_suggestions_header_view_controller_delegate.h"
#import "ios/chrome/browser/ui/content_suggestions/ntp_home_constant.h"
#import "ios/chrome/browser/ui/ntp/new_tab_page_header_constants.h"
#import "ios/chrome/browser/ui/ntp/new_tab_page_header_view.h"
#import "ios/chrome/browser/ui/toolbar/adaptive/primary_toolbar_view.h"
#import "ios/chrome/browser/ui/toolbar/adaptive/primary_toolbar_view_controller.h"
#import "ios/chrome/browser/ui/toolbar/public/fakebox_focuser.h"
#import "ios/chrome/browser/ui/toolbar/public/omnibox_focuser.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/browser/ui/url_loader.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#import "ios/chrome/browser/ui/util/named_guide.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ios/public/provider/chrome/browser/ui/logo_vendor.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using base::UserMetricsAction;

namespace {
const UIEdgeInsets kSearchBoxStretchInsets = {3, 3, 3, 3};
}  // namespace

@interface ContentSuggestionsHeaderViewController ()

// |YES| when notifications indicate the omnibox is focused.
@property(nonatomic, assign, getter=isOmniboxFocused) BOOL omniboxFocused;

// |YES| if this consumer is has voice search enabled.
@property(nonatomic, assign) BOOL voiceSearchIsEnabled;

// Exposes view and methods to drive the doodle.
@property(nonatomic, weak) id<LogoVendor> logoVendor;

// |YES| if the google landing toolbar can show the forward arrow, cached and
// pushed into the header view.
@property(nonatomic, assign) BOOL canGoForward;

// |YES| if the google landing toolbar can show the back arrow, cached and
// pushed into the header view.
@property(nonatomic, assign) BOOL canGoBack;

// The number of tabs to show in the google landing fake toolbar.
@property(nonatomic, assign) int tabCount;

@property(nonatomic, strong) UIView<NTPHeaderViewAdapter>* headerView;
@property(nonatomic, strong) UIButton* fakeOmnibox;
@property(nonatomic, strong) UILabel* searchHintLabel;
@property(nonatomic, strong) NSLayoutConstraint* hintLabelLeadingConstraint;
@property(nonatomic, strong) NSLayoutConstraint* voiceTapTrailingConstraint;
@property(nonatomic, strong) NSLayoutConstraint* doodleHeightConstraint;
@property(nonatomic, strong) NSLayoutConstraint* doodleTopMarginConstraint;
@property(nonatomic, strong) NSLayoutConstraint* fakeOmniboxWidthConstraint;
@property(nonatomic, strong) NSLayoutConstraint* fakeOmniboxHeightConstraint;
@property(nonatomic, strong) NSLayoutConstraint* fakeOmniboxTopMarginConstraint;
@property(nonatomic, assign) BOOL logoFetched;

@end

@implementation ContentSuggestionsHeaderViewController

@synthesize dispatcher = _dispatcher;
@synthesize delegate = _delegate;
@synthesize commandHandler = _commandHandler;
@synthesize searchHintLabel = _searchHintLabel;
@synthesize collectionSynchronizer = _collectionSynchronizer;
@synthesize readingListModel = _readingListModel;
@synthesize toolbarDelegate = _toolbarDelegate;
@synthesize logoVendor = _logoVendor;
@synthesize promoCanShow = _promoCanShow;
@synthesize canGoForward = _canGoForward;
@synthesize canGoBack = _canGoBack;
@synthesize isShowing = _isShowing;
@synthesize omniboxFocused = _omniboxFocused;
@synthesize tabCount = _tabCount;

@synthesize headerView = _headerView;
@synthesize fakeOmnibox = _fakeOmnibox;
@synthesize hintLabelLeadingConstraint = _hintLabelLeadingConstraint;
@synthesize voiceTapTrailingConstraint = _voiceTapTrailingConstraint;
@synthesize doodleHeightConstraint = _doodleHeightConstraint;
@synthesize doodleTopMarginConstraint = _doodleTopMarginConstraint;
@synthesize fakeOmniboxWidthConstraint = _fakeOmniboxWidthConstraint;
@synthesize fakeOmniboxHeightConstraint = _fakeOmniboxHeightConstraint;
@synthesize fakeOmniboxTopMarginConstraint = _fakeOmniboxTopMarginConstraint;
@synthesize voiceSearchIsEnabled = _voiceSearchIsEnabled;
@synthesize logoIsShowing = _logoIsShowing;
@synthesize logoFetched = _logoFetched;
@synthesize toolbarViewController = _toolbarViewController;

#pragma mark - Public

- (instancetype)initWithVoiceSearchEnabled:(BOOL)voiceSearchIsEnabled {
  self = [super initWithNibName:nil bundle:nil];
  if (self) {
    _voiceSearchIsEnabled = voiceSearchIsEnabled;
  }
  return self;
}

- (UIView*)toolBarView {
  return self.headerView.toolBarView;
}

- (void)willTransitionToTraitCollection:(UITraitCollection*)newCollection
              withTransitionCoordinator:
                  (id<UIViewControllerTransitionCoordinator>)coordinator {
  [super willTransitionToTraitCollection:newCollection
               withTransitionCoordinator:coordinator];
  if (!IsUIRefreshPhase1Enabled())
    return;

  void (^transition)(id<UIViewControllerTransitionCoordinatorContext>) =
      ^(id<UIViewControllerTransitionCoordinatorContext> context) {
        // Ensure omnibox is reset when not a regular tablet.
        if (IsCompactWidth(self) || IsCompactHeight(self)) {
          [self.toolbarDelegate setScrollProgressForTabletOmnibox:1];
        }

        // Update the doodle top margin to the new -doodleTopMargin value.
        self.doodleTopMarginConstraint.constant =
            content_suggestions::doodleTopMargin(YES);
      };

  [coordinator animateAlongsideTransition:transition completion:nil];
}

#pragma mark - Property

- (void)setIsShowing:(BOOL)isShowing {
  _isShowing = isShowing;
  if (isShowing)
    [self.headerView hideToolbarViewsForNewTabPage];
}

#pragma mark - ContentSuggestionsHeaderControlling

- (void)updateFakeOmniboxForOffset:(CGFloat)offset
                       screenWidth:(CGFloat)screenWidth
                    safeAreaInsets:(UIEdgeInsets)safeAreaInsets {
  if (self.headerView.cr_widthSizeClass == REGULAR &&
      self.headerView.cr_heightSizeClass == REGULAR &&
      IsUIRefreshPhase1Enabled()) {
    CGFloat progress =
        [self.headerView searchFieldProgressForOffset:offset
                                       safeAreaInsets:safeAreaInsets];
    if (self.isShowing) {
      [self.toolbarDelegate setScrollProgressForTabletOmnibox:progress];
    }
  }

  NSArray* constraints =
      @[ self.hintLabelLeadingConstraint, self.voiceTapTrailingConstraint ];

  [self.headerView updateSearchFieldWidth:self.fakeOmniboxWidthConstraint
                                   height:self.fakeOmniboxHeightConstraint
                                topMargin:self.fakeOmniboxTopMarginConstraint
                                hintLabel:self.searchHintLabel
                       subviewConstraints:constraints
                                forOffset:offset
                              screenWidth:screenWidth
                           safeAreaInsets:safeAreaInsets];

  // Before constraining the |fakeTapView| to |locationBarContainer| make sure
  // to activate the constraints first.
  if (IsUIRefreshPhase1Enabled())
    [self.toolbarViewController contractLocationBar];
}

- (void)updateFakeOmniboxForWidth:(CGFloat)width {
  self.fakeOmniboxWidthConstraint.constant =
      content_suggestions::searchFieldWidth(width);
}

- (void)unfocusOmnibox {
  if (self.omniboxFocused) {
    [self.dispatcher cancelOmniboxEdit];
  } else {
    [self locationBarResignsFirstResponder];
  }
}

- (void)layoutHeader {
  [self.headerView layoutIfNeeded];
}

- (CGFloat)pinnedOffsetY {
  CGFloat headerHeight = content_suggestions::heightForLogoHeader(
      self.logoIsShowing, self.promoCanShow, YES);

  CGFloat offsetY =
      headerHeight - ntp_header::kScrolledToTopOmniboxBottomMargin;
  if (!content_suggestions::IsRegularXRegularSizeClass(self.view)) {
    CGFloat top = 0;
    if (@available(iOS 11, *)) {
      top = self.parentViewController.view.safeAreaInsets.top;
    } else if (IsUIRefreshPhase1Enabled()) {
      // TODO(crbug.com/826369) Replace this when the NTP is contained by the
      // BVC with |self.parentViewController.topLayoutGuide.length|.
      top = StatusBarHeight();
    }
    offsetY -= ntp_header::ToolbarHeight() + top;
  }

  return offsetY;
}

- (CGFloat)headerHeight {
  return content_suggestions::heightForLogoHeader(self.logoIsShowing,
                                                  self.promoCanShow, YES);
}

#pragma mark - ContentSuggestionsHeaderProvider

- (UIView*)headerForWidth:(CGFloat)width {
  if (!self.headerView) {
    if (IsUIRefreshPhase1Enabled()) {
      self.headerView = [[ContentSuggestionsHeaderView alloc] init];
      [self addFakeTapView];
    } else {
      self.headerView = [[NewTabPageHeaderView alloc] init];
    }
    [self addFakeOmnibox];

    [self.headerView addSubview:self.logoVendor.view];
    [self.headerView addSubview:self.fakeOmnibox];
    self.logoVendor.view.translatesAutoresizingMaskIntoConstraints = NO;
    self.fakeOmnibox.translatesAutoresizingMaskIntoConstraints = NO;

    // -headerForView is regularly called before self.headerView has been added
    // to the view hierarchy, so there's no simple way to get the correct
    // safeAreaInsets.  Since this situation is universally called for the full
    // screen new tab animation, it's safe to check the rootViewController's
    // view instead.
    // TODO(crbug.com/791784) : Remove use of rootViewController.
    UIView* insetsView = self.headerView;
    if (!self.headerView.window) {
      insetsView =
          [[UIApplication sharedApplication] keyWindow].rootViewController.view;
    }
    UIEdgeInsets safeAreaInsets = SafeAreaInsetsForView(insetsView);
    width = std::max<CGFloat>(
        0, width - safeAreaInsets.left - safeAreaInsets.right);

    self.fakeOmniboxWidthConstraint = [self.fakeOmnibox.widthAnchor
        constraintEqualToConstant:content_suggestions::searchFieldWidth(width)];
    [self addConstraintsForLogoView:self.logoVendor.view
                        fakeOmnibox:self.fakeOmnibox
                      andHeaderView:self.headerView];

    // iPhone header also contains a toolbar since the normal toolbar is
    // hidden.
    if (IsUIRefreshPhase1Enabled()) {
      // This view controller's view is never actually used, so add to the
      // parent view controller to avoid hierarchy inconsistencies.
      [self.parentViewController
          addChildViewController:self.toolbarViewController];
      [_headerView addToolbarView:self.toolbarViewController.view];
      [self.toolbarViewController
          didMoveToParentViewController:self.parentViewController];
    } else if (!IsIPadIdiom()) {
      [_headerView addToolbarWithReadingListModel:self.readingListModel
                                       dispatcher:self.dispatcher];
      [_headerView setToolbarTabCount:self.tabCount];
      [_headerView setCanGoForward:self.canGoForward];
      [_headerView setCanGoBack:self.canGoBack];
    }

    [self.headerView addViewsToSearchField:self.fakeOmnibox];
    [self.logoVendor fetchDoodle];
  }
  return self.headerView;
}

#pragma mark - Private

// Initialize and add a search field tap target and a voice search button.
- (void)addFakeOmnibox {
  self.fakeOmnibox = [[UIButton alloc] init];
  if (IsIPadIdiom() && !IsUIRefreshPhase1Enabled()) {
    UIImage* searchBoxImage = [[UIImage imageNamed:@"ntp_google_search_box"]
        resizableImageWithCapInsets:kSearchBoxStretchInsets];
    [self.fakeOmnibox setBackgroundImage:searchBoxImage
                                forState:UIControlStateNormal];
  }
  [self.fakeOmnibox setAdjustsImageWhenHighlighted:NO];

  // Set isAccessibilityElement to NO so that Voice Search button is accessible.
  [self.fakeOmnibox setIsAccessibilityElement:NO];
  self.fakeOmnibox.accessibilityIdentifier =
      ntp_home::FakeOmniboxAccessibilityID();

  // Set up fakebox hint label.
  _searchHintLabel = [[UILabel alloc] init];
  UIView* hintLabelContainer = [[UIView alloc] init];
  content_suggestions::configureSearchHintLabel(
      _searchHintLabel, hintLabelContainer, self.fakeOmnibox);

  self.hintLabelLeadingConstraint = [hintLabelContainer.leadingAnchor
      constraintEqualToAnchor:[self.fakeOmnibox leadingAnchor]
                     constant:ntp_header::kHintLabelSidePadding];
  if (!IsUIRefreshPhase1Enabled())
    self.hintLabelLeadingConstraint.constant =
        ntp_header::kHintLabelSidePaddingLegacy;
  [self.hintLabelLeadingConstraint setActive:YES];

  // Set a button the same size as the fake omnibox as the accessibility
  // element. If the hint is the only accessible element, when the fake omnibox
  // is taking the full width, there are few points that are not accessible and
  // allow to select the content below it.
  _searchHintLabel.isAccessibilityElement = NO;
  UIButton* accessibilityButton = [[UIButton alloc] init];
  [accessibilityButton addTarget:self
                          action:@selector(fakeOmniboxTapped:)
                forControlEvents:UIControlEventTouchUpInside];
  accessibilityButton.isAccessibilityElement = YES;
  accessibilityButton.accessibilityLabel =
      l10n_util::GetNSString(IDS_OMNIBOX_EMPTY_HINT);
  [self.fakeOmnibox addSubview:accessibilityButton];
  accessibilityButton.translatesAutoresizingMaskIntoConstraints = NO;
  AddSameConstraints(self.fakeOmnibox, accessibilityButton);

  // Add a voice search button.
  UIButton* voiceTapTarget = [[UIButton alloc] init];
  content_suggestions::configureVoiceSearchButton(voiceTapTarget,
                                                  self.fakeOmnibox);

  self.voiceTapTrailingConstraint = [voiceTapTarget.trailingAnchor
      constraintEqualToAnchor:[self.fakeOmnibox trailingAnchor]];
  [NSLayoutConstraint activateConstraints:@[
    [hintLabelContainer.trailingAnchor
        constraintEqualToAnchor:voiceTapTarget.leadingAnchor],
    _voiceTapTrailingConstraint
  ]];

  if (self.voiceSearchIsEnabled) {
    [voiceTapTarget addTarget:self
                       action:@selector(loadVoiceSearch:)
             forControlEvents:UIControlEventTouchUpInside];
    [voiceTapTarget addTarget:self
                       action:@selector(preloadVoiceSearch:)
             forControlEvents:UIControlEventTouchDown];
  } else {
    [voiceTapTarget setEnabled:NO];
  }
}

- (void)addFakeTapView {
  UIButton* fakeTapButton = [[UIButton alloc] init];
  fakeTapButton.translatesAutoresizingMaskIntoConstraints = NO;
  [self.toolbarViewController.view addSubview:fakeTapButton];
  PrimaryToolbarView* primaryToolbarView =
      base::mac::ObjCCastStrict<PrimaryToolbarView>(
          self.toolbarViewController.view);
  UIView* locationBarContainer = primaryToolbarView.locationBarContainer;
  AddSameConstraints(locationBarContainer, fakeTapButton);
  [fakeTapButton addTarget:self
                    action:@selector(fakeOmniboxTapped:)
          forControlEvents:UIControlEventTouchUpInside];
}

- (void)loadVoiceSearch:(id)sender {
  [self.commandHandler dismissModals];

  DCHECK(self.voiceSearchIsEnabled);
  base::RecordAction(UserMetricsAction("MobileNTPMostVisitedVoiceSearch"));
  UIView* voiceSearchButton = base::mac::ObjCCastStrict<UIView>(sender);
  [NamedGuide guideWithName:kVoiceSearchButtonGuide view:voiceSearchButton]
      .constrainedView = voiceSearchButton;
  [self.dispatcher startVoiceSearch];
}

- (void)preloadVoiceSearch:(id)sender {
  DCHECK(self.voiceSearchIsEnabled);
  [sender removeTarget:self
                action:@selector(preloadVoiceSearch:)
      forControlEvents:UIControlEventTouchDown];
  [self.dispatcher preloadVoiceSearch];
}

- (void)fakeOmniboxTapped:(id)sender {
  [self.dispatcher focusFakebox];
}

// If Google is not the default search engine, hide the logo, doodle and
// fakebox. Make them appear if Google is set as default.
- (void)updateLogoAndFakeboxDisplay {
  if (self.logoVendor.showingLogo != self.logoIsShowing) {
    self.logoVendor.showingLogo = self.logoIsShowing;
    [self.doodleHeightConstraint
        setConstant:content_suggestions::doodleHeight(self.logoIsShowing)];
    if (content_suggestions::IsRegularXRegularSizeClass(self.view))
      [self.fakeOmnibox setHidden:!self.logoIsShowing];
    [self.collectionSynchronizer invalidateLayout];
  }
}

// Adds the constraints for the |logoView|, the |fakeomnibox| related to the
// |headerView|. It also sets the properties constraints related to those views.
- (void)addConstraintsForLogoView:(UIView*)logoView
                      fakeOmnibox:(UIView*)fakeOmnibox
                    andHeaderView:(UIView*)headerView {
  self.doodleTopMarginConstraint = [logoView.topAnchor
      constraintEqualToAnchor:headerView.topAnchor
                     constant:content_suggestions::doodleTopMargin(YES)];
  self.doodleHeightConstraint = [logoView.heightAnchor
      constraintEqualToConstant:content_suggestions::doodleHeight(
                                    self.logoIsShowing)];
  self.fakeOmniboxHeightConstraint = [fakeOmnibox.heightAnchor
      constraintEqualToConstant:content_suggestions::kSearchFieldHeight];
  self.fakeOmniboxTopMarginConstraint = [logoView.bottomAnchor
      constraintEqualToAnchor:fakeOmnibox.topAnchor
                     constant:-content_suggestions::searchFieldTopMargin()];
  self.doodleTopMarginConstraint.active = YES;
  self.doodleHeightConstraint.active = YES;
  self.fakeOmniboxWidthConstraint.active = YES;
  self.fakeOmniboxHeightConstraint.active = YES;
  self.fakeOmniboxTopMarginConstraint.active = YES;
  [logoView.widthAnchor constraintEqualToAnchor:headerView.widthAnchor].active =
      YES;
  [logoView.leadingAnchor constraintEqualToAnchor:headerView.leadingAnchor]
      .active = YES;
  [fakeOmnibox.centerXAnchor constraintEqualToAnchor:headerView.centerXAnchor]
      .active = YES;
}

- (void)shiftTilesDown {
  if (!content_suggestions::IsRegularXRegularSizeClass(self.view)) {
    self.fakeOmnibox.hidden = NO;
    [self.dispatcher onFakeboxBlur];
  }

  [self.collectionSynchronizer shiftTilesDown];

  [self.commandHandler dismissModals];
}

- (void)shiftTilesUp {
  void (^completionBlock)() = ^{
    if (!content_suggestions::IsRegularXRegularSizeClass(self.view)) {
      [self.dispatcher onFakeboxAnimationComplete];
      [self.headerView fadeOutShadow];
      [self.fakeOmnibox setHidden:YES];
    }
  };
  [self.collectionSynchronizer shiftTilesUpWithCompletionBlock:completionBlock];
}

#pragma mark - ToolbarOwner

- (CGRect)toolbarFrame {
  return [self.headerView toolbarFrame];
}

- (id<ToolbarSnapshotProviding>)toolbarSnapshotProvider {
  return self.headerView.toolbarSnapshotProvider;
}

#pragma mark - LogoAnimationControllerOwnerOwner

- (id<LogoAnimationControllerOwner>)logoAnimationControllerOwner {
  return [self.logoVendor logoAnimationControllerOwner];
}

#pragma mark - NTPHomeConsumer

- (void)setLogoIsShowing:(BOOL)logoIsShowing {
  _logoIsShowing = logoIsShowing;
  [self updateLogoAndFakeboxDisplay];
}

- (void)setTabCount:(int)tabCount {
  _tabCount = tabCount;
  [self.headerView setToolbarTabCount:tabCount];
}

- (void)setCanGoForward:(BOOL)canGoForward {
  _canGoForward = canGoForward;
  [self.headerView setCanGoForward:self.canGoForward];
}

- (void)setCanGoBack:(BOOL)canGoBack {
  _canGoBack = canGoBack;
  [self.headerView setCanGoBack:self.canGoBack];
}

- (void)locationBarBecomesFirstResponder {
  if (!self.isShowing)
    return;

  self.omniboxFocused = YES;
  [self shiftTilesUp];
}

- (void)locationBarResignsFirstResponder {
  if (!self.isShowing && ![self.delegate isScrolledToTop])
    return;

  self.omniboxFocused = NO;
  if ([self.delegate isContextMenuVisible]) {
    return;
  }

  [self shiftTilesDown];
}

@end
