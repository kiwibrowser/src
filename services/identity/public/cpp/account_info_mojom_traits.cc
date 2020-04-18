// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/identity/public/cpp/account_info_mojom_traits.h"

namespace mojo {

// static
bool StructTraits<identity::mojom::AccountInfo::DataView, ::AccountInfo>::Read(
    identity::mojom::AccountInfo::DataView data,
    ::AccountInfo* out) {
  std::string account_id;
  std::string gaia;
  std::string email;
  std::string full_name;
  std::string given_name;
  std::string hosted_domain;
  std::string locale;
  std::string picture_url;
  if (!data.ReadAccountId(&account_id) || !data.ReadGaia(&gaia) ||
      !data.ReadEmail(&email) || !data.ReadFullName(&full_name) ||
      !data.ReadGivenName(&given_name) ||
      !data.ReadHostedDomain(&hosted_domain) || !data.ReadLocale(&locale) ||
      !data.ReadPictureUrl(&picture_url)) {
    return false;
  }

  out->account_id = account_id;
  out->gaia = gaia;
  out->email = email;
  out->full_name = full_name;
  out->given_name = given_name;
  out->hosted_domain = hosted_domain;
  out->locale = locale;
  out->picture_url = picture_url;
  out->is_child_account = data.is_child_account();

  return true;
}

// static
bool StructTraits<identity::mojom::AccountInfo::DataView,
                  ::AccountInfo>::IsNull(const ::AccountInfo& input) {
  // Note that an AccountInfo being null cannot be defined as
  // !AccountInfo::IsValid(), as IsValid() requires that *every* field is
  // populated, which is too stringent a requirement.
  return input.account_id.empty() || input.gaia.empty() || input.email.empty();
}

}  // namespace mojo
