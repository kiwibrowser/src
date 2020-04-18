// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_MANAGER_H_
#define IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_MANAGER_H_

#import <UIKit/UIKit.h>

// Names identifying the errors used in Payment Request API:
// https://www.w3.org/TR/payment-request/
// A complete list of DOMException error names and descriptions can be found at:
// https://developer.mozilla.org/en-US/docs/Web/API/DOMException#Error_names
extern NSString* const kAbortError;
extern NSString* const kInvalidStateError;
extern NSString* const kNotAllowedError;
extern NSString* const kNotSupportedError;

@protocol ApplicationCommands;
class ToolbarModel;

namespace ios {
class ChromeBrowserState;
}  // namespace ios

namespace web {
class WebState;
}  // namespace web

// Manager for handling invocations of the Payment Request API.
// Implements the app-side of the Payment Request JavaScript API. Injects and
// listens to the injected JavaScript and invokes the creation of the user
// interface.
@interface PaymentRequestManager : NSObject

// ToolbarModel that is used for grabbing security info.
@property(nonatomic, assign) ToolbarModel* toolbarModel;

// The WebState being observed for invocations of the Payment Request API.
// Should outlive this instance. May be nullptr.
@property(nonatomic, assign) web::WebState* activeWebState;

// Designated initializer.
- (instancetype)initWithBaseViewController:(UIViewController*)viewController
                              browserState:
                                  (ios::ChromeBrowserState*)browserState
                                dispatcher:(id<ApplicationCommands>)dispatcher
    NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

// Stops tracking instances of payments::PaymentRequest for |webState|. This is
// called before the tab |webState| is associated with is removed.
- (void)stopTrackingWebState:(web::WebState*)webState;

// Enables or disables the Payment Request API for the active webState.
- (void)enablePaymentRequest:(BOOL)enabled;

// If there is a pending request, cancels it and dismisses the UI. This must be
// called if the pending request has to be terminated, e.g., when the active
// webState changes.
- (void)cancelRequest;

// Destroys the receiver. Any following call is not supported.
- (void)close;

@end

#endif  // IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_MANAGER_H_
