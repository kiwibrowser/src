// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_INTROSPECTOR_IMPL_H_
#define SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_INTROSPECTOR_IMPL_H_

#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/resource_coordinator/public/mojom/coordination_unit_introspector.mojom.h"
#include "services/service_manager/public/cpp/bind_source_info.h"

namespace service_manager {
struct BindSourceInfo;
}  // namespace service_manager

namespace resource_coordinator {

class CoordinationUnitIntrospectorImpl
    : public mojom::CoordinationUnitIntrospector {
 public:
  CoordinationUnitIntrospectorImpl();
  ~CoordinationUnitIntrospectorImpl() override;

  void BindToInterface(
      resource_coordinator::mojom::CoordinationUnitIntrospectorRequest request,
      const service_manager::BindSourceInfo& source_info);

  // Overridden from mojom::CoordinationUnitIntrospector:
  void GetProcessToURLMap(GetProcessToURLMapCallback callback) override;

 private:
  mojo::BindingSet<mojom::CoordinationUnitIntrospector> bindings_;

  DISALLOW_COPY_AND_ASSIGN(CoordinationUnitIntrospectorImpl);
};

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_INTROSPECTOR_IMPL_H_
