// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/model/conflict_resolution.h"

#include <utility>

namespace syncer {

// static
ConflictResolution ConflictResolution::UseLocal() {
  return ConflictResolution(USE_LOCAL, nullptr);
}

// static
ConflictResolution ConflictResolution::UseRemote() {
  return ConflictResolution(USE_REMOTE, nullptr);
}

// static
ConflictResolution ConflictResolution::UseNew(
    std::unique_ptr<EntityData> data) {
  DCHECK(data);
  return ConflictResolution(USE_NEW, std::move(data));
}

ConflictResolution::ConflictResolution(ConflictResolution&& other)
    : ConflictResolution(other.type(), other.ExtractData()) {}

ConflictResolution::~ConflictResolution() {}

std::unique_ptr<EntityData> ConflictResolution::ExtractData() {
  // Has data if and only if type is USE_NEW.
  DCHECK((type_ == USE_NEW) == !!data_);
  return std::move(data_);
}

ConflictResolution::ConflictResolution(Type type,
                                       std::unique_ptr<EntityData> data)
    : type_(type), data_(std::move(data)) {}

}  // namespace syncer
