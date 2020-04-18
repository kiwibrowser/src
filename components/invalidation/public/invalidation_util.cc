// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/invalidation/public/invalidation_util.h"

#include <memory>
#include <ostream>
#include <sstream>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "components/invalidation/public/invalidation.h"
#include "components/invalidation/public/invalidation_object_id.h"
#include "google/cacheinvalidation/include/types.h"
#include "google/cacheinvalidation/types.pb.h"

namespace syncer {

bool ObjectIdLessThan::operator()(const invalidation::ObjectId& lhs,
                                  const invalidation::ObjectId& rhs) const {
  return (lhs.source() < rhs.source()) ||
         (lhs.source() == rhs.source() && lhs.name() < rhs.name());
}

bool InvalidationVersionLessThan::operator()(const Invalidation& a,
                                             const Invalidation& b) const {
  DCHECK(a.object_id() == b.object_id())
      << "a: " << ObjectIdToString(a.object_id()) << ", "
      << "b: " << ObjectIdToString(a.object_id());

  if (a.is_unknown_version() && !b.is_unknown_version())
    return true;

  if (!a.is_unknown_version() && b.is_unknown_version())
    return false;

  if (a.is_unknown_version() && b.is_unknown_version())
    return false;

  return a.version() < b.version();
}

std::unique_ptr<base::DictionaryValue> ObjectIdToValue(
    const invalidation::ObjectId& object_id) {
  std::unique_ptr<base::DictionaryValue> value(new base::DictionaryValue());
  value->SetInteger("source", object_id.source());
  value->SetString("name", object_id.name());
  return value;
}

bool ObjectIdFromValue(const base::DictionaryValue& value,
                       invalidation::ObjectId* out) {
  *out = invalidation::ObjectId();
  std::string name;
  int source = 0;
  if (!value.GetInteger("source", &source) || !value.GetString("name", &name)) {
    return false;
  }
  *out = invalidation::ObjectId(source, name);
  return true;
}

std::string ObjectIdToString(const invalidation::ObjectId& object_id) {
  std::string str;
  base::JSONWriter::Write(*ObjectIdToValue(object_id), &str);
  return str;
}

bool InvalidationObjectIdLessThan::operator()(
    const invalidation::InvalidationObjectId& lhs,
    const invalidation::InvalidationObjectId& rhs) const {
  return (lhs.source() < rhs.source()) ||
         (lhs.source() == rhs.source() && lhs.name() < rhs.name());
}

std::unique_ptr<base::DictionaryValue> InvalidationObjectIdToValue(
    const invalidation::InvalidationObjectId& object_id) {
  std::unique_ptr<base::DictionaryValue> value(new base::DictionaryValue());
  value->SetInteger("source", object_id.source());
  value->SetString("name", object_id.name());
  return value;
}

std::string InvalidationObjectIdToString(
    const invalidation::InvalidationObjectId& object_id) {
  std::string str;
  base::JSONWriter::Write(*InvalidationObjectIdToValue(object_id), &str);
  return str;
}

}  // namespace syncer
