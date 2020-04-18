// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/public/cpp/coordination_unit_id.h"

#include "base/numerics/safe_conversions.h"
#include "base/unguessable_token.h"
#include "third_party/smhasher/src/City.h"

namespace resource_coordinator {

namespace {

uint64_t CreateCityHash64(const std::string& id) {
  DCHECK(base::IsValueInRangeForNumericType<int>(id.size()));

  return CityHash64(&id.front(), static_cast<int>(id.size()));
}

}  // namespace

CoordinationUnitID::CoordinationUnitID()
    : id(0), type(CoordinationUnitType::kInvalidType) {}

CoordinationUnitID::CoordinationUnitID(const CoordinationUnitType& type,
                                       const std::string& new_id)
    : type(type) {
  id = CreateCityHash64(
      !new_id.empty() ? new_id : base::UnguessableToken().Create().ToString());
}

CoordinationUnitID::CoordinationUnitID(const CoordinationUnitType& type,
                                       uint64_t new_id)
    : id(new_id), type(type) {}

}  // namespace resource_coordinator
