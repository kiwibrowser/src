// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/location_bar/location_bar_legacy_view.h"

#import "ios/chrome/browser/ui/animation_util.h"
#import "ios/chrome/browser/ui/omnibox/clipping_textfield_container.h"
#import "ios/chrome/browser/ui/omnibox/omnibox_text_field_ios.h"
#include "ios/chrome/browser/ui/rtl_geometry.h"
#include "ios/chrome/browser/ui/ui_util.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/common/material_timing.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ios/chrome/grit/ios_theme_resources.h"
#include "skia/ext/skia_utils_ios.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/image/image.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const CGFloat kLeadingButtonEdgeOffset = 9;
// Offset from the leading edge to the textfield when no image is shown.
const CGFloat kTextFieldLeadingOffsetNoImage = 16;
// Space between the leading button and the textfield when a button is shown.
const CGFloat kTextFieldLeadingOffsetImage = 6;
// The default position animation is 10 pixels toward the trailing side, so
// that's a negative leading offset.
const LayoutOffset kPositionAnimationLeadingOffset = -10;
}  // namespace

@interface OmniboxTextFieldIOS ()

// Gets the bounds of the rect covering the URL.
- (CGRect)preEditLabelRectForBounds:(CGRect)bounds;
// Creates the UILabel if it doesn't already exist and adds it as a
// subview.
- (void)createSelectionViewIfNecessary;
// Helper method used to set the text of this field.  Updates the selection view
// to contain the correct inline autocomplete text.
- (void)setTextInternal:(NSAttributedString*)text
     autocompleteLength:(NSUInteger)autocompleteLength;
// Override deleteBackward so that backspace can clear query refinement chips.
- (void)deleteBackward;
// Returns the layers affected by animations added by |-animateFadeWithStyle:|.
- (NSArray*)fadeAnimationLayers;
// Returns the text that is displayed in the field, including any inline
// autocomplete text that may be present as an NSString. Returns the same
// value as -|displayedText| but prefer to use this to avoid unnecessary
// conversion from NSString to base::string16 if possible.
- (NSString*)nsDisplayedText;

@end

#pragma mark - LocationBarEditView

@interface LocationBarLegacyView ()
// Constraints the leading textfield side to the leading of |self|.
// Active when the |leadingView| is nil or hidden.
@property(nonatomic, strong) NSLayoutConstraint* leadingTextfieldConstraint;
// When the |leadingButton| is not hidden, this is a constraint that links the
// leading edge of the button to self leading edge. Used for animations.
@property(nonatomic, strong) NSLayoutConstraint* leadingButtonLeadingConstraint;
// The textfield container. The |textField| is contained in it, and its frame
// should not be managed directly, instead the location bar uses this container.
// This is required to achieve desired text clipping of long URLs.
@property(nonatomic, strong) ClippingTextFieldContainer* textFieldContainer;
@end

@implementation LocationBarLegacyView
@synthesize textField = _textField;
@synthesize leadingButton = _leadingButton;
@synthesize leadingTextfieldConstraint = _leadingTextfieldConstraint;
@synthesize incognito = _incognito;
@synthesize leadingButtonLeadingConstraint = _leadingButtonLeadingConstraint;
@synthesize textFieldContainer = _textFieldContainer;

#pragma mark - Public properties

- (void)setLeadingButton:(UIButton*)leadingButton {
  _leadingButton = leadingButton;
  _leadingButton.translatesAutoresizingMaskIntoConstraints = NO;
  [_leadingButton
      setContentCompressionResistancePriority:UILayoutPriorityRequired
                                      forAxis:UILayoutConstraintAxisHorizontal];
  [_leadingButton
      setContentCompressionResistancePriority:UILayoutPriorityRequired
                                      forAxis:UILayoutConstraintAxisVertical];
  [_leadingButton setContentHuggingPriority:UILayoutPriorityRequired
                                    forAxis:UILayoutConstraintAxisHorizontal];
  [_leadingButton setContentHuggingPriority:UILayoutPriorityRequired
                                    forAxis:UILayoutConstraintAxisVertical];
}

#pragma mark - Public methods

- (instancetype)initWithFrame:(CGRect)frame
                         font:(UIFont*)font
                    textColor:(UIColor*)textColor
                    tintColor:(UIColor*)tintColor {
  self = [super initWithFrame:frame];
  if (self) {
    _textField = [[OmniboxTextFieldIOS alloc] initWithFrame:frame
                                                       font:font
                                                  textColor:textColor
                                                  tintColor:tintColor];

    // The text field is put into a container.

    // TODO(crbug.com/789968): remove these insets when the location bar
    // background is managed by this view and not toolbar controller. These
    // insets allow the gradient masking of the omnibox to not extend beyond
    // the omnibox background's visible frame.
    self.layoutMargins = UIEdgeInsetsMake(3, 3, 3, 3);

    _textFieldContainer = [[ClippingTextFieldContainer alloc]
        initWithClippingTextField:_textField];
    [self addSubview:_textFieldContainer];

    _leadingTextfieldConstraint = [_textFieldContainer.leadingAnchor
        constraintEqualToAnchor:self.leadingAnchor
                       constant:kTextFieldLeadingOffsetNoImage];

    [NSLayoutConstraint activateConstraints:@[
      [_textFieldContainer.trailingAnchor
          constraintEqualToAnchor:self.layoutMarginsGuide.trailingAnchor],
      [_textFieldContainer.topAnchor
          constraintEqualToAnchor:self.layoutMarginsGuide.topAnchor],
      [_textFieldContainer.bottomAnchor
          constraintEqualToAnchor:self.layoutMarginsGuide.bottomAnchor],
      _leadingTextfieldConstraint,
    ]];

    _textFieldContainer.translatesAutoresizingMaskIntoConstraints = NO;
    [_textFieldContainer
        setContentCompressionResistancePriority:UILayoutPriorityDefaultLow
                                        forAxis:
                                            UILayoutConstraintAxisHorizontal];
  }
  return self;
}

- (void)setLeadingButtonHidden:(BOOL)hidden {
  if (!_leadingButton) {
    return;
  }

  if (hidden) {
    [_leadingButton removeFromSuperview];
    self.leadingTextfieldConstraint.active = YES;
  } else {
    [self addSubview:_leadingButton];
    self.leadingTextfieldConstraint.active = NO;
    self.leadingButtonLeadingConstraint = [self.layoutMarginsGuide.leadingAnchor
        constraintEqualToAnchor:self.leadingButton.leadingAnchor
                       constant:-kLeadingButtonEdgeOffset];

    NSLayoutConstraint* leadingButtonToTextField = nil;
    leadingButtonToTextField = [self.leadingButton.trailingAnchor
        constraintEqualToAnchor:self.textFieldContainer.leadingAnchor
                       constant:-kTextFieldLeadingOffsetImage];

    [NSLayoutConstraint activateConstraints:@[
      [_leadingButton.centerYAnchor
          constraintEqualToAnchor:self.layoutMarginsGuide.centerYAnchor],
      self.leadingButtonLeadingConstraint,
      leadingButtonToTextField,
    ]];
  }
}

- (void)setLeadingButtonEnabled:(BOOL)enabled {
  _leadingButton.enabled = enabled;
}

- (void)setPlaceholderImage:(int)imageID {
  [self.leadingButton setImage:[self placeholderImageWithId:imageID]
                      forState:UIControlStateNormal];
  [self.leadingButton setTintColor:[self tintColorForLeftImageWithID:imageID]];

  // TODO(crbug.com/774121): This should not be done like this; instead the
  // responder status of the textfield should be broadcasted and observed
  // by the mediator of location bar, that would then show/hide the
  // leading button.
  BOOL hidden = (!IsIPadIdiom() && [self.textField isFirstResponder]);
  [self setLeadingButtonHidden:hidden];
}

- (void)fadeInLeadingButton {
  self.leadingButton.alpha = 0;
  // Instead of passing a delay into -fadeInView:, wait to call -fadeInView:.
  // The CABasicAnimation's start and end positions are calculated immediately
  // instead of after the animation's delay, but the omnibox's layer isn't set
  // yet to its final state and as a result the start and end positions will not
  // be correct.
  dispatch_time_t delay = dispatch_time(
      DISPATCH_TIME_NOW, ios::material::kDuration2 * NSEC_PER_SEC);
  dispatch_after(delay, dispatch_get_main_queue(), ^(void) {
    UIView* view = self.leadingButton;
    LayoutOffset leadingOffset = kPositionAnimationLeadingOffset;
    NSTimeInterval duration = ios::material::kDuration1;
    NSTimeInterval delay = 0;

    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    [CATransaction setCompletionBlock:^{
      [view.layer removeAnimationForKey:@"fadeIn"];
    }];
    view.alpha = 1.0;

    // Animate the position of |view| |leadingOffset| pixels after |delay|.
    CGRect shiftedFrame = CGRectLayoutOffset(view.frame, leadingOffset);
    CAAnimation* shiftAnimation =
        FrameAnimationMake(view.layer, shiftedFrame, view.frame);
    shiftAnimation.duration = duration;
    shiftAnimation.beginTime = delay;
    shiftAnimation.timingFunction =
        TimingFunction(ios::material::CurveEaseInOut);

    // Animate the opacity of |view| to 1 after |delay|.
    CAAnimation* fadeAnimation = OpacityAnimationMake(0.0, 1.0);
    fadeAnimation.duration = duration;
    fadeAnimation.beginTime = delay;
    shiftAnimation.timingFunction =
        TimingFunction(ios::material::CurveEaseInOut);

    // Add group animation to layer.
    CAAnimation* group = AnimationGroupMake(@[ shiftAnimation, fadeAnimation ]);
    [view.layer addAnimation:group forKey:@"fadeIn"];

    [CATransaction commit];
  });
}

- (void)fadeOutLeadingButton {
  [self setLeadingButtonHidden:NO];

  UIView* leadingView = [self leadingButton];

  // Move the leadingButton outside of the bounds; this constraint will be
  // created from scratch when the button is shown.
  self.leadingButtonLeadingConstraint.constant = leadingView.frame.size.width;
  [UIView animateWithDuration:ios::material::kDuration2
      delay:0
      options:UIViewAnimationOptionCurveEaseInOut
      animations:^{
        // Fade out the alpha and apply the constraint change above.
        leadingView.alpha = 0;
        [self setNeedsLayout];
        [self layoutIfNeeded];
      }
      completion:^(BOOL finished) {
        // Restore alpha and update the hidden state.
        leadingView.alpha = 1;
        [self setLeadingButtonHidden:YES];
      }];
}

- (void)addExpandOmniboxAnimations:(UIViewPropertyAnimator*)animator
                completionAnimator:(UIViewPropertyAnimator*)completionAnimator {
  // TODO(crbug.com/791455): Due to crbug.com/774121 |self.leadingButton| is
  // hidden in line 151 before the animation starts. For this reason any
  // animation we try doing on |self.leadingButton| will not be visible.
  [self.textField addExpandOmniboxAnimations:animator
                          completionAnimator:completionAnimator];
}

- (void)addContractOmniboxAnimations:(UIViewPropertyAnimator*)animator {
  [self setLeadingButtonHidden:NO];
  self.leadingButton.alpha = 0;
  [animator addAnimations:^{
    self.leadingButton.alpha = 1;
    [self setLeadingButtonHidden:YES];
    [self layoutIfNeeded];
  }];
  [self.textField addContractOmniboxAnimations:animator];
}

#pragma mark - Private methods

// Retrieves a resource image by ID and returns it as UIImage.
- (UIImage*)placeholderImageWithId:(int)imageID {
  return [NativeImage(imageID)
      imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
}

// Returns the tint color for the left image. This is necessary because the
// resource images are stored as templates, but in non-incognito the securtiy
// indicator needs to have a color depending on security status.
- (UIColor*)tintColorForLeftImageWithID:(int)imageID {
  UIColor* tint = [UIColor whiteColor];
  if (!self.incognito) {
    switch (imageID) {
      case IDR_IOS_LOCATION_BAR_HTTP:
        tint = [UIColor darkGrayColor];
        break;
      case IDR_IOS_OMNIBOX_HTTPS_VALID:
        tint = skia::UIColorFromSkColor(gfx::kGoogleGreen700);
        break;
      case IDR_IOS_OMNIBOX_HTTPS_POLICY_WARNING:
        tint = skia::UIColorFromSkColor(gfx::kGoogleYellow700);
        break;
      case IDR_IOS_OMNIBOX_HTTPS_INVALID:
        tint = skia::UIColorFromSkColor(gfx::kGoogleRed700);
        break;
      default:
        tint = [UIColor darkGrayColor];
    }
  }
  return tint;
}

#pragma mark - OmniboxLeftImageConsumer

- (void)setLeftImageId:(int)imageId {
  [self setPlaceholderImage:imageId];
}

@end
