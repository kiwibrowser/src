// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// For performance reasons, the composition of the card frame is broken up into
// four pieces.  The overall structure of the CardView is:
// - CardView
//   - Snapshot (UIImageView)
//   - FrameTop (UIImageView)
//   - FrameLeft (UIImageView)
//   - FrameRight (UIImageView)
//   - FrameBottom (UIImageView)
//   - CardTabView (UIView::DrawRect)
//
// While it would be simpler to put the frame in one transparent UIImageView,
// that would make the entire snapshot area needlessly color-blended.  Instead
// the frame is broken up into four pieces, top, left, bottom, right.
//
// The frame's tab gets its own view above everything else (CardTabView) so that
// it can be animated out. It's also transparent since the tab has a curve and
// a shadow.

#import "ios/chrome/browser/ui/stack_view/card_view.h"

#import <QuartzCore/QuartzCore.h>
#include <algorithm>

#import "base/mac/foundation_util.h"

#include "components/strings/grit/components_strings.h"
#import "ios/chrome/browser/ui/animation_util.h"
#import "ios/chrome/browser/ui/reversed_animation.h"
#import "ios/chrome/browser/ui/rtl_geometry.h"
#import "ios/chrome/browser/ui/stack_view/close_button.h"
#import "ios/chrome/browser/ui/stack_view/title_label.h"
#import "ios/chrome/browser/ui/ui_util.h"
#import "ios/chrome/common/material_timing.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/favicon_size.h"
#include "ui/gfx/image/image.h"
#import "ui/gfx/ios/uikit_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using ios::material::TimingFunction;

const UIEdgeInsets kCardImageInsets = {48.0, 4.0, 4.0, 4.0};
const CGFloat kCardFrameInset = 1.0;
const CGFloat kCardShadowThickness = 16.0;
const CGFloat kCardFrameCornerRadius = 4.0;
const CGFloat kCardTabTopInset = 4;
const CGFloat kCardFrameBackgroundBrightness = 242.0 / 255.0;
const CGFloat kCardFrameBackgroundBrightnessIncognito = 80.0 / 255.0;
const CGFloat kCardFrameImageSnapshotOverlap = 1.0;
NSString* const kCardShadowImageName = @"card_frame_shadow";
const UIEdgeInsets kCardShadowLayoutOutsets = {-14.0, -22.0, -14.0, -22.0};

namespace {

// Chrome corners and edges.
const CGFloat kCardTabTitleMargin = 4;
const CGFloat kCardTabFavIconLeadingMargin = 12;
const CGFloat kCardTabFavIconCloseButtonPadding = 8.0;
const UIEdgeInsets kCloseButtonContentInsets = {12.0, 12.0, 12.0, 12.0};
const UIEdgeInsets kShadowStretchInsets = {51.0, 47.0, 51.0, 47.0};

// Animation key for explicit animations added by
// |-animateFromBeginFrame:toEndFrame:tabAnimationStyle:|.
NSString* const kCardViewAnimationKey = @"CardViewAnimation";

// Returns the appropriate variant of the image for |image_name| based on
// |is_incognito|.
UIImage* ImageWithName(NSString* image_name, BOOL is_incognito) {
  NSString* name = is_incognito
                       ? [image_name stringByAppendingString:@"_incognito"]
                       : image_name;
  return [UIImage imageNamed:name];
}

}  // namespace

#pragma mark -

@interface CardTabView : UIView

@property(nonatomic, assign) CardCloseButtonSide closeButtonSide;
@property(nonatomic, strong) UIImageView* favIconView;
@property(nonatomic, strong) UIImage* favicon;
@property(nonatomic, strong) CloseButton* closeButton;
@property(nonatomic, strong) TitleLabel* titleLabel;
@property(nonatomic, assign) BOOL isIncognito;

// Layout helper selectors that calculate the frames for subviews given the
// bounds of the card tab.  Note that the frames returned by these selectors
// will be different depending on the value of the |displaySide| property.
- (LayoutRect)faviconLayoutForBounds:(CGRect)bounds;
- (CGRect)faviconFrameForBounds:(CGRect)bounds;
- (LayoutRect)titleLabelLayoutForBounds:(CGRect)bounds;
- (CGRect)titleLabelFrameForBounds:(CGRect)bounds;
- (LayoutRect)closeButtonLayoutForBounds:(CGRect)bounds;
- (CGRect)closeButtonFrameForBounds:(CGRect)bounds;

// Adds frame animations for the favicon, title, and close button.  The subviews
// will be faded in if |tabAnimationStyle| = CardTabAnimationStyleFadeIn and
// faded out if |tabAnimationStyle| = CardTabAnimationStyleFadeOut.
- (void)animateFromBeginFrame:(CGRect)beginFrame
                   toEndFrame:(CGRect)endFrame
            tabAnimationStyle:(CardTabAnimationStyle)tabAnimationStyle;

// Called by CardView's selector of the same name and reverses animations added
// by |-animateFromBeginFrame:toEndFrame:tabAnimationStyle:|.
- (void)reverseAnimations;

// Called by CardView's selector of the same name and removes animations added
// by |-animateFromBeginFrame:toEndFrame:tabAnimationStyle:|.
- (void)cleanUpAnimations;

// Initialize a CardTabView with |frame| and |isIncognito| state.
- (instancetype)initWithFrame:(CGRect)frame
                  isIncognito:(BOOL)isIncognito NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithCoder:(NSCoder*)aDecoder NS_UNAVAILABLE;

@end

@implementation CardTabView

#pragma mark - Property Implementation

@synthesize closeButtonSide = _closeButtonSide;
@synthesize favIconView = _faviconView;
@synthesize favicon = _favicon;
@synthesize closeButton = _closeButton;
@synthesize titleLabel = _titleLabel;
@synthesize isIncognito = _isIncognito;

- (instancetype)initWithFrame:(CGRect)frame {
  return [self initWithFrame:frame isIncognito:NO];
}

- (instancetype)initWithFrame:(CGRect)frame isIncognito:(BOOL)isIncognito {
  self = [super initWithFrame:frame];
  if (!self)
    return self;

  _isIncognito = isIncognito;

  UIImage* image = ImageWithName(@"default_favicon", _isIncognito);
  _faviconView = [[UIImageView alloc] initWithImage:image];
  [self addSubview:_faviconView];

  UIImage* normal = ImageWithName(@"card_close_button", _isIncognito);
  UIImage* pressed = ImageWithName(@"card_close_button_pressed", _isIncognito);

  _closeButton = [[CloseButton alloc] initWithFrame:CGRectZero];
  [_closeButton setAccessibilityLabel:l10n_util::GetNSString(IDS_CLOSE)];
  [_closeButton setImage:normal forState:UIControlStateNormal];
  [_closeButton setImage:pressed forState:UIControlStateHighlighted];
  [_closeButton setContentEdgeInsets:kCloseButtonContentInsets];
  [_closeButton sizeToFit];
  [self addSubview:_closeButton];

  _titleLabel = [[TitleLabel alloc] initWithFrame:CGRectZero];
  [_titleLabel setFont:[MDCTypography body1Font]];
  [self addSubview:_titleLabel];

  [self updateTitleColors];

  return self;
}

- (instancetype)initWithCoder:(NSCoder*)aDecoder {
  NOTREACHED();
  return nil;
}

- (void)setCloseButtonSide:(CardCloseButtonSide)closeButtonSide {
  if (_closeButtonSide != closeButtonSide) {
    _closeButtonSide = closeButtonSide;
    [self setNeedsLayout];
  }
}

- (void)layoutSubviews {
  [super layoutSubviews];
  self.favIconView.frame = [self faviconFrameForBounds:self.bounds];
  self.titleLabel.frame = [self titleLabelFrameForBounds:self.bounds];
  self.closeButton.frame = [self closeButtonFrameForBounds:self.bounds];
}

- (LayoutRect)faviconLayoutForBounds:(CGRect)bounds {
  LayoutRect layout;
  layout.boundingWidth = CGRectGetWidth(bounds);
  layout.size = CGSizeMake(gfx::kFaviconSize, gfx::kFaviconSize);
  layout.position.originY = CGRectGetMidY(bounds) - 0.5 * layout.size.height;
  if (self.closeButtonSide == CardCloseButtonSide::LEADING) {
    // Layout |kCardTabFavIconCloseButtonPadding| from the close button's
    // trailing edge.
    layout.position.leading =
        LayoutRectGetTrailingEdge([self closeButtonLayoutForBounds:bounds]) +
        kCardTabFavIconCloseButtonPadding;
  } else {
    // Layout |kCardTabFavIconLeadingMargin| from the leading edge of the
    // bounds.
    layout.position.leading = kCardTabFavIconLeadingMargin;
  }
  return layout;
}

- (CGRect)faviconFrameForBounds:(CGRect)bounds {
  return AlignRectOriginAndSizeToPixels(
      LayoutRectGetRect([self faviconLayoutForBounds:bounds]));
}

- (LayoutRect)titleLabelLayoutForBounds:(CGRect)bounds {
  LayoutRect layout;
  layout.boundingWidth = CGRectGetWidth(bounds);
  layout.size = [self.titleLabel sizeThatFits:bounds.size];
  layout.position.originY = CGRectGetMidY(bounds) - 0.5 * layout.size.height;
  const CGFloat titlePadding = 2.0 * kCardTabTitleMargin;
  CGFloat faviconTrailingEdge =
      LayoutRectGetTrailingEdge([self faviconLayoutForBounds:bounds]);
  CGFloat maxWidth = CGFLOAT_MAX;
  if (self.closeButtonSide == CardCloseButtonSide::LEADING) {
    // Layout between the favicon and the trailing edge of the bounds.
    maxWidth = CGRectGetMaxX(bounds) - faviconTrailingEdge - titlePadding;
  } else {
    // Lay out between the favicon and the close button.
    maxWidth = [self closeButtonLayoutForBounds:bounds].position.leading -
               faviconTrailingEdge - titlePadding;
  }
  layout.size.width = std::min(layout.size.width, maxWidth);
  layout.position.leading = faviconTrailingEdge + kCardTabTitleMargin;
  return layout;
}

- (CGRect)titleLabelFrameForBounds:(CGRect)bounds {
  return AlignRectOriginAndSizeToPixels(
      LayoutRectGetRect([self titleLabelLayoutForBounds:bounds]));
}

- (LayoutRect)closeButtonLayoutForBounds:(CGRect)bounds {
  LayoutRect layout;
  layout.boundingWidth = CGRectGetWidth(bounds);
  layout.size = [self.closeButton sizeThatFits:bounds.size];
  layout.position.originY = CGRectGetMidY(bounds) - 0.5 * layout.size.height;
  layout.position.leading = self.closeButtonSide == CardCloseButtonSide::LEADING
                                ? 0.0
                                : layout.boundingWidth - layout.size.width;
  return layout;
}

- (CGRect)closeButtonFrameForBounds:(CGRect)bounds {
  return AlignRectOriginAndSizeToPixels(
      LayoutRectGetRect([self closeButtonLayoutForBounds:bounds]));
}

- (CGRect)closeButtonRect {
  return [_closeButton frame];
}

- (void)setTitle:(NSString*)title {
  [_titleLabel setText:title];
  [_closeButton setAccessibilityValue:title];

  [self setNeedsUpdateConstraints];
}

- (void)setFavicon:(UIImage*)favicon {
  if (favicon != _favicon) {
    _favicon = favicon;
    [self updateFaviconImage];
  }
}

- (void)updateTitleColors {
  UIColor* titleColor = [UIColor blackColor];
  if (_isIncognito)
    titleColor = [UIColor whiteColor];

  [_titleLabel setTextColor:titleColor];
}

- (void)updateFaviconImage {
  UIImage* favicon = _favicon;
  if (!favicon)
    favicon = ImageWithName(@"default_favicon", _isIncognito);

  [_faviconView setImage:favicon];

  [self setNeedsUpdateConstraints];
}

- (void)animateFromBeginFrame:(CGRect)beginFrame
                   toEndFrame:(CGRect)endFrame
            tabAnimationStyle:(CardTabAnimationStyle)tabAnimationStyle {
  // Animation values.
  CAAnimation* frameAnimation = nil;
  CFTimeInterval frameDuration = ios::material::kDuration1;
  CAMediaTimingFunction* frameTiming =
      TimingFunction(ios::material::CurveEaseInOut);
  CGRect beginBounds = {CGPointZero, beginFrame.size};
  CGRect endBounds = {CGPointZero, endFrame.size};
  BOOL shouldAnimateFade = (tabAnimationStyle != CARD_TAB_ANIMATION_STYLE_NONE);
  BOOL shouldFadeIn = (tabAnimationStyle == CARD_TAB_ANIMATION_STYLE_FADE_IN);
  CAAnimation* opacityAnimation = nil;
  CAMediaTimingFunction* opacityTiming = nil;
  if (shouldAnimateFade) {
    opacityTiming = TimingFunction(shouldFadeIn ? ios::material::CurveEaseOut
                                                : ios::material::CurveEaseIn);
  }
  CGFloat beginOpacity = shouldFadeIn ? 0.0 : 1.0;
  CGFloat endOpacity = shouldFadeIn ? 1.0 : 0.0;
  CAAnimation* animation = nil;

  // Update layer geometry.
  frameAnimation = FrameAnimationMake(self.layer, beginFrame, endFrame);
  frameAnimation.duration = frameDuration;
  frameAnimation.timingFunction = frameTiming;
  [self.layer addAnimation:frameAnimation forKey:kCardViewAnimationKey];

  // Update favicon.
  frameAnimation = FrameAnimationMake(_faviconView.layer,
                                      [self faviconFrameForBounds:beginBounds],
                                      [self faviconFrameForBounds:endBounds]);
  frameAnimation.duration = frameDuration;
  frameAnimation.timingFunction = frameTiming;
  if (shouldAnimateFade) {
    opacityAnimation = OpacityAnimationMake(beginOpacity, endOpacity);
    opacityAnimation.duration = ios::material::kDuration8;
    opacityAnimation.timingFunction = opacityTiming;
    opacityAnimation.beginTime = shouldFadeIn ? ios::material::kDuration8 : 0.0;
    animation = AnimationGroupMake(@[ frameAnimation, opacityAnimation ]);
  } else {
    animation = frameAnimation;
  }
  [_faviconView.layer addAnimation:animation forKey:kCardViewAnimationKey];

  // Update close button.
  frameAnimation = FrameAnimationMake(
      _closeButton.layer, [self closeButtonFrameForBounds:beginBounds],
      [self closeButtonFrameForBounds:endBounds]);
  frameAnimation.duration = frameDuration;
  frameAnimation.timingFunction = frameTiming;
  if (shouldAnimateFade) {
    opacityAnimation = OpacityAnimationMake(beginOpacity, endOpacity);
    opacityAnimation.timingFunction = opacityTiming;
    opacityAnimation.duration =
        shouldFadeIn ? ios::material::kDuration1 : ios::material::kDuration8;
    opacityAnimation.beginTime = shouldFadeIn ? ios::material::kDuration1 : 0.0;
    animation = AnimationGroupMake(@[ frameAnimation, opacityAnimation ]);
  } else {
    animation = frameAnimation;
  }
  [_closeButton.layer addAnimation:animation forKey:kCardViewAnimationKey];

  // Update title.
  [_titleLabel animateFromBeginFrame:[self titleLabelFrameForBounds:beginBounds]
                          toEndFrame:[self titleLabelFrameForBounds:endBounds]
                            duration:frameDuration
                      timingFunction:frameTiming];
  if (shouldAnimateFade) {
    opacityAnimation = OpacityAnimationMake(beginOpacity, endOpacity);
    opacityAnimation.timingFunction = opacityTiming;
    opacityAnimation.duration =
        shouldFadeIn ? ios::material::kDuration6 : ios::material::kDuration8;
    CFTimeInterval delay = shouldFadeIn ? ios::material::kDuration2 : 0.0;
    opacityAnimation = DelayedAnimationMake(opacityAnimation, delay);
    [_titleLabel.layer addAnimation:opacityAnimation
                             forKey:kCardViewAnimationKey];
  }
}

- (void)reverseAnimations {
  [_titleLabel reverseAnimations];
  ReverseAnimationsForKeyForLayers(kCardViewAnimationKey, @[
    self.layer, _faviconView.layer, _closeButton.layer, _titleLabel.layer
  ]);
}

- (void)cleanUpAnimations {
  [_titleLabel cleanUpAnimations];
  RemoveAnimationForKeyFromLayers(kCardViewAnimationKey, @[
    self.layer, _faviconView.layer, _closeButton.layer, _titleLabel.layer
  ]);
}

// Implementation of this protocol forces VoiceOver to read the titleLabel and
// close button in the correct order (top left to bottom right, by row). Because
// the top border the close button's focus area is higher than the label's, in
// portrait mode VoiceOver automatically orders the close button first, although
// it looks like the default behavior should read the elements from left to
// right.
#pragma mark - UIAccessibilityContainer methods

// CardTabView accessibility elements: titleLabel and closeButton.
- (NSInteger)accessibilityElementCount {
  return 2;
}

// Returns the leftmost accessibility element if |index| = 0 and the rightmost
// accessibility element if |index| = 1.  The display side determines the
// location of the close button relative to the title label.
- (id)accessibilityElementAtIndex:(NSInteger)index {
  BOOL closeButtonLeading =
      self.closeButtonSide == CardCloseButtonSide::LEADING;
  id element = nil;
  if (index == 0)
    element = closeButtonLeading ? _closeButton : _titleLabel;
  else if (index == 1)
    element = closeButtonLeading ? _titleLabel : _closeButton;
  return element;
}

// Returns 0 if element is on the left (titleLabel in portrait, closeButton in
// landscape), and 1 if the element is on the right. Otherwise returns
// NSNotFound.
- (NSInteger)indexOfAccessibilityElement:(id)element {
  BOOL closeButtonLeading =
      self.closeButtonSide == CardCloseButtonSide::LEADING;
  NSInteger index = NSNotFound;
  if (element == _closeButton)
    index = closeButtonLeading ? 0 : 1;
  else if (element == _titleLabel)
    index = closeButtonLeading ? 1 : 0;
  return index;
}

@end

#pragma mark -

@interface CardView () {
  UIImageView* _contents;
  CardTabView* _tab;
  __weak id _cardCloseTarget;
  SEL _cardCloseAction;
  __weak id _accessibilityTarget;
  SEL _accessibilityAction;

  BOOL _isIncognito;  // YES if the card should use the incognito styling.

  // Pieces of the card frame, split into four UIViews.
  UIImageView* _frameLeft;
  UIImageView* _frameRight;
  UIImageView* _frameTop;
  UIImageView* _frameBottom;
  UIImageView* _frameShadowImageView;
  CALayer* _shadowMask;
}

// The LayoutRect for the CardTabView.
- (LayoutRect)tabLayout;

// Sends |_cardCloseAction| to |_cardCloseTarget| with |self| as the sender.
- (void)closeButtonWasTapped:(id)sender;

// Resizes/zooms the snapshot to avoid stretching given the card's current
// bounds.
- (void)updateImageBoundsAndZoom;

// If the image is reset during an animation, this is called to update the
// snapshot's contentsRect animation for the new image.
- (void)updateSnapshotAnimations;

// The frame to use for the shadow mask for a CardVeiw with |bounds|.
- (CGRect)shadowMaskFrameForBounds:(CGRect)bounds;

// Updates the mask used for the shadow.
- (void)updateShadowMask;

// Returns the contentsRect that will correctly zoom the image's layer for the
// given card size.
- (CGRect)imageContentsRectForCardSize:(CGSize)cardSize;

// Animates the frame image such that it no longer overlaps the image snapshot.
- (void)animateOutFrameImageOverlap;
// Returns the frames used for the image views for a given bounds.
- (CGRect)frameLeftFrameForBounds:(CGRect)bounds;
- (CGRect)frameRightFrameForBounds:(CGRect)bounds;
- (CGRect)frameTopFrameForBounds:(CGRect)bounds;
- (CGRect)frameBottomFrameForBounds:(CGRect)bounds;

@end

@implementation CardView

@synthesize isActiveTab = _isActiveTab;
@synthesize shouldShowShadow = _shouldShowShadow;
@synthesize shouldMaskShadow = _shouldMaskShadow;

- (instancetype)initWithFrame:(CGRect)frame {
  return [self initWithFrame:frame isIncognito:NO];
}

- (instancetype)initWithFrame:(CGRect)frame isIncognito:(BOOL)isIncognito {
  self = [super initWithFrame:frame];
  if (!self)
    return self;

  _isIncognito = isIncognito;
  CGRect bounds = self.bounds;

  self.opaque = NO;
  self.contentMode = UIViewContentModeRedraw;

  CGRect shadowFrame = UIEdgeInsetsInsetRect(bounds, kCardShadowLayoutOutsets);
  _frameShadowImageView = [[UIImageView alloc] initWithFrame:shadowFrame];
  [_frameShadowImageView
      setAutoresizingMask:(UIViewAutoresizingFlexibleWidth |
                           UIViewAutoresizingFlexibleHeight)];
  [self addSubview:_frameShadowImageView];

  // Calling properties for side-effects.
  self.shouldShowShadow = YES;
  self.shouldMaskShadow = YES;

  UIImage* image = [UIImage imageNamed:kCardShadowImageName];
  image = [image resizableImageWithCapInsets:kShadowStretchInsets];
  [_frameShadowImageView setImage:image];

  CGRect snapshotFrame = UIEdgeInsetsInsetRect(bounds, kCardImageInsets);
  _contents = [[UIImageView alloc] initWithFrame:snapshotFrame];
  [_contents setClipsToBounds:YES];
  [_contents setContentMode:UIViewContentModeScaleAspectFill];
  [_contents setFrame:snapshotFrame];
  [_contents setAutoresizingMask:UIViewAutoresizingFlexibleWidth |
                                 UIViewAutoresizingFlexibleHeight];
  [self addSubview:_contents];

  image = [UIImage imageNamed:isIncognito ? @"border_frame_incognito_left"
                                          : @"border_frame_left"];
  UIEdgeInsets imageStretchInsets = UIEdgeInsetsMake(
      0.5 * image.size.height, 0.0, 0.5 * image.size.height, 0.0);
  image = [image resizableImageWithCapInsets:imageStretchInsets];
  _frameLeft = [[UIImageView alloc] initWithImage:image];
  [self addSubview:_frameLeft];

  image = [UIImage imageNamed:isIncognito ? @"border_frame_incognito_right"
                                          : @"border_frame_right"];
  imageStretchInsets = UIEdgeInsetsMake(0.5 * image.size.height, 0.0,
                                        0.5 * image.size.height, 0.0);
  image = [image resizableImageWithCapInsets:imageStretchInsets];
  _frameRight = [[UIImageView alloc] initWithImage:image];
  [self addSubview:_frameRight];

  image = [UIImage imageNamed:isIncognito ? @"border_frame_incognito_top"
                                          : @"border_frame_top"];
  imageStretchInsets = UIEdgeInsetsMake(0.0, 0.5 * image.size.width, 0.0,
                                        0.5 * image.size.width);
  image = [image resizableImageWithCapInsets:imageStretchInsets];
  _frameTop = [[UIImageView alloc] initWithImage:image];
  [self addSubview:_frameTop];

  image = [UIImage imageNamed:isIncognito ? @"border_frame_incognito_bottom"
                                          : @"border_frame_bottom"];
  imageStretchInsets = UIEdgeInsetsMake(0.0, 0.5 * image.size.width, 0.0,
                                        0.5 * image.size.width);
  image = [image resizableImageWithCapInsets:imageStretchInsets];
  _frameBottom = [[UIImageView alloc] initWithImage:image];
  [self addSubview:_frameBottom];

  _tab = [[CardTabView alloc] initWithFrame:LayoutRectGetRect([self tabLayout])
                                isIncognito:_isIncognito];
  [_tab setCloseButtonSide:IsPortrait() ? CardCloseButtonSide::TRAILING
                                        : CardCloseButtonSide::LEADING];
  [[_tab closeButton] addTarget:self
                         action:@selector(closeButtonWasTapped:)
               forControlEvents:UIControlEventTouchUpInside];
  [[_tab closeButton]
      addAccessibilityElementFocusedTarget:self
                                    action:@selector(elementDidBecomeFocused:)];
  [_tab closeButton].accessibilityIdentifier = [self closeButtonId];

  [[_tab titleLabel]
      addAccessibilityElementFocusedTarget:self
                                    action:@selector(elementDidBecomeFocused:)];

  [self addSubview:_tab];

  self.accessibilityIdentifier = @"Tab";
  self.isAccessibilityElement = NO;
  self.accessibilityElementsHidden = NO;

  return self;
}

- (instancetype)initWithCoder:(NSCoder*)aDecoder {
  NOTREACHED();
  return nil;
}

- (void)setImage:(UIImage*)image {
  [_contents setImage:image];
  [self updateImageBoundsAndZoom];
  [self updateSnapshotAnimations];
}

- (UIImage*)image {
  return [_contents image];
}

- (void)setShouldShowShadow:(BOOL)shouldShowShadow {
  if (_shouldShowShadow != shouldShowShadow) {
    _shouldShowShadow = shouldShowShadow;
    [_frameShadowImageView setHidden:!_shouldShowShadow];
    if (_shouldShowShadow)
      [self updateShadowMask];
  }
}

- (void)setShouldMaskShadow:(BOOL)shouldMaskShadow {
  if (_shouldMaskShadow != shouldMaskShadow) {
    _shouldMaskShadow = shouldMaskShadow;
    if (self.shouldShowShadow)
      [self updateShadowMask];
  }
}

- (void)setCloseButtonSide:(CardCloseButtonSide)closeButtonSide {
  if ([_tab closeButtonSide] == closeButtonSide)
    return;
  [_tab setCloseButtonSide:closeButtonSide];
}

- (CardCloseButtonSide)closeButtonSide {
  return [_tab closeButtonSide];
}

- (void)setTitle:(NSString*)title {
  [_tab setTitle:title];
}

- (TitleLabel*)titleLabel {
  return [_tab titleLabel];
}

- (void)setFavicon:(UIImage*)favicon {
  [_tab setFavicon:favicon];
}

- (void)closeButtonWasTapped:(id)sender {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  [_cardCloseTarget performSelector:_cardCloseAction withObject:self];
#pragma clang diagnostic pop
  // Disable the tab's close button to prevent touch handling from the button
  // while it's animating closed.
  [_tab closeButton].enabled = NO;
}

- (void)addCardCloseTarget:(id)target action:(SEL)action {
  DCHECK(!target || [target respondsToSelector:action]);
  _cardCloseTarget = target;
  _cardCloseAction = action;
}

- (CGRect)closeButtonFrame {
  CGRect frame = [_tab closeButtonRect];
  return [self convertRect:frame fromView:_tab];
}

- (NSString*)closeButtonId {
  return [NSString stringWithFormat:@"%p", [_tab closeButton]];
}

- (CGRect)imageContentsRectForCardSize:(CGSize)cardSize {
  CGRect cardBounds = {CGPointZero, cardSize};
  CGSize viewSize = UIEdgeInsetsInsetRect(cardBounds, kCardImageInsets).size;
  CGSize imageSize = [_contents image].size;
  CGFloat zoomRatio = std::max(viewSize.height / imageSize.height,
                               viewSize.width / imageSize.width);
  return CGRectMake(0.0, 0.0, viewSize.width / (zoomRatio * imageSize.width),
                    viewSize.height / (zoomRatio * imageSize.height));
}

- (void)updateImageBoundsAndZoom {
  UIImageView* imageView = _contents;
  DCHECK(!CGRectEqualToRect(self.bounds, CGRectZero));

  imageView.frame = UIEdgeInsetsInsetRect(self.bounds, kCardImageInsets);
  if (imageView.image) {
    // Zoom the image to fill the available space.
    imageView.layer.contentsRect =
        [self imageContentsRectForCardSize:self.bounds.size];
  }
}

- (void)updateSnapshotAnimations {
  CAAnimation* snapshotAnimation =
      [[_contents layer] animationForKey:kCardViewAnimationKey];
  if (!snapshotAnimation)
    return;

  // Create copy of animation (animations become immutable after they're added
  // to the layer).
  CAAnimationGroup* updatedAnimation = [snapshotAnimation copy];
  // Extract begin and end sizes of the card.
  CAAnimation* cardAnimation =
      [self.layer animationForKey:kCardViewAnimationKey];
  CABasicAnimation* cardBoundsAnimation =
      FindAnimationForKeyPath(@"bounds", cardAnimation);
  CGSize beginCardSize = [cardBoundsAnimation.fromValue CGRectValue].size;
  CGSize endCardSize = [cardBoundsAnimation.toValue CGRectValue].size;
  // Update the contentsRect animation.
  CABasicAnimation* contentsRectAnimation =
      FindAnimationForKeyPath(@"contentsRect", updatedAnimation);
  contentsRectAnimation.fromValue = [NSValue
      valueWithCGRect:[self imageContentsRectForCardSize:beginCardSize]];
  contentsRectAnimation.toValue =
      [NSValue valueWithCGRect:[self imageContentsRectForCardSize:endCardSize]];
  // Replace with updated animation.
  [[_contents layer] removeAnimationForKey:kCardViewAnimationKey];
  [[_contents layer] addAnimation:updatedAnimation
                           forKey:kCardViewAnimationKey];
}

- (CGRect)shadowMaskFrameForBounds:(CGRect)bounds {
  CGRect shadowFrame = UIEdgeInsetsInsetRect(bounds, kCardShadowLayoutOutsets);
  LayoutRect maskLayout = LayoutRectZero;
  maskLayout.size = shadowFrame.size;
  maskLayout.boundingWidth = maskLayout.size.width;
  if (IsPortrait()) {
    maskLayout.position.leading =
        -UIEdgeInsetsGetLeading(kCardShadowLayoutOutsets);
    maskLayout.size.width = CGRectGetWidth(bounds);
    maskLayout.size.height =
        kCardFrameCornerRadius + kCardFrameInset - kCardShadowLayoutOutsets.top;
  } else {
    maskLayout.position.originY = -kCardShadowLayoutOutsets.top;
    maskLayout.size.width = kCardFrameCornerRadius + kCardFrameInset -
                            UIEdgeInsetsGetLeading(kCardShadowLayoutOutsets);
    maskLayout.size.height = CGRectGetHeight(bounds);
  }
  return LayoutRectGetRect(maskLayout);
}

- (void)updateShadowMask {
  if (!self.shouldShowShadow)
    return;

  if (self.shouldMaskShadow) {
    if (!_shadowMask) {
      _shadowMask = [[CALayer alloc] init];
      [_shadowMask setBackgroundColor:[UIColor blackColor].CGColor];
    }
    [_frameShadowImageView layer].mask = _shadowMask;
    [_shadowMask setFrame:[self shadowMaskFrameForBounds:self.bounds]];
  } else {
    [_frameShadowImageView layer].mask = nil;
  }
}

- (LayoutRect)tabLayout {
  LayoutRect tabLayout;
  tabLayout.position.leading = kCardFrameInset;
  tabLayout.position.originY = kCardTabTopInset;
  tabLayout.boundingWidth = CGRectGetWidth(self.bounds);
  tabLayout.size.width = tabLayout.boundingWidth - 2.0 * kCardFrameInset;
  tabLayout.size.height = kCardImageInsets.top - kCardTabTopInset;
  return tabLayout;
}

- (void)layoutSubviews {
  [_tab setFrame:LayoutRectGetRect([self tabLayout])];

  [_tab setFrame:LayoutRectGetRect([self tabLayout])];
  [_frameLeft setFrame:[self frameLeftFrameForBounds:self.bounds]];
  [_frameRight setFrame:[self frameRightFrameForBounds:self.bounds]];
  [_frameTop setFrame:[self frameTopFrameForBounds:self.bounds]];
  [_frameBottom setFrame:[self frameBottomFrameForBounds:self.bounds]];

  [self updateImageBoundsAndZoom];
  [self updateShadowMask];
}

- (CGRect)frameLeftFrameForBounds:(CGRect)bounds {
  return AlignRectToPixel(CGRectMake(
      bounds.origin.x, bounds.origin.y + kCardImageInsets.top,
      [_frameLeft image].size.width,
      bounds.size.height - kCardImageInsets.top - kCardImageInsets.bottom));
}

- (CGRect)frameRightFrameForBounds:(CGRect)bounds {
  CGSize size = ui::AlignSizeToUpperPixel(CGSizeMake(
      [_frameRight image].size.width,
      bounds.size.height - kCardImageInsets.top - kCardImageInsets.bottom));
  CGFloat rightEdge = CGRectGetMaxX([self frameTopFrameForBounds:bounds]);
  CGPoint origin = CGPointMake(rightEdge - size.width,
                               bounds.origin.y + kCardImageInsets.top);
  return {origin, size};
}

- (CGRect)frameTopFrameForBounds:(CGRect)bounds {
  return AlignRectToPixel(CGRectMake(bounds.origin.x, bounds.origin.y,
                                     bounds.size.width,
                                     [_frameTop image].size.height));
}

- (CGRect)frameBottomFrameForBounds:(CGRect)bounds {
  CGFloat imageHeight = [_frameBottom image].size.height;
  return AlignRectToPixel(CGRectMake(bounds.origin.x,
                                     CGRectGetMaxY(bounds) - imageHeight,
                                     bounds.size.width, imageHeight));
}

- (void)setTabOpacity:(CGFloat)opacity {
  [_tab setAlpha:opacity];
}

- (NSString*)accessibilityValue {
  return self.isActiveTab ? @"active" : @"inactive";
}

// Accounts for the fact that the tab's close button can extend beyond the
// bounds of the card.
- (BOOL)pointInside:(CGPoint)point withEvent:(UIEvent*)event {
  if ([super pointInside:point withEvent:event])
    return YES;
  CGPoint convertedPoint = [self convertPoint:point toView:_tab];
  if ([_tab pointInside:convertedPoint withEvent:event])
    return YES;
  return NO;
}

- (void)installDummyToolbarBackgroundView:(UIView*)dummyToolbarBackgroundView {
  [_tab insertSubview:dummyToolbarBackgroundView atIndex:0];
}

- (CGRect)dummyToolbarFrameForRect:(CGRect)rect inView:(UIView*)view {
  return [_tab convertRect:rect fromView:view];
}

- (void)animateOutFrameImageOverlap {
  // Calculate end frame for image.  Applying the inverse of |kCardImageInsets|
  // on the content snapshot's frame results in an overlap of
  // |kCardFrameImageSnapshotOverlap|, so this needs to be included in the
  // outsets.
  CGRect contentFrame = [[_contents layer].presentationLayer frame];
  UIEdgeInsets endFrameOutsets = UIEdgeInsetsMake(
      -(kCardFrameImageSnapshotOverlap + kCardImageInsets.top),
      -(kCardFrameImageSnapshotOverlap + kCardImageInsets.left),
      -(kCardFrameImageSnapshotOverlap + kCardImageInsets.bottom),
      -(kCardFrameImageSnapshotOverlap + kCardImageInsets.right));
  CGRect endBounds = UIEdgeInsetsInsetRect(contentFrame, endFrameOutsets);

  // Remove old frame animation and add new overlap animation.
  CALayer* frameLayer = [_frameLeft layer];
  CGRect beginFrame = [frameLayer.presentationLayer frame];
  CGRect endFrame = [self frameLeftFrameForBounds:endBounds];
  [frameLayer removeAnimationForKey:kCardViewAnimationKey];
  CAAnimation* frameAnimation =
      FrameAnimationMake(frameLayer, beginFrame, endFrame);
  frameAnimation.duration = ios::material::kDuration2;
  [frameLayer addAnimation:frameAnimation forKey:kCardViewAnimationKey];

  frameLayer = [_frameRight layer];
  beginFrame = [frameLayer.presentationLayer frame];
  endFrame = [self frameRightFrameForBounds:endBounds];
  [frameLayer removeAnimationForKey:kCardViewAnimationKey];
  frameAnimation = FrameAnimationMake(frameLayer, beginFrame, endFrame);
  frameAnimation.duration = ios::material::kDuration2;
  [frameLayer addAnimation:frameAnimation forKey:kCardViewAnimationKey];

  frameLayer = [_frameTop layer];
  beginFrame = [frameLayer.presentationLayer frame];
  endFrame = [self frameTopFrameForBounds:endBounds];
  [frameLayer removeAnimationForKey:kCardViewAnimationKey];
  frameAnimation = FrameAnimationMake(frameLayer, beginFrame, endFrame);
  frameAnimation.duration = ios::material::kDuration2;
  [frameLayer addAnimation:frameAnimation forKey:kCardViewAnimationKey];

  frameLayer = [_frameBottom layer];
  beginFrame = [frameLayer.presentationLayer frame];
  endFrame = [self frameBottomFrameForBounds:endBounds];
  [frameLayer removeAnimationForKey:kCardViewAnimationKey];
  frameAnimation = FrameAnimationMake(frameLayer, beginFrame, endFrame);
  frameAnimation.duration = ios::material::kDuration2;
  [frameLayer addAnimation:frameAnimation forKey:kCardViewAnimationKey];
}

- (void)animateFromBeginFrame:(CGRect)beginFrame
                   toEndFrame:(CGRect)endFrame
            tabAnimationStyle:(CardTabAnimationStyle)tabAnimationStyle {
  // Animation values
  CAAnimation* frameAnimation = nil;
  CFTimeInterval frameDuration = ios::material::kDuration1;
  CAMediaTimingFunction* frameTiming =
      TimingFunction(ios::material::CurveEaseInOut);

  // Update layer geometry
  frameAnimation = FrameAnimationMake(self.layer, beginFrame, endFrame);
  frameAnimation.duration = frameDuration;
  frameAnimation.timingFunction = frameTiming;
  [self.layer addAnimation:frameAnimation forKey:kCardViewAnimationKey];

  // Update frame image.  If the tab is being faded out, finish the frame
  // image's animation by animating out the overlap.
  BOOL shouldAnimateOverlap =
      tabAnimationStyle == CARD_TAB_ANIMATION_STYLE_FADE_OUT;
  if (shouldAnimateOverlap) {
    [CATransaction begin];
    [CATransaction setCompletionBlock:^(void) {
      [self animateOutFrameImageOverlap];
    }];
  }
  CGRect beginBounds = {CGPointZero, beginFrame.size};
  CGRect endBounds = {CGPointZero, endFrame.size};
  frameAnimation = FrameAnimationMake(
      [_frameLeft layer], [self frameLeftFrameForBounds:beginBounds],
      [self frameLeftFrameForBounds:endBounds]);
  frameAnimation.duration = frameDuration;
  frameAnimation.timingFunction = frameTiming;
  [[_frameLeft layer] addAnimation:frameAnimation forKey:kCardViewAnimationKey];
  frameAnimation = FrameAnimationMake(
      [_frameRight layer], [self frameRightFrameForBounds:beginBounds],
      [self frameRightFrameForBounds:endBounds]);
  frameAnimation.duration = frameDuration;
  frameAnimation.timingFunction = frameTiming;
  [[_frameRight layer] addAnimation:frameAnimation
                             forKey:kCardViewAnimationKey];
  frameAnimation = FrameAnimationMake([_frameTop layer],
                                      [self frameTopFrameForBounds:beginBounds],
                                      [self frameTopFrameForBounds:endBounds]);
  frameAnimation.duration = frameDuration;
  frameAnimation.timingFunction = frameTiming;
  [[_frameTop layer] addAnimation:frameAnimation forKey:kCardViewAnimationKey];
  frameAnimation = FrameAnimationMake(
      [_frameBottom layer], [self frameBottomFrameForBounds:beginBounds],
      [self frameBottomFrameForBounds:endBounds]);
  frameAnimation.duration = frameDuration;
  frameAnimation.timingFunction = frameTiming;
  [[_frameBottom layer] addAnimation:frameAnimation
                              forKey:kCardViewAnimationKey];
  if (shouldAnimateOverlap)
    [CATransaction commit];

  // Update frame shadow and its mask
  if (self.shouldShowShadow) {
    frameAnimation = FrameAnimationMake(
        [_frameShadowImageView layer],
        UIEdgeInsetsInsetRect(beginBounds, kCardShadowLayoutOutsets),
        UIEdgeInsetsInsetRect(endBounds, kCardShadowLayoutOutsets));
    frameAnimation.duration = frameDuration;
    frameAnimation.timingFunction = frameTiming;
    [[_frameShadowImageView layer] addAnimation:frameAnimation
                                         forKey:kCardViewAnimationKey];
    if (self.shouldMaskShadow) {
      frameAnimation = FrameAnimationMake(
          _shadowMask, [self shadowMaskFrameForBounds:beginBounds],
          [self shadowMaskFrameForBounds:endBounds]);
      frameAnimation.duration = frameDuration;
      frameAnimation.timingFunction = frameTiming;
      [_shadowMask addAnimation:frameAnimation forKey:kCardViewAnimationKey];
    }
  }

  // Update content snapshot
  CGRect beginContentFrame =
      UIEdgeInsetsInsetRect(beginBounds, kCardImageInsets);
  CGRect endContentFrame = UIEdgeInsetsInsetRect(endBounds, kCardImageInsets);
  frameAnimation =
      FrameAnimationMake([_contents layer], beginContentFrame, endContentFrame);
  frameAnimation.duration = frameDuration;
  frameAnimation.timingFunction = frameTiming;
  CABasicAnimation* contentsRectAnimation =
      [CABasicAnimation animationWithKeyPath:@"contentsRect"];
  CGRect beginContentsRect =
      [self imageContentsRectForCardSize:beginBounds.size];
  contentsRectAnimation.fromValue = [NSValue valueWithCGRect:beginContentsRect];
  CGRect endContentsRect = [self imageContentsRectForCardSize:endBounds.size];
  contentsRectAnimation.toValue = [NSValue valueWithCGRect:endContentsRect];
  contentsRectAnimation.duration = frameDuration;
  contentsRectAnimation.timingFunction = frameTiming;
  CAAnimation* imageAnimation =
      AnimationGroupMake(@[ frameAnimation, contentsRectAnimation ]);
  [[_contents layer] addAnimation:imageAnimation forKey:kCardViewAnimationKey];

  // Update tab view
  CGPoint tabOrigin = CGPointMake(kCardFrameInset, kCardTabTopInset);
  CGSize beginTabSize =
      CGSizeMake(beginFrame.size.width - 2.0 * kCardFrameInset,
                 kCardImageInsets.top - kCardTabTopInset);
  CGSize endTabSize = CGSizeMake(endFrame.size.width - 2.0 * kCardFrameInset,
                                 kCardImageInsets.top - kCardTabTopInset);
  [_tab animateFromBeginFrame:{ tabOrigin, beginTabSize }
      toEndFrame:{ tabOrigin, endTabSize }
      tabAnimationStyle:tabAnimationStyle];
}

- (void)removeFrameAnimation {
  [self.layer removeAnimationForKey:kCardViewAnimationKey];
}

- (void)reverseAnimations {
  [_tab reverseAnimations];
  ReverseAnimationsForKeyForLayers(kCardViewAnimationKey, @[
    self.layer, [_frameShadowImageView layer], _shadowMask, [_frameLeft layer],
    [_frameRight layer], [_frameTop layer], [_frameBottom layer],
    [_contents layer]
  ]);
}

- (void)cleanUpAnimations {
  [_tab cleanUpAnimations];
  RemoveAnimationForKeyFromLayers(kCardViewAnimationKey, @[
    self.layer, [_frameShadowImageView layer], _shadowMask, [_frameLeft layer],
    [_frameRight layer], [_frameTop layer], [_frameBottom layer],
    [_contents layer]
  ]);
}

#pragma mark - Accessibility Methods

- (void)postAccessibilityNotification {
  UIAccessibilityPostNotification(UIAccessibilityScreenChangedNotification,
                                  [_tab titleLabel]);
}

- (void)addAccessibilityTarget:(id)target action:(SEL)action {
  DCHECK(!target || [target respondsToSelector:action]);
  _accessibilityTarget = target;
  _accessibilityAction = action;
}

- (void)elementDidBecomeFocused:(id)sender {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  [_accessibilityTarget performSelector:_accessibilityAction withObject:sender];
#pragma clang diagnostic pop
}

@end
