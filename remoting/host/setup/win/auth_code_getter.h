// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_SETUP_WIN_AUTH_CODE_GETTER_H
#define REMOTING_HOST_SETUP_WIN_AUTH_CODE_GETTER_H

#include <ole2.h>
#include <exdisp.h>
#include <wrl/client.h>

#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/sequence_checker.h"
#include "base/timer/timer.h"

namespace remoting {

// A class for getting an OAuth authorization code.
class AuthCodeGetter {
 public:
  AuthCodeGetter();
  ~AuthCodeGetter();

  // Starts a browser and navigates it to a URL that starts an Installed
  // Application OAuth flow. |on_auth_code| will be called with an
  // authorization code, or an empty string on error.
  void GetAuthCode(base::Callback<void(const std::string&)> on_auth_code);

 private:
  // Starts a timer used to poll the browser's URL.
  void StartTimer();
  // Called when that timer fires.
  void OnTimer();
  // Returns whether to stop polling the browser's URL. If true, then
  // |auth_code| is an authorization code, or the empty string on an error.
  bool TestBrowserUrl(std::string* auth_code);
  // Kills the browser.
  void KillBrowser();

  // The authorization code callback.
  base::Callback<void(const std::string&)> on_auth_code_;
  // The browser through which the user requests an authorization code.
  Microsoft::WRL::ComPtr<IWebBrowser2> browser_;
  // A timer used to poll the browser's URL.
  base::OneShotTimer timer_;
  // The interval at which the timer fires.
  base::TimeDelta timer_interval_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(AuthCodeGetter);
};

}  // namespace remoting

#endif  // REMOTING_HOST_SETUP_WIN_AUTH_CODE_GETTER_H
