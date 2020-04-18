// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/elements/selector_picker_presentation_controller.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface SelectorPickerPresentationController ()
@property(nonatomic, strong) UIView* dimmingView;
@end

@implementation SelectorPickerPresentationController

@synthesize dimmingView = _dimmingView;

- (instancetype)initWithPresentedViewController:(UIViewController*)presented
                       presentingViewController:(UIViewController*)presenting {
  self = [super initWithPresentedViewController:presented
                       presentingViewController:presenting];
  if (self) {
    _dimmingView = [[UIView alloc] initWithFrame:CGRectZero];
    _dimmingView.backgroundColor = [UIColor colorWithWhite:0 alpha:0.4];
  }
  return self;
}

- (CGRect)frameOfPresentedViewInContainerView {
  CGSize containerSize = self.containerView.frame.size;
  CGSize pickerSize = [self.presentedView
      systemLayoutSizeFittingSize:UILayoutFittingCompressedSize];
  return CGRectMake(0, containerSize.height - pickerSize.height,
                    containerSize.width, pickerSize.height);
}

- (void)containerViewWillLayoutSubviews {
  self.dimmingView.frame = self.containerView.bounds;
  self.presentedView.frame = [self frameOfPresentedViewInContainerView];
}

- (void)presentationTransitionWillBegin {
  [self.containerView addSubview:self.dimmingView];
}

- (void)dismissalTransitionDidEnd:(BOOL)completed {
  [self.dimmingView removeFromSuperview];
}

@end
