// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/identity/identity_get_profile_user_info_function.h"

#include "chrome/browser/extensions/api/identity/identity_constants.h"
#include "chrome/common/extensions/api/identity.h"
#include "content/public/browser/browser_context.h"
#include "content/public/common/service_manager_connection.h"
#include "extensions/common/extension.h"
#include "extensions/common/permissions/permissions_data.h"
#include "services/identity/public/mojom/constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace extensions {

IdentityGetProfileUserInfoFunction::IdentityGetProfileUserInfoFunction() {
}

IdentityGetProfileUserInfoFunction::~IdentityGetProfileUserInfoFunction() {
}

ExtensionFunction::ResponseAction IdentityGetProfileUserInfoFunction::Run() {
  if (browser_context()->IsOffTheRecord()) {
    return RespondNow(Error(identity_constants::kOffTheRecord));
  }

  if (!extension()->permissions_data()->HasAPIPermission(
          APIPermission::kIdentityEmail)) {
    api::identity::ProfileUserInfo profile_user_info;
    return RespondNow(OneArgument(profile_user_info.ToValue()));
  }

  content::BrowserContext::GetConnectorFor(browser_context())
      ->BindInterface(identity::mojom::kServiceName,
                      mojo::MakeRequest(&identity_manager_));

  identity_manager_->GetPrimaryAccountInfo(base::BindOnce(
      &IdentityGetProfileUserInfoFunction::OnReceivedPrimaryAccountInfo, this));

  return RespondLater();
}

void IdentityGetProfileUserInfoFunction::OnReceivedPrimaryAccountInfo(
    const base::Optional<AccountInfo>& account_info,
    const identity::AccountState& account_state) {
  DCHECK(extension()->permissions_data()->HasAPIPermission(
      APIPermission::kIdentityEmail));

  api::identity::ProfileUserInfo profile_user_info;

  if (account_info) {
    profile_user_info.email = account_info->email;
    profile_user_info.id = account_info->gaia;
  }

  Respond(OneArgument(profile_user_info.ToValue()));
}

}  // namespace extensions
