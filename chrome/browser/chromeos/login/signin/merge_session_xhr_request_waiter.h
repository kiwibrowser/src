// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SIGNIN_MERGE_SESSION_XHR_REQUEST_WAITER_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SIGNIN_MERGE_SESSION_XHR_REQUEST_WAITER_H_

#include "chrome/browser/chromeos/login/signin/merge_session_xhr_request_waiter.h"

#include "base/bind.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/chromeos/login/signin/merge_session_throttling_utils.h"
#include "chrome/browser/chromeos/login/signin/oauth2_login_manager.h"
#include "chrome/browser/chromeos/login/signin/oauth2_login_manager_factory.h"

class Profile;

namespace chromeos {

class MergeSessionXHRRequestWaiter : public OAuth2LoginManager::Observer {
 public:
  MergeSessionXHRRequestWaiter(
      Profile* profile,
      const merge_session_throttling_utils::CompletionCallback& callback);
  ~MergeSessionXHRRequestWaiter() override;

  // Starts waiting for merge session completion for |profile_|.
  void StartWaiting();

 private:
  // OAuth2LoginManager::Observer overrides.
  void OnSessionRestoreStateChanged(
      Profile* user_profile,
      OAuth2LoginManager::SessionRestoreState state) override;

  // Timeout callback.
  void OnTimeout();

  // Notifies callback that waiting is done.
  void NotifyBlockingDone();

  Profile* profile_;
  merge_session_throttling_utils::CompletionCallback callback_;
  base::WeakPtrFactory<MergeSessionXHRRequestWaiter> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(MergeSessionXHRRequestWaiter);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SIGNIN_MERGE_SESSION_XHR_REQUEST_WAITER_H_
