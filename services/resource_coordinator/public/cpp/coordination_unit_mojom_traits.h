// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_COORDINATION_UNIT_MOJOM_TRAITS_H_
#define SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_COORDINATION_UNIT_MOJOM_TRAITS_H_

#include "base/component_export.h"
#include "services/resource_coordinator/public/cpp/coordination_unit_types.h"
#include "services/resource_coordinator/public/mojom/coordination_unit.mojom.h"

namespace mojo {

template <>
struct COMPONENT_EXPORT(RESOURCE_COORDINATOR_PUBLIC_MOJOM)
    EnumTraits<resource_coordinator::mojom::CoordinationUnitType,
               resource_coordinator::CoordinationUnitType> {
  static resource_coordinator::mojom::CoordinationUnitType ToMojom(
      resource_coordinator::CoordinationUnitType type);
  static bool FromMojom(resource_coordinator::mojom::CoordinationUnitType input,
                        resource_coordinator::CoordinationUnitType* out);
};

template <>
struct COMPONENT_EXPORT(RESOURCE_COORDINATOR_PUBLIC_MOJOM)
    StructTraits<resource_coordinator::mojom::CoordinationUnitIDDataView,
                 resource_coordinator::CoordinationUnitID> {
  static uint64_t id(const resource_coordinator::CoordinationUnitID& id) {
    return id.id;
  }
  static resource_coordinator::CoordinationUnitType type(
      const resource_coordinator::CoordinationUnitID& id) {
    return id.type;
  }
  static bool Read(
      resource_coordinator::mojom::CoordinationUnitIDDataView input,
      resource_coordinator::CoordinationUnitID* out);
};

}  // namespace mojo

#endif  // SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_COORDINATION_UNIT_MOJOM_TRAITS_H_
