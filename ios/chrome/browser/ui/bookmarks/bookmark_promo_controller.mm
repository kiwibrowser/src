// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/bookmarks/bookmark_promo_controller.h"

#include <memory>

#include "components/signin/core/browser/signin_manager.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/signin/signin_manager_factory.h"
#import "ios/chrome/browser/ui/authentication/signin_promo_view_configurator.h"
#import "ios/chrome/browser/ui/authentication/signin_promo_view_consumer.h"
#import "ios/chrome/browser/ui/authentication/signin_promo_view_mediator.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
class SignInObserver;
}  // namespace

@interface BookmarkPromoController ()<SigninPromoViewConsumer> {
  bool _isIncognito;
  ios::ChromeBrowserState* _browserState;
  std::unique_ptr<SignInObserver> _signinObserver;
}

// Mediator to use for the sign-in promo view displayed in the bookmark view.
@property(nonatomic, readwrite, strong)
    SigninPromoViewMediator* signinPromoViewMediator;

// SignInObserver Callbacks

// Called when a user signs into Google services such as sync.
- (void)googleSigninSucceededWithAccountId:(const std::string&)account_id
                                  username:(const std::string&)username;

// Called when the currently signed-in user for a user has been signed out.
- (void)googleSignedOutWithAcountId:(const std::string&)account_id
                           username:(const std::string&)username;

@end

namespace {
class SignInObserver : public SigninManagerBase::Observer {
 public:
  SignInObserver(BookmarkPromoController* controller)
      : controller_(controller) {}

  void GoogleSigninSucceeded(const std::string& account_id,
                             const std::string& username) override {
    [controller_ googleSigninSucceededWithAccountId:account_id
                                           username:username];
  }

  void GoogleSignedOut(const std::string& account_id,
                       const std::string& username) override {
    [controller_ googleSignedOutWithAcountId:account_id username:username];
  }

 private:
  __weak BookmarkPromoController* controller_;
};
}  // namespace

@implementation BookmarkPromoController

@synthesize delegate = _delegate;
@synthesize shouldShowSigninPromo = _shouldShowSigninPromo;
@synthesize signinPromoViewMediator = _signinPromoViewMediator;

- (instancetype)initWithBrowserState:(ios::ChromeBrowserState*)browserState
                            delegate:
                                (id<BookmarkPromoControllerDelegate>)delegate
                           presenter:(id<SigninPresenter>)presenter {
  self = [super init];
  if (self) {
    _delegate = delegate;
    // Incognito browserState can go away before this class is released, this
    // code avoids keeping a pointer to it.
    _isIncognito = browserState->IsOffTheRecord();
    if (!_isIncognito) {
      _browserState = browserState;
      _signinObserver.reset(new SignInObserver(self));
      SigninManager* signinManager =
          ios::SigninManagerFactory::GetForBrowserState(_browserState);
      signinManager->AddObserver(_signinObserver.get());
      _signinPromoViewMediator = [[SigninPromoViewMediator alloc]
          initWithBrowserState:_browserState
                   accessPoint:signin_metrics::AccessPoint::
                                   ACCESS_POINT_BOOKMARK_MANAGER
                     presenter:presenter];
      _signinPromoViewMediator.consumer = self;
    }
    [self updateShouldShowSigninPromo];
  }
  return self;
}

- (void)dealloc {
  [_signinPromoViewMediator signinPromoViewRemoved];
  if (!_isIncognito) {
    DCHECK(_browserState);
    SigninManager* signinManager =
        ios::SigninManagerFactory::GetForBrowserState(_browserState);
    signinManager->RemoveObserver(_signinObserver.get());
  }
}

- (void)hidePromoCell {
  DCHECK(!_isIncognito);
  DCHECK(_browserState);
  self.shouldShowSigninPromo = NO;
}

- (void)setShouldShowSigninPromo:(BOOL)shouldShowSigninPromo {
  if (_shouldShowSigninPromo != shouldShowSigninPromo) {
    _shouldShowSigninPromo = shouldShowSigninPromo;
    [self.delegate promoStateChanged:shouldShowSigninPromo];
  }
}

- (void)updateShouldShowSigninPromo {
  self.shouldShowSigninPromo = NO;
  if (_isIncognito)
    return;

  DCHECK(_browserState);
  if ([SigninPromoViewMediator
          shouldDisplaySigninPromoViewWithAccessPoint:
              signin_metrics::AccessPoint::ACCESS_POINT_BOOKMARK_MANAGER
                                         browserState:_browserState]) {
    SigninManager* signinManager =
        ios::SigninManagerFactory::GetForBrowserState(_browserState);
    self.shouldShowSigninPromo = !signinManager->IsAuthenticated();
  }
}

#pragma mark - SignInObserver

// Called when a user signs into Google services such as sync.
- (void)googleSigninSucceededWithAccountId:(const std::string&)account_id
                                  username:(const std::string&)username {
  if (!self.signinPromoViewMediator.isSigninInProgress)
    self.shouldShowSigninPromo = NO;
}

// Called when the currently signed-in user for a user has been signed out.
- (void)googleSignedOutWithAcountId:(const std::string&)account_id
                           username:(const std::string&)username {
  [self updateShouldShowSigninPromo];
}

#pragma mark - SigninPromoViewConsumer

- (void)configureSigninPromoWithConfigurator:
            (SigninPromoViewConfigurator*)configurator
                             identityChanged:(BOOL)identityChanged {
  [self.delegate configureSigninPromoWithConfigurator:configurator
                                      identityChanged:identityChanged];
}

- (void)signinDidFinish {
  [self updateShouldShowSigninPromo];
}

- (void)signinPromoViewMediatorCloseButtonWasTapped:
    (SigninPromoViewMediator*)mediator {
  [self updateShouldShowSigninPromo];
}

@end
