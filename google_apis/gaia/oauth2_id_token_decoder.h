// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GOOGLE_APIS_GAIA_OAUTH2_ID_TOKEN_DECODER_H_
#define GOOGLE_APIS_GAIA_OAUTH2_ID_TOKEN_DECODER_H_

#include <string>
#include <vector>

// This file holds methods decodes the id token received for OAuth2 token
// endpoint, and derive useful information from it, such as whether the account
// is a child account.

namespace gaia {

// Detects if the signed-in account is a child account from service flags
// retrieved in the ID token.
bool IsChildAccountFromIdToken(const std::string& id_token);

}  // namespace gaia

#endif  // GOOGLE_APIS_GAIA_OAUTH2_ID_TOKEN_DECODER_H_
