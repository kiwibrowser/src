// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/json_schema/json_schema_validator.h"

#include <stddef.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <memory>
#include <vector>

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "components/json_schema/json_schema_constants.h"
#include "third_party/re2/src/re2/re2.h"

namespace schema = json_schema_constants;

namespace {

double GetNumberValue(const base::Value* value) {
  double result = 0;
  CHECK(value->GetAsDouble(&result))
      << "Unexpected value type: " << value->type();
  return result;
}

bool IsValidType(const std::string& type) {
  static const char* kValidTypes[] = {
    schema::kAny,
    schema::kArray,
    schema::kBoolean,
    schema::kInteger,
    schema::kNull,
    schema::kNumber,
    schema::kObject,
    schema::kString,
  };
  const char** end = kValidTypes + arraysize(kValidTypes);
  return std::find(kValidTypes, end, type) != end;
}

// Maps a schema attribute name to its expected type.
struct ExpectedType {
  const char* key;
  base::Value::Type type;
};

// Helper for std::lower_bound.
bool CompareToString(const ExpectedType& entry, const std::string& key) {
  return entry.key < key;
}

// If |value| is a dictionary, returns the "name" attribute of |value| or NULL
// if |value| does not contain a "name" attribute. Otherwise, returns |value|.
const base::Value* ExtractNameFromDictionary(const base::Value* value) {
  const base::DictionaryValue* value_dict = nullptr;
  const base::Value* name_value = nullptr;
  if (value->GetAsDictionary(&value_dict)) {
    value_dict->Get("name", &name_value);
    return name_value;
  }
  return value;
}

bool IsValidSchema(const base::DictionaryValue* dict,
                   int options,
                   std::string* error) {
  // This array must be sorted, so that std::lower_bound can perform a
  // binary search.
  static const ExpectedType kExpectedTypes[] = {
    // Note: kRef == "$ref", kSchema == "$schema"
    { schema::kRef,                     base::Value::Type::STRING      },
    { schema::kSchema,                  base::Value::Type::STRING      },

    { schema::kAdditionalProperties,    base::Value::Type::DICTIONARY  },
    { schema::kChoices,                 base::Value::Type::LIST        },
    { schema::kDescription,             base::Value::Type::STRING      },
    { schema::kEnum,                    base::Value::Type::LIST        },
    { schema::kId,                      base::Value::Type::STRING      },
    { schema::kMaxItems,                base::Value::Type::INTEGER     },
    { schema::kMaxLength,               base::Value::Type::INTEGER     },
    { schema::kMaximum,                 base::Value::Type::DOUBLE      },
    { schema::kMinItems,                base::Value::Type::INTEGER     },
    { schema::kMinLength,               base::Value::Type::INTEGER     },
    { schema::kMinimum,                 base::Value::Type::DOUBLE      },
    { schema::kOptional,                base::Value::Type::BOOLEAN     },
    { schema::kPattern,                 base::Value::Type::STRING      },
    { schema::kPatternProperties,       base::Value::Type::DICTIONARY  },
    { schema::kProperties,              base::Value::Type::DICTIONARY  },
    { schema::kTitle,                   base::Value::Type::STRING      },
  };

  bool has_type_or_ref = false;
  const base::ListValue* list_value = nullptr;
  const base::DictionaryValue* dictionary_value = nullptr;
  std::string string_value;

  for (base::DictionaryValue::Iterator it(*dict); !it.IsAtEnd(); it.Advance()) {
    // Validate the "type" attribute, which may be a string or a list.
    if (it.key() == schema::kType) {
      switch (it.value().type()) {
        case base::Value::Type::STRING:
          it.value().GetAsString(&string_value);
          if (!IsValidType(string_value)) {
            *error = "Invalid value for type attribute";
            return false;
          }
          break;
        case base::Value::Type::LIST:
          it.value().GetAsList(&list_value);
          for (size_t i = 0; i < list_value->GetSize(); ++i) {
            if (!list_value->GetString(i, &string_value) ||
                !IsValidType(string_value)) {
              *error = "Invalid value for type attribute";
              return false;
            }
          }
          break;
        default:
          *error = "Invalid value for type attribute";
          return false;
      }
      has_type_or_ref = true;
      continue;
    }

    // Validate the "items" attribute, which is a schema or a list of schemas.
    if (it.key() == schema::kItems) {
      if (it.value().GetAsDictionary(&dictionary_value)) {
        if (!IsValidSchema(dictionary_value, options, error)) {
          DCHECK(!error->empty());
          return false;
        }
      } else if (it.value().GetAsList(&list_value)) {
        for (size_t i = 0; i < list_value->GetSize(); ++i) {
          if (!list_value->GetDictionary(i, &dictionary_value)) {
            *error = base::StringPrintf(
                "Invalid entry in items attribute at index %d",
                static_cast<int>(i));
            return false;
          }
          if (!IsValidSchema(dictionary_value, options, error)) {
            DCHECK(!error->empty());
            return false;
          }
        }
      } else {
        *error = "Invalid value for items attribute";
        return false;
      }
      continue;
    }

    // All the other attributes have a single valid type.
    const ExpectedType* end = kExpectedTypes + arraysize(kExpectedTypes);
    const ExpectedType* entry = std::lower_bound(
        kExpectedTypes, end, it.key(), CompareToString);
    if (entry == end || entry->key != it.key()) {
      if (options & JSONSchemaValidator::OPTIONS_IGNORE_UNKNOWN_ATTRIBUTES)
        continue;
      *error = base::StringPrintf("Invalid attribute %s", it.key().c_str());
      return false;
    }

    // Integer can be converted to double.
    if (!(it.value().type() == entry->type ||
          (it.value().is_int() && entry->type == base::Value::Type::DOUBLE))) {
      *error = base::StringPrintf("Invalid value for %s attribute",
                                  it.key().c_str());
      return false;
    }

    // base::Value::Type::INTEGER attributes must be >= 0.
    // This applies to "minItems", "maxItems", "minLength" and "maxLength".
    if (it.value().is_int()) {
      int integer_value;
      it.value().GetAsInteger(&integer_value);
      if (integer_value < 0) {
        *error = base::StringPrintf("Value of %s must be >= 0, got %d",
                                    it.key().c_str(), integer_value);
        return false;
      }
    }

    // Validate the "properties" attribute. Each entry maps a key to a schema.
    if (it.key() == schema::kProperties) {
      it.value().GetAsDictionary(&dictionary_value);
      for (base::DictionaryValue::Iterator iter(*dictionary_value);
           !iter.IsAtEnd(); iter.Advance()) {
        if (!iter.value().GetAsDictionary(&dictionary_value)) {
          *error = "properties must be a dictionary";
          return false;
        }
        if (!IsValidSchema(dictionary_value, options, error)) {
          DCHECK(!error->empty());
          return false;
        }
      }
    }

    // Validate the "patternProperties" attribute. Each entry maps a regular
    // expression to a schema. The validity of the regular expression expression
    // won't be checked here for performance reasons. Instead, invalid regular
    // expressions will be caught as validation errors in Validate().
    if (it.key() == schema::kPatternProperties) {
      it.value().GetAsDictionary(&dictionary_value);
      for (base::DictionaryValue::Iterator iter(*dictionary_value);
           !iter.IsAtEnd(); iter.Advance()) {
        if (!iter.value().GetAsDictionary(&dictionary_value)) {
          *error = "patternProperties must be a dictionary";
          return false;
        }
        if (!IsValidSchema(dictionary_value, options, error)) {
          DCHECK(!error->empty());
          return false;
        }
      }
    }

    // Validate "additionalProperties" attribute, which is a schema.
    if (it.key() == schema::kAdditionalProperties) {
      it.value().GetAsDictionary(&dictionary_value);
      if (!IsValidSchema(dictionary_value, options, error)) {
        DCHECK(!error->empty());
        return false;
      }
    }

    // Validate the values contained in an "enum" attribute.
    if (it.key() == schema::kEnum) {
      it.value().GetAsList(&list_value);
      for (size_t i = 0; i < list_value->GetSize(); ++i) {
        const base::Value* value = nullptr;
        list_value->Get(i, &value);
        // Sometimes the enum declaration is a dictionary with the enum value
        // under "name".
        value = ExtractNameFromDictionary(value);
        if (!value) {
          *error = "Invalid value in enum attribute";
          return false;
        }
        switch (value->type()) {
          case base::Value::Type::NONE:
          case base::Value::Type::BOOLEAN:
          case base::Value::Type::INTEGER:
          case base::Value::Type::DOUBLE:
          case base::Value::Type::STRING:
            break;
          default:
            *error = "Invalid value in enum attribute";
            return false;
        }
      }
    }

    // Validate the schemas contained in a "choices" attribute.
    if (it.key() == schema::kChoices) {
      it.value().GetAsList(&list_value);
      for (size_t i = 0; i < list_value->GetSize(); ++i) {
        if (!list_value->GetDictionary(i, &dictionary_value)) {
          *error = "Invalid choices attribute";
          return false;
        }
        if (!IsValidSchema(dictionary_value, options, error)) {
          DCHECK(!error->empty());
          return false;
        }
      }
    }

    if (it.key() == schema::kRef)
      has_type_or_ref = true;
  }

  if (!has_type_or_ref) {
    *error = "Schema must have a type or a $ref attribute";
    return false;
  }

  return true;
}

}  // namespace


JSONSchemaValidator::Error::Error() {
}

JSONSchemaValidator::Error::Error(const std::string& message)
    : path(message) {
}

JSONSchemaValidator::Error::Error(const std::string& path,
                                  const std::string& message)
    : path(path), message(message) {
}


const char JSONSchemaValidator::kUnknownTypeReference[] =
    "Unknown schema reference: *.";
const char JSONSchemaValidator::kInvalidChoice[] =
    "Value does not match any valid type choices.";
const char JSONSchemaValidator::kInvalidEnum[] =
    "Value does not match any valid enum choices.";
const char JSONSchemaValidator::kObjectPropertyIsRequired[] =
    "Property is required.";
const char JSONSchemaValidator::kUnexpectedProperty[] =
    "Unexpected property.";
const char JSONSchemaValidator::kArrayMinItems[] =
    "Array must have at least * items.";
const char JSONSchemaValidator::kArrayMaxItems[] =
    "Array must not have more than * items.";
const char JSONSchemaValidator::kArrayItemRequired[] =
    "Item is required.";
const char JSONSchemaValidator::kStringMinLength[] =
    "String must be at least * characters long.";
const char JSONSchemaValidator::kStringMaxLength[] =
    "String must not be more than * characters long.";
const char JSONSchemaValidator::kStringPattern[] =
    "String must match the pattern: *.";
const char JSONSchemaValidator::kNumberMinimum[] =
    "Value must not be less than *.";
const char JSONSchemaValidator::kNumberMaximum[] =
    "Value must not be greater than *.";
const char JSONSchemaValidator::kInvalidType[] =
    "Expected '*' but got '*'.";
const char JSONSchemaValidator::kInvalidTypeIntegerNumber[] =
    "Expected 'integer' but got 'number', consider using Math.round().";
const char JSONSchemaValidator::kInvalidRegex[] =
    "Regular expression /*/ is invalid: *";


// static
std::string JSONSchemaValidator::GetJSONSchemaType(const base::Value* value) {
  switch (value->type()) {
    case base::Value::Type::NONE:
      return schema::kNull;
    case base::Value::Type::BOOLEAN:
      return schema::kBoolean;
    case base::Value::Type::INTEGER:
      return schema::kInteger;
    case base::Value::Type::DOUBLE: {
      double double_value = 0;
      value->GetAsDouble(&double_value);
      if (std::abs(double_value) <= std::pow(2.0, DBL_MANT_DIG) &&
          double_value == floor(double_value)) {
        return schema::kInteger;
      }
      return schema::kNumber;
    }
    case base::Value::Type::STRING:
      return schema::kString;
    case base::Value::Type::DICTIONARY:
      return schema::kObject;
    case base::Value::Type::LIST:
      return schema::kArray;
    default:
      NOTREACHED() << "Unexpected value type: " << value->type();
      return std::string();
  }
}

// static
std::string JSONSchemaValidator::FormatErrorMessage(const std::string& format,
                                                    const std::string& s1) {
  std::string ret_val = format;
  base::ReplaceFirstSubstringAfterOffset(&ret_val, 0, "*", s1);
  return ret_val;
}

// static
std::string JSONSchemaValidator::FormatErrorMessage(const std::string& format,
                                                    const std::string& s1,
                                                    const std::string& s2) {
  std::string ret_val = format;
  base::ReplaceFirstSubstringAfterOffset(&ret_val, 0, "*", s1);
  base::ReplaceFirstSubstringAfterOffset(&ret_val, 0, "*", s2);
  return ret_val;
}

// static
std::unique_ptr<base::DictionaryValue> JSONSchemaValidator::IsValidSchema(
    const std::string& schema,
    std::string* error) {
  return JSONSchemaValidator::IsValidSchema(schema, 0, error);
}

// static
std::unique_ptr<base::DictionaryValue> JSONSchemaValidator::IsValidSchema(
    const std::string& schema,
    int validator_options,
    std::string* error) {
  base::JSONParserOptions json_options = base::JSON_PARSE_RFC;
  std::unique_ptr<base::Value> json = base::JSONReader::ReadAndReturnError(
      schema, json_options, nullptr, error);
  if (!json)
    return std::unique_ptr<base::DictionaryValue>();
  base::DictionaryValue* dict = nullptr;
  if (!json->GetAsDictionary(&dict)) {
    *error = "Schema must be a JSON object";
    return std::unique_ptr<base::DictionaryValue>();
  }
  if (!::IsValidSchema(dict, validator_options, error))
    return std::unique_ptr<base::DictionaryValue>();
  ignore_result(json.release());
  return base::WrapUnique(dict);
}

JSONSchemaValidator::JSONSchemaValidator(base::DictionaryValue* schema)
    : schema_root_(schema), default_allow_additional_properties_(false) {
}

JSONSchemaValidator::JSONSchemaValidator(base::DictionaryValue* schema,
                                         base::ListValue* types)
    : schema_root_(schema), default_allow_additional_properties_(false) {
  if (!types)
    return;

  for (size_t i = 0; i < types->GetSize(); ++i) {
    base::DictionaryValue* type = nullptr;
    CHECK(types->GetDictionary(i, &type));

    std::string id;
    CHECK(type->GetString(schema::kId, &id));

    CHECK(types_.find(id) == types_.end());
    types_[id] = type;
  }
}

JSONSchemaValidator::~JSONSchemaValidator() {}

bool JSONSchemaValidator::Validate(const base::Value* instance) {
  errors_.clear();
  Validate(instance, schema_root_, std::string());
  return errors_.empty();
}

void JSONSchemaValidator::Validate(const base::Value* instance,
                                   const base::DictionaryValue* schema,
                                   const std::string& path) {
  // If this schema defines itself as reference type, save it in this.types.
  std::string id;
  if (schema->GetString(schema::kId, &id)) {
    TypeMap::iterator iter = types_.find(id);
    if (iter == types_.end())
      types_[id] = schema;
    else
      DCHECK(iter->second == schema);
  }

  // If the schema has a $ref property, the instance must validate against
  // that schema. It must be present in types_ to be referenced.
  std::string ref;
  if (schema->GetString(schema::kRef, &ref)) {
    TypeMap::iterator type = types_.find(ref);
    if (type == types_.end()) {
      errors_.push_back(
          Error(path, FormatErrorMessage(kUnknownTypeReference, ref)));
    } else {
      Validate(instance, type->second, path);
    }
    return;
  }

  // If the schema has a choices property, the instance must validate against at
  // least one of the items in that array.
  const base::ListValue* choices = nullptr;
  if (schema->GetList(schema::kChoices, &choices)) {
    ValidateChoices(instance, choices, path);
    return;
  }

  // If the schema has an enum property, the instance must be one of those
  // values.
  const base::ListValue* enumeration = nullptr;
  if (schema->GetList(schema::kEnum, &enumeration)) {
    ValidateEnum(instance, enumeration, path);
    return;
  }

  std::string type;
  schema->GetString(schema::kType, &type);
  CHECK(!type.empty());
  if (type != schema::kAny) {
    if (!ValidateType(instance, type, path))
      return;

    // These casts are safe because of checks in ValidateType().
    if (type == schema::kObject) {
      ValidateObject(static_cast<const base::DictionaryValue*>(instance),
                     schema,
                     path);
    } else if (type == schema::kArray) {
      ValidateArray(static_cast<const base::ListValue*>(instance),
                    schema, path);
    } else if (type == schema::kString) {
      // Intentionally NOT downcasting to StringValue*. Type::STRING only
      // implies GetAsString() can safely be carried out, not that it's a
      // StringValue.
      ValidateString(instance, schema, path);
    } else if (type == schema::kNumber || type == schema::kInteger) {
      ValidateNumber(instance, schema, path);
    } else if (type != schema::kBoolean && type != schema::kNull) {
      NOTREACHED() << "Unexpected type: " << type;
    }
  }
}

void JSONSchemaValidator::ValidateChoices(const base::Value* instance,
                                          const base::ListValue* choices,
                                          const std::string& path) {
  size_t original_num_errors = errors_.size();

  for (size_t i = 0; i < choices->GetSize(); ++i) {
    const base::DictionaryValue* choice = nullptr;
    CHECK(choices->GetDictionary(i, &choice));

    Validate(instance, choice, path);
    if (errors_.size() == original_num_errors)
      return;

    // We discard the error from each choice. We only want to know if any of the
    // validations succeeded.
    errors_.resize(original_num_errors);
  }

  // Now add a generic error that no choices matched.
  errors_.push_back(Error(path, kInvalidChoice));
  return;
}

void JSONSchemaValidator::ValidateEnum(const base::Value* instance,
                                       const base::ListValue* choices,
                                       const std::string& path) {
  for (size_t i = 0; i < choices->GetSize(); ++i) {
    const base::Value* choice = nullptr;
    CHECK(choices->Get(i, &choice));
    // Sometimes the enum declaration is a dictionary with the enum value under
    // "name".
    choice = ExtractNameFromDictionary(choice);
    if (!choice) {
      NOTREACHED();
    }
    switch (choice->type()) {
      case base::Value::Type::NONE:
      case base::Value::Type::BOOLEAN:
      case base::Value::Type::STRING:
        if (instance->Equals(choice))
          return;
        break;

      case base::Value::Type::INTEGER:
      case base::Value::Type::DOUBLE:
        if (instance->is_int() || instance->is_double()) {
          if (GetNumberValue(choice) == GetNumberValue(instance))
            return;
        }
        break;

      default:
        NOTREACHED() << "Unexpected type in enum: " << choice->type();
    }
  }

  errors_.push_back(Error(path, kInvalidEnum));
}

void JSONSchemaValidator::ValidateObject(const base::DictionaryValue* instance,
                                         const base::DictionaryValue* schema,
                                         const std::string& path) {
  const base::DictionaryValue* properties = nullptr;
  if (schema->GetDictionary(schema::kProperties, &properties)) {
    for (base::DictionaryValue::Iterator it(*properties); !it.IsAtEnd();
         it.Advance()) {
      std::string prop_path = path.empty() ? it.key() : (path + "." + it.key());
      const base::DictionaryValue* prop_schema = nullptr;
      CHECK(it.value().GetAsDictionary(&prop_schema));

      const base::Value* prop_value = nullptr;
      if (instance->Get(it.key(), &prop_value)) {
        Validate(prop_value, prop_schema, prop_path);
      } else {
        // Properties are required unless there is an optional field set to
        // 'true'.
        bool is_optional = false;
        prop_schema->GetBoolean(schema::kOptional, &is_optional);
        if (!is_optional) {
          errors_.push_back(Error(prop_path, kObjectPropertyIsRequired));
        }
      }
    }
  }

  const base::DictionaryValue* additional_properties_schema = nullptr;
  bool allow_any_additional_properties =
      SchemaAllowsAnyAdditionalItems(schema, &additional_properties_schema);

  const base::DictionaryValue* pattern_properties = nullptr;
  std::vector<std::unique_ptr<re2::RE2>> pattern_properties_pattern;
  std::vector<const base::DictionaryValue*> pattern_properties_schema;

  if (schema->GetDictionary(schema::kPatternProperties, &pattern_properties)) {
    for (base::DictionaryValue::Iterator it(*pattern_properties); !it.IsAtEnd();
         it.Advance()) {
      auto prop_pattern = std::make_unique<re2::RE2>(it.key());
      if (!prop_pattern->ok()) {
        LOG(WARNING) << "Regular expression /" << it.key()
                     << "/ is invalid: " << prop_pattern->error() << ".";
        errors_.push_back(
            Error(path,
                  FormatErrorMessage(
                      kInvalidRegex, it.key(), prop_pattern->error())));
        continue;
      }
      const base::DictionaryValue* prop_schema = nullptr;
      CHECK(it.value().GetAsDictionary(&prop_schema));
      pattern_properties_pattern.push_back(std::move(prop_pattern));
      pattern_properties_schema.push_back(prop_schema);
    }
  }

  // Validate pattern properties and additional properties.
  for (base::DictionaryValue::Iterator it(*instance); !it.IsAtEnd();
       it.Advance()) {
    std::string prop_path = path.empty() ? it.key() : path + "." + it.key();

    bool found_matching_pattern = false;
    for (size_t index = 0; index < pattern_properties_pattern.size(); ++index) {
      if (re2::RE2::PartialMatch(it.key(),
                                 *pattern_properties_pattern[index])) {
        found_matching_pattern = true;
        Validate(&it.value(), pattern_properties_schema[index], prop_path);
        break;
      }
    }

    if (found_matching_pattern || allow_any_additional_properties ||
        (properties && properties->HasKey(it.key())))
      continue;

    if (!additional_properties_schema) {
      errors_.push_back(Error(prop_path, kUnexpectedProperty));
    } else {
      Validate(&it.value(), additional_properties_schema, prop_path);
    }
  }
}

void JSONSchemaValidator::ValidateArray(const base::ListValue* instance,
                                        const base::DictionaryValue* schema,
                                        const std::string& path) {
  const base::DictionaryValue* single_type = nullptr;
  size_t instance_size = instance->GetSize();
  if (schema->GetDictionary(schema::kItems, &single_type)) {
    int min_items = 0;
    if (schema->GetInteger(schema::kMinItems, &min_items)) {
      CHECK(min_items >= 0);
      if (instance_size < static_cast<size_t>(min_items)) {
        errors_.push_back(Error(path, FormatErrorMessage(
            kArrayMinItems, base::IntToString(min_items))));
      }
    }

    int max_items = 0;
    if (schema->GetInteger(schema::kMaxItems, &max_items)) {
      CHECK(max_items >= 0);
      if (instance_size > static_cast<size_t>(max_items)) {
        errors_.push_back(Error(path, FormatErrorMessage(
            kArrayMaxItems, base::IntToString(max_items))));
      }
    }

    // If the items property is a single schema, each item in the array must
    // validate against that schema.
    for (size_t i = 0; i < instance_size; ++i) {
      const base::Value* item = nullptr;
      CHECK(instance->Get(i, &item));
      std::string i_str = base::NumberToString(i);
      std::string item_path = path.empty() ? i_str : (path + "." + i_str);
      Validate(item, single_type, item_path);
    }

    return;
  }

  // Otherwise, the list must be a tuple type, where each item in the list has a
  // particular schema.
  ValidateTuple(instance, schema, path);
}

void JSONSchemaValidator::ValidateTuple(const base::ListValue* instance,
                                        const base::DictionaryValue* schema,
                                        const std::string& path) {
  const base::ListValue* tuple_type = nullptr;
  schema->GetList(schema::kItems, &tuple_type);
  size_t tuple_size = tuple_type ? tuple_type->GetSize() : 0;
  if (tuple_type) {
    for (size_t i = 0; i < tuple_size; ++i) {
      std::string i_str = base::NumberToString(i);
      std::string item_path = path.empty() ? i_str : (path + "." + i_str);
      const base::DictionaryValue* item_schema = nullptr;
      CHECK(tuple_type->GetDictionary(i, &item_schema));
      const base::Value* item_value = nullptr;
      instance->Get(i, &item_value);
      if (item_value && item_value->type() != base::Value::Type::NONE) {
        Validate(item_value, item_schema, item_path);
      } else {
        bool is_optional = false;
        item_schema->GetBoolean(schema::kOptional, &is_optional);
        if (!is_optional) {
          errors_.push_back(Error(item_path, kArrayItemRequired));
          return;
        }
      }
    }
  }

  const base::DictionaryValue* additional_properties_schema = nullptr;
  if (SchemaAllowsAnyAdditionalItems(schema, &additional_properties_schema))
    return;

  size_t instance_size = instance->GetSize();
  if (additional_properties_schema) {
    // Any additional properties must validate against the additionalProperties
    // schema.
    for (size_t i = tuple_size; i < instance_size; ++i) {
      std::string i_str = base::NumberToString(i);
      std::string item_path = path.empty() ? i_str : (path + "." + i_str);
      const base::Value* item_value = nullptr;
      CHECK(instance->Get(i, &item_value));
      Validate(item_value, additional_properties_schema, item_path);
    }
  } else if (instance_size > tuple_size) {
    errors_.push_back(Error(
        path,
        FormatErrorMessage(kArrayMaxItems, base::NumberToString(tuple_size))));
  }
}

void JSONSchemaValidator::ValidateString(const base::Value* instance,
                                         const base::DictionaryValue* schema,
                                         const std::string& path) {
  std::string value;
  CHECK(instance->GetAsString(&value));

  int min_length = 0;
  if (schema->GetInteger(schema::kMinLength, &min_length)) {
    CHECK(min_length >= 0);
    if (value.size() < static_cast<size_t>(min_length)) {
      errors_.push_back(Error(path, FormatErrorMessage(
          kStringMinLength, base::IntToString(min_length))));
    }
  }

  int max_length = 0;
  if (schema->GetInteger(schema::kMaxLength, &max_length)) {
    CHECK(max_length >= 0);
    if (value.size() > static_cast<size_t>(max_length)) {
      errors_.push_back(Error(path, FormatErrorMessage(
          kStringMaxLength, base::IntToString(max_length))));
    }
  }

  std::string pattern;
  if (schema->GetString(schema::kPattern, &pattern)) {
    re2::RE2 compiled_regex(pattern);
    if (!compiled_regex.ok()) {
      LOG(WARNING) << "Regular expression /" << pattern
                   << "/ is invalid: " << compiled_regex.error() << ".";
      errors_.push_back(Error(
          path,
          FormatErrorMessage(kInvalidRegex, pattern, compiled_regex.error())));
    } else if (!re2::RE2::PartialMatch(value, compiled_regex)) {
      errors_.push_back(
          Error(path, FormatErrorMessage(kStringPattern, pattern)));
    }
  }
}

void JSONSchemaValidator::ValidateNumber(const base::Value* instance,
                                         const base::DictionaryValue* schema,
                                         const std::string& path) {
  double value = GetNumberValue(instance);

  // TODO(aa): It would be good to test that the double is not infinity or nan,
  // but isnan and isinf aren't defined on Windows.

  double minimum = 0;
  if (schema->GetDouble(schema::kMinimum, &minimum)) {
    if (value < minimum)
      errors_.push_back(Error(
          path,
          FormatErrorMessage(kNumberMinimum, base::NumberToString(minimum))));
  }

  double maximum = 0;
  if (schema->GetDouble(schema::kMaximum, &maximum)) {
    if (value > maximum)
      errors_.push_back(Error(
          path,
          FormatErrorMessage(kNumberMaximum, base::NumberToString(maximum))));
  }
}

bool JSONSchemaValidator::ValidateType(const base::Value* instance,
                                       const std::string& expected_type,
                                       const std::string& path) {
  std::string actual_type = GetJSONSchemaType(instance);
  if (expected_type == actual_type ||
      (expected_type == schema::kNumber && actual_type == schema::kInteger)) {
    return true;
  }
  if (expected_type == schema::kInteger && actual_type == schema::kNumber) {
    errors_.push_back(Error(path, kInvalidTypeIntegerNumber));
    return false;
  }
  errors_.push_back(Error(
      path, FormatErrorMessage(kInvalidType, expected_type, actual_type)));
  return false;
}

bool JSONSchemaValidator::SchemaAllowsAnyAdditionalItems(
    const base::DictionaryValue* schema,
    const base::DictionaryValue** additional_properties_schema) {
  // If the validator allows additional properties globally, and this schema
  // doesn't override, then we can exit early.
  schema->GetDictionary(schema::kAdditionalProperties,
                        additional_properties_schema);

  if (*additional_properties_schema) {
    std::string additional_properties_type(schema::kAny);
    CHECK((*additional_properties_schema)->GetString(
        schema::kType, &additional_properties_type));
    return additional_properties_type == schema::kAny;
  }
  return default_allow_additional_properties_;
}
