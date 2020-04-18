// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SERVICE_MANAGER_PUBLIC_CPP_IDENTITY_STRUCT_TRAITS_H_
#define SERVICES_SERVICE_MANAGER_PUBLIC_CPP_IDENTITY_STRUCT_TRAITS_H_

#include "services/service_manager/public/cpp/identity.h"
#include "services/service_manager/public/mojom/connector.mojom.h"

namespace mojo {

template <>
struct COMPONENT_EXPORT(SERVICE_MANAGER_MOJOM)
    StructTraits<service_manager::mojom::IdentityDataView,
                 service_manager::Identity> {
  static const std::string& name(const service_manager::Identity& identity) {
    return identity.name();
  }
  static const std::string& user_id(const service_manager::Identity& identity) {
    return identity.user_id();
  }
  static const std::string& instance(
      const service_manager::Identity& identity) {
    return identity.instance();
  }
  static bool Read(service_manager::mojom::IdentityDataView data,
                   service_manager::Identity* out) {
    std::string name, user_id, instance;
    if (!data.ReadName(&name))
      return false;

    if (!data.ReadUserId(&user_id))
      return false;

    if (!data.ReadInstance(&instance))
      return false;

    *out = service_manager::Identity(name, user_id, instance);
    return true;
  }
};

}  // namespace mojo

#endif  // SERVICES_SERVICE_MANAGER_PUBLIC_CPP_IDENTITY_STRUCT_TRAITS_H_
