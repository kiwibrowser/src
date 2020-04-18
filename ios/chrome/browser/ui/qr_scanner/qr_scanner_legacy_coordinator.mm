// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/qr_scanner/qr_scanner_legacy_coordinator.h"

#import "ios/chrome/browser/ui/commands/browser_commands.h"
#import "ios/chrome/browser/ui/commands/command_dispatcher.h"
#import "ios/chrome/browser/ui/qr_scanner/qr_scanner_view_controller.h"
#import "ios/chrome/browser/ui/qr_scanner/requirements/qr_scanner_presenting.h"
#import "ios/chrome/browser/ui/toolbar/public/omnibox_focuser.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface QRScannerLegacyCoordinator ()

@property(nonatomic, readwrite, strong) QRScannerViewController* viewController;

@end

@implementation QRScannerLegacyCoordinator

@synthesize dispatcher = _dispatcher;
@synthesize loadProvider = _loadProvider;
@synthesize presentationProvider = _presentationProvider;
@synthesize viewController = _viewController;

- (void)disconnect {
  self.dispatcher = nil;
}

- (void)setDispatcher:(CommandDispatcher*)dispatcher {
  if (dispatcher == self.dispatcher) {
    return;
  }

  if (self.dispatcher) {
    [self.dispatcher stopDispatchingToTarget:self];
  }

  [dispatcher startDispatchingToTarget:self
                           forSelector:@selector(showQRScanner)];
  _dispatcher = dispatcher;
}

- (void)showQRScanner {
  [static_cast<id<OmniboxFocuser>>(self.dispatcher) cancelOmniboxEdit];
  self.viewController = [[QRScannerViewController alloc]
      initWithPresentationProvider:self.presentationProvider
                      loadProvider:self.loadProvider];
  [self.presentationProvider
      presentQRScannerViewController:[self.viewController
                                             getViewControllerToPresent]];
}

@end
