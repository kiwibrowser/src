// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tab_grid/grid/grid_cell.h"

#import "base/logging.h"
#import "ios/chrome/browser/ui/tab_grid/grid/grid_constants.h"
#import "ios/chrome/browser/ui/tab_grid/grid/top_aligned_image_view.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface GridCell ()
// Visual components of the cell.
@property(nonatomic, weak) UIView* topBar;
@property(nonatomic, weak) UIImageView* iconView;
@property(nonatomic, weak) TopAlignedImageView* snapshotView;
@property(nonatomic, weak) UILabel* titleLabel;
@property(nonatomic, weak) UIButton* closeButton;
@property(nonatomic, weak) UIView* border;
@end

@implementation GridCell
// Public properties.
@synthesize delegate = _delegate;
@synthesize theme = _theme;
@synthesize itemIdentifier = _itemIdentifier;
@synthesize icon = _icon;
@synthesize snapshot = _snapshot;
@synthesize title = _title;
// Private properties.
@synthesize topBar = _topBar;
@synthesize iconView = _iconView;
@synthesize snapshotView = _snapshotView;
@synthesize titleLabel = _titleLabel;
@synthesize closeButton = _closeButton;
@synthesize border = _border;

// |-dequeueReusableCellWithReuseIdentifier:forIndexPath:| calls this method to
// initialize a cell.
- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    [self setupSelectedBackgroundView];
    UIView* contentView = self.contentView;
    contentView.layer.cornerRadius = kGridCellCornerRadius;
    contentView.layer.masksToBounds = YES;
    UIView* topBar = [self setupTopBar];
    TopAlignedImageView* snapshotView = [[TopAlignedImageView alloc] init];
    snapshotView.translatesAutoresizingMaskIntoConstraints = NO;
    [contentView addSubview:topBar];
    [contentView addSubview:snapshotView];
    _topBar = topBar;
    _snapshotView = snapshotView;
    NSArray* constraints = @[
      [topBar.topAnchor constraintEqualToAnchor:contentView.topAnchor],
      [topBar.leadingAnchor constraintEqualToAnchor:contentView.leadingAnchor],
      [topBar.trailingAnchor
          constraintEqualToAnchor:contentView.trailingAnchor],
      [topBar.heightAnchor constraintEqualToConstant:kGridCellHeaderHeight],
      [snapshotView.topAnchor constraintEqualToAnchor:topBar.bottomAnchor],
      [snapshotView.leadingAnchor
          constraintEqualToAnchor:contentView.leadingAnchor],
      [snapshotView.trailingAnchor
          constraintEqualToAnchor:contentView.trailingAnchor],
      [snapshotView.bottomAnchor
          constraintEqualToAnchor:contentView.bottomAnchor],
    ];
    [NSLayoutConstraint activateConstraints:constraints];
  }
  return self;
}

#pragma mark - UICollectionViewCell

- (void)setHighlighted:(BOOL)highlighted {
  // NO-OP to disable highlighting and only allow selection.
}

- (void)prepareForReuse {
  [super prepareForReuse];
  self.itemIdentifier = nil;
  self.title = nil;
  self.icon = nil;
  self.snapshot = nil;
  self.selected = NO;
}

#pragma mark - Accessibility

- (BOOL)isAccessibilityElement {
  // This makes the whole cell tappable in VoiceOver rather than the individual
  // title and close button.
  return YES;
}

- (NSArray*)accessibilityCustomActions {
  // Each cell has 2 custom actions, which is accessible through swiping. The
  // default is to select the cell. Another is to close the cell.
  return @[ [[UIAccessibilityCustomAction alloc]
      initWithName:l10n_util::GetNSString(IDS_IOS_TAB_SWITCHER_CLOSE_TAB)
            target:self
          selector:@selector(closeButtonTapped:)] ];
}

#pragma mark - Public

// Updates the theme to either dark or light. Updating is only done if the
// current theme is not the desired theme.
- (void)setTheme:(GridTheme)theme {
  if (_theme == theme)
    return;
  self.iconView.backgroundColor = UIColorFromRGB(kGridCellIconBackgroundColor);
  self.snapshotView.backgroundColor =
      UIColorFromRGB(kGridCellSnapshotBackgroundColor);
  switch (theme) {
    case GridThemeLight:
      self.topBar.backgroundColor =
          UIColorFromRGB(kGridLightThemeCellHeaderColor);
      self.titleLabel.textColor = UIColorFromRGB(kGridLightThemeCellTitleColor);
      self.closeButton.tintColor =
          UIColorFromRGB(kGridLightThemeCellCloseButtonTintColor);
      self.border.layer.borderColor =
          UIColorFromRGB(kGridLightThemeCellSelectionColor).CGColor;
      break;
    case GridThemeDark:
      self.topBar.backgroundColor =
          UIColorFromRGB(kGridDarkThemeCellHeaderColor);
      self.titleLabel.textColor = UIColorFromRGB(kGridDarkThemeCellTitleColor);
      self.closeButton.tintColor =
          UIColorFromRGB(kGridDarkThemeCellCloseButtonTintColor);
      self.border.layer.borderColor =
          UIColorFromRGB(kGridDarkThemeCellSelectionColor).CGColor;
      break;
  }
  _theme = theme;
}

- (void)setIcon:(UIImage*)icon {
  self.iconView.image = icon;
  _icon = icon;
}

- (void)setSnapshot:(UIImage*)snapshot {
  self.snapshotView.image = snapshot;
  _snapshot = snapshot;
}

- (void)setTitle:(NSString*)title {
  self.titleLabel.text = title;
  self.accessibilityLabel = title;
  _title = title;
}

- (GridCell*)proxyForTransitions {
  GridCell* proxy = [[[self class] alloc] initWithFrame:self.bounds];
  proxy.selected = NO;
  proxy.theme = self.theme;
  proxy.icon = self.icon;
  proxy.snapshot = self.snapshot;
  proxy.title = self.title;
  return proxy;
}

#pragma mark - Private

// Sets up the top bar with icon, title, and close button.
- (UIView*)setupTopBar {
  UIView* topBar = [[UIView alloc] init];
  topBar.translatesAutoresizingMaskIntoConstraints = NO;

  UIImageView* iconView = [[UIImageView alloc] init];
  iconView.translatesAutoresizingMaskIntoConstraints = NO;
  iconView.contentMode = UIViewContentModeScaleAspectFill;
  iconView.layer.cornerRadius = kGridCellIconCornerRadius;
  iconView.layer.masksToBounds = YES;

  UILabel* titleLabel = [[UILabel alloc] init];
  titleLabel.translatesAutoresizingMaskIntoConstraints = NO;
  titleLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleFootnote];
  titleLabel.adjustsFontForContentSizeCategory = YES;

  UIButton* closeButton = [UIButton buttonWithType:UIButtonTypeCustom];
  closeButton.translatesAutoresizingMaskIntoConstraints = NO;
  UIImage* closeImage = [[UIImage imageNamed:@"grid_cell_close_button"]
      imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
  [closeButton setImage:closeImage forState:UIControlStateNormal];
  closeButton.contentEdgeInsets = UIEdgeInsetsMake(
      0, kGridCellCloseButtonContentInset, 0, kGridCellCloseButtonContentInset);
  [closeButton addTarget:self
                  action:@selector(closeButtonTapped:)
        forControlEvents:UIControlEventTouchUpInside];
  closeButton.accessibilityIdentifier = kGridCellCloseButtonIdentifier;

  [topBar addSubview:iconView];
  [topBar addSubview:titleLabel];
  [topBar addSubview:closeButton];
  _iconView = iconView;
  _titleLabel = titleLabel;
  _closeButton = closeButton;

  NSArray* constraints = @[
    [iconView.leadingAnchor
        constraintEqualToAnchor:topBar.leadingAnchor
                       constant:kGridCellHeaderLeadingInset],
    [iconView.centerYAnchor constraintEqualToAnchor:topBar.centerYAnchor],
    [iconView.widthAnchor constraintEqualToConstant:kGridCellIconDiameter],
    [iconView.heightAnchor constraintEqualToConstant:kGridCellIconDiameter],
    [titleLabel.leadingAnchor
        constraintEqualToAnchor:iconView.trailingAnchor
                       constant:kGridCellHeaderLeadingInset],
    [titleLabel.centerYAnchor constraintEqualToAnchor:topBar.centerYAnchor],
    [titleLabel.trailingAnchor
        constraintLessThanOrEqualToAnchor:closeButton.leadingAnchor
                                 constant:kGridCellCloseButtonContentInset],
    [closeButton.topAnchor constraintEqualToAnchor:topBar.topAnchor],
    [closeButton.bottomAnchor constraintEqualToAnchor:topBar.bottomAnchor],
    [closeButton.trailingAnchor constraintEqualToAnchor:topBar.trailingAnchor],
  ];
  [NSLayoutConstraint activateConstraints:constraints];
  [titleLabel
      setContentCompressionResistancePriority:UILayoutPriorityDefaultLow
                                      forAxis:UILayoutConstraintAxisHorizontal];
  [closeButton
      setContentCompressionResistancePriority:UILayoutPriorityRequired
                                      forAxis:UILayoutConstraintAxisHorizontal];
  [closeButton setContentHuggingPriority:UILayoutPriorityRequired
                                 forAxis:UILayoutConstraintAxisHorizontal];
  return topBar;
}

// Sets up the selection border. The tint color is set when the theme is
// selected.
- (void)setupSelectedBackgroundView {
  self.selectedBackgroundView = [[UIView alloc] init];
  self.selectedBackgroundView.backgroundColor =
      UIColorFromRGB(kGridBackgroundColor);
  UIView* border = [[UIView alloc] init];
  border.translatesAutoresizingMaskIntoConstraints = NO;
  border.backgroundColor = UIColorFromRGB(kGridBackgroundColor);
  border.layer.cornerRadius = kGridCellCornerRadius +
                              kGridCellSelectionRingGapWidth +
                              kGridCellSelectionRingTintWidth;
  border.layer.borderWidth = kGridCellSelectionRingTintWidth;
  [self.selectedBackgroundView addSubview:border];
  _border = border;
  [NSLayoutConstraint activateConstraints:@[
    [border.topAnchor
        constraintEqualToAnchor:self.selectedBackgroundView.topAnchor
                       constant:-kGridCellSelectionRingTintWidth -
                                kGridCellSelectionRingGapWidth],
    [border.leadingAnchor
        constraintEqualToAnchor:self.selectedBackgroundView.leadingAnchor
                       constant:-kGridCellSelectionRingTintWidth -
                                kGridCellSelectionRingGapWidth],
    [border.trailingAnchor
        constraintEqualToAnchor:self.selectedBackgroundView.trailingAnchor
                       constant:kGridCellSelectionRingTintWidth +
                                kGridCellSelectionRingGapWidth],
    [border.bottomAnchor
        constraintEqualToAnchor:self.selectedBackgroundView.bottomAnchor
                       constant:kGridCellSelectionRingTintWidth +
                                kGridCellSelectionRingGapWidth]
  ]];
}

// Selector registered to the close button.
- (void)closeButtonTapped:(id)sender {
  [self.delegate closeButtonTappedForCell:self];
}

@end
