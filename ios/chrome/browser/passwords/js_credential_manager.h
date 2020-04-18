// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_PASSWORDS_JS_CREDENTIAL_MANAGER_H_
#define IOS_CHROME_BROWSER_PASSWORDS_JS_CREDENTIAL_MANAGER_H_

#include "base/optional.h"
#include "components/password_manager/core/common/credential_manager_types.h"

namespace web {
class WebState;
}

// Resolves the Promise identified by |promise_id| with either Credential or
// undefined. |promise_id| is unique number of a pending promise resolver stored
// in |__gCrWeb.credentialManager|.
void ResolveCredentialPromiseWithCredentialInfo(
    web::WebState* web_state,
    int promise_id,
    const base::Optional<password_manager::CredentialInfo>& info);

// Resolves the Promise identified by |promise_id| with undefined. |promise_id|
// is unique number of a pending promise resolver stored in
// |__gCrWeb.credentialManager|.
void ResolveCredentialPromiseWithUndefined(web::WebState* web_state,
                                           int promise_id);

// Rejects the Promise identified by |promise_id| with TypeError. This may be a
// result of failed parsing of arguments passed to exposed API method.
// |promise_id| is unique number of a pending promise rejecter stored in
// |__gCrWeb.credentialManager|.
void RejectCredentialPromiseWithTypeError(web::WebState* web_state,
                                          int promise_id,
                                          const base::StringPiece16& message);

// Rejects the Promise identified by |promise_id| with InvalidStateError. This
// should happen when credential manager is disabled or there is a pending 'get'
// request. |promise_id| is unique number of a pending promise rejecter stored
// in |__gCrWeb.credentialManager|.
void RejectCredentialPromiseWithInvalidStateError(
    web::WebState* web_state,
    int promise_id,
    const base::StringPiece16& message);

// Rejects the Promise identified by |promise_id| with NotSupportedError. This
// should happen when password store is unavailable or an unknown error occurs.
// |promise_id| is unique number of a pending promise rejecter stored in
// |__gCrWeb.credentialManager|.
void RejectCredentialPromiseWithNotSupportedError(
    web::WebState* web_state,
    int promise_id,
    const base::StringPiece16& message);

#endif  // IOS_CHROME_BROWSER_PASSWORDS_JS_CREDENTIAL_MANAGER_H_
