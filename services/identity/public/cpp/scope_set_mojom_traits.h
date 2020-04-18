// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_IDENTITY_PUBLIC_CPP_SCOPE_SET_MOJOM_TRAITS_H_
#define SERVICES_IDENTITY_PUBLIC_CPP_SCOPE_SET_MOJOM_TRAITS_H_

#include "services/identity/public/cpp/scope_set.h"

namespace mojo {

template <>
struct StructTraits<identity::mojom::ScopeSet::DataView, identity::ScopeSet> {
  static std::vector<std::string> scopes(const identity::ScopeSet& scope_set) {
    std::vector<std::string> entries;
    for (const auto& scope : scope_set)
      entries.push_back(scope);
    return entries;
  }

  static bool Read(identity::mojom::ScopeSetDataView data,
                   identity::ScopeSet* out) {
    ArrayDataView<StringDataView> scopes_data_view;
    data.GetScopesDataView(&scopes_data_view);
    for (size_t i = 0; i < scopes_data_view.size(); ++i) {
      std::string scope;
      if (!scopes_data_view.Read(i, &scope))
        return false;
      out->insert(std::move(scope));
    }
    return true;
  }
};

}  // namespace mojo

#endif  // SERVICES_IDENTITY_PUBLIC_CPP_SCOPE_SET_MOJOM_TRAITS_H_
