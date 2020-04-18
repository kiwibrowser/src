// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/ntp/incognito_view.h"

#include "components/google/core/browser/google_util.h"
#include "components/strings/grit/components_strings.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/ui/rtl_geometry.h"
#include "ios/chrome/browser/ui/ui_util.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/browser/ui/url_loader.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#import "ios/third_party/material_components_ios/src/components/Buttons/src/MaterialButtons.h"
#import "ios/third_party/material_components_ios/src/components/Palettes/src/MaterialPalettes.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"
#import "ios/web/public/navigation_manager.h"
#include "ios/web/public/referrer.h"
#import "net/base/mac/url_conversions.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// The URL for the the Learn More page shown on incognito new tab.
// Taken from ntp_resource_cache.cc.
const char kLearnMoreIncognitoUrl[] =
    "https://www.google.com/support/chrome/bin/answer.py?answer=95464";

GURL GetUrlWithLang(const GURL& url) {
  std::string locale = GetApplicationContext()->GetApplicationLocale();
  return google_util::AppendGoogleLocaleParam(url, locale);
}

const CGFloat kStackViewHorizontalMargin = 24.0;
const CGFloat kStackViewMaxWidth = 416.0;
const CGFloat kStackViewDefaultSpacing = 32.0;
const CGFloat kStackViewImageSpacing = 24.0;
const CGFloat kLayoutGuideVerticalMargin = 8.0;
const CGFloat kLayoutGuideMinHeight = 12.0;

const int kLinkColor = 0x03A9F4;
}  // namespace

@implementation IncognitoView {
  __weak id<UrlLoader> _loader;
  UIView* _containerView;
  UIStackView* _stackView;

  // Layout Guide whose height is the height of the bottom unsafe area.
  UILayoutGuide* _bottomUnsafeAreaGuide;
  UILayoutGuide* _bottomUnsafeAreaGuideInSuperview;

  // Constraint ensuring that |containerView| is at least as high as the
  // superview of the IncognitoNTPView, i.e. the Incognito panel.
  // This ensures that if the Incognito panel is higher than a compact
  // |containerView|, the |containerView|'s |topGuide| and |bottomGuide| are
  // forced to expand, centering the views in between them.
  NSArray<NSLayoutConstraint*>* _superViewConstraints;
}

- (instancetype)initWithFrame:(CGRect)frame urlLoader:(id<UrlLoader>)loader {
  self = [super initWithFrame:frame];
  if (self) {
    _loader = loader;

    self.alwaysBounceVertical = YES;
    if (@available(iOS 11.0, *)) {
      // The bottom safe area is taken care of with the bottomUnsafeArea guides.
      self.contentInsetAdjustmentBehavior =
          UIScrollViewContentInsetAdjustmentNever;
    }

    // Container to hold and vertically position the stack view.
    _containerView = [[UIView alloc] initWithFrame:frame];
    [_containerView setTranslatesAutoresizingMaskIntoConstraints:NO];

    // Stackview in which all the subviews (image, labels, button) are added.
    _stackView = [[UIStackView alloc] init];
    [_stackView setTranslatesAutoresizingMaskIntoConstraints:NO];
    _stackView.axis = UILayoutConstraintAxisVertical;
    _stackView.spacing = kStackViewDefaultSpacing;
    _stackView.distribution = UIStackViewDistributionFill;
    _stackView.alignment = UIStackViewAlignmentCenter;
    [_containerView addSubview:_stackView];

    // Incognito image.
    UIImageView* incognitoImage = [[UIImageView alloc]
        initWithImage:[UIImage imageNamed:@"incognito_icon"]];
    [incognitoImage setTranslatesAutoresizingMaskIntoConstraints:NO];
    [_stackView addArrangedSubview:incognitoImage];
    if (@available(iOS 11.0, *)) {
      [_stackView setCustomSpacing:kStackViewImageSpacing
                         afterView:incognitoImage];
    }

    // Title.
    UIFont* titleFont = [[MDCTypography fontLoader] lightFontOfSize:24];
    UILabel* incognitoTabHeading =
        [self labelWithString:l10n_util::GetNSString(IDS_NEW_TAB_OTR_HEADING)
                         font:titleFont
                        alpha:0.8];
    [_stackView addArrangedSubview:incognitoTabHeading];

    // Description paragraph.
    UIFont* regularFont = [[MDCTypography fontLoader] regularFontOfSize:14];
    UILabel* incognitoTabDescription = [self
        labelWithString:l10n_util::GetNSString(IDS_NEW_TAB_OTR_DESCRIPTION)
                   font:regularFont
                  alpha:0.7];
    [_stackView addArrangedSubview:incognitoTabDescription];

    // Warning paragraph.
    UILabel* incognitoTabWarning = [self
        labelWithString:l10n_util::GetNSString(IDS_NEW_TAB_OTR_MESSAGE_WARNING)
                   font:regularFont
                  alpha:0.7];
    [_stackView addArrangedSubview:incognitoTabWarning];

    // Learn more button.
    MDCButton* learnMore = [[MDCFlatButton alloc] init];
    [learnMore setBackgroundColor:[UIColor clearColor]
                         forState:UIControlStateNormal];
    UIColor* inkColor =
        [[[MDCPalette greyPalette] tint300] colorWithAlphaComponent:0.25];
    [learnMore setInkColor:inkColor];
    [learnMore setTranslatesAutoresizingMaskIntoConstraints:NO];
    [learnMore setTitle:l10n_util::GetNSString(IDS_NEW_TAB_OTR_LEARN_MORE_LINK)
               forState:UIControlStateNormal];
    [learnMore setTitleColor:UIColorFromRGB(kLinkColor)
                    forState:UIControlStateNormal];
    UIFont* buttonFont = [[MDCTypography fontLoader] boldFontOfSize:14];
    [[learnMore titleLabel] setFont:buttonFont];
    [learnMore addTarget:self
                  action:@selector(learnMoreButtonPressed)
        forControlEvents:UIControlEventTouchUpInside];
    [_stackView addArrangedSubview:learnMore];

    // |topGuide| and |bottomGuide| exist to vertically position the stackview
    // inside the container scrollview.
    UILayoutGuide* topGuide = [[UILayoutGuide alloc] init];
    UILayoutGuide* bottomGuide = [[UILayoutGuide alloc] init];
    _bottomUnsafeAreaGuide = [[UILayoutGuide alloc] init];
    [_containerView addLayoutGuide:topGuide];
    [_containerView addLayoutGuide:bottomGuide];
    [_containerView addLayoutGuide:_bottomUnsafeAreaGuide];

    [self addSubview:_containerView];

    [NSLayoutConstraint activateConstraints:@[
      // Position the stackview between the two guides.
      [topGuide.topAnchor constraintEqualToAnchor:_containerView.topAnchor],
      [_stackView.topAnchor constraintEqualToAnchor:topGuide.bottomAnchor
                                           constant:kLayoutGuideVerticalMargin],
      [bottomGuide.topAnchor
          constraintEqualToAnchor:_stackView.bottomAnchor
                         constant:kLayoutGuideVerticalMargin],
      [_containerView.bottomAnchor
          constraintEqualToAnchor:bottomGuide.bottomAnchor],

      // Center the stackview horizontally with a minimum margin.
      [_stackView.leadingAnchor
          constraintGreaterThanOrEqualToAnchor:_containerView.leadingAnchor
                                      constant:kStackViewHorizontalMargin],
      [_stackView.trailingAnchor
          constraintLessThanOrEqualToAnchor:_containerView.trailingAnchor
                                   constant:-kStackViewHorizontalMargin],
      [_stackView.centerXAnchor
          constraintEqualToAnchor:_containerView.centerXAnchor],

      // Constraint the _bottomUnsafeAreaGuide to the stack view and the
      // container view. Its height is set in the -didMoveToSuperview to take
      // into account the unsafe area.
      [_bottomUnsafeAreaGuide.topAnchor
          constraintEqualToAnchor:_stackView.bottomAnchor
                         constant:2 * kLayoutGuideMinHeight +
                                  kLayoutGuideVerticalMargin],
      [_bottomUnsafeAreaGuide.bottomAnchor
          constraintEqualToAnchor:_containerView.bottomAnchor],

      // Ensure that the stackview width is constrained.
      [_stackView.widthAnchor
          constraintLessThanOrEqualToConstant:kStackViewMaxWidth],

      // Set a minimum top margin and make the bottom guide twice as tall as the
      // top guide.
      [topGuide.heightAnchor
          constraintGreaterThanOrEqualToConstant:kLayoutGuideMinHeight],
      [bottomGuide.heightAnchor constraintEqualToAnchor:topGuide.heightAnchor
                                             multiplier:2.0],
    ]];

    // Constraints comunicating the size of the contentView to the scrollview.
    // See UIScrollView autolayout information at
    // https://developer.apple.com/library/ios/releasenotes/General/RN-iOSSDK-6_0/index.html
    NSDictionary* viewsDictionary = @{@"containerView" : _containerView};
    NSArray* constraints = @[
      @"V:|-0-[containerView]-0-|",
      @"H:|-0-[containerView]-0-|",
    ];
    ApplyVisualConstraints(constraints, viewsDictionary);
  }
  return self;
}

#pragma mark - UIView overrides

- (void)didMoveToSuperview {
  [super didMoveToSuperview];
  if (!self.superview)
    return;

  id<LayoutGuideProvider> safeAreaGuide =
      SafeAreaLayoutGuideForView(self.superview);
  _bottomUnsafeAreaGuideInSuperview = [[UILayoutGuide alloc] init];
  [self.superview addLayoutGuide:_bottomUnsafeAreaGuideInSuperview];

  _superViewConstraints = @[
    [safeAreaGuide.bottomAnchor
        constraintEqualToAnchor:_bottomUnsafeAreaGuideInSuperview.topAnchor],
    [self.superview.bottomAnchor
        constraintEqualToAnchor:_bottomUnsafeAreaGuideInSuperview.bottomAnchor],
    [_bottomUnsafeAreaGuide.heightAnchor
        constraintGreaterThanOrEqualToAnchor:_bottomUnsafeAreaGuideInSuperview
                                                 .heightAnchor],
    [_containerView.widthAnchor
        constraintEqualToAnchor:self.superview.widthAnchor],
    [_containerView.heightAnchor
        constraintGreaterThanOrEqualToAnchor:self.superview.heightAnchor],
  ];

  [NSLayoutConstraint activateConstraints:_superViewConstraints];
}

- (void)willMoveToSuperview:(UIView*)newSuperview {
  [NSLayoutConstraint deactivateConstraints:_superViewConstraints];
  [self.superview removeLayoutGuide:_bottomUnsafeAreaGuideInSuperview];
  [super willMoveToSuperview:newSuperview];
}

#pragma mark - Private

// Returns an autoreleased label with the right format.
- (UILabel*)labelWithString:(NSString*)string
                       font:(UIFont*)font
                      alpha:(float)alpha {
  NSMutableAttributedString* attributedString =
      [[NSMutableAttributedString alloc] initWithString:string];
  NSMutableParagraphStyle* paragraphStyle =
      [[NSMutableParagraphStyle alloc] init];
  [paragraphStyle setLineSpacing:4];
  [paragraphStyle setAlignment:NSTextAlignmentJustified];
  [attributedString addAttribute:NSParagraphStyleAttributeName
                           value:paragraphStyle
                           range:NSMakeRange(0, string.length)];
  UILabel* label = [[UILabel alloc] initWithFrame:CGRectZero];
  [label setTranslatesAutoresizingMaskIntoConstraints:NO];
  [label setNumberOfLines:0];
  [label setFont:font];
  [label setAttributedText:attributedString];
  [label setTextColor:[UIColor colorWithWhite:1.0 alpha:alpha]];
  return label;
}

// Triggers a navigation to the help page.
- (void)learnMoreButtonPressed {
  web::NavigationManager::WebLoadParams params(
      GetUrlWithLang(GURL(kLearnMoreIncognitoUrl)));
  [_loader loadURLWithParams:params];
}

@end
