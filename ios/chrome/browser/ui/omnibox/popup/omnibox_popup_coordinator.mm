// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/omnibox/popup/omnibox_popup_coordinator.h"

#import "components/image_fetcher/ios/ios_image_data_fetcher_wrapper.h"
#include "components/omnibox/browser/autocomplete_result.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#import "ios/chrome/browser/ui/commands/command_dispatcher.h"
#import "ios/chrome/browser/ui/omnibox/popup/omnibox_popup_mediator.h"
#import "ios/chrome/browser/ui/omnibox/popup/omnibox_popup_presenter.h"
#import "ios/chrome/browser/ui/omnibox/popup/omnibox_popup_view_controller.h"
#include "ios/chrome/browser/ui/omnibox/popup/omnibox_popup_view_ios.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface OmniboxPopupCoordinator () {
  std::unique_ptr<OmniboxPopupViewIOS> _popupView;
}

@property(nonatomic, strong) OmniboxPopupViewController* popupViewController;
@property(nonatomic, strong) OmniboxPopupMediator* mediator;

@end

@implementation OmniboxPopupCoordinator

@synthesize browserState = _browserState;
@synthesize mediator = _mediator;
@synthesize popupViewController = _popupViewController;
@synthesize positioner = _positioner;
@synthesize dispatcher = _dispatcher;

#pragma mark - Public

- (instancetype)initWithPopupView:
    (std::unique_ptr<OmniboxPopupViewIOS>)popupView {
  self = [super init];
  if (self) {
    _popupView = std::move(popupView);
  }
  return self;
}

- (void)start {
  std::unique_ptr<image_fetcher::IOSImageDataFetcherWrapper> imageFetcher =
      std::make_unique<image_fetcher::IOSImageDataFetcherWrapper>(
          self.browserState->GetRequestContext());

  self.mediator =
      [[OmniboxPopupMediator alloc] initWithFetcher:std::move(imageFetcher)
                                           delegate:_popupView.get()];
  self.popupViewController = [[OmniboxPopupViewController alloc] init];
  self.popupViewController.incognito = self.browserState->IsOffTheRecord();

  self.mediator.incognito = self.browserState->IsOffTheRecord();
  self.mediator.consumer = self.popupViewController;
  self.mediator.presenter = [[OmniboxPopupPresenter alloc]
      initWithPopupPositioner:self.positioner
          popupViewController:self.popupViewController];

  self.popupViewController.imageRetriever = self.mediator;
  self.popupViewController.delegate = self.mediator;
  [self.dispatcher
      startDispatchingToTarget:self.popupViewController
                   forProtocol:@protocol(OmniboxSuggestionCommands)];

  _popupView->SetMediator(self.mediator);
}

- (void)stop {
  _popupView.reset();
  [self.dispatcher
      stopDispatchingForProtocol:@protocol(OmniboxSuggestionCommands)];
}

- (BOOL)isOpen {
  return self.mediator.isOpen;
}

#pragma mark - Property accessor

- (BOOL)hasResults {
  return self.mediator.hasResults;
}

@end
