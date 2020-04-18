// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/toolbar/clean/toolbar_view.h"

#import "base/mac/foundation_util.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_button.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_button_factory.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_configuration.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_constants.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_tools_menu_button.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#include "ios/chrome/grit/ios_theme_resources.h"
#import "ios/third_party/material_components_ios/src/components/ProgressView/src/MaterialProgressView.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface ToolbarView ()
// Redefined as readwrite.
@property(nonatomic, strong, readwrite) UIStackView* leadingStackView;
@property(nonatomic, strong, readwrite) UIView* locationBarContainer;
@property(nonatomic, strong, readwrite) UIStackView* trailingStackView;
@property(nonatomic, strong, readwrite) ToolbarToolsMenuButton* toolsMenuButton;
@property(nonatomic, strong, readwrite) ToolbarButton* backButton;
@property(nonatomic, strong, readwrite) ToolbarButton* forwardButton;
@property(nonatomic, strong, readwrite) ToolbarButton* tabSwitchStripButton;
@property(nonatomic, strong, readwrite) ToolbarButton* shareButton;
@property(nonatomic, strong, readwrite) ToolbarButton* reloadButton;
@property(nonatomic, strong, readwrite) ToolbarButton* stopButton;
@property(nonatomic, strong, readwrite)
    UIStackView* locationBarContainerStackView;
@property(nonatomic, strong, readwrite) ToolbarButton* locationBarLeadingButton;
@property(nonatomic, strong, readwrite) ToolbarButton* contractButton;
@property(nonatomic, strong, readwrite) ToolbarButton* voiceSearchButton;
@property(nonatomic, strong, readwrite) ToolbarButton* bookmarkButton;
@property(nonatomic, strong, readwrite) UIImageView* shadowView;
@property(nonatomic, strong, readwrite) UIImageView* fullBleedShadowView;
@property(nonatomic, strong, readwrite) UIImageView* locationBarShadow;
@property(nonatomic, strong, readwrite) MDCProgressView* progressBar;
@property(nonatomic, strong, readwrite) UIView* backgroundView;
@property(nonatomic, strong, readwrite)
    NSMutableArray* regularToolbarConstraints;
@property(nonatomic, strong, readwrite) NSArray* expandedToolbarConstraints;
@property(nonatomic, strong, readwrite)
    NSLayoutConstraint* leadingSafeAreaConstraint;
@property(nonatomic, strong, readwrite)
    NSLayoutConstraint* trailingSafeAreaConstraint;
@property(nonatomic, strong, readwrite)
    NSLayoutConstraint* leadingFakeSafeAreaConstraint;
@property(nonatomic, strong, readwrite)
    NSLayoutConstraint* trailingFakeSafeAreaConstraint;
@property(nonatomic, strong, readwrite)
    NSArray<ToolbarButton*>* leadingStackViewButtons;
@property(nonatomic, strong, readwrite)
    NSArray<ToolbarButton*>* trailingStackViewButtons;
@end

@implementation ToolbarView

@synthesize delegate = _delegate;
@synthesize buttonFactory = _buttonFactory;
@synthesize leadingStackViewButtons = _leadingStackViewButtons;
@synthesize trailingStackViewButtons = _trailingStackViewButtons;
@synthesize backgroundView = _backgroundView;
@synthesize leadingStackView = _leadingStackView;
@synthesize trailingStackView = _trailingStackView;
@synthesize locationBarContainer = _locationBarContainer;
@synthesize backButton = _backButton;
@synthesize forwardButton = _forwardButton;
@synthesize tabSwitchStripButton = _tabSwitchStripButton;
@synthesize toolsMenuButton = _toolsMenuButton;
@synthesize shareButton = _shareButton;
@synthesize reloadButton = _reloadButton;
@synthesize stopButton = _stopButton;
@synthesize voiceSearchButton = _voiceSearchButton;
@synthesize bookmarkButton = _bookmarkButton;
@synthesize contractButton = _contractButton;
@synthesize locationBarLeadingButton = _locationBarLeadingButton;
@synthesize progressBar = _progressBar;
@synthesize locationBarContainerStackView = _locationBarContainerStackView;
@synthesize shadowView = _shadowView;
@synthesize fullBleedShadowView = _fullBleedShadowView;
@synthesize locationBarShadow = _locationBarShadow;
@synthesize expandedToolbarConstraints = _expandedToolbarConstraints;
@synthesize regularToolbarConstraints = _regularToolbarConstraints;
@synthesize leadingFakeSafeAreaConstraint = _leadingFakeSafeAreaConstraint;
@synthesize trailingFakeSafeAreaConstraint = _trailingFakeSafeAreaConstraint;
@synthesize leadingSafeAreaConstraint = _leadingSafeAreaConstraint;
@synthesize trailingSafeAreaConstraint = _trailingSafeAreaConstraint;
@synthesize locationBarView = _locationBarView;
@synthesize leadingMargin = _leadingMargin;

#pragma mark - UIView

- (void)setFrame:(CGRect)frame {
  [super setFrame:frame];
  [self.delegate toolbarViewFrameChanged];
}

- (CGSize)intrinsicContentSize {
  return CGSizeMake(UIViewNoIntrinsicMetric, kToolbarHeight);
}

#pragma mark - Public

- (void)setUp {
  [self setUpToolbarButtons];
  [self setUpLocationBarContainer];
  [self setUpProgressBar];
  // The view can be obstructed by the background view.
  self.backgroundColor =
      [self.buttonFactory.toolbarConfiguration backgroundColor];

  [self setUpToolbarStackView];
  [self setUpLocationBarContainerView];
  [self addSubview:self.leadingStackView];
  [self addSubview:self.trailingStackView];
  // Since the |_locationBarContainer| will expand and cover the stackViews, its
  // important to add it after them so the |_locationBarContainer| has a higher
  // Z order.
  [self addSubview:self.locationBarContainer];
  [self addSubview:self.shadowView];
  [self addSubview:self.fullBleedShadowView];
  [self addSubview:self.progressBar];
  [self setConstraints];
}

- (void)setUpToolbarButtons {
  self.backButton = [self.buttonFactory backButton];
  self.forwardButton = [self.buttonFactory forwardButton];
  self.tabSwitchStripButton = [self.buttonFactory stackViewButton];
  self.toolsMenuButton = [self.buttonFactory toolsMenuButton];
  self.shareButton = [self.buttonFactory shareButton];
  self.reloadButton = [self.buttonFactory reloadButton];
  self.stopButton = [self.buttonFactory stopButton];
  self.bookmarkButton = [self.buttonFactory bookmarkButton];

  // Voice Search button.
  self.voiceSearchButton = [self.buttonFactory voiceSearchButton];
  self.voiceSearchButton.enabled = NO;

  // Contract button.
  self.contractButton = [self.buttonFactory contractButton];

  self.contractButton.alpha = 0;
  self.contractButton.hidden = YES;

  // LocationBar LeadingButton.
  self.locationBarLeadingButton = [self.buttonFactory locationBarLeadingButton];
  self.locationBarLeadingButton.alpha = 0;
  self.locationBarLeadingButton.hidden = YES;
}

#pragma mark - Properties

- (UIView*)locationBarView {
  if (!_locationBarView) {
    _locationBarView = [[UIView alloc] initWithFrame:CGRectZero];
    _locationBarView.translatesAutoresizingMaskIntoConstraints = NO;
    [_locationBarView
        setContentHuggingPriority:UILayoutPriorityDefaultLow
                          forAxis:UILayoutConstraintAxisHorizontal];
    _locationBarView.clipsToBounds = YES;
  }
  return _locationBarView;
}

- (void)setLocationBarView:(UIView*)view {
  if (_locationBarView == view) {
    return;
  }
  view.translatesAutoresizingMaskIntoConstraints = NO;
  [view setContentHuggingPriority:UILayoutPriorityDefaultLow
                          forAxis:UILayoutConstraintAxisHorizontal];
  if (self.locationBarContainerStackView) {
    NSInteger index = 0;
    for (UIView* arrangedView in self.locationBarContainerStackView
             .arrangedSubviews) {
      if (arrangedView == _locationBarView)
        break;
      index++;
    }
    [self.locationBarContainerStackView removeArrangedSubview:_locationBarView];
    [self.locationBarContainerStackView insertArrangedSubview:view
                                                      atIndex:index];
  }
  _locationBarView = view;
}

- (NSArray<ToolbarButton*>*)leadingStackViewButtons {
  if (!_leadingStackViewButtons) {
    _leadingStackViewButtons =
        [self toolbarButtonsInStackView:self.leadingStackView];
  }
  return _leadingStackViewButtons;
}

- (NSArray<ToolbarButton*>*)trailingStackViewButtons {
  if (!_trailingStackViewButtons) {
    _trailingStackViewButtons =
        [self toolbarButtonsInStackView:self.trailingStackView];
  }
  return _trailingStackViewButtons;
}

- (UIView*)backgroundView {
  if (!_backgroundView) {
    _backgroundView = [[UIView alloc] initWithFrame:CGRectZero];
    _backgroundView.translatesAutoresizingMaskIntoConstraints = NO;
    _backgroundView.backgroundColor =
        self.buttonFactory.toolbarConfiguration.NTPBackgroundColor;
    [self insertSubview:_backgroundView atIndex:0];
    AddSameConstraints(self, _backgroundView);
    _backgroundView.alpha = 0;
  }
  return _backgroundView;
}

- (NSMutableArray*)regularToolbarConstraints {
  if (!_regularToolbarConstraints) {
    _regularToolbarConstraints = [[NSMutableArray alloc] init];
  }
  return _regularToolbarConstraints;
}

- (NSArray*)expandedToolbarConstraints {
  if (!_expandedToolbarConstraints) {
    _expandedToolbarConstraints = @[
      [self.locationBarContainer.topAnchor
          constraintEqualToAnchor:self.topAnchor],
      [self.locationBarContainer.bottomAnchor
          constraintEqualToAnchor:self.bottomAnchor],
      [self.locationBarContainer.leadingAnchor
          constraintEqualToAnchor:self.leadingAnchor],
      [self.locationBarContainer.trailingAnchor
          constraintEqualToAnchor:self.trailingAnchor],
    ];
  }
  return _expandedToolbarConstraints;
}

- (UIImageView*)shadowView {
  if (!_shadowView) {
    _shadowView = [[UIImageView alloc] init];
    _shadowView.translatesAutoresizingMaskIntoConstraints = NO;
    _shadowView.userInteractionEnabled = NO;
    _shadowView.image = NativeImage(IDR_IOS_TOOLBAR_SHADOW);
  }
  return _shadowView;
}

- (UIImageView*)fullBleedShadowView {
  if (!_fullBleedShadowView) {
    _fullBleedShadowView = [[UIImageView alloc] init];
    _fullBleedShadowView.translatesAutoresizingMaskIntoConstraints = NO;
    _fullBleedShadowView.userInteractionEnabled = NO;
    _fullBleedShadowView.alpha = 0;
    _fullBleedShadowView.image = NativeImage(IDR_IOS_TOOLBAR_SHADOW_FULL_BLEED);
  }
  return _fullBleedShadowView;
}

#pragma mark - Private

// Sets up the StackView that contains toolbar navigation items.
- (void)setUpToolbarStackView {
  self.leadingStackView = [[UIStackView alloc] initWithArrangedSubviews:@[
    self.backButton, self.forwardButton, self.reloadButton, self.stopButton
  ]];
  self.leadingStackView.translatesAutoresizingMaskIntoConstraints = NO;
  self.leadingStackView.distribution = UIStackViewDistributionFill;

  self.trailingStackView = [[UIStackView alloc] initWithArrangedSubviews:@[
    self.shareButton, self.tabSwitchStripButton, self.toolsMenuButton
  ]];
  self.trailingStackView.translatesAutoresizingMaskIntoConstraints = NO;
  self.trailingStackView.spacing = kStackViewSpacing;
  self.trailingStackView.distribution = UIStackViewDistributionFill;
}

// Sets up the LocationContainerView. Which contains a StackView containing the
// locationBarView, and other buttons like Voice Search, Bookmarks and Contract
// Toolbar.
- (void)setUpLocationBarContainerView {
  self.locationBarContainerStackView =
      [[UIStackView alloc] initWithArrangedSubviews:@[ self.locationBarView ]];
  self.locationBarContainerStackView.translatesAutoresizingMaskIntoConstraints =
      NO;
  self.locationBarContainerStackView.spacing = kStackViewSpacing;
  self.locationBarContainerStackView.distribution = UIStackViewDistributionFill;
  // Bookmarks and Voice Search buttons will only be part of the Toolbar on
  // iPad. On the other hand the contract button is only needed on non iPad
  // devices, since iPad doesn't animate, thus it doesn't need to contract.
  if (IsIPadIdiom()) {
    [self.locationBarContainerStackView addArrangedSubview:self.bookmarkButton];
    [self.locationBarContainerStackView
        addArrangedSubview:self.voiceSearchButton];
  } else {
    [self.locationBarContainerStackView addArrangedSubview:self.contractButton];
  }
  // If |self.locationBarLeadingButton| exists add it to the StackView.
  if (self.locationBarLeadingButton) {
    [self.locationBarContainerStackView
        insertArrangedSubview:self.locationBarLeadingButton
                      atIndex:0];
  }
  [self.locationBarContainer addSubview:self.locationBarContainerStackView];
}

- (void)setUpLocationBarContainer {
  UIView* locationBarContainer = [[UIView alloc] initWithFrame:CGRectZero];
  locationBarContainer.translatesAutoresizingMaskIntoConstraints = NO;
  locationBarContainer.backgroundColor =
      [self.buttonFactory.toolbarConfiguration omniboxBackgroundColor];
  locationBarContainer.layer.borderWidth = kLocationBarBorderWidth;
  locationBarContainer.layer.cornerRadius = kLocationBarCornerRadius;
  locationBarContainer.layer.borderColor =
      [self.buttonFactory.toolbarConfiguration omniboxBorderColor].CGColor;

  self.locationBarShadow =
      [[UIImageView alloc] initWithImage:NativeImage(IDR_IOS_TOOLBAR_SHADOW)];
  self.locationBarShadow.translatesAutoresizingMaskIntoConstraints = NO;
  self.locationBarShadow.userInteractionEnabled = NO;

  [locationBarContainer addSubview:self.locationBarShadow];

  [locationBarContainer
      setContentHuggingPriority:UILayoutPriorityDefaultLow
                        forAxis:UILayoutConstraintAxisHorizontal];
  self.locationBarContainer = locationBarContainer;
}

- (void)setUpProgressBar {
  MDCProgressView* progressBar = [[MDCProgressView alloc] init];
  progressBar.translatesAutoresizingMaskIntoConstraints = NO;
  progressBar.hidden = YES;
  self.progressBar = progressBar;
}

// Sets the constraints for the different subviews.
- (void)setConstraints {
  self.translatesAutoresizingMaskIntoConstraints = NO;

  // ProgressBar constraints.
  [NSLayoutConstraint activateConstraints:@[
    [self.progressBar.leadingAnchor constraintEqualToAnchor:self.leadingAnchor],
    [self.progressBar.trailingAnchor
        constraintEqualToAnchor:self.trailingAnchor],
    [self.progressBar.bottomAnchor constraintEqualToAnchor:self.bottomAnchor],
    [self.progressBar.heightAnchor
        constraintEqualToConstant:kProgressBarHeight],
  ]];

  // Shadows constraints.
  [NSLayoutConstraint activateConstraints:@[
    [self.shadowView.topAnchor constraintEqualToAnchor:self.bottomAnchor],
    [self.shadowView.leadingAnchor constraintEqualToAnchor:self.leadingAnchor],
    [self.shadowView.trailingAnchor
        constraintEqualToAnchor:self.trailingAnchor],
    [self.shadowView.heightAnchor
        constraintEqualToConstant:kToolbarShadowHeight],
    [self.fullBleedShadowView.topAnchor
        constraintEqualToAnchor:self.bottomAnchor],
    [self.fullBleedShadowView.leadingAnchor
        constraintEqualToAnchor:self.leadingAnchor],
    [self.fullBleedShadowView.trailingAnchor
        constraintEqualToAnchor:self.trailingAnchor],
    [self.fullBleedShadowView.heightAnchor
        constraintEqualToConstant:kToolbarFullBleedShadowHeight],
  ]];

  // Stack views constraints.
  // Layout: |[leadingStackView]-[locationBarContainer]-[trailingStackView]|.
  // Safe Area constraints.
  id<LayoutGuideProvider> viewSafeAreaGuide = SafeAreaLayoutGuideForView(self);
  self.leadingSafeAreaConstraint = [self.leadingStackView.leadingAnchor
      constraintEqualToAnchor:viewSafeAreaGuide.leadingAnchor
                     constant:self.leadingMargin];
  self.trailingSafeAreaConstraint = [self.trailingStackView.trailingAnchor
      constraintEqualToAnchor:viewSafeAreaGuide.trailingAnchor];
  [NSLayoutConstraint activateConstraints:@[
    self.leadingSafeAreaConstraint, self.trailingSafeAreaConstraint
  ]];

  // Fake safe area constraints. Not activated by default.
  self.leadingFakeSafeAreaConstraint = [self.leadingStackView.leadingAnchor
      constraintEqualToAnchor:self.leadingAnchor];
  self.trailingFakeSafeAreaConstraint = [self.trailingStackView.trailingAnchor
      constraintEqualToAnchor:self.trailingAnchor];

  // Stackviews and locationBar Spacing constraints. These will be disabled when
  // expanding the omnibox.
  NSArray<NSLayoutConstraint*>* stackViewSpacingConstraint = [NSLayoutConstraint
      constraintsWithVisualFormat:
          @"H:[leadingStack]-(spacing)-[locationBar]-(spacing)-[trailingStack]"
                          options:0
                          metrics:@{
                            @"spacing" : @(kHorizontalMargin)
                          }
                            views:@{
                              @"leadingStack" : self.leadingStackView,
                              @"locationBar" : self.locationBarContainer,
                              @"trailingStack" : self.trailingStackView
                            }];
  [self.regularToolbarConstraints
      addObjectsFromArray:stackViewSpacingConstraint];
  // Vertical constraints.
  [NSLayoutConstraint activateConstraints:stackViewSpacingConstraint];
  ApplyVisualConstraintsWithMetrics(
      @[
        @"V:[leadingStack(height)]-(margin)-|",
        @"V:[trailingStack(height)]-(margin)-|"
      ],
      @{
        @"leadingStack" : self.leadingStackView,
        @"trailingStack" : self.trailingStackView
      },
      @{
        @"height" : @(kToolbarHeight - 2 * kButtonVerticalMargin),
        @"margin" : @(kButtonVerticalMargin),
      });

  // LocationBarContainer constraints.
  NSArray* locationBarRegularConstraints = @[
    [self.bottomAnchor
        constraintEqualToAnchor:self.locationBarContainer.bottomAnchor
                       constant:kLocationBarVerticalMargin],
    [self.locationBarContainer.heightAnchor
        constraintEqualToConstant:kToolbarHeight -
                                  2 * kLocationBarVerticalMargin],
  ];
  [self.regularToolbarConstraints
      addObjectsFromArray:locationBarRegularConstraints];
  [NSLayoutConstraint activateConstraints:locationBarRegularConstraints];
  // LocationBarContainer shadow constraints.
  [NSLayoutConstraint activateConstraints:@[
    [self.locationBarShadow.heightAnchor
        constraintEqualToConstant:kLocationBarShadowHeight],
    [self.locationBarShadow.leadingAnchor
        constraintEqualToAnchor:self.locationBarContainer.leadingAnchor
                       constant:kLocationBarShadowInset],
    [self.locationBarShadow.trailingAnchor
        constraintEqualToAnchor:self.locationBarContainer.trailingAnchor
                       constant:-kLocationBarShadowInset],
    [self.locationBarShadow.topAnchor
        constraintEqualToAnchor:self.locationBarContainer.bottomAnchor
                       constant:-kLocationBarBorderWidth],
  ]];

  // LocationBarStackView constraints. The StackView inside the
  // LocationBarContainer View.
  id<LayoutGuideProvider> locationBarContainerSafeAreaGuide =
      SafeAreaLayoutGuideForView(self.locationBarContainer);
  [NSLayoutConstraint activateConstraints:@[
    [self.locationBarContainerStackView.bottomAnchor
        constraintEqualToAnchor:self.bottomAnchor
                       constant:-kLocationBarVerticalMargin],
    [self.locationBarContainerStackView.trailingAnchor
        constraintEqualToAnchor:locationBarContainerSafeAreaGuide
                                    .trailingAnchor],
    [self.locationBarContainerStackView.leadingAnchor
        constraintEqualToAnchor:locationBarContainerSafeAreaGuide
                                    .leadingAnchor],
    [self.locationBarContainerStackView.heightAnchor
        constraintEqualToConstant:kToolbarHeight -
                                  2 * kLocationBarVerticalMargin],
  ]];
}

// Returns all the ToolbarButtons in a given UIStackView.
- (NSArray<ToolbarButton*>*)toolbarButtonsInStackView:(UIStackView*)stackView {
  NSMutableArray<ToolbarButton*>* buttons = [[NSMutableArray alloc] init];
  for (UIView* view in stackView.arrangedSubviews) {
    if ([view isKindOfClass:[ToolbarButton class]]) {
      ToolbarButton* button = base::mac::ObjCCastStrict<ToolbarButton>(view);
      [buttons addObject:button];
    }
  }
  return buttons;
}

@end
