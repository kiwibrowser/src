// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/model/entity_data.h"

#include <algorithm>
#include <ostream>
#include <utility>

#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/trace_event/memory_usage_estimator.h"
#include "components/sync/base/time.h"
#include "components/sync/base/unique_position.h"
#include "components/sync/protocol/proto_memory_estimations.h"
#include "components/sync/protocol/proto_value_conversions.h"

namespace syncer {

namespace {

std::string UniquePositionToString(
    const sync_pb::UniquePosition& unique_position) {
  return UniquePosition::FromProto(unique_position).ToDebugString();
}

}  // namespace

EntityData::EntityData() = default;

EntityData::EntityData(EntityData&& other)
    : id(std::move(other.id)),
      client_tag_hash(std::move(other.client_tag_hash)),
      server_defined_unique_tag(std::move(other.server_defined_unique_tag)),
      non_unique_name(std::move(other.non_unique_name)),
      creation_time(other.creation_time),
      modification_time(other.modification_time),
      parent_id(std::move(other.parent_id)),
      is_folder(other.is_folder) {
  specifics.Swap(&other.specifics);
  unique_position.Swap(&other.unique_position);
}

EntityData::EntityData(const EntityData& src) = default;

EntityData::~EntityData() = default;

EntityData& EntityData::operator=(EntityData&& other) {
  id = std::move(other.id);
  client_tag_hash = std::move(other.client_tag_hash);
  server_defined_unique_tag = std::move(other.server_defined_unique_tag);
  non_unique_name = std::move(other.non_unique_name);
  creation_time = other.creation_time;
  modification_time = other.modification_time;
  parent_id = other.parent_id;
  is_folder = other.is_folder;
  specifics.Swap(&other.specifics);
  unique_position.Swap(&other.unique_position);
  return *this;
}

EntityDataPtr EntityData::PassToPtr() {
  EntityDataPtr target;
  target.swap_value(this);
  return target;
}

EntityDataPtr EntityData::UpdateId(const std::string& new_id) const {
  EntityData entity_data(*this);
  entity_data.id = new_id;
  EntityDataPtr target;
  target.swap_value(&entity_data);
  return target;
}

EntityDataPtr EntityData::UpdateSpecifics(
    const sync_pb::EntitySpecifics& new_specifics) const {
  EntityData entity_data(*this);
  entity_data.specifics = new_specifics;
  EntityDataPtr target;
  target.swap_value(&entity_data);
  return target;
}

#define ADD_TO_DICT(dict, value) \
  dict->SetString(base::ToUpperASCII(#value), value);

#define ADD_TO_DICT_WITH_TRANSFORM(dict, value, transform) \
  dict->SetString(base::ToUpperASCII(#value), transform(value));

std::unique_ptr<base::DictionaryValue> EntityData::ToDictionaryValue() {
  std::unique_ptr<base::DictionaryValue> dict =
      std::make_unique<base::DictionaryValue>();
  dict->Set("SPECIFICS", EntitySpecificsToValue(specifics));
  ADD_TO_DICT(dict, id);
  ADD_TO_DICT(dict, client_tag_hash);
  ADD_TO_DICT(dict, server_defined_unique_tag);
  ADD_TO_DICT(dict, non_unique_name);
  ADD_TO_DICT(dict, parent_id);
  ADD_TO_DICT_WITH_TRANSFORM(dict, creation_time, GetTimeDebugString);
  ADD_TO_DICT_WITH_TRANSFORM(dict, modification_time, GetTimeDebugString);
  ADD_TO_DICT_WITH_TRANSFORM(dict, unique_position, UniquePositionToString);
  dict->SetBoolean("IS_FOLDER", is_folder);
  return dict;
}

#undef ADD_TO_DICT
#undef ADD_TO_DICT_WITH_TRANSFORM

size_t EntityData::EstimateMemoryUsage() const {
  using base::trace_event::EstimateMemoryUsage;
  size_t memory_usage = 0;
  memory_usage += EstimateMemoryUsage(id);
  memory_usage += EstimateMemoryUsage(client_tag_hash);
  memory_usage += EstimateMemoryUsage(server_defined_unique_tag);
  memory_usage += EstimateMemoryUsage(non_unique_name);
  memory_usage += EstimateMemoryUsage(specifics);
  memory_usage += EstimateMemoryUsage(parent_id);
  memory_usage += EstimateMemoryUsage(unique_position);
  return memory_usage;
}

void EntityDataTraits::SwapValue(EntityData* dest, EntityData* src) {
  std::swap(*dest, *src);
}

bool EntityDataTraits::HasValue(const EntityData& value) {
  return !value.client_tag_hash.empty() || !value.id.empty();
}

const EntityData& EntityDataTraits::DefaultValue() {
  CR_DEFINE_STATIC_LOCAL(EntityData, default_instance, ());
  return default_instance;
}

void PrintTo(const EntityData& entity_data, std::ostream* os) {
  std::string specifics;
  base::JSONWriter::WriteWithOptions(
      *syncer::EntitySpecificsToValue(entity_data.specifics),
      base::JSONWriter::OPTIONS_PRETTY_PRINT, &specifics);
  *os << "{ id: '" << entity_data.id << "', client_tag_hash: '"
      << entity_data.client_tag_hash << "', server_defined_unique_tag: '"
      << entity_data.server_defined_unique_tag << "', specifics: " << specifics
      << "}";
}

}  // namespace syncer
