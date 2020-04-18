// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_ID_H_
#define SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_ID_H_

#include <string>
#include <tuple>

#include "services/resource_coordinator/public/cpp/coordination_unit_types.h"
#include "services/resource_coordinator/public/cpp/resource_coordinator_base_export.h"

namespace resource_coordinator {

// This is a native struct rather than a mojom struct as we eventually want
// to annotate base::TaskRunner with CUs for cost attribution purses and
// would like to move it to base/ as easily as possible at that point.
// TODO(oysteine): Rename to CoordinationUnitGUID to better differentiate the
// class from the internal id
struct SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_BASE_EXPORT CoordinationUnitID {
  typedef uint64_t CoordinationUnitTypeId;

  CoordinationUnitID();
  CoordinationUnitID(const CoordinationUnitType& type,
                     const std::string& new_id);
  CoordinationUnitID(const CoordinationUnitType& type,
                     CoordinationUnitTypeId new_id);

  bool operator==(const CoordinationUnitID& b) const {
    return id == b.id && type == b.type;
  }

  bool operator!=(const CoordinationUnitID& b) const { return !(*this == b); }

  bool operator<(const CoordinationUnitID& b) const {
    return std::tie(id, type) < std::tie(b.id, b.type);
  }

  CoordinationUnitTypeId id;
  CoordinationUnitType type;
};

}  // resource_coordinator

namespace std {

template <>
struct hash<resource_coordinator::CoordinationUnitID> {
  uint64_t operator()(
      const resource_coordinator::CoordinationUnitID& id) const {
    return ((static_cast<uint64_t>(id.type)) << 32) |
           static_cast<uint64_t>(id.id);
  }
};

}  // namespace std

#endif  // SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_ID_H_
