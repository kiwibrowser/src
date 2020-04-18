// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/policy/core/common/schema.h"

#include <limits.h>
#include <stddef.h>

#include <algorithm>
#include <climits>
#include <map>
#include <memory>
#include <utility>

#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/stl_util.h"
#include "base/strings/stringprintf.h"
#include "components/json_schema/json_schema_constants.h"
#include "components/json_schema/json_schema_validator.h"
#include "components/policy/core/common/schema_internal.h"
#include "third_party/re2/src/re2/re2.h"

namespace schema = json_schema_constants;

namespace policy {

using internal::PropertiesNode;
using internal::PropertyNode;
using internal::RestrictionNode;
using internal::SchemaData;
using internal::SchemaNode;

namespace {

// Maps schema "id" attributes to the corresponding SchemaNode index.
typedef std::map<std::string, int> IdMap;

// List of pairs of references to be assigned later. The string is the "id"
// whose corresponding index should be stored in the pointer, once all the IDs
// are available.
typedef std::vector<std::pair<std::string, int*> > ReferenceList;

// Sizes for the storage arrays. These are calculated in advance so that the
// arrays don't have to be resized during parsing, which would invalidate
// pointers into their contents (i.e. string's c_str() and address of indices
// for "$ref" attributes).
struct StorageSizes {
  StorageSizes()
      : strings(0), schema_nodes(0), property_nodes(0), properties_nodes(0),
        restriction_nodes(0), int_enums(0), string_enums(0) { }
  size_t strings;
  size_t schema_nodes;
  size_t property_nodes;
  size_t properties_nodes;
  size_t restriction_nodes;
  size_t int_enums;
  size_t string_enums;
};

// An invalid index, indicating that a node is not present; similar to a NULL
// pointer.
const int kInvalid = -1;

bool SchemaTypeToValueType(const std::string& type_string,
                           base::Value::Type* type) {
  // Note: "any" is not an accepted type.
  static const struct {
    const char* schema_type;
    base::Value::Type value_type;
  } kSchemaToValueTypeMap[] = {
    { schema::kArray,        base::Value::Type::LIST       },
    { schema::kBoolean,      base::Value::Type::BOOLEAN    },
    { schema::kInteger,      base::Value::Type::INTEGER    },
    { schema::kNull,         base::Value::Type::NONE       },
    { schema::kNumber,       base::Value::Type::DOUBLE     },
    { schema::kObject,       base::Value::Type::DICTIONARY },
    { schema::kString,       base::Value::Type::STRING     },
  };
  for (size_t i = 0; i < arraysize(kSchemaToValueTypeMap); ++i) {
    if (kSchemaToValueTypeMap[i].schema_type == type_string) {
      *type = kSchemaToValueTypeMap[i].value_type;
      return true;
    }
  }
  return false;
}

bool StrategyAllowInvalidOnTopLevel(SchemaOnErrorStrategy strategy) {
  return strategy == SCHEMA_ALLOW_INVALID ||
         strategy == SCHEMA_ALLOW_INVALID_TOPLEVEL ||
         strategy == SCHEMA_ALLOW_INVALID_TOPLEVEL_AND_ALLOW_UNKNOWN;
}

bool StrategyAllowUnknownOnTopLevel(SchemaOnErrorStrategy strategy) {
  return strategy != SCHEMA_STRICT;
}

SchemaOnErrorStrategy StrategyForNextLevel(SchemaOnErrorStrategy strategy) {
  static SchemaOnErrorStrategy next_level_strategy[] = {
    SCHEMA_STRICT,         // SCHEMA_STRICT
    SCHEMA_STRICT,         // SCHEMA_ALLOW_UNKNOWN_TOPLEVEL
    SCHEMA_ALLOW_UNKNOWN,  // SCHEMA_ALLOW_UNKNOWN
    SCHEMA_STRICT,         // SCHEMA_ALLOW_INVALID_TOPLEVEL
    SCHEMA_ALLOW_UNKNOWN,  // SCHEMA_ALLOW_INVALID_TOPLEVEL_AND_ALLOW_UNKNOWN
    SCHEMA_ALLOW_INVALID,  // SCHEMA_ALLOW_INVALID
  };
  return next_level_strategy[static_cast<int>(strategy)];
}

void SchemaErrorFound(std::string* error_path,
                     std::string* error,
                     const std::string& msg) {
  if (error_path)
    *error_path = "";
  *error = msg;
}

void AddListIndexPrefixToPath(int index, std::string* path) {
  if (path) {
    if (path->empty())
      *path = base::StringPrintf("items[%d]", index);
    else
      *path = base::StringPrintf("items[%d].", index) + *path;
  }
}

void AddDictKeyPrefixToPath(const std::string& key, std::string* path) {
  if (path) {
    if (path->empty())
      *path = key;
    else
      *path = key + "." + *path;
  }
}

}  // namespace

// Contains the internal data representation of a Schema. This can either wrap
// a SchemaData owned elsewhere (currently used to wrap the Chrome schema, which
// is generated at compile time), or it can own its own SchemaData.
class Schema::InternalStorage
    : public base::RefCountedThreadSafe<InternalStorage> {
 public:
  static scoped_refptr<const InternalStorage> Wrap(const SchemaData* data);

  static scoped_refptr<const InternalStorage> ParseSchema(
      const base::DictionaryValue& schema,
      std::string* error);

  const SchemaData* data() const { return &schema_data_; }

  const SchemaNode* root_node() const {
    return schema(0);
  }

  const SchemaNode* schema(int index) const {
    return schema_data_.schema_nodes + index;
  }

  const PropertiesNode* properties(int index) const {
    return schema_data_.properties_nodes + index;
  }

  const PropertyNode* property(int index) const {
    return schema_data_.property_nodes + index;
  }

  const RestrictionNode* restriction(int index) const {
    return schema_data_.restriction_nodes + index;
  }

  const int* int_enums(int index) const {
    return schema_data_.int_enums + index;
  }

  const char* const* string_enums(int index) const {
    return schema_data_.string_enums + index;
  }

  // Compiles regular expression |pattern|. The result is cached and will be
  // returned directly next time.
  re2::RE2* CompileRegex(const std::string& pattern) const;

 private:
  friend class base::RefCountedThreadSafe<InternalStorage>;

  InternalStorage();
  ~InternalStorage();

  // Determines the expected |sizes| of the storage for the representation
  // of |schema|.
  static void DetermineStorageSizes(const base::DictionaryValue& schema,
                                   StorageSizes* sizes);

  // Parses the JSON schema in |schema|.
  //
  // If |schema| has a "$ref" attribute then a pending reference is appended
  // to the |reference_list|, and nothing else is done.
  //
  // Otherwise, |index| gets assigned the index of the corresponding SchemaNode
  // in |schema_nodes_|. If the |schema| contains an "id" then that ID is mapped
  // to the |index| in the |id_map|.
  //
  // If |schema| is invalid then |error| gets the error reason and false is
  // returned. Otherwise returns true.
  bool Parse(const base::DictionaryValue& schema,
             int* index,
             IdMap* id_map,
             ReferenceList* reference_list,
             std::string* error);

  // Helper for Parse() that gets an already assigned |schema_node| instead of
  // an |index| pointer.
  bool ParseDictionary(const base::DictionaryValue& schema,
                       SchemaNode* schema_node,
                       IdMap* id_map,
                       ReferenceList* reference_list,
                       std::string* error);

  // Helper for Parse() that gets an already assigned |schema_node| instead of
  // an |index| pointer.
  bool ParseList(const base::DictionaryValue& schema,
                 SchemaNode* schema_node,
                 IdMap* id_map,
                 ReferenceList* reference_list,
                 std::string* error);

  bool ParseEnum(const base::DictionaryValue& schema,
                 base::Value::Type type,
                 SchemaNode* schema_node,
                 std::string* error);

  bool ParseRangedInt(const base::DictionaryValue& schema,
                       SchemaNode* schema_node,
                       std::string* error);

  bool ParseStringPattern(const base::DictionaryValue& schema,
                          SchemaNode* schema_node,
                          std::string* error);

  // Assigns the IDs in |id_map| to the pending references in the
  // |reference_list|. If an ID is missing then |error| is set and false is
  // returned; otherwise returns true.
  static bool ResolveReferences(const IdMap& id_map,
                                const ReferenceList& reference_list,
                                std::string* error);

  // Cache for CompileRegex(), will memorize return value of every call to
  // CompileRegex() and return results directly next time.
  mutable std::map<std::string, std::unique_ptr<re2::RE2>> regex_cache_;

  SchemaData schema_data_;
  std::vector<std::string> strings_;
  std::vector<SchemaNode> schema_nodes_;
  std::vector<PropertyNode> property_nodes_;
  std::vector<PropertiesNode> properties_nodes_;
  std::vector<RestrictionNode> restriction_nodes_;
  std::vector<int> int_enums_;
  std::vector<const char*> string_enums_;

  DISALLOW_COPY_AND_ASSIGN(InternalStorage);
};

Schema::InternalStorage::InternalStorage() {
}

Schema::InternalStorage::~InternalStorage() {
}

// static
scoped_refptr<const Schema::InternalStorage> Schema::InternalStorage::Wrap(
    const SchemaData* data) {
  InternalStorage* storage = new InternalStorage();
  storage->schema_data_.schema_nodes = data->schema_nodes;
  storage->schema_data_.property_nodes = data->property_nodes;
  storage->schema_data_.properties_nodes = data->properties_nodes;
  storage->schema_data_.restriction_nodes = data->restriction_nodes;
  storage->schema_data_.int_enums = data->int_enums;
  storage->schema_data_.string_enums = data->string_enums;
  return storage;
}

// static
scoped_refptr<const Schema::InternalStorage>
Schema::InternalStorage::ParseSchema(const base::DictionaryValue& schema,
                                     std::string* error) {
  // Determine the sizes of the storage arrays and reserve the capacity before
  // starting to append nodes and strings. This is important to prevent the
  // arrays from being reallocated, which would invalidate the c_str() pointers
  // and the addresses of indices to fix.
  StorageSizes sizes;
  DetermineStorageSizes(schema, &sizes);

  scoped_refptr<InternalStorage> storage = new InternalStorage();
  storage->strings_.reserve(sizes.strings);
  storage->schema_nodes_.reserve(sizes.schema_nodes);
  storage->property_nodes_.reserve(sizes.property_nodes);
  storage->properties_nodes_.reserve(sizes.properties_nodes);
  storage->restriction_nodes_.reserve(sizes.restriction_nodes);
  storage->int_enums_.reserve(sizes.int_enums);
  storage->string_enums_.reserve(sizes.string_enums);

  int root_index = kInvalid;
  IdMap id_map;
  ReferenceList reference_list;
  if (!storage->Parse(schema, &root_index, &id_map, &reference_list, error))
    return nullptr;

  if (root_index == kInvalid) {
    *error = "The main schema can't have a $ref";
    return nullptr;
  }

  // None of this should ever happen without having been already detected.
  // But, if it does happen, then it will lead to corrupted memory; drop
  // everything in that case.
  if (root_index != 0 ||
      sizes.strings != storage->strings_.size() ||
      sizes.schema_nodes != storage->schema_nodes_.size() ||
      sizes.property_nodes != storage->property_nodes_.size() ||
      sizes.properties_nodes != storage->properties_nodes_.size() ||
      sizes.restriction_nodes != storage->restriction_nodes_.size() ||
      sizes.int_enums != storage->int_enums_.size() ||
      sizes.string_enums != storage->string_enums_.size()) {
    *error = "Failed to parse the schema due to a Chrome bug. Please file a "
             "new issue at http://crbug.com";
    return nullptr;
  }

  if (!ResolveReferences(id_map, reference_list, error))
    return nullptr;

  SchemaData* data = &storage->schema_data_;
  data->schema_nodes = storage->schema_nodes_.data();
  data->property_nodes = storage->property_nodes_.data();
  data->properties_nodes = storage->properties_nodes_.data();
  data->restriction_nodes = storage->restriction_nodes_.data();
  data->int_enums = storage->int_enums_.data();
  data->string_enums = storage->string_enums_.data();
  return storage;
}

re2::RE2* Schema::InternalStorage::CompileRegex(
    const std::string& pattern) const {
  auto it = regex_cache_.find(pattern);
  if (it == regex_cache_.end()) {
    std::unique_ptr<re2::RE2> compiled(new re2::RE2(pattern));
    re2::RE2* compiled_ptr = compiled.get();
    regex_cache_.insert(std::make_pair(pattern, std::move(compiled)));
    return compiled_ptr;
  }
  return it->second.get();
}

// static
void Schema::InternalStorage::DetermineStorageSizes(
    const base::DictionaryValue& schema,
    StorageSizes* sizes) {
  std::string ref_string;
  if (schema.GetString(schema::kRef, &ref_string)) {
    // Schemas with a "$ref" attribute don't take additional storage.
    return;
  }

  std::string type_string;
  base::Value::Type type = base::Value::Type::NONE;
  if (!schema.GetString(schema::kType, &type_string) ||
      !SchemaTypeToValueType(type_string, &type)) {
    // This schema is invalid.
    return;
  }

  sizes->schema_nodes++;

  if (type == base::Value::Type::LIST) {
    const base::DictionaryValue* items = nullptr;
    if (schema.GetDictionary(schema::kItems, &items))
      DetermineStorageSizes(*items, sizes);
  } else if (type == base::Value::Type::DICTIONARY) {
    sizes->properties_nodes++;

    const base::DictionaryValue* dict = nullptr;
    if (schema.GetDictionary(schema::kAdditionalProperties, &dict))
      DetermineStorageSizes(*dict, sizes);

    const base::DictionaryValue* properties = nullptr;
    if (schema.GetDictionary(schema::kProperties, &properties)) {
      for (base::DictionaryValue::Iterator it(*properties);
           !it.IsAtEnd(); it.Advance()) {
        // This should have been verified by the JSONSchemaValidator.
        CHECK(it.value().GetAsDictionary(&dict));
        DetermineStorageSizes(*dict, sizes);
        sizes->strings++;
        sizes->property_nodes++;
      }
    }

    const base::DictionaryValue* pattern_properties = nullptr;
    if (schema.GetDictionary(schema::kPatternProperties, &pattern_properties)) {
      for (base::DictionaryValue::Iterator it(*pattern_properties);
           !it.IsAtEnd(); it.Advance()) {
        CHECK(it.value().GetAsDictionary(&dict));
        DetermineStorageSizes(*dict, sizes);
        sizes->strings++;
        sizes->property_nodes++;
      }
    }
  } else if (schema.HasKey(schema::kEnum)) {
    const base::ListValue* possible_values = nullptr;
    if (schema.GetList(schema::kEnum, &possible_values)) {
      if (type == base::Value::Type::INTEGER) {
        sizes->int_enums += possible_values->GetSize();
      } else if (type == base::Value::Type::STRING) {
        sizes->string_enums += possible_values->GetSize();
        sizes->strings += possible_values->GetSize();
      }
      sizes->restriction_nodes++;
    }
  } else if (type == base::Value::Type::INTEGER) {
    if (schema.HasKey(schema::kMinimum) || schema.HasKey(schema::kMaximum))
      sizes->restriction_nodes++;
  } else if (type == base::Value::Type::STRING) {
    if (schema.HasKey(schema::kPattern)) {
      sizes->strings++;
      sizes->string_enums++;
      sizes->restriction_nodes++;
    }
  }
}

bool Schema::InternalStorage::Parse(const base::DictionaryValue& schema,
                                    int* index,
                                    IdMap* id_map,
                                    ReferenceList* reference_list,
                                    std::string* error) {
  std::string ref_string;
  if (schema.GetString(schema::kRef, &ref_string)) {
    std::string id_string;
    if (schema.GetString(schema::kId, &id_string)) {
      *error = "Schemas with a $ref can't have an id";
      return false;
    }
    reference_list->push_back(std::make_pair(ref_string, index));
    return true;
  }

  std::string type_string;
  if (!schema.GetString(schema::kType, &type_string)) {
    *error = "The schema type must be declared.";
    return false;
  }

  base::Value::Type type = base::Value::Type::NONE;
  if (!SchemaTypeToValueType(type_string, &type)) {
    *error = "Type not supported: " + type_string;
    return false;
  }

  *index = static_cast<int>(schema_nodes_.size());
  schema_nodes_.push_back(SchemaNode());
  SchemaNode* schema_node = &schema_nodes_.back();
  schema_node->type = type;
  schema_node->extra = kInvalid;

  if (type == base::Value::Type::DICTIONARY) {
    if (!ParseDictionary(schema, schema_node, id_map, reference_list, error))
      return false;
  } else if (type == base::Value::Type::LIST) {
    if (!ParseList(schema, schema_node, id_map, reference_list, error))
      return false;
  } else if (schema.HasKey(schema::kEnum)) {
    if (!ParseEnum(schema, type, schema_node, error))
      return false;
  } else if (schema.HasKey(schema::kPattern)) {
    if (!ParseStringPattern(schema, schema_node, error))
      return false;
  } else if (schema.HasKey(schema::kMinimum) ||
             schema.HasKey(schema::kMaximum)) {
    if (type != base::Value::Type::INTEGER) {
      *error = "Only integers can have minimum and maximum";
      return false;
    }
    if (!ParseRangedInt(schema, schema_node, error))
      return false;
  }
  std::string id_string;
  if (schema.GetString(schema::kId, &id_string)) {
    if (base::ContainsKey(*id_map, id_string)) {
      *error = "Duplicated id: " + id_string;
      return false;
    }
    (*id_map)[id_string] = *index;
  }

  return true;
}

bool Schema::InternalStorage::ParseDictionary(
    const base::DictionaryValue& schema,
    SchemaNode* schema_node,
    IdMap* id_map,
    ReferenceList* reference_list,
    std::string* error) {
  int extra = static_cast<int>(properties_nodes_.size());
  properties_nodes_.push_back(PropertiesNode());
  properties_nodes_[extra].additional = kInvalid;
  schema_node->extra = extra;

  const base::DictionaryValue* dict = nullptr;
  if (schema.GetDictionary(schema::kAdditionalProperties, &dict)) {
    if (!Parse(*dict, &properties_nodes_[extra].additional,
               id_map, reference_list, error)) {
      return false;
    }
  }

  properties_nodes_[extra].begin = static_cast<int>(property_nodes_.size());

  const base::DictionaryValue* properties = nullptr;
  if (schema.GetDictionary(schema::kProperties, &properties)) {
    // This and below reserves nodes for all of the |properties|, and makes sure
    // they are contiguous. Recursive calls to Parse() will append after these
    // elements.
    property_nodes_.resize(property_nodes_.size() + properties->size());
  }

  properties_nodes_[extra].end = static_cast<int>(property_nodes_.size());

  const base::DictionaryValue* pattern_properties = nullptr;
  if (schema.GetDictionary(schema::kPatternProperties, &pattern_properties))
    property_nodes_.resize(property_nodes_.size() + pattern_properties->size());

  properties_nodes_[extra].pattern_end =
      static_cast<int>(property_nodes_.size());

  if (properties != nullptr) {
    int base_index = properties_nodes_[extra].begin;
    int index = base_index;

    for (base::DictionaryValue::Iterator it(*properties);
         !it.IsAtEnd(); it.Advance(), ++index) {
      // This should have been verified by the JSONSchemaValidator.
      CHECK(it.value().GetAsDictionary(&dict));
      strings_.push_back(it.key());
      property_nodes_[index].key = strings_.back().c_str();
      if (!Parse(*dict, &property_nodes_[index].schema,
                 id_map, reference_list, error)) {
        return false;
      }
    }
    CHECK_EQ(static_cast<int>(properties->size()), index - base_index);
  }

  if (pattern_properties != nullptr) {
    int base_index = properties_nodes_[extra].end;
    int index = base_index;

    for (base::DictionaryValue::Iterator it(*pattern_properties);
         !it.IsAtEnd(); it.Advance(), ++index) {
      CHECK(it.value().GetAsDictionary(&dict));
      re2::RE2* compiled_regex = CompileRegex(it.key());
      if (!compiled_regex->ok()) {
        *error =
            "/" + it.key() + "/ is a invalid regex: " + compiled_regex->error();
        return false;
      }
      strings_.push_back(it.key());
      property_nodes_[index].key = strings_.back().c_str();
      if (!Parse(*dict, &property_nodes_[index].schema,
                 id_map, reference_list, error)) {
        return false;
      }
    }
    CHECK_EQ(static_cast<int>(pattern_properties->size()), index - base_index);
  }

  if (properties_nodes_[extra].begin == properties_nodes_[extra].pattern_end) {
    properties_nodes_[extra].begin = kInvalid;
    properties_nodes_[extra].end = kInvalid;
    properties_nodes_[extra].pattern_end = kInvalid;
  }

  return true;
}

bool Schema::InternalStorage::ParseList(const base::DictionaryValue& schema,
                                        SchemaNode* schema_node,
                                        IdMap* id_map,
                                        ReferenceList* reference_list,
                                        std::string* error) {
  const base::DictionaryValue* dict = nullptr;
  if (!schema.GetDictionary(schema::kItems, &dict)) {
    *error = "Arrays must declare a single schema for their items.";
    return false;
  }
  return Parse(*dict, &schema_node->extra, id_map, reference_list, error);
}

bool Schema::InternalStorage::ParseEnum(const base::DictionaryValue& schema,
                                        base::Value::Type type,
                                        SchemaNode* schema_node,
                                        std::string* error) {
  const base::ListValue* possible_values = nullptr;
  if (!schema.GetList(schema::kEnum, &possible_values)) {
    *error = "Enum attribute must be a list value";
    return false;
  }
  if (possible_values->empty()) {
    *error = "Enum attribute must be non-empty";
    return false;
  }
  int offset_begin;
  int offset_end;
  if (type == base::Value::Type::INTEGER) {
    offset_begin = static_cast<int>(int_enums_.size());
    int value;
    for (base::ListValue::const_iterator it = possible_values->begin();
         it != possible_values->end(); ++it) {
      if (!it->GetAsInteger(&value)) {
        *error = "Invalid enumeration member type";
        return false;
      }
      int_enums_.push_back(value);
    }
    offset_end = static_cast<int>(int_enums_.size());
  } else if (type == base::Value::Type::STRING) {
    offset_begin = static_cast<int>(string_enums_.size());
    std::string value;
    for (base::ListValue::const_iterator it = possible_values->begin();
         it != possible_values->end(); ++it) {
      if (!it->GetAsString(&value)) {
        *error = "Invalid enumeration member type";
        return false;
      }
      strings_.push_back(value);
      string_enums_.push_back(strings_.back().c_str());
    }
    offset_end = static_cast<int>(string_enums_.size());
  } else {
    *error = "Enumeration is only supported for integer and string.";
    return false;
  }
  schema_node->extra = static_cast<int>(restriction_nodes_.size());
  restriction_nodes_.push_back(RestrictionNode());
  restriction_nodes_.back().enumeration_restriction.offset_begin = offset_begin;
  restriction_nodes_.back().enumeration_restriction.offset_end = offset_end;
  return true;
}

bool Schema::InternalStorage::ParseRangedInt(
    const base::DictionaryValue& schema,
    SchemaNode* schema_node,
    std::string* error) {
  int min_value = INT_MIN;
  int max_value = INT_MAX;
  int value;
  if (schema.GetInteger(schema::kMinimum, &value))
    min_value = value;
  if (schema.GetInteger(schema::kMaximum, &value))
    max_value = value;
  if (min_value > max_value) {
    *error = "Invalid range restriction for int type.";
    return false;
  }
  schema_node->extra = static_cast<int>(restriction_nodes_.size());
  restriction_nodes_.push_back(RestrictionNode());
  restriction_nodes_.back().ranged_restriction.max_value = max_value;
  restriction_nodes_.back().ranged_restriction.min_value = min_value;
  return true;
}

bool Schema::InternalStorage::ParseStringPattern(
    const base::DictionaryValue& schema,
    SchemaNode* schema_node,
    std::string* error) {
  std::string pattern;
  if (!schema.GetString(schema::kPattern, &pattern)) {
    *error = "Schema pattern must be a string.";
    return false;
  }
  re2::RE2* compiled_regex = CompileRegex(pattern);
  if (!compiled_regex->ok()) {
    *error = "/" + pattern + "/ is invalid regex: " + compiled_regex->error();
    return false;
  }
  int index = static_cast<int>(string_enums_.size());
  strings_.push_back(pattern);
  string_enums_.push_back(strings_.back().c_str());
  schema_node->extra = static_cast<int>(restriction_nodes_.size());
  restriction_nodes_.push_back(RestrictionNode());
  restriction_nodes_.back().string_pattern_restriction.pattern_index = index;
  restriction_nodes_.back().string_pattern_restriction.pattern_index_backup =
      index;
  return true;
}

// static
bool Schema::InternalStorage::ResolveReferences(
    const IdMap& id_map,
    const ReferenceList& reference_list,
    std::string* error) {
  for (ReferenceList::const_iterator ref = reference_list.begin();
       ref != reference_list.end(); ++ref) {
    IdMap::const_iterator id = id_map.find(ref->first);
    if (id == id_map.end()) {
      *error = "Invalid $ref: " + ref->first;
      return false;
    }
    *ref->second = id->second;
  }
  return true;
}

Schema::Iterator::Iterator(const scoped_refptr<const InternalStorage>& storage,
                           const PropertiesNode* node)
    : storage_(storage),
      it_(storage->property(node->begin)),
      end_(storage->property(node->end)) {}

Schema::Iterator::Iterator(const Iterator& iterator)
    : storage_(iterator.storage_),
      it_(iterator.it_),
      end_(iterator.end_) {}

Schema::Iterator::~Iterator() {}

Schema::Iterator& Schema::Iterator::operator=(const Iterator& iterator) {
  storage_ = iterator.storage_;
  it_ = iterator.it_;
  end_ = iterator.end_;
  return *this;
}

bool Schema::Iterator::IsAtEnd() const {
  return it_ == end_;
}

void Schema::Iterator::Advance() {
  ++it_;
}

const char* Schema::Iterator::key() const {
  return it_->key;
}

Schema Schema::Iterator::schema() const {
  return Schema(storage_, storage_->schema(it_->schema));
}

Schema::Schema() : node_(nullptr) {}

Schema::Schema(const scoped_refptr<const InternalStorage>& storage,
               const SchemaNode* node)
    : storage_(storage), node_(node) {}

Schema::Schema(const Schema& schema)
    : storage_(schema.storage_), node_(schema.node_) {}

Schema::~Schema() {}

Schema& Schema::operator=(const Schema& schema) {
  storage_ = schema.storage_;
  node_ = schema.node_;
  return *this;
}

// static
Schema Schema::Wrap(const SchemaData* data) {
  scoped_refptr<const InternalStorage> storage = InternalStorage::Wrap(data);
  return Schema(storage, storage->root_node());
}

bool Schema::Validate(const base::Value& value,
                      SchemaOnErrorStrategy strategy,
                      std::string* error_path,
                      std::string* error) const {
  if (!valid()) {
    SchemaErrorFound(error_path, error, "The schema is invalid.");
    return false;
  }

  if (value.type() != type()) {
    // Allow the integer to double promotion. Note that range restriction on
    // double is not supported now.
    if (value.is_int() && type() == base::Value::Type::DOUBLE) {
      return true;
    }

    SchemaErrorFound(
        error_path, error, "The value type doesn't match the schema type.");
    return false;
  }

  const base::DictionaryValue* dict = nullptr;
  const base::ListValue* list = nullptr;
  int int_value;
  std::string str_value;
  if (value.GetAsDictionary(&dict)) {
    for (base::DictionaryValue::Iterator it(*dict); !it.IsAtEnd();
         it.Advance()) {
      SchemaList schema_list = GetMatchingProperties(it.key());
      if (schema_list.empty()) {
        // Unknown property was detected.
        SchemaErrorFound(error_path, error, "Unknown property: " + it.key());
        if (!StrategyAllowUnknownOnTopLevel(strategy))
          return false;
      } else {
        for (SchemaList::iterator subschema = schema_list.begin();
             subschema != schema_list.end(); ++subschema) {
          if (!subschema->Validate(it.value(),
                                   StrategyForNextLevel(strategy),
                                   error_path,
                                   error)) {
            // Invalid property was detected.
            AddDictKeyPrefixToPath(it.key(), error_path);
            if (!StrategyAllowInvalidOnTopLevel(strategy))
              return false;
          }
        }
      }
    }
  } else if (value.GetAsList(&list)) {
    for (base::ListValue::const_iterator it = list->begin(); it != list->end();
         ++it) {
      if (!GetItems().Validate(*it, StrategyForNextLevel(strategy), error_path,
                               error)) {
        // Invalid list item was detected.
        AddListIndexPrefixToPath(it - list->begin(), error_path);
        if (!StrategyAllowInvalidOnTopLevel(strategy))
          return false;
      }
    }
  } else if (value.GetAsInteger(&int_value)) {
    if (node_->extra != kInvalid &&
        !ValidateIntegerRestriction(node_->extra, int_value)) {
      SchemaErrorFound(error_path, error, "Invalid value for integer");
      return false;
    }
  } else if (value.GetAsString(&str_value)) {
    if (node_->extra != kInvalid &&
        !ValidateStringRestriction(node_->extra, str_value.c_str())) {
      SchemaErrorFound(error_path, error, "Invalid value for string");
      return false;
    }
  }

  return true;
}

bool Schema::Normalize(base::Value* value,
                       SchemaOnErrorStrategy strategy,
                       std::string* error_path,
                       std::string* error,
                       bool* changed) const {
  if (!valid()) {
    SchemaErrorFound(error_path, error, "The schema is invalid.");
    return false;
  }

  if (value->type() != type()) {
    // Allow the integer to double promotion. Note that range restriction on
    // double is not supported now.
    if (value->is_int() && type() == base::Value::Type::DOUBLE) {
      return true;
    }

    SchemaErrorFound(
        error_path, error, "The value type doesn't match the schema type.");
    return false;
  }

  base::DictionaryValue* dict = nullptr;
  base::ListValue* list = nullptr;
  if (value->GetAsDictionary(&dict)) {
    std::vector<std::string> drop_list;  // Contains the keys to drop.
    for (base::DictionaryValue::Iterator it(*dict); !it.IsAtEnd();
         it.Advance()) {
      SchemaList schema_list = GetMatchingProperties(it.key());
      if (schema_list.empty()) {
        // Unknown property was detected.
        SchemaErrorFound(error_path, error, "Unknown property: " + it.key());
        if (StrategyAllowUnknownOnTopLevel(strategy))
          drop_list.push_back(it.key());
        else
          return false;
      } else {
        for (SchemaList::iterator subschema = schema_list.begin();
             subschema != schema_list.end(); ++subschema) {
          base::Value* sub_value = nullptr;
          dict->GetWithoutPathExpansion(it.key(), &sub_value);
          if (!subschema->Normalize(sub_value,
                                    StrategyForNextLevel(strategy),
                                    error_path,
                                    error,
                                    changed)) {
            // Invalid property was detected.
            AddDictKeyPrefixToPath(it.key(), error_path);
            if (StrategyAllowInvalidOnTopLevel(strategy)) {
              drop_list.push_back(it.key());
              break;
            } else {
              return false;
            }
          }
        }
      }
    }
    if (changed && !drop_list.empty())
      *changed = true;
    for (std::vector<std::string>::const_iterator it = drop_list.begin();
         it != drop_list.end();
         ++it) {
      dict->RemoveWithoutPathExpansion(*it, nullptr);
    }
    return true;
  } else if (value->GetAsList(&list)) {
    std::vector<size_t> drop_list;  // Contains the indexes to drop.
    for (size_t index = 0; index < list->GetSize(); index++) {
      base::Value* sub_value = nullptr;
      list->Get(index, &sub_value);
      if (!sub_value || !GetItems().Normalize(sub_value,
                                              StrategyForNextLevel(strategy),
                                              error_path,
                                              error,
                                              changed)) {
        // Invalid list item was detected.
        AddListIndexPrefixToPath(index, error_path);
        if (StrategyAllowInvalidOnTopLevel(strategy))
          drop_list.push_back(index);
        else
          return false;
      }
    }
    if (changed && !drop_list.empty())
      *changed = true;
    for (std::vector<size_t>::reverse_iterator it = drop_list.rbegin();
         it != drop_list.rend(); ++it) {
      list->Remove(*it, nullptr);
    }
    return true;
  }

  return Validate(*value, strategy, error_path, error);
}

// static
Schema Schema::Parse(const std::string& content, std::string* error) {
  // Validate as a generic JSON schema, and ignore unknown attributes; they
  // may become used in a future version of the schema format.
  std::unique_ptr<base::DictionaryValue> dict =
      JSONSchemaValidator::IsValidSchema(
          content, JSONSchemaValidator::OPTIONS_IGNORE_UNKNOWN_ATTRIBUTES,
          error);
  if (!dict)
    return Schema();

  // Validate the main type.
  std::string string_value;
  if (!dict->GetString(schema::kType, &string_value) ||
      string_value != schema::kObject) {
    *error =
        "The main schema must have a type attribute with \"object\" value.";
    return Schema();
  }

  // Checks for invalid attributes at the top-level.
  if (dict->HasKey(schema::kAdditionalProperties) ||
      dict->HasKey(schema::kPatternProperties)) {
    *error = "\"additionalProperties\" and \"patternProperties\" are not "
             "supported at the main schema.";
    return Schema();
  }

  scoped_refptr<const InternalStorage> storage =
      InternalStorage::ParseSchema(*dict, error);
  if (!storage)
    return Schema();
  return Schema(storage, storage->root_node());
}

base::Value::Type Schema::type() const {
  CHECK(valid());
  return node_->type;
}

Schema::Iterator Schema::GetPropertiesIterator() const {
  CHECK(valid());
  CHECK_EQ(base::Value::Type::DICTIONARY, type());
  return Iterator(storage_, storage_->properties(node_->extra));
}

namespace {

bool CompareKeys(const PropertyNode& node, const std::string& key) {
  return node.key < key;
}

}  // namespace

Schema Schema::GetKnownProperty(const std::string& key) const {
  CHECK(valid());
  CHECK_EQ(base::Value::Type::DICTIONARY, type());
  const PropertiesNode* node = storage_->properties(node_->extra);
  const PropertyNode* begin = storage_->property(node->begin);
  const PropertyNode* end = storage_->property(node->end);
  const PropertyNode* it = std::lower_bound(begin, end, key, CompareKeys);
  if (it != end && it->key == key)
    return Schema(storage_, storage_->schema(it->schema));
  return Schema();
}

Schema Schema::GetAdditionalProperties() const {
  CHECK(valid());
  CHECK_EQ(base::Value::Type::DICTIONARY, type());
  const PropertiesNode* node = storage_->properties(node_->extra);
  if (node->additional == kInvalid)
    return Schema();
  return Schema(storage_, storage_->schema(node->additional));
}

SchemaList Schema::GetPatternProperties(const std::string& key) const {
  CHECK(valid());
  CHECK_EQ(base::Value::Type::DICTIONARY, type());
  const PropertiesNode* node = storage_->properties(node_->extra);
  const PropertyNode* begin = storage_->property(node->end);
  const PropertyNode* end = storage_->property(node->pattern_end);
  SchemaList matching_properties;
  for (const PropertyNode* it = begin; it != end; ++it) {
    if (re2::RE2::PartialMatch(key, *storage_->CompileRegex(it->key))) {
      matching_properties.push_back(
          Schema(storage_, storage_->schema(it->schema)));
    }
  }
  return matching_properties;
}

Schema Schema::GetProperty(const std::string& key) const {
  Schema schema = GetKnownProperty(key);
  if (schema.valid())
    return schema;
  return GetAdditionalProperties();
}

SchemaList Schema::GetMatchingProperties(const std::string& key) const {
  SchemaList schema_list;

  Schema known_property = GetKnownProperty(key);
  if (known_property.valid())
    schema_list.push_back(known_property);

  SchemaList pattern_properties = GetPatternProperties(key);
  schema_list.insert(
      schema_list.end(), pattern_properties.begin(), pattern_properties.end());

  if (schema_list.empty()) {
    Schema additional_property = GetAdditionalProperties();
    if (additional_property.valid())
      schema_list.push_back(additional_property);
  }

  return schema_list;
}

Schema Schema::GetItems() const {
  CHECK(valid());
  CHECK_EQ(base::Value::Type::LIST, type());
  if (node_->extra == kInvalid)
    return Schema();
  return Schema(storage_, storage_->schema(node_->extra));
}

bool Schema::ValidateIntegerRestriction(int index, int value) const {
  const RestrictionNode* rnode = storage_->restriction(index);
  if (rnode->ranged_restriction.min_value <=
      rnode->ranged_restriction.max_value) {
    return rnode->ranged_restriction.min_value <= value &&
           rnode->ranged_restriction.max_value >= value;
  } else {
    for (int i = rnode->enumeration_restriction.offset_begin;
         i < rnode->enumeration_restriction.offset_end; ++i) {
      if (*storage_->int_enums(i) == value)
        return true;
    }
    return false;
  }
}

bool Schema::ValidateStringRestriction(int index, const char* str) const {
  const RestrictionNode* rnode = storage_->restriction(index);
  if (rnode->enumeration_restriction.offset_begin <
      rnode->enumeration_restriction.offset_end) {
    for (int i = rnode->enumeration_restriction.offset_begin;
         i < rnode->enumeration_restriction.offset_end; ++i) {
      if (strcmp(*storage_->string_enums(i), str) == 0)
        return true;
    }
    return false;
  } else {
    int index = rnode->string_pattern_restriction.pattern_index;
    DCHECK(index == rnode->string_pattern_restriction.pattern_index_backup);
    re2::RE2* regex = storage_->CompileRegex(*storage_->string_enums(index));
    return re2::RE2::PartialMatch(str, *regex);
  }
}

}  // namespace policy
