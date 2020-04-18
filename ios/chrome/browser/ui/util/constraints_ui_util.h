// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_UTIL_CONSTRAINTS_UI_UTIL_H_
#define IOS_CHROME_BROWSER_UI_UTIL_CONSTRAINTS_UI_UTIL_H_

#import <UIKit/UIKit.h>

// A bitmask to refer to sides of a layout rectangle.
enum class LayoutSides {
  kTop = 1 << 0,
  kLeading = 1 << 1,
  kBottom = 1 << 2,
  kTrailing = 1 << 3,
};

// Implementation of bitwise "or", "and" operators (as those are not
// automatically defined for "class enum").
constexpr LayoutSides operator|(LayoutSides lhs, LayoutSides rhs) {
  return static_cast<LayoutSides>(
      static_cast<std::underlying_type<LayoutSides>::type>(lhs) |
      static_cast<std::underlying_type<LayoutSides>::type>(rhs));
};

constexpr LayoutSides operator&(LayoutSides lhs, LayoutSides rhs) {
  return static_cast<LayoutSides>(
      static_cast<std::underlying_type<LayoutSides>::type>(lhs) &
      static_cast<std::underlying_type<LayoutSides>::type>(rhs));
};

// Returns whether the |flag| is set in |mask|.
constexpr bool IsLayoutSidesMaskSet(LayoutSides mask, LayoutSides flag) {
  return (mask & flag) == flag;
}

// Same as NSDirectionalEdgeInsets but available on iOS 10.
struct ChromeDirectionalEdgeInsets {
  CGFloat top, leading, bottom, trailing;  // specify amount to inset (positive)
                                           // for each of the edges. values can
                                           // be negative to 'outset'
};

inline ChromeDirectionalEdgeInsets ChromeDirectionalEdgeInsetsMake(
    CGFloat top,
    CGFloat leading,
    CGFloat bottom,
    CGFloat trailing) {
  ChromeDirectionalEdgeInsets insets = {top, leading, bottom, trailing};
  return insets;
}

// Defines a protocol for common -...Anchor methods of UIView and UILayoutGuide.
@protocol LayoutGuideProvider<NSObject>
@property(nonatomic, readonly, strong) NSLayoutXAxisAnchor* leadingAnchor;
@property(nonatomic, readonly, strong) NSLayoutXAxisAnchor* trailingAnchor;
@property(nonatomic, readonly, strong) NSLayoutXAxisAnchor* leftAnchor;
@property(nonatomic, readonly, strong) NSLayoutXAxisAnchor* rightAnchor;
@property(nonatomic, readonly, strong) NSLayoutYAxisAnchor* topAnchor;
@property(nonatomic, readonly, strong) NSLayoutYAxisAnchor* bottomAnchor;
@property(nonatomic, readonly, strong) NSLayoutDimension* widthAnchor;
@property(nonatomic, readonly, strong) NSLayoutDimension* heightAnchor;
@property(nonatomic, readonly, strong) NSLayoutXAxisAnchor* centerXAnchor;
@property(nonatomic, readonly, strong) NSLayoutYAxisAnchor* centerYAnchor;
@end

// UIView already supports the methods in LayoutGuideProvider.
@interface UIView (LayoutGuideProvider)<LayoutGuideProvider>
@end

// UILayoutGuide already supports the methods in LayoutGuideProvider.
@interface UILayoutGuide (LayoutGuideProvider)<LayoutGuideProvider>
@end

#pragma mark - Visual constraints.

// Applies all |constraints| to views in |subviewsDictionary|.
void ApplyVisualConstraints(NSArray* constraints,
                            NSDictionary* subviewsDictionary);

// Applies all |constraints| with |metrics| to views in |subviewsDictionary|.
void ApplyVisualConstraintsWithMetrics(NSArray* constraints,
                                       NSDictionary* subviewsDictionary,
                                       NSDictionary* metrics);

// Applies all |constraints| with |metrics| and |options| to views in
// |subviewsDictionary|.
void ApplyVisualConstraintsWithMetricsAndOptions(
    NSArray* constraints,
    NSDictionary* subviewsDictionary,
    NSDictionary* metrics,
    NSLayoutFormatOptions options);

// Returns constraints based on the visual constraints described with
// |constraints| and |metrics| to views in |subviewsDictionary|.
NSArray* VisualConstraintsWithMetrics(NSArray* constraints,
                                      NSDictionary* subviewsDictionary,
                                      NSDictionary* metrics);

// Returns constraints based on the visual constraints described with
// |constraints|, |metrics| and |options| to views in |subviewsDictionary|.
NSArray* VisualConstraintsWithMetricsAndOptions(
    NSArray* constraints,
    NSDictionary* subviewsDictionary,
    NSDictionary* metrics,
    NSLayoutFormatOptions options);

#pragma mark - Constraints between two views.
// Most methods in this group can take a layout guide or a view.

// Adds a constraint that |view1| and |view2| are center-aligned horizontally
// and vertically.
void AddSameCenterConstraints(id<LayoutGuideProvider> view1,
                              id<LayoutGuideProvider> view2);

// Adds a constraint that |view1| and |view2| are center-aligned horizontally.
// |view1| and |view2| must be in the same view hierarchy.
void AddSameCenterXConstraint(id<LayoutGuideProvider> view1,
                              id<LayoutGuideProvider> view2);
// Deprecated version:
void AddSameCenterXConstraint(UIView* unused_parentView,
                              UIView* subview1,
                              UIView* subview2);

// Adds a constraint that |view1| and |view2| are center-aligned vertically.
// |view1| and |view2| must be in the same view hierarchy.
void AddSameCenterYConstraint(id<LayoutGuideProvider> view1,
                              id<LayoutGuideProvider> view2);
// Deprecated version:
void AddSameCenterYConstraint(UIView* unused_parentView,
                              UIView* subview1,
                              UIView* subview2);

// Adds constraints to make two views' size and center equal by pinning leading,
// trailing, top and bottom anchors.
void AddSameConstraints(id<LayoutGuideProvider> view1,
                        id<LayoutGuideProvider> view2);

// Adds constraints to make |innerView| leading, trailing, top and bottom
// anchors equals to |outerView| safe area (or view bounds) anchors.
void PinToSafeArea(id<LayoutGuideProvider> innerView, UIView* outerView);

// Constraints |side_flags| of |view1| and |view2| together.
// Example usage: AddSameConstraintsToSides(view1, view2,
// LayoutSides::kTop|LayoutSides::kLeading)
void AddSameConstraintsToSides(id<LayoutGuideProvider> view1,
                               id<LayoutGuideProvider> view2,
                               LayoutSides side_flags);

// Constraints |side_flags| sides of |innerView| and |outerView| together, with
// |innerView| inset by |insets|. Example usage:
// AddSameConstraintsToSidesWithInsets(view1, view2,
// LayoutSides::kTop|LayoutSides::kLeading, ChromeDirectionalEdgeInsets{10, 5,
// 10, 5}) - This will constraint innerView to be inside of outerView, with
// leading/trailing inset by 10 and top/bottom inset by 5.
// Edge insets for sides not listed in |side_flags| are ignored.
void AddSameConstraintsToSidesWithInsets(id<LayoutGuideProvider> innerView,
                                         id<LayoutGuideProvider> outerView,
                                         LayoutSides side_flags,
                                         ChromeDirectionalEdgeInsets insets);

#pragma mark - Safe Area.

// Returns a safeAreaLayoutGuide for a given view.
id<LayoutGuideProvider> SafeAreaLayoutGuideForView(UIView* view);

#endif  // IOS_CHROME_BROWSER_UI_UTIL_CONSTRAINTS_UI_UTIL_H_
