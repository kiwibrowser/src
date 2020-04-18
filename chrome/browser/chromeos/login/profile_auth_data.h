// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_PROFILE_AUTH_DATA_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_PROFILE_AUTH_DATA_H_

#include "base/callback_forward.h"
#include "base/macros.h"

namespace net {
class URLRequestContextGetter;
}

namespace chromeos {

// Helper class that transfers authentication-related data from a BrowserContext
// used for authentication to the user's actual BrowserContext.
class ProfileAuthData {
 public:
  // Transfers authentication-related data from |from_context| to |to_context|
  // and invokes |completion_callback| on the UI thread when the operation has
  // completed. The following data is transferred:
  // * The proxy authentication state.
  // * All authentication cookies and channel IDs, if
  //   |transfer_auth_cookies_and_channel_ids_on_first_login| is true and
  //   |to_context|'s cookie jar is empty. If the cookie jar is not empty, the
  //   authentication states in |from_context| and |to_context| should be merged
  //   using /MergeSession instead.
  // * The authentication cookies set by a SAML IdP, if
  //   |transfer_saml_auth_cookies_on_subsequent_login| is true and
  //   |to_context|'s cookie jar is not empty.
  static void Transfer(
      net::URLRequestContextGetter* from_context,
      net::URLRequestContextGetter* to_context,
      bool transfer_auth_cookies_and_channel_ids_on_first_login,
      bool transfer_saml_auth_cookies_on_subsequent_login,
      const base::Closure& completion_callback);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ProfileAuthData);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_PROFILE_AUTH_DATA_H_
