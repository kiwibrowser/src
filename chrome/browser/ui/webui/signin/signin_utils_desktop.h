// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SIGNIN_SIGNIN_UTILS_DESKTOP_H_
#define CHROME_BROWSER_UI_WEBUI_SIGNIN_SIGNIN_UTILS_DESKTOP_H_

#include <string>

class Profile;

// Argument for |CanOfferSignin|.
enum CanOfferSigninType {
  CAN_OFFER_SIGNIN_FOR_ALL_ACCOUNTS,
  CAN_OFFER_SIGNIN_FOR_SECONDARY_ACCOUNT
};

// Returns true if sign-in is allowed for account with |email| and |gaia_id| to
// |profile|. If the sign-in is not allowed, then the error message is passed
// to the called in |out_error_message|
bool CanOfferSignin(Profile* profile,
                    CanOfferSigninType can_offer_type,
                    const std::string& gaia_id,
                    const std::string& email,
                    std::string* out_error_message);

// Return true if the account given by |email| and |gaia_id| is signed in to
// Chrome in a different profile.
bool IsCrossAccountError(Profile* profile,
                         const std::string& email,
                         const std::string& gaia_id);

#endif  // CHROME_BROWSER_UI_WEBUI_SIGNIN_SIGNIN_UTILS_DESKTOP_H_
