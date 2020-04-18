// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_IDENTITY_PUBLIC_CPP_ACCOUNT_INFO_MOJOM_TRAITS_H_
#define SERVICES_IDENTITY_PUBLIC_CPP_ACCOUNT_INFO_MOJOM_TRAITS_H_

#include <string>

#include "components/signin/core/browser/account_info.h"
#include "services/identity/public/mojom/account_info.mojom.h"

namespace mojo {

template <>
struct StructTraits<identity::mojom::AccountInfo::DataView, ::AccountInfo> {
  static const std::string& account_id(const ::AccountInfo& r) {
    return r.account_id;
  }

  static const std::string& gaia(const ::AccountInfo& r) { return r.gaia; }

  static const std::string& email(const ::AccountInfo& r) { return r.email; }

  static const std::string& full_name(const ::AccountInfo& r) {
    return r.full_name;
  }

  static const std::string& given_name(const ::AccountInfo& r) {
    return r.given_name;
  }

  static const std::string& hosted_domain(const ::AccountInfo& r) {
    return r.hosted_domain;
  }

  static const std::string& locale(const ::AccountInfo& r) { return r.locale; }

  static const std::string& picture_url(const ::AccountInfo& r) {
    return r.picture_url;
  }

  static bool is_child_account(const ::AccountInfo& r) {
    return r.is_child_account;
  }

  static bool Read(identity::mojom::AccountInfo::DataView data,
                   ::AccountInfo* out);

  static bool IsNull(const ::AccountInfo& input);

  static void SetToNull(::AccountInfo* output) { *output = AccountInfo(); }
};

}  // namespace mojo

#endif  // SERVICES_IDENTITY_PUBLIC_CPP_ACCOUNT_INFO_MOJOM_TRAITS_H_
