// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_PUBLIC_PROVIDER_CHROME_BROWSER_SIGNIN_SIGNIN_ERROR_PROVIDER_H_
#define IOS_PUBLIC_PROVIDER_CHROME_BROWSER_SIGNIN_SIGNIN_ERROR_PROVIDER_H_

#include "base/macros.h"

@class NSError;
@class NSString;

namespace ios {

class SigninErrorProvider;

// Signin error categories.
// Can be used to figure out whether the error is because the user has their
// credentials revoked, deleted or disabled so the appropriate action can be
// taken.
enum class SigninErrorCategory {
  UNKNOWN_ERROR,
  AUTHORIZATION_ERROR,     // Should be handled by signing out the user.
  NETWORK_ERROR,           // Should be treated as transient/offline errors.
  USER_CANCELLATION_ERROR  // Should be treated as a no-op.
};

enum class SigninError {
  CANCELED,           // Operation canceled.
  MISSING_IDENTITY,   // Request is missing identity.
  HANDLED_INTERNALLY  // Has been displayed to the user already.
};

// Provides utility methods and constants for interpreting signin errors.
class SigninErrorProvider {
 public:
  SigninErrorProvider();
  virtual ~SigninErrorProvider();

  // Returns what family an error belongs to.
  virtual SigninErrorCategory GetErrorCategory(NSError* error);

  // Tests if an NSError is user cancellation error.
  virtual bool IsCanceled(NSError* error);

  // Tests if an NSError is an authorization forbidden error.
  virtual bool IsForbidden(NSError* error);

  // Tests if an NSError is a bad request error.
  virtual bool IsBadRequest(NSError* error);

  // Constant in JSON error responses to server requests indicating that
  // the authentication was revoked by the server.
  virtual NSString* GetInvalidGrantJsonErrorKey();

  // Gets the signin error domain.
  virtual NSString* GetSigninErrorDomain();

  // Gets the error code corresponding to |error|.
  virtual int GetCode(SigninError error);

 private:
  DISALLOW_COPY_AND_ASSIGN(SigninErrorProvider);
};

}  // namespace ios

#endif  // IOS_PUBLIC_PROVIDER_CHROME_BROWSER_SIGNIN_SIGNIN_ERROR_PROVIDER_H_
