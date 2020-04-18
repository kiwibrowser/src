// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_STACK_VIEW_CARD_VIEW_H_
#define IOS_CHROME_BROWSER_UI_STACK_VIEW_CARD_VIEW_H_

#import <UIKit/UIKit.h>

struct LayoutRect;

// The insets of the snapshot image within the card.
extern const UIEdgeInsets kCardImageInsets;
// Inset from the edge of the card view to the edge of the card frame image.
extern const CGFloat kCardFrameInset;
// Thickness of the shadow cast by a card view.
extern const CGFloat kCardShadowThickness;
// The corner radius of the card frame image.
extern const CGFloat kCardFrameCornerRadius;
// The inset from the top of the card to the beginning of its tab view.
extern const CGFloat kCardTabTopInset;
// The brightness values for the background colors of the card frame images.
extern const CGFloat kCardFrameBackgroundBrightness;
extern const CGFloat kCardFrameBackgroundBrightnessIncognito;
// The amount by which the card frame image overlaps the content snapshot.
extern const CGFloat kCardFrameImageSnapshotOverlap;
// The name of the shadow image used for the card views.
extern NSString* const kCardShadowImageName;
// The outsets that will line the edge of a view with the shadow image.
extern const UIEdgeInsets kCardShadowLayoutOutsets;

// Enum class describing on which side to draw the close button.
enum class CardCloseButtonSide : short { LEADING = 0, TRAILING };

typedef enum {
  CARD_TAB_ANIMATION_STYLE_NONE = 0,
  CARD_TAB_ANIMATION_STYLE_FADE_IN,
  CARD_TAB_ANIMATION_STYLE_FADE_OUT
} CardTabAnimationStyle;

@class CardSnapshotView;
@class CardTabView;
@class TitleLabel;

// A view class for displaying a card in the tab-switching stack. Has a title
// and an image. On its target, has actions for when the close box is clicked,
// when the card is selected, when it's dragged, and when VoiceOver focuses on
// its title label or close button.
@interface CardView : UIView

// |YES| if this card represents the current active tab.
@property(nonatomic, assign) BOOL isActiveTab;

// The snapshot displayed on the card.
@property(nonatomic, strong) UIImage* image;

// Whether the card view should render its shadow.
@property(nonatomic, assign) BOOL shouldShowShadow;

// Whether the card view should mask its shadow to only the overlapping portion.
@property(nonatomic, assign) BOOL shouldMaskShadow;

// The side on which to draw the close button.
@property(nonatomic, assign) CardCloseButtonSide closeButtonSide;

// Initializes CardView with |frame| and |isIncognito| state.
- (instancetype)initWithFrame:(CGRect)frame
                  isIncognito:(BOOL)isIncognito NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithCoder:(NSCoder*)aDecoder NS_UNAVAILABLE;

// Sets the page title. Can be nil.
- (void)setTitle:(NSString*)title;

// Returns TitleLabel element.
- (TitleLabel*)titleLabel;

// Sets the favicon. Can be nil, in which case the default favicon will be
// displayed.
- (void)setFavicon:(UIImage*)favicon;

// Sets the opacity of the tab. Uses opacity instead of hidden so that it's
// animatable.
- (void)setTabOpacity:(CGFloat)opacity;

// Sets target-action that is called when the close button is tapped. |target|
// can be nil and is not retained.
- (void)addCardCloseTarget:(id)target action:(SEL)action;

// Returns the touch target area of the button used to close the card.
- (CGRect)closeButtonFrame;

// Sets target-action that is called when VoiceOver focuses on the card view.
// |target| can be nil and is not retained.
- (void)addAccessibilityTarget:(id)target action:(SEL)action;

// Posts accessibility notification in VoiceOver to indicate when screen
// has changed and move cursor to CardView's titleLabel.
- (void)postAccessibilityNotification;

// Animates from |beginFrame| to |endFrame|, animating the card's tab as
// specified by |tabAnimationStyle|:
//   - CardTabAnimationStyleNone: Animate frames of tab subviews
//   - CardTabAnimationStyleFadeIn: Animate frames and fade in tab subviews
//   - CardTabAnimationStyleFadeOut: Aniamte frames and fade out tab subviews
- (void)animateFromBeginFrame:(CGRect)beginFrame
                   toEndFrame:(CGRect)endFrame
            tabAnimationStyle:(CardTabAnimationStyle)tabAnimationStyle;

// Removes the top-level frame animation added by
// |-animateFromBeginFrame:toEndFrame:tabAnimationStyle:|.  This is necessary to
// avoid animation glitches that occur when a card is closed mid-animation, as
// page_animation_util::AnimateOutWithCompletion updates the CardView's
// anchorPoint.
- (void)removeFrameAnimation;

// Adds the dummy toolbar background view to the back of the card tab view.
- (void)installDummyToolbarBackgroundView:(UIView*)dummyToolbarBackgroundView;

// Reverses animations added by
// |-animateFromBeginFrame:toEndFrame:tabAnimationStyle:|.
- (void)reverseAnimations;

// Removes animations added by
// |-animateFromBeginFrame:toEndFrame:tabAnimationStyle:|.
- (void)cleanUpAnimations;

@end

@interface CardView (ExposedForTesting)

// The LayoutRect for CardTabView.
- (LayoutRect)tabLayout;

// The a11y ID of the "close" button in the find-in-page bar.
@property(nonatomic, readonly) NSString* closeButtonId;

@end

#endif  // IOS_CHROME_BROWSER_UI_STACK_VIEW_CARD_VIEW_H_
