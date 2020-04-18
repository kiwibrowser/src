// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/share_extension/share_extension_view.h"

#include "base/logging.h"
#import "ios/chrome/share_extension/ui_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

const CGFloat kCornerRadius = 6;
// Minimum size around the widget
const CGFloat kDividerHeight = 0.5;
const CGFloat kShareExtensionPadding = 16;
const CGFloat kButtonHeight = 44;
const CGFloat kLowAlpha = 0.3;

// Size of the icon if present.
const CGFloat kScreenshotSize = 80;

// Size for the buttons font.
const CGFloat kButtonFontSize = 17;

}  // namespace

#pragma mark - Share Extension Button

// UIButton with the background color changing when it is highlighted.
@interface ShareExtensionButton : UIButton
@end

@implementation ShareExtensionButton

- (void)setHighlighted:(BOOL)highlighted {
  [super setHighlighted:highlighted];

  if (highlighted)
    self.backgroundColor = [UIColor colorWithWhite:217.0f / 255.0f alpha:1];
  else
    self.backgroundColor = [UIColor clearColor];
}

@end

#pragma mark - Share Extension View

@interface ShareExtensionView () {
  __weak id<ShareExtensionViewActionTarget> _target;

  // Track if a button has been pressed. All button pressing will have no effect
  // if |_dismissed| is YES.
  BOOL _dismissed;
}

// Keep strong references of the views that need to be updated.
@property(nonatomic, strong) UILabel* titleLabel;
@property(nonatomic, strong) UILabel* URLLabel;
@property(nonatomic, strong) UIView* titleURLContainer;
@property(nonatomic, strong) UIButton* readingListButton;
@property(nonatomic, strong) UIImageView* screenshotView;
@property(nonatomic, strong) UIView* itemView;

// View creation helpers.
// Returns a view containing the shared items (title, URL, screenshot). This
// method will set the ivars.
- (UIView*)sharedItemView;

// Returns a button containing the with title |title| and action |selector| on
// |_target|.
- (UIButton*)buttonWithTitle:(NSString*)title selector:(SEL)selector;

// Returns a view containing a divider with vibrancy effect.
- (UIView*)dividerViewWithVibrancy:(UIVisualEffect*)vibrancyEffect;

// Returns a navigationBar.
- (UINavigationBar*)navigationBar;

// Called when "Read Later" button has been pressed.
- (void)addToReadingListPressed:(UIButton*)sender;

// Called when "Add to bookmarks" button has been pressed.
- (void)addToBookmarksPressed:(UIButton*)sender;

// Called when "Cancel" button has been pressed.
- (void)cancelPressed:(UIButton*)sender;

// Animates the button |sender| by replaceing its string to "Added", then call
// completion.
- (void)animateButtonPressed:(UIButton*)sender
              withCompletion:(void (^)(void))completion;

@end

@implementation ShareExtensionView

@synthesize titleLabel = _titleLabel;
@synthesize URLLabel = _URLLabel;
@synthesize titleURLContainer = _titleURLContainer;
@synthesize readingListButton = _readingListButton;
@synthesize screenshotView = _screenshotView;
@synthesize itemView = _itemView;

#pragma mark - Lifecycle

- (instancetype)initWithActionTarget:
    (id<ShareExtensionViewActionTarget>)target {
  self = [super initWithFrame:CGRectZero];
  if (self) {
    DCHECK(target);
    _target = target;
    _dismissed = NO;

    [self.layer setCornerRadius:kCornerRadius];
    [self setClipsToBounds:YES];
    UIBlurEffect* blurEffect =
        [UIBlurEffect effectWithStyle:UIBlurEffectStyleExtraLight];
    UIVibrancyEffect* vibrancyEffect =
        [UIVibrancyEffect effectForBlurEffect:blurEffect];

    self.backgroundColor = [UIColor colorWithWhite:242.0f / 255.0f alpha:1];

    // Add the blur effect to the whole widget.
    UIVisualEffectView* blurringView =
        [[UIVisualEffectView alloc] initWithEffect:blurEffect];
    [self addSubview:blurringView];
    ui_util::ConstrainAllSidesOfViewToView(self, blurringView);
    [[blurringView layer] setCornerRadius:kCornerRadius];
    [blurringView setClipsToBounds:YES];

    NSString* addToReadingListTitle = NSLocalizedString(
        @"IDS_IOS_ADD_READING_LIST_SHARE_EXTENSION",
        @"The add to reading list button text in share extension.");
    self.readingListButton =
        [self buttonWithTitle:addToReadingListTitle
                     selector:@selector(addToReadingListPressed:)];

    NSString* addToBookmarksTitle = NSLocalizedString(
        @"IDS_IOS_ADD_BOOKMARKS_SHARE_EXTENSION",
        @"The Add to bookmarks button text in share extension.");
    UIButton* bookmarksButton =
        [self buttonWithTitle:addToBookmarksTitle
                     selector:@selector(addToBookmarksPressed:)];

    UIStackView* contentStack = [[UIStackView alloc] initWithArrangedSubviews:@[
      [self navigationBar], [self dividerViewWithVibrancy:vibrancyEffect],
      [self sharedItemView], [self dividerViewWithVibrancy:vibrancyEffect],
      self.readingListButton, [self dividerViewWithVibrancy:vibrancyEffect],
      bookmarksButton
    ]];
    [contentStack setAxis:UILayoutConstraintAxisVertical];
    [[blurringView contentView] addSubview:contentStack];

    [blurringView setTranslatesAutoresizingMaskIntoConstraints:NO];
    [contentStack setTranslatesAutoresizingMaskIntoConstraints:NO];

    ui_util::ConstrainAllSidesOfViewToView([blurringView contentView],
                                           contentStack);
  }
  return self;
}

#pragma mark Init helpers

- (UIView*)sharedItemView {
  // Title label. Text will be filled by |setTitle:| when available.
  _titleLabel = [[UILabel alloc] initWithFrame:CGRectZero];
  [_titleLabel setFont:[UIFont boldSystemFontOfSize:16]];
  [_titleLabel setTranslatesAutoresizingMaskIntoConstraints:NO];

  // URL label. Text will be filled by |setURL:| when available.
  _URLLabel = [[UILabel alloc] initWithFrame:CGRectZero];
  [_URLLabel setTranslatesAutoresizingMaskIntoConstraints:NO];
  [_URLLabel setNumberOfLines:3];
  [_URLLabel setLineBreakMode:NSLineBreakByWordWrapping];
  [_URLLabel setFont:[UIFont systemFontOfSize:12]];

  // Screenshot view. Image will be filled by |setScreenshot:| when available.
  _screenshotView = [[UIImageView alloc] initWithFrame:CGRectZero];
  [_screenshotView setTranslatesAutoresizingMaskIntoConstraints:NO];
  NSLayoutConstraint* imageWidthConstraint =
      [_screenshotView.widthAnchor constraintEqualToConstant:0];
  imageWidthConstraint.priority = UILayoutPriorityDefaultHigh;
  imageWidthConstraint.active = YES;

  [_screenshotView.heightAnchor
      constraintEqualToAnchor:_screenshotView.widthAnchor]
      .active = YES;
  [_screenshotView setContentMode:UIViewContentModeScaleAspectFill];
  [_screenshotView setClipsToBounds:YES];

  // |_screenshotView| should take as much space as needed. Lower compression
  // resistance of the other elements.
  [_titleLabel
      setContentCompressionResistancePriority:UILayoutPriorityDefaultLow
                                      forAxis:UILayoutConstraintAxisHorizontal];
  [_titleLabel setContentHuggingPriority:UILayoutPriorityDefaultHigh
                                 forAxis:UILayoutConstraintAxisVertical];
  [_URLLabel setContentHuggingPriority:UILayoutPriorityDefaultHigh
                               forAxis:UILayoutConstraintAxisVertical];

  [_URLLabel
      setContentCompressionResistancePriority:UILayoutPriorityDefaultLow
                                      forAxis:UILayoutConstraintAxisHorizontal];

  _titleURLContainer = [[UIView alloc] initWithFrame:CGRectZero];
  [_titleURLContainer setTranslatesAutoresizingMaskIntoConstraints:NO];

  [_titleURLContainer addSubview:_titleLabel];
  [_titleURLContainer addSubview:_URLLabel];

  _itemView = [[UIView alloc] init];
  [_itemView setTranslatesAutoresizingMaskIntoConstraints:NO];
  [_itemView addSubview:_titleURLContainer];
  [_itemView addSubview:_screenshotView];

  [NSLayoutConstraint activateConstraints:@[
    [_titleLabel.topAnchor
        constraintEqualToAnchor:_titleURLContainer.topAnchor],
    [_URLLabel.topAnchor constraintEqualToAnchor:_titleLabel.bottomAnchor],
    [_URLLabel.bottomAnchor
        constraintEqualToAnchor:_titleURLContainer.bottomAnchor],
    [_titleLabel.trailingAnchor
        constraintEqualToAnchor:_titleURLContainer.trailingAnchor],
    [_URLLabel.trailingAnchor
        constraintEqualToAnchor:_titleURLContainer.trailingAnchor],
    [_titleLabel.leadingAnchor
        constraintEqualToAnchor:_titleURLContainer.leadingAnchor],
    [_URLLabel.leadingAnchor
        constraintEqualToAnchor:_titleURLContainer.leadingAnchor],
    [_titleURLContainer.centerYAnchor
        constraintEqualToAnchor:_itemView.centerYAnchor],
    [_itemView.heightAnchor
        constraintGreaterThanOrEqualToAnchor:_titleURLContainer.heightAnchor
                                  multiplier:1
                                    constant:2 * kShareExtensionPadding],
    [_titleURLContainer.leadingAnchor
        constraintEqualToAnchor:_itemView.leadingAnchor
                       constant:kShareExtensionPadding],
    [_screenshotView.trailingAnchor
        constraintEqualToAnchor:_itemView.trailingAnchor
                       constant:-kShareExtensionPadding],
    [_itemView.heightAnchor
        constraintGreaterThanOrEqualToAnchor:_screenshotView.heightAnchor
                                  multiplier:1
                                    constant:2 * kShareExtensionPadding],
    [_screenshotView.centerYAnchor
        constraintEqualToAnchor:_itemView.centerYAnchor],
  ]];

  NSLayoutConstraint* titleURLScreenshotConstraint =
      [_titleURLContainer.trailingAnchor
          constraintEqualToAnchor:_screenshotView.leadingAnchor];
  titleURLScreenshotConstraint.priority = UILayoutPriorityDefaultHigh;
  titleURLScreenshotConstraint.active = YES;

  return _itemView;
}

- (UIView*)dividerViewWithVibrancy:(UIVisualEffect*)vibrancyEffect {
  UIVisualEffectView* dividerVibrancy =
      [[UIVisualEffectView alloc] initWithEffect:vibrancyEffect];
  UIView* divider = [[UIView alloc] initWithFrame:CGRectZero];
  [divider setBackgroundColor:[UIColor colorWithWhite:0 alpha:kLowAlpha]];
  [[dividerVibrancy contentView] addSubview:divider];
  [dividerVibrancy setTranslatesAutoresizingMaskIntoConstraints:NO];
  [divider setTranslatesAutoresizingMaskIntoConstraints:NO];
  ui_util::ConstrainAllSidesOfViewToView([dividerVibrancy contentView],
                                         divider);
  CGFloat slidingConstant = ui_util::AlignValueToPixel(kDividerHeight);
  [[dividerVibrancy heightAnchor] constraintEqualToConstant:slidingConstant]
      .active = YES;
  return dividerVibrancy;
}

- (UIButton*)buttonWithTitle:(NSString*)title selector:(SEL)selector {
  UIButton* systemButton = [UIButton buttonWithType:UIButtonTypeSystem];
  UIColor* systemColor = [systemButton titleColorForState:UIControlStateNormal];

  UIButton* button = [[ShareExtensionButton alloc] initWithFrame:CGRectZero];
  [button setTitle:title forState:UIControlStateNormal];
  [button setTitleColor:systemColor forState:UIControlStateNormal];
  [[button titleLabel] setFont:[UIFont systemFontOfSize:kButtonFontSize]];
  [button setTranslatesAutoresizingMaskIntoConstraints:NO];
  [button addTarget:self
                action:selector
      forControlEvents:UIControlEventTouchUpInside];
  [button.heightAnchor constraintEqualToConstant:kButtonHeight].active = YES;
  return button;
}

- (UINavigationBar*)navigationBar {
  // Create the navigation bar.
  UINavigationBar* navigationBar =
      [[UINavigationBar alloc] initWithFrame:CGRectZero];
  [[navigationBar layer] setCornerRadius:kCornerRadius];
  [navigationBar setClipsToBounds:YES];

  // Create an empty image to replace the standard gray background of the
  // UINavigationBar.
  UIImage* emptyImage = [[UIImage alloc] init];
  [navigationBar setBackgroundImage:emptyImage
                      forBarMetrics:UIBarMetricsDefault];
  [navigationBar setShadowImage:emptyImage];
  [navigationBar setTranslucent:YES];
  [navigationBar setTranslatesAutoresizingMaskIntoConstraints:NO];

  UIButton* systemButton = [UIButton buttonWithType:UIButtonTypeSystem];
  UIColor* systemColor = [systemButton titleColorForState:UIControlStateNormal];
  UIBarButtonItem* cancelButton = [[UIBarButtonItem alloc]
      initWithBarButtonSystemItem:UIBarButtonSystemItemCancel
                           target:self
                           action:@selector(cancelPressed:)];
  [cancelButton setTintColor:systemColor];

  NSString* appName =
      [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleDisplayName"];
  UINavigationItem* titleItem =
      [[UINavigationItem alloc] initWithTitle:appName];
  [titleItem setLeftBarButtonItem:cancelButton];
  [titleItem setHidesBackButton:YES];
  [navigationBar pushNavigationItem:titleItem animated:NO];
  return navigationBar;
}

- (void)addToReadingListPressed:(UIButton*)sender {
  if (_dismissed) {
    return;
  }
  _dismissed = YES;
  [self animateButtonPressed:sender
              withCompletion:^{
                [_target shareExtensionViewDidSelectAddToReadingList:sender];
              }];
}

- (void)addToBookmarksPressed:(UIButton*)sender {
  if (_dismissed) {
    return;
  }
  _dismissed = YES;
  [self animateButtonPressed:sender
              withCompletion:^{
                [_target shareExtensionViewDidSelectAddToBookmarks:sender];
              }];
}

- (void)animateButtonPressed:(UIButton*)sender
              withCompletion:(void (^)(void))completion {
  NSString* addedString =
      NSLocalizedString(@"IDS_IOS_ADDED_ITEM_SHARE_EXTENSION",
                        @"Button label after being pressed.");
  NSString* addedCheckedString =
      [addedString stringByAppendingString:@" \u2713"];
  // Create a label with the same style as the split animation between the text
  // and the checkmark.
  UILabel* addedLabel = [[UILabel alloc] initWithFrame:CGRectZero];
  [addedLabel setTranslatesAutoresizingMaskIntoConstraints:NO];
  [addedLabel setText:addedString];
  [self addSubview:addedLabel];
  [addedLabel setFont:[sender titleLabel].font];
  [addedLabel setTextColor:[sender titleColorForState:UIControlStateNormal]];
  [addedLabel.leadingAnchor
      constraintEqualToAnchor:[sender titleLabel].leadingAnchor]
      .active = YES;
  [addedLabel.centerYAnchor
      constraintEqualToAnchor:[sender titleLabel].centerYAnchor]
      .active = YES;
  [addedLabel setAlpha:0];

  void (^step3ShowCheck)() = ^{
    [UIView animateWithDuration:ui_util::kAnimationDuration
        animations:^{
          [addedLabel setAlpha:0];
          [sender setAlpha:1];
        }
        completion:^(BOOL finished) {
          if (completion) {
            completion();
          }
        }];
  };

  void (^step2ShowTextWithoutCheck)() = ^{
    [sender setTitle:addedCheckedString forState:UIControlStateNormal];
    [UIView animateWithDuration:ui_util::kAnimationDuration
        animations:^{
          [addedLabel setAlpha:1];
        }
        completion:^(BOOL finished) {
          step3ShowCheck();
        }];
  };

  void (^step1HideText)() = ^{
    [UIView animateWithDuration:ui_util::kAnimationDuration
        animations:^{
          [sender setAlpha:0];
        }
        completion:^(BOOL finished) {
          step2ShowTextWithoutCheck();
        }];
  };
  step1HideText();
}

- (void)cancelPressed:(UIButton*)sender {
  if (_dismissed) {
    return;
  }
  _dismissed = YES;
  [_target shareExtensionViewDidSelectCancel:sender];
}

#pragma mark - Content getters and setters.

- (void)setURL:(NSURL*)URL {
  [[self URLLabel] setText:[URL absoluteString]];
}

- (void)setTitle:(NSString*)title {
  [[self titleLabel] setText:title];
}

- (void)setScreenshot:(UIImage*)screenshot {
  [self.screenshotView.widthAnchor constraintEqualToConstant:kScreenshotSize]
      .active = YES;
  [self.titleURLContainer.trailingAnchor
      constraintEqualToAnchor:self.screenshotView.leadingAnchor
                     constant:-kShareExtensionPadding]
      .active = YES;
  [[self screenshotView] setImage:screenshot];
}

@end
