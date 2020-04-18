// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/alert_coordinator/action_sheet_coordinator.h"

#import "base/logging.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

enum class AnchorMode {
  VIEW,
  BAR_BUTTON_ITEM,
};

}  // namespace

@interface ActionSheetCoordinator () {
  // The anchor mode for the popover alert.
  AnchorMode _anchorMode;

  // Rectangle for the popover alert. Only used when |_anchorMode| is VIEW.
  CGRect _rect;
  // View for the popovert alert. Only used when |_anchorMode| is VIEW.
  UIView* _view;

  // Bar button item for the popover alert.  Only used when |_anchorMode| is
  // BAR_BUTTON_ITEM.
  UIBarButtonItem* _barButtonItem;
}

@end

@implementation ActionSheetCoordinator

- (instancetype)initWithBaseViewController:(UIViewController*)viewController
                                     title:(NSString*)title
                                   message:(NSString*)message {
  NOTREACHED();
  return nil;
}

- (instancetype)initWithBaseViewController:(UIViewController*)viewController
                                     title:(NSString*)title
                                   message:(NSString*)message
                                      rect:(CGRect)rect
                                      view:(UIView*)view {
  self = [super initWithBaseViewController:viewController
                                     title:title
                                   message:message];
  if (self) {
    _anchorMode = AnchorMode::VIEW;
    _rect = rect;
    _view = view;
  }
  return self;
}

- (instancetype)initWithBaseViewController:(UIViewController*)viewController
                                     title:(NSString*)title
                                   message:(NSString*)message
                             barButtonItem:(UIBarButtonItem*)barButtonItem {
  self = [super initWithBaseViewController:viewController
                                     title:title
                                   message:message];
  if (self) {
    _anchorMode = AnchorMode::BAR_BUTTON_ITEM;
    _barButtonItem = barButtonItem;
  }
  return self;
}

- (UIAlertController*)alertControllerWithTitle:(NSString*)title
                                       message:(NSString*)message {
  UIAlertController* alert = [UIAlertController
      alertControllerWithTitle:title
                       message:message
                preferredStyle:UIAlertControllerStyleActionSheet];

  switch (_anchorMode) {
    case AnchorMode::VIEW:
      alert.popoverPresentationController.sourceView = _view;
      alert.popoverPresentationController.sourceRect = _rect;
      break;
    case AnchorMode::BAR_BUTTON_ITEM:
      alert.popoverPresentationController.barButtonItem = _barButtonItem;
      break;
  }

  return alert;
}

@end
