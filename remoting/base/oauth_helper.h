// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_BASE_OAUTH_HELPER_H
#define REMOTING_BASE_OAUTH_HELPER_H

#include <string>

namespace remoting {

// Gets the OAuth scope of the host's refresh token.
std::string GetOauthScope();

// Gets the default redirect URL for the OAuth dance.
std::string GetDefaultOauthRedirectUrl();

// Gets a URL at which the OAuth dance starts.
std::string GetOauthStartUrl(const std::string& redirect_url);

// Returns the OAuth authorization code embedded in a URL, or the empty string
// if there is no such code.
// To get an OAuth authorization code, (i) start a browser, (ii) navigate it
// to |GetOauthStartUrl()|, (iii) ask the user to sign on to their account,
// and grant the requested permissions, (iv) monitor the URLs that the browser
// shows, passing each one to |GetOauthCodeInUrl()|, until that function returns
// a non-empty string. That string is the authorization code.
std::string GetOauthCodeInUrl(const std::string& url,
                              const std::string& redirect_url);

}  // namespace remoting

#endif  // REMOTING_BASE_OAUTH_HELPER_H
