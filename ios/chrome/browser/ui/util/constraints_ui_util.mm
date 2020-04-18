// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/util/constraints_ui_util.h"

#import <UIKit/UIKit.h>

#include "base/logging.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

void ApplyVisualConstraints(NSArray* constraints,
                            NSDictionary* subviewsDictionary) {
  ApplyVisualConstraintsWithMetricsAndOptions(constraints, subviewsDictionary,
                                              nil, 0);
}

void ApplyVisualConstraintsWithMetrics(NSArray* constraints,
                                       NSDictionary* subviewsDictionary,
                                       NSDictionary* metrics) {
  ApplyVisualConstraintsWithMetricsAndOptions(constraints, subviewsDictionary,
                                              metrics, 0);
}

void ApplyVisualConstraintsWithMetricsAndOptions(
    NSArray* constraints,
    NSDictionary* subviewsDictionary,
    NSDictionary* metrics,
    NSLayoutFormatOptions options) {
  NSArray* layoutConstraints = VisualConstraintsWithMetricsAndOptions(
      constraints, subviewsDictionary, metrics, options);
  [NSLayoutConstraint activateConstraints:layoutConstraints];
}

NSArray* VisualConstraintsWithMetrics(NSArray* constraints,
                                      NSDictionary* subviewsDictionary,
                                      NSDictionary* metrics) {
  return VisualConstraintsWithMetricsAndOptions(constraints, subviewsDictionary,
                                                metrics, 0);
}

NSArray* VisualConstraintsWithMetricsAndOptions(
    NSArray* constraints,
    NSDictionary* subviewsDictionary,
    NSDictionary* metrics,
    NSLayoutFormatOptions options) {
  NSMutableArray* layoutConstraints = [NSMutableArray array];
  for (NSString* constraint in constraints) {
    DCHECK([constraint isKindOfClass:[NSString class]]);
    [layoutConstraints addObjectsFromArray:
                           [NSLayoutConstraint
                               constraintsWithVisualFormat:constraint
                                                   options:options
                                                   metrics:metrics
                                                     views:subviewsDictionary]];
  }
  return [layoutConstraints copy];
}

void AddSameCenterConstraints(id<LayoutGuideProvider> view1,
                              id<LayoutGuideProvider> view2) {
  AddSameCenterXConstraint(view1, view2);
  AddSameCenterYConstraint(view1, view2);
}

void AddSameCenterXConstraint(id<LayoutGuideProvider> view1,
                              id<LayoutGuideProvider> view2) {
  [view1.centerXAnchor constraintEqualToAnchor:view2.centerXAnchor].active =
      YES;
}

void AddSameCenterXConstraint(UIView* unused_parentView,
                              UIView* subview1,
                              UIView* subview2) {
  AddSameCenterXConstraint(subview1, subview2);
}

void AddSameCenterYConstraint(id<LayoutGuideProvider> view1,
                              id<LayoutGuideProvider> view2) {
  [view1.centerYAnchor constraintEqualToAnchor:view2.centerYAnchor].active =
      YES;
}

void AddSameCenterYConstraint(UIView* unused_parentView,
                              UIView* subview1,
                              UIView* subview2) {
  AddSameCenterYConstraint(subview1, subview2);
}

void AddSameConstraints(id<LayoutGuideProvider> view1,
                        id<LayoutGuideProvider> view2) {
  [NSLayoutConstraint activateConstraints:@[
    [view1.leadingAnchor constraintEqualToAnchor:view2.leadingAnchor],
    [view1.trailingAnchor constraintEqualToAnchor:view2.trailingAnchor],
    [view1.topAnchor constraintEqualToAnchor:view2.topAnchor],
    [view1.bottomAnchor constraintEqualToAnchor:view2.bottomAnchor]
  ]];
}

void PinToSafeArea(id<LayoutGuideProvider> innerView, UIView* outerView) {
  id<LayoutGuideProvider> outerSafeAreaGuide =
      SafeAreaLayoutGuideForView(outerView);
  AddSameConstraints(innerView, outerSafeAreaGuide);
}

id<LayoutGuideProvider> SafeAreaLayoutGuideForView(UIView* view) {
  if (@available(iOS 11, *)) {
    return view.safeAreaLayoutGuide;
  } else {
    return view;
  }
}

void AddSameConstraintsToSides(id<LayoutGuideProvider> view1,
                               id<LayoutGuideProvider> view2,
                               LayoutSides side_flags) {
  AddSameConstraintsToSidesWithInsets(
      view1, view2, side_flags, ChromeDirectionalEdgeInsetsMake(0, 0, 0, 0));
}

void AddSameConstraintsToSidesWithInsets(id<LayoutGuideProvider> innerView,
                                         id<LayoutGuideProvider> outerView,
                                         LayoutSides side_flags,
                                         ChromeDirectionalEdgeInsets insets) {
  NSMutableArray* constraints = [[NSMutableArray alloc] init];
  if (IsLayoutSidesMaskSet(side_flags, LayoutSides::kTop)) {
    [constraints addObject:[innerView.topAnchor
                               constraintEqualToAnchor:outerView.topAnchor
                                              constant:insets.top]];
  }
  if (IsLayoutSidesMaskSet(side_flags, LayoutSides::kLeading)) {
    [constraints addObject:[innerView.leadingAnchor
                               constraintEqualToAnchor:outerView.leadingAnchor
                                              constant:insets.leading]];
  }
  if (IsLayoutSidesMaskSet(side_flags, LayoutSides::kBottom)) {
    [constraints addObject:[innerView.bottomAnchor
                               constraintEqualToAnchor:outerView.bottomAnchor
                                              constant:-insets.bottom]];
  }
  if (IsLayoutSidesMaskSet(side_flags, LayoutSides::kTrailing)) {
    [constraints addObject:[innerView.trailingAnchor
                               constraintEqualToAnchor:outerView.trailingAnchor
                                              constant:-insets.trailing]];
  }

  [NSLayoutConstraint activateConstraints:constraints];
}
