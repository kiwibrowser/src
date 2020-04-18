// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SIGNIN_SIGNIN_ERROR_NOTIFIER_ASH_H_
#define CHROME_BROWSER_SIGNIN_SIGNIN_ERROR_NOTIFIER_ASH_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/signin/core/browser/signin_error_controller.h"

class Profile;

// Shows signin-related errors as notifications in Ash.
class SigninErrorNotifier : public SigninErrorController::Observer,
                            public KeyedService {
 public:
  SigninErrorNotifier(SigninErrorController* controller, Profile* profile);
  ~SigninErrorNotifier() override;

  // KeyedService:
  void Shutdown() override;

  // SigninErrorController::Observer:
  void OnErrorChanged() override;

 private:
  base::string16 GetMessageBody() const;

  // The error controller to query for error details.
  SigninErrorController* error_controller_;

  // The Profile this service belongs to.
  Profile* profile_;

  // Used to keep track of the message center notification.
  std::string notification_id_;

  DISALLOW_COPY_AND_ASSIGN(SigninErrorNotifier);
};

#endif  // CHROME_BROWSER_SIGNIN_SIGNIN_ERROR_NOTIFIER_ASH_H_
