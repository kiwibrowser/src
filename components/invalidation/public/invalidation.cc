// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/invalidation/public/invalidation.h"

#include <cstddef>

#include "base/bind.h"
#include "base/json/json_string_value_serializer.h"
#include "base/location.h"
#include "base/memory/ptr_util.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "components/invalidation/public/ack_handler.h"
#include "components/invalidation/public/invalidation_util.h"

namespace syncer {

namespace {
const char kObjectIdKey[] = "objectId";
const char kIsUnknownVersionKey[] = "isUnknownVersion";
const char kVersionKey[] = "version";
const char kPayloadKey[] = "payload";
const int64_t kInvalidVersion = -1;
}

Invalidation Invalidation::Init(const invalidation::ObjectId& id,
                                int64_t version,
                                const std::string& payload) {
  return Invalidation(id, false, version, payload, AckHandle::CreateUnique());
}

Invalidation Invalidation::InitUnknownVersion(
    const invalidation::ObjectId& id) {
  return Invalidation(
      id, true, kInvalidVersion, std::string(), AckHandle::CreateUnique());
}

Invalidation Invalidation::InitFromDroppedInvalidation(
    const Invalidation& dropped) {
  return Invalidation(
      dropped.id_, true, kInvalidVersion, std::string(), dropped.ack_handle_);
}

std::unique_ptr<Invalidation> Invalidation::InitFromValue(
    const base::DictionaryValue& value) {
  invalidation::ObjectId id;

  const base::DictionaryValue* object_id_dict;
  if (!value.GetDictionary(kObjectIdKey, &object_id_dict) ||
      !ObjectIdFromValue(*object_id_dict, &id)) {
    DLOG(WARNING) << "Failed to parse id";
    return nullptr;
  }
  bool is_unknown_version;
  if (!value.GetBoolean(kIsUnknownVersionKey, &is_unknown_version)) {
    DLOG(WARNING) << "Failed to parse is_unknown_version flag";
    return nullptr;
  }
  if (is_unknown_version) {
    return base::WrapUnique(new Invalidation(
        id, true, kInvalidVersion, std::string(), AckHandle::CreateUnique()));
  }
  int64_t version = 0;
  std::string version_as_string;
  if (!value.GetString(kVersionKey, &version_as_string)
      || !base::StringToInt64(version_as_string, &version)) {
    DLOG(WARNING) << "Failed to parse version";
    return nullptr;
  }
  std::string payload;
  if (!value.GetString(kPayloadKey, &payload)) {
    DLOG(WARNING) << "Failed to parse payload";
    return nullptr;
  }
  return base::WrapUnique(
      new Invalidation(id, false, version, payload, AckHandle::CreateUnique()));
}

Invalidation::Invalidation(const Invalidation& other) = default;

Invalidation::~Invalidation() {
}

invalidation::ObjectId Invalidation::object_id() const {
  return id_;
}

bool Invalidation::is_unknown_version() const {
  return is_unknown_version_;
}

int64_t Invalidation::version() const {
  DCHECK(!is_unknown_version_);
  return version_;
}

const std::string& Invalidation::payload() const {
  DCHECK(!is_unknown_version_);
  return payload_;
}

const AckHandle& Invalidation::ack_handle() const {
  return ack_handle_;
}

void Invalidation::SetAckHandler(
    base::WeakPtr<AckHandler> handler,
    scoped_refptr<base::SequencedTaskRunner> handler_task_runner) {
  ack_handler_ = handler;
  ack_handler_task_runner_ = handler_task_runner;
}

bool Invalidation::SupportsAcknowledgement() const {
  return !!ack_handler_task_runner_;
}

void Invalidation::Acknowledge() const {
  if (SupportsAcknowledgement()) {
    ack_handler_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&AckHandler::Acknowledge, ack_handler_, id_, ack_handle_));
  }
}

void Invalidation::Drop() {
  if (SupportsAcknowledgement()) {
    ack_handler_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&AckHandler::Drop, ack_handler_, id_, ack_handle_));
  }
}

bool Invalidation::Equals(const Invalidation& other) const {
  return id_ == other.id_ && is_unknown_version_ == other.is_unknown_version_ &&
         version_ == other.version_ && payload_ == other.payload_;
}

std::unique_ptr<base::DictionaryValue> Invalidation::ToValue() const {
  std::unique_ptr<base::DictionaryValue> value(new base::DictionaryValue());
  value->Set(kObjectIdKey, ObjectIdToValue(id_));
  if (is_unknown_version_) {
    value->SetBoolean(kIsUnknownVersionKey, true);
  } else {
    value->SetBoolean(kIsUnknownVersionKey, false);
    value->SetString(kVersionKey, base::Int64ToString(version_));
    value->SetString(kPayloadKey, payload_);
  }
  return value;
}

std::string Invalidation::ToString() const {
  std::string output;
  JSONStringValueSerializer serializer(&output);
  serializer.set_pretty_print(true);
  serializer.Serialize(*ToValue());
  return output;
}

Invalidation::Invalidation(const invalidation::ObjectId& id,
                           bool is_unknown_version,
                           int64_t version,
                           const std::string& payload,
                           AckHandle ack_handle)
    : id_(id),
      is_unknown_version_(is_unknown_version),
      version_(version),
      payload_(payload),
      ack_handle_(ack_handle) {}

}  // namespace syncer
