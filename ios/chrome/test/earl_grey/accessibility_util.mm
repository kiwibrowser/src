// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <EarlGrey/EarlGrey.h>
#import <XCTest/XCTest.h>

#include "ios/chrome/test/earl_grey/accessibility_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Returns whether a UIView is hidden from the screen, based on the alpha
// and hidden properties of the UIView.
bool ViewIsHidden(UIView* view) {
  return view.alpha == 0 || view.hidden;
}

// Returns whether a view should be accessible. A view should be accessible if
// either it is a whitelisted type or its isAccessibilityElement property is
// enabled.
bool ViewShouldBeAccessible(UIView* view) {
  NSArray* classList = @[
    [UILabel class], [UIControl class], [UITableView class],
    [UICollectionViewCell class]
  ];
  bool viewIsAccessibleType = false;
  for (Class accessibleClass in classList) {
    if ([view isKindOfClass:accessibleClass]) {
      viewIsAccessibleType = true;
      break;
    }
  }
  return (viewIsAccessibleType || view.isAccessibilityElement) &&
         !ViewIsHidden(view);
}

// Returns true if |element| or descendant has property accessibilityLabel
// that is not an empty string. If |element| has isAccessibilityElement set to
// true, then the |element| itself must have a label.
bool ViewOrDescendantHasAccessibilityLabel(UIView* element) {
  if ([element.accessibilityLabel length])
    return true;
  if (element.isAccessibilityElement)
    return false;
  for (UIView* view in element.subviews) {
    if (ViewOrDescendantHasAccessibilityLabel(view))
      return true;
  }
  return false;
}

// Returns true if |element|'s accessibilityLabel is a not a bad default value,
// particularly that it does not match the name of an image in the bundle, since
// UIKit can set the accessibilityLabel to an associated image name if no other
// data is available.
bool ViewHasNonDefaultAccessibilityLabel(UIView* element) {
  if (element.accessibilityLabel) {
    // Replace all spaces with underscores because when UIKit converts an
    // image name to an accessibility label by default, it replaces underscores
    // with spaces.
    NSString* fileName =
        [element.accessibilityLabel stringByReplacingOccurrencesOfString:@" "
                                                              withString:@"_"];
    if ([fileName rangeOfString:@"/"].location != NSNotFound)
      return true;  // "/" is not a valid character in a file name
    if ([UIImage imageNamed:fileName])
      return false;
    if ([UIImage imageNamed:[fileName stringByAppendingString:@".jpg"]])
      return false;
    if ([UIImage imageNamed:[fileName stringByAppendingString:@".gif"]])
      return false;
  }
  return true;
}

// Returns an array of elements that should be accessible.
// Helper method for accessibilityElementsStartingFromView, so that
// |ancestorString|, which handles internal bookkeeping, is hidden from the top
// level API. |ancestorString| is the description of the most recent ancestor
// with isAccessibilityElement set to true, and thus when the method is first
// called, it should always be set to nil.
NSArray* AccessibilityElementsHelperStartingFromView(UIView* view,
                                                     NSString* ancestorString,
                                                     NSError** error) {
  NSMutableArray* results = [NSMutableArray array];
  // Add |view| to |results| if it should be accessible and is not hidden.
  if (ViewShouldBeAccessible(view)) {
    // UILabels have an extra check since some labels are dynamically set, and
    // may not have text or an accessibilityLabel at a given point of
    // execution.
    if ([view isKindOfClass:[UILabel class]]) {
      UILabel* label = static_cast<UILabel*>(view);
      if ([label.text length])
        [results addObject:label];
    } else {
      [results addObject:view];
      if ([ancestorString length]) {
        if (error != nil && !*error)
          *error = [NSError errorWithDomain:@"Ancestor blocks VoiceOver"
                                       code:1
                                   userInfo:nil];
        // The most recent ancestor with Accessibility Element set to true
        // blocks VoiceOver for Element ancestorString
      }
    }
  }
  if (view.isAccessibilityElement && ![view isKindOfClass:[UILabel class]]) {
    ancestorString = [view description];
  }
  if (![view isKindOfClass:[UITableView class]]) {
    // Do not recurse below views which are accessible but may have children
    // that default to being accessible. Also, do not recurse below views which
    // implement the UIAccessibilityContainer informal protocol, as these views
    // have taken ownership of accessibility behavior of descendents.
    if ([view isKindOfClass:[UISwitch class]] ||
        [view respondsToSelector:@selector(accessibilityElements)])
      return results;
    for (UIView* subView in [view subviews]) {
      [results addObjectsFromArray:AccessibilityElementsHelperStartingFromView(
                                       subView, ancestorString, error)];
    }
  }
  return results;
}

// Recursively traverses the UIView tree and returns all views that should be
// accessible. Views should be accessible if they are of type UIControl,
// UILabel, UITableViewCell, or UICollectionViewCell or if their
// isAccessibilityElement property is set to true. Also, it prints errors when
// an ancestor of an element which should be accessible has
// isAccessibilityElement set to true. It notifies the calling method of this
// error by assigning |error| to a new NSError object. By passing in an NSError
// object set to nil, it can be used to determine if there was an error.
NSArray* AccessibilityElementsStartingFromView(UIView* view, NSError** error) {
  return AccessibilityElementsHelperStartingFromView(view, nil, error);
}

// Starting from |view|, verifies that no view masks its descendants by having
// isAccessibilityElement set to true. |ancestorString| is the description of
// the most recent ancestor with isAccessibilityElement set to true, and thus
// when the method is first called, it should always be set to nil.
bool ViewAndDescendantsDoNotBlockVoiceOver(UIView* view,
                                           NSString* ancestorString) {
  if (![view isKindOfClass:[UILabel class]] && ViewShouldBeAccessible(view)) {
    if ([ancestorString length]) {
      // Ancestor String masks Descendant because the ancestor's
      // isAccessibilityElement is set to true.
      return false;
    }
    if (view.isAccessibilityElement) {
      ancestorString = [view description];
    }
  }
  // Do not recurse through hidden elements, as their descendants are also
  // hidden.
  if (ViewIsHidden(view))
    return true;
  // Do not recurse below views which are accessible but may have children
  // that default to being accessible. Also, do not recurse below views which
  // implement the UIAccessibilityContainer informal protocol, as these views
  // have taken ownership of accessibility behavior of descendents.
  if ([view isKindOfClass:[UISwitch class]] ||
      [view respondsToSelector:@selector(accessibilityElements)])
    return true;
  bool ancestorMasksDescendant = true;
  for (UIView* subView in [view subviews]) {
    if (!ViewAndDescendantsDoNotBlockVoiceOver(subView, ancestorString)) {
      ancestorMasksDescendant = false;
    }
  }
  return ancestorMasksDescendant;
}

// Run accessibilityLabel tests on |view|. This method is used for cases where
// tests are grouped by element, instead of the typical case where elements are
// grouped by test.
bool VerifyElementAccessibilityLabel(UIView* view) {
  if (view && !ViewIsHidden(view)) {
    if (!ViewOrDescendantHasAccessibilityLabel(view)) {
      // TODO: (crbug.com/650800) Add more verbose fail case logging.
      return false;
    }
    if (!ViewHasNonDefaultAccessibilityLabel(view)) {
      // TODO: (crbug.com/650800) Add more verbose fail case logging.
      return false;
    }
  }
  return true;
}

// Verifies |tableView|'s accessibility by scrolling through to ensure that
// accessibility tests are run on each cell. Cells which are offscreen may
// not be in the UIView hierarchy, so UITableViews must be scrolled in order to
// verify all of its cells. The method will scroll through the |tableView| no
// more than the number of times specified with the |kMaxTableViewScrolls|
// constant so that dynamically updated UITableViews do not scroll infinitely.
bool VerifyTableViewAccessibility(UITableView* tableView) {
  // Reload |tableView| in order to update its representation in the view
  // hierarchy, which can be stale.
  [tableView reloadData];
  [tableView layoutIfNeeded];
  bool hasRows = false;
  NSInteger numberOfSections = [tableView numberOfSections];
  for (NSInteger section = 0; section < numberOfSections; section++) {
    if ([tableView numberOfRowsInSection:section]) {
      hasRows = true;
      break;
    }
  }
  if (!hasRows) {
    return ViewAndDescendantsDoNotBlockVoiceOver(tableView, nil);
  }
  bool tableViewIsAccessible = true;
  NSIndexPath* prevIndexPath = nil;
  // Cell index path to scroll to on each iteration.
  NSIndexPath* nextIndexPath = [NSIndexPath indexPathForRow:0 inSection:0];
  bool hasMoreRows = true;
  // The maximum number of times that the test will scroll through a
  // UITableView.
  const NSUInteger kMaxTableViewScrolls = 1000;
  NSUInteger numberOfScrolls = 0;
  // Iterate until the tests have run on every cell in |tableView| or max number
  // of scrolls is reached.
  while (hasMoreRows && numberOfScrolls < kMaxTableViewScrolls) {
    [tableView scrollToRowAtIndexPath:nextIndexPath
                     atScrollPosition:UITableViewScrollPositionTop
                             animated:false];
    [tableView reloadData];
    [tableView layoutIfNeeded];
    if (!ViewAndDescendantsDoNotBlockVoiceOver(tableView, nil)) {
      tableViewIsAccessible = false;
      // TODO: (crbug.com/650800) Add more verbose fail case logging.
    }
    for (UITableViewCell* cell in tableView.visibleCells) {
      NSError* error = nil;
      NSArray* accessibleElements =
          AccessibilityElementsStartingFromView(cell, &error);
      for (UIView* view in accessibleElements) {
        if (!VerifyElementAccessibilityLabel(view))
          tableViewIsAccessible = false;
        // TODO: (crbug.com/650800) Add more verbose fail case logging.
      }
    }
    nextIndexPath =
        [tableView indexPathForCell:[tableView.visibleCells lastObject]];
    // If nextIndexPath is nil or it is greater than or equal to the last cell
    // in the |tableView|, end loop.
    if (!nextIndexPath ||
        (nextIndexPath.section >= tableView.numberOfSections - 1 &&
         nextIndexPath.row >=
             [tableView numberOfRowsInSection:nextIndexPath.section] - 1)) {
      hasMoreRows = false;
    }
    // If nextIndexPath is the same value as prev, which can happen if the
    // scrolling fails, set nextIndexPath to the next cell.
    if ([nextIndexPath isEqual:prevIndexPath]) {
      if (nextIndexPath.row ==
          [tableView numberOfRowsInSection:nextIndexPath.section] - 1) {
        nextIndexPath =
            [NSIndexPath indexPathForRow:0 inSection:nextIndexPath.section + 1];
      } else {
        nextIndexPath = [NSIndexPath indexPathForRow:nextIndexPath.row + 1
                                           inSection:nextIndexPath.section];
      }
    }
    prevIndexPath = nextIndexPath;
    numberOfScrolls++;
  }
  return tableViewIsAccessible;
}
}

namespace chrome_test_util {

void VerifyAccessibilityForCurrentScreen() {
  NSMutableArray* accessibilityElements = [NSMutableArray array];
  NSError* inaccessibleChildrenError = nil;
  // Checking for elements that are inaccessible because
  // they have an ancestor whose isAccessibilityElement is set to
  // true, blocking VoiceOver from reaching them...
  for (UIWindow* window in [[UIApplication sharedApplication] windows]) {
    // If window is UITextEffectsWindow or UIRemoteKeyboardWindow skip
    // accessibility check as this is likely a native keyboard.
    if (!([NSStringFromClass([window class])
            isEqualToString:@"UITextEffectsWindow"]) &&
        !([NSStringFromClass([window class])
            isEqualToString:@"UIRemoteKeyboardWindow"])) {
      NSArray* windowElements = AccessibilityElementsStartingFromView(
          window, &inaccessibleChildrenError);
      [accessibilityElements addObjectsFromArray:windowElements];
    }
  }

  // Special case UITableViews. Some elements on UITableViews are not in
  // the view tree, so we must scroll to ensure that each row in the
  // UITableView is visible when the accessibility tests are run. Also
  // removes all UITableViews from accessibilityElements to stop other
  // tests from running on the table.
  bool tableViewError = false;
  NSMutableIndexSet* tableViewsToBeRemoved = [NSMutableIndexSet indexSet];
  NSUInteger accessibilityIndex = 0;
  // Checking for TableView errors...
  for (UIView* view in accessibilityElements) {
    if ([view isKindOfClass:[UITableView class]]) {
      UITableView* table_view = static_cast<UITableView*>(view);
      if (!VerifyTableViewAccessibility(table_view)) {
        tableViewError = true;
      }
      [tableViewsToBeRemoved addIndex:accessibilityIndex];
    }
    accessibilityIndex++;
  }
  [accessibilityElements removeObjectsAtIndexes:tableViewsToBeRemoved];
  // Find all elements without labels and generate associated error
  // messages.
  bool noLabels = false;
  NSString* noLabelElementDesc = @"";
  // Checking for elements without labels...
  for (UIView* view in accessibilityElements) {
    if (!ViewOrDescendantHasAccessibilityLabel(view)) {
      [noLabelElementDesc
          stringByAppendingString:[NSString
                                      stringWithFormat:@"\n'%@'",
                                                       [view description]]];
      noLabels = true;
    }
  }
  // Find all elements which have set their accessibility labels to the
  // name of an associated image, and generate associated error
  // messages.
  bool badDefaultLabel = false;
  NSString* badDefaultLabelDesc = @"";
  // Checking for labels with default values...
  for (UIView* view in accessibilityElements) {
    if (!ViewHasNonDefaultAccessibilityLabel(view)) {
      [badDefaultLabelDesc
          stringByAppendingString:[NSString
                                      stringWithFormat:@"\n'%@'",
                                                       [view description]]];
      badDefaultLabel = true;
    }
  }

  GREYAssert(!inaccessibleChildrenError,
             @"The accessibility tests failed: Inaccessible children error");
  GREYAssert(!noLabels, @"The accessibility tests failed: No labels error");
  GREYAssert(!badDefaultLabel,
             @"The accessibility tests failed: Bad default labels error");
  GREYAssert(!tableViewError,
             @"The accessibility tests failed: Table view error");
}

}  // namespace chrome_test_util
