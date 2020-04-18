// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CONTENT_COMMON_CREDENTIAL_MANAGER_MOJOM_TRAITS_H_
#define COMPONENTS_PASSWORD_MANAGER_CONTENT_COMMON_CREDENTIAL_MANAGER_MOJOM_TRAITS_H_

#include "base/optional.h"
#include "base/strings/string16.h"
#include "components/password_manager/core/common/credential_manager_types.h"
#include "mojo/public/cpp/bindings/struct_traits.h"
#include "third_party/blink/public/platform/modules/credentialmanager/credential_manager.mojom.h"

namespace mojo {

template <>
struct EnumTraits<password_manager::mojom::CredentialType,
                  password_manager::CredentialType> {
  static password_manager::mojom::CredentialType ToMojom(
      password_manager::CredentialType input);
  static bool FromMojom(password_manager::mojom::CredentialType input,
                        password_manager::CredentialType* output);
};

template <>
struct EnumTraits<password_manager::mojom::CredentialManagerError,
                  password_manager::CredentialManagerError> {
  static password_manager::mojom::CredentialManagerError ToMojom(
      password_manager::CredentialManagerError input);
  static bool FromMojom(password_manager::mojom::CredentialManagerError input,
                        password_manager::CredentialManagerError* output);
};

template <>
struct EnumTraits<password_manager::mojom::CredentialMediationRequirement,
                  password_manager::CredentialMediationRequirement> {
  static password_manager::mojom::CredentialMediationRequirement ToMojom(
      password_manager::CredentialMediationRequirement input);
  static bool FromMojom(
      password_manager::mojom::CredentialMediationRequirement input,
      password_manager::CredentialMediationRequirement* output);
};

template <>
struct StructTraits<password_manager::mojom::CredentialInfoDataView,
                    password_manager::CredentialInfo> {
  static password_manager::CredentialType type(
      const password_manager::CredentialInfo& r) {
    return r.type;
  }

  static const base::Optional<base::string16>& id(
      const password_manager::CredentialInfo& r) {
    return r.id;
  }

  static const base::Optional<base::string16>& name(
      const password_manager::CredentialInfo& r) {
    return r.name;
  }

  static const GURL& icon(const password_manager::CredentialInfo& r) {
    return r.icon;
  }

  static const base::Optional<base::string16>& password(
      const password_manager::CredentialInfo& r) {
    return r.password;
  }

  static const url::Origin& federation(
      const password_manager::CredentialInfo& r) {
    return r.federation;
  }

  static bool Read(password_manager::mojom::CredentialInfoDataView data,
                   password_manager::CredentialInfo* out);
};

}  // namespace mojo

#endif  // COMPONENTS_PASSWORD_MANAGER_CONTENT_COMMON_CREDENTIAL_MANAGER_MOJOM_TRAITS_H_
