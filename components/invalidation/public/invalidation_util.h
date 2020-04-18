// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Various utilities for dealing with invalidation data types.

#ifndef COMPONENTS_INVALIDATION_PUBLIC_INVALIDATION_UTIL_H_
#define COMPONENTS_INVALIDATION_PUBLIC_INVALIDATION_UTIL_H_

#include <map>
#include <memory>
#include <set>
#include <string>

#include "base/callback.h"
#include "base/optional.h"
#include "base/values.h"
#include "components/invalidation/public/invalidation_export.h"

namespace base {
class DictionaryValue;
}  // namespace

namespace invalidation {
class ObjectId;
class InvalidationObjectId;
}  // namespace invalidation

namespace syncer {

class Invalidation;

struct INVALIDATION_EXPORT ObjectIdLessThan {
  bool operator()(const invalidation::ObjectId& lhs,
                  const invalidation::ObjectId& rhs) const;
};

struct INVALIDATION_EXPORT InvalidationVersionLessThan {
  bool operator()(const Invalidation& a, const Invalidation& b) const;
};

typedef std::set<invalidation::ObjectId, ObjectIdLessThan> ObjectIdSet;

typedef std::map<invalidation::ObjectId, int, ObjectIdLessThan>
    ObjectIdCountMap;

// Caller owns the returned DictionaryValue.
std::unique_ptr<base::DictionaryValue> ObjectIdToValue(
    const invalidation::ObjectId& object_id);

bool ObjectIdFromValue(const base::DictionaryValue& value,
                       invalidation::ObjectId* out);

INVALIDATION_EXPORT std::string ObjectIdToString(
    const invalidation::ObjectId& object_id);

// Same set of utils as above but for the InvalidationObjectId.

struct INVALIDATION_EXPORT InvalidationObjectIdLessThan {
  bool operator()(const invalidation::InvalidationObjectId& lhs,
                  const invalidation::InvalidationObjectId& rhs) const;
};

typedef std::set<invalidation::InvalidationObjectId,
                 InvalidationObjectIdLessThan>
    InvalidationObjectIdSet;

typedef std::
    map<invalidation::InvalidationObjectId, int, InvalidationObjectIdLessThan>
        InvalidationObjectIdCountMap;

std::unique_ptr<base::DictionaryValue> InvalidationObjectIdToValue(
    const invalidation::InvalidationObjectId& object_id);

INVALIDATION_EXPORT std::string InvalidationObjectIdToString(
    const invalidation::InvalidationObjectId& object_id);

}  // namespace syncer

#endif  // COMPONENTS_INVALIDATION_PUBLIC_INVALIDATION_UTIL_H_
