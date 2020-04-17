// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/cddl/sema.h"

#include <string.h>
#include <unistd.h>

#include <cinttypes>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/strings/numbers.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "tools/cddl/logging.h"

std::vector<CppType*> CppSymbolTable::TypesWithId() {
  if (!this->TypesWithId_.size()) {
    for (const std::unique_ptr<CppType>& ptr : this->cpp_types) {
      if (ptr->type_key == absl::nullopt) {
        continue;
      }
      this->TypesWithId_.emplace_back(ptr.get());
    }
  }
  return this->TypesWithId_;
}

CddlType::CddlType()
    : map(nullptr), op(CddlType::Op::kNone), constraint_type(nullptr) {}
CddlType::~CddlType() {
  switch (which) {
    case CddlType::Which::kDirectChoice:
      direct_choice.std::vector<CddlType*>::~vector();
      break;
    case CddlType::Which::kValue:
      value.std::string::~basic_string();
      break;
    case CddlType::Which::kId:
      id.std::string::~basic_string();
      break;
    case CddlType::Which::kMap:
      break;
    case CddlType::Which::kArray:
      break;
    case CddlType::Which::kGroupChoice:
      break;
    case CddlType::Which::kGroupnameChoice:
      break;
    case CddlType::Which::kTaggedType:
      tagged_type.~TaggedType();
      break;
  }
}

CddlGroup::Entry::Entry() : group(nullptr) {}
CddlGroup::Entry::~Entry() {
  switch (which) {
    case CddlGroup::Entry::Which::kUninitialized:
      break;
    case CddlGroup::Entry::Which::kType:
      type.~EntryType();
      break;
    case CddlGroup::Entry::Which::kGroup:
      break;
  }
}

CppType::CppType() : vector_type() {}
CppType::~CppType() {
  switch (which) {
    case CppType::Which::kUninitialized:
      break;
    case CppType::Which::kUint64:
      break;
    case CppType::Which::kString:
      break;
    case CppType::Which::kBytes:
      break;
    case CppType::Which::kVector:
      break;
    case CppType::Which::kEnum:
      enum_type.~Enum();
      break;
    case CppType::Which::kStruct:
      struct_type.~Struct();
      break;
    case CppType::Which::kOptional:
      break;
    case CppType::Which::kDiscriminatedUnion:
      discriminated_union.~DiscriminatedUnion();
      break;
    case CppType::Which::kTaggedType:
      break;
  }
}

void CppType::InitVector() {
  which = Which::kVector;
  new (&vector_type) Vector();
}

void CppType::InitEnum() {
  which = Which::kEnum;
  new (&enum_type) Enum();
}

void CppType::InitStruct() {
  which = Which::kStruct;
  new (&struct_type) Struct();
}

void CppType::InitDiscriminatedUnion() {
  which = Which::kDiscriminatedUnion;
  new (&discriminated_union) DiscriminatedUnion();
}

void CppType::InitBytes() {
  which = Which::kBytes;
}

void InitString(std::string* s, absl::string_view value) {
  new (s) std::string(value);
}

void InitDirectChoice(std::vector<CddlType*>* direct_choice) {
  new (direct_choice) std::vector<CddlType*>();
}

void InitGroupEntry(CddlGroup::Entry::EntryType* entry) {
  new (entry) CddlGroup::Entry::EntryType();
}

CddlType* AddCddlType(CddlSymbolTable* table, CddlType::Which which) {
  table->types.emplace_back(new CddlType);
  CddlType* value = table->types.back().get();
  value->which = which;
  return value;
}

CddlType* AnalyzeType(CddlSymbolTable* table, const AstNode& type);
CddlGroup* AnalyzeGroup(CddlSymbolTable* table, const AstNode& group);

CddlType* AnalyzeType2(CddlSymbolTable* table, const AstNode& type2) {
  const AstNode* node = type2.children;
  if (node->type == AstNode::Type::kNumber ||
      node->type == AstNode::Type::kText ||
      node->type == AstNode::Type::kBytes) {
    CddlType* value = AddCddlType(table, CddlType::Which::kValue);
    InitString(&value->value, node->text);
    return value;
  } else if (node->type == AstNode::Type::kTypename) {
    if (type2.text[0] == '~') {
      dprintf(STDERR_FILENO, "We don't support the '~' operator.\n");
      return nullptr;
    }
    CddlType* id = AddCddlType(table, CddlType::Which::kId);
    InitString(&id->id, node->text);
    return id;
  } else if (node->type == AstNode::Type::kType) {
    if (type2.text[0] == '#' && type2.text[1] == '6' && type2.text[2] == '.') {
      CddlType* tagged_type = AddCddlType(table, CddlType::Which::kTaggedType);
      tagged_type->tagged_type.tag_value =
          atoll(type2.text.substr(3 /* #6. */).data());
      tagged_type->tagged_type.type = AnalyzeType(table, *node);
      return tagged_type;
    }
    dprintf(STDERR_FILENO, "Unknown type2 value, expected #6.[uint]\n");
  } else if (node->type == AstNode::Type::kGroup) {
    if (type2.text[0] == '{') {
      CddlType* map = AddCddlType(table, CddlType::Which::kMap);
      map->map = AnalyzeGroup(table, *node);
      return map;
    } else if (type2.text[0] == '[') {
      CddlType* array = AddCddlType(table, CddlType::Which::kArray);
      array->array = AnalyzeGroup(table, *node);
      return array;
    } else if (type2.text[0] == '&') {
      // Represents a choice between options in this group (ie an enum), not a
      // choice between groups (which is currently unsupported).
      CddlType* group_choice =
          AddCddlType(table, CddlType::Which::kGroupChoice);
      group_choice->group_choice = AnalyzeGroup(table, *node);
      return group_choice;
    }
  } else if (node->type == AstNode::Type::kGroupname) {
    if (type2.text[0] == '&') {
      CddlType* group_choice =
          AddCddlType(table, CddlType::Which::kGroupnameChoice);
      InitString(&group_choice->id, node->text);
      return group_choice;
    }
  }
  return nullptr;
}

CddlType::Op AnalyzeRangeop(const AstNode& rangeop) {
  if (rangeop.text == "..") {
    return CddlType::Op::kInclusiveRange;
  } else if (rangeop.text == "...") {
    return CddlType::Op::kExclusiveRange;
  } else {
    dprintf(STDERR_FILENO, "Unsupported '%s' range operator.\n",
            rangeop.text.c_str());
    return CddlType::Op::kNone;
  }
}

CddlType::Op AnalyzeCtlop(const AstNode& ctlop) {
  if (!ctlop.children) {
    dprintf(STDERR_FILENO, "Missing id for control operator '%s'.\n",
            ctlop.text.c_str());
    return CddlType::Op::kNone;
  }
  const std::string& id = ctlop.children->text;
  if (id == "size") {
    return CddlType::Op::kSize;
  } else if (id == "bits") {
    return CddlType::Op::kBits;
  } else if (id == "regexp") {
    return CddlType::Op::kRegexp;
  } else if (id == "cbor") {
    return CddlType::Op::kCbor;
  } else if (id == "cborseq") {
    return CddlType::Op::kCborseq;
  } else if (id == "within") {
    return CddlType::Op::kWithin;
  } else if (id == "and") {
    return CddlType::Op::kAnd;
  } else if (id == "lt") {
    return CddlType::Op::kLess;
  } else if (id == "le") {
    return CddlType::Op::kLessOrEqual;
  } else if (id == "gt") {
    return CddlType::Op::kGreater;
  } else if (id == "ge") {
    return CddlType::Op::kGreaterOrEqual;
  } else if (id == "eq") {
    return CddlType::Op::kEqual;
  } else if (id == "ne") {
    return CddlType::Op::kNotEqual;
  } else if (id == "default") {
    return CddlType::Op::kDefault;
  } else {
    dprintf(STDERR_FILENO, "Unsupported '%s' control operator.\n",
            ctlop.text.c_str());
    return CddlType::Op::kNone;
  }
}

// Produces CddlType by analyzing AST parsed from type1 rule
// ABNF rule: type1 = type2 [S (rangeop / ctlop) S type2]
CddlType* AnalyzeType1(CddlSymbolTable* table, const AstNode& type1) {
  if (!type1.children) {
    dprintf(STDERR_FILENO, "Missing type2 in type1 '%s'.\n",
            type1.text.c_str());
    return nullptr;
  }
  const AstNode& target_type = *type1.children;
  CddlType* analyzed_type = AnalyzeType2(table, target_type);
  if (!analyzed_type) {
    dprintf(STDERR_FILENO, "Invalid type2 '%s' in type1 '%s'.\n",
            target_type.text.c_str(), type1.text.c_str());
    return nullptr;
  }
  if (!target_type.sibling) {
    // No optional range or control operator, return type as-is
    return analyzed_type;
  }
  const AstNode& operator_type = *target_type.sibling;
  CddlType::Op op;
  if (operator_type.type == AstNode::Type::kRangeop) {
    op = AnalyzeRangeop(operator_type);
  } else if (operator_type.type == AstNode::Type::kCtlop) {
    op = AnalyzeCtlop(operator_type);
  } else {
    op = CddlType::Op::kNone;
  }
  if (op == CddlType::Op::kNone) {
    dprintf(STDERR_FILENO,
            "Unsupported or missing operator '%s' in type1 '%s'.\n",
            operator_type.text.c_str(), type1.text.c_str());
    return nullptr;
  }
  if (!operator_type.sibling) {
    dprintf(STDERR_FILENO,
            "Missing controller type for operator '%s' in type1 '%s'.\n",
            operator_type.text.c_str(), type1.text.c_str());
    return nullptr;
  }
  const AstNode& controller_type = *operator_type.sibling;
  CddlType* constraint_type = AnalyzeType2(table, controller_type);
  if (!constraint_type) {
    dprintf(STDERR_FILENO,
            "Invalid controller type '%s' for operator '%s' in type1 '%s'.\n",
            controller_type.text.c_str(), operator_type.text.c_str(),
            type1.text.c_str());
    return nullptr;
  }
  analyzed_type->op = op;
  analyzed_type->constraint_type = constraint_type;
  return analyzed_type;
}

CddlType* AnalyzeType(CddlSymbolTable* table, const AstNode& type) {
  const AstNode* type1 = type.children;
  if (type1->sibling) {
    // If the type we are looking at has a type choice, create a top-level
    // choice object, with a vector containing all valid choices.
    CddlType* type_choice = AddCddlType(table, CddlType::Which::kDirectChoice);
    InitDirectChoice(&type_choice->direct_choice);
    while (type1) {
      type_choice->direct_choice.push_back(AnalyzeType1(table, *type1));
      type1 = type1->sibling;
    }
    return type_choice;
  } else {
    // Else just return the single choice.
    return AnalyzeType1(table, *type1);
  }
}

bool AnalyzeGroupEntry(CddlSymbolTable* table,
                       const AstNode& group_entry,
                       CddlGroup::Entry* entry);

CddlGroup* AnalyzeGroup(CddlSymbolTable* table, const AstNode& group) {
  // NOTE: |group.children| is a grpchoice, which we don't currently handle.
  // Therefore, we assume it has no siblings and move on to immediately handling
  // its grpent children.
  const AstNode* node = group.children->children;
  table->groups.emplace_back(new CddlGroup);
  CddlGroup* group_def = table->groups.back().get();
  while (node) {
    group_def->entries.emplace_back(new CddlGroup::Entry);
    AnalyzeGroupEntry(table, *node, group_def->entries.back().get());
    node = node->sibling;
  }
  return group_def;
}

// Parses a string into an optional uint64_t, with the value being that
// represented by the string if it is present and nullopt if it cannot
// be parsed.
// TODO(rwkeane): Add support for hex and binary options.
absl::optional<uint64_t> ParseOptionalUint(const std::string& text) {
  if (text == "0") {
    return 0;
  }

  uint64_t parsed = std::strtoul(text.c_str(), nullptr, 10);
  if (!parsed) {
    return absl::nullopt;
  }
  return parsed;
}

bool AnalyzeGroupEntry(CddlSymbolTable* table,
                       const AstNode& group_entry,
                       CddlGroup::Entry* entry) {
  const AstNode* node = group_entry.children;

  // If it's an occurance operator (so the entry is optional), mark it as such
  // and proceed to the next the node.
  if (node->type == AstNode::Type::kOccur) {
    if (node->text == "?") {
      entry->opt_occurrence_min = CddlGroup::Entry::kOccurrenceMinUnbounded;
      entry->opt_occurrence_max = 1;
    } else if (node->text == "+") {
      entry->opt_occurrence_min = 1;
      entry->opt_occurrence_max = CddlGroup::Entry::kOccurrenceMaxUnbounded;
    } else {
      auto index = node->text.find('*');
      if (index == std::string::npos) {
        return false;
      }

      int lower_bound = CddlGroup::Entry::kOccurrenceMinUnbounded;
      std::string first_half = node->text.substr(0, index);
      if ((first_half.length() != 1 || first_half.at(0) != '0') &&
          first_half.length() != 0) {
        lower_bound = std::atoi(first_half.c_str());
        if (!lower_bound) {
          return false;
        }
      }

      int upper_bound = CddlGroup::Entry::kOccurrenceMaxUnbounded;
      std::string second_half =
          index >= node->text.length() ? "" : node->text.substr(index + 1);
      if ((second_half.length() != 1 || second_half.at(0) != '0') &&
          second_half.length() != 0) {
        upper_bound = std::atoi(second_half.c_str());
        if (!upper_bound) {
          return false;
        }
      }

      entry->opt_occurrence_min = lower_bound;
      entry->opt_occurrence_max = upper_bound;
    }
    entry->occurrence_specified = true;
    node = node->sibling;
  } else {
    entry->opt_occurrence_min = 1;
    entry->opt_occurrence_max = 1;
    entry->occurrence_specified = false;
  }

  // If it's a member key (key in a map), save it and go to next node.
  if (node->type == AstNode::Type::kMemberKey) {
    if (node->text[node->text.size() - 1] == '>')
      return false;
    entry->which = CddlGroup::Entry::Which::kType;
    InitGroupEntry(&entry->type);
    entry->type.opt_key = std::string(node->children->text);
    entry->type.integer_key = ParseOptionalUint(node->integer_member_key_text);
    node = node->sibling;
  }

  // If it's a type, process it as such.
  if (node->type == AstNode::Type::kType) {
    if (entry->which == CddlGroup::Entry::Which::kUninitialized) {
      entry->which = CddlGroup::Entry::Which::kType;
      InitGroupEntry(&entry->type);
    }
    entry->type.value = AnalyzeType(table, *node);
  } else if (node->type == AstNode::Type::kGroupname) {
    return false;
  } else if (node->type == AstNode::Type::kGroup) {
    entry->which = CddlGroup::Entry::Which::kGroup;
    entry->group = AnalyzeGroup(table, *node);
  }
  return true;
}

std::pair<bool, CddlSymbolTable> BuildSymbolTable(const AstNode& rules) {
  std::pair<bool, CddlSymbolTable> result;
  result.first = false;
  auto& table = result.second;

  // Parse over all rules iteratively.
  for (const AstNode* rule = &rules; rule; rule = rule->sibling) {
    AstNode* node = rule->children;

    // Ensure that the node is either a type or group definition.
    if (node->type != AstNode::Type::kTypename &&
        node->type != AstNode::Type::kGroupname) {
      Logger::Error("Error parsing node with text '%s'. Unexpected node type.",
                    node->text);
      return result;
    }
    bool is_type = node->type == AstNode::Type::kTypename;
    absl::string_view name = node->text;

    // Ensure that the node is assignment.
    node = node->sibling;
    if (node->type != AstNode::Type::kAssign) {
      Logger::Error("Error parsing node with text '%s'. Node type != kAssign.",
                    node->text);
      return result;
    }

    // Process the definition.
    node = node->sibling;
    if (is_type) {
      CddlType* type = AnalyzeType(&table, *node);
      if (rule->type_key != absl::nullopt) {
        auto parsed_type_key = ParseOptionalUint(rule->type_key.value());
        if (parsed_type_key == absl::nullopt) {
          return result;
        }
        type->type_key = parsed_type_key.value();
      }
      if (!type) {
        Logger::Error(
            "Error parsing node with text '%s'."
            "Failed to analyze node type.",
            node->text);
      }
      table.type_map.emplace(std::string(name), type);
    } else {
      table.groups.emplace_back(new CddlGroup);
      CddlGroup* group = table.groups.back().get();
      group->entries.emplace_back(new CddlGroup::Entry);
      AnalyzeGroupEntry(&table, *node, group->entries.back().get());
      table.group_map.emplace(std::string(name), group);
    }
  }

  DumpSymbolTable(&result.second);

  result.first = true;
  return result;
}

// Fetches a C++ Type from all known definitons, or inserts a placeholder to be
// updated later if the type hasn't been defined yet.
CppType* GetCppType(CppSymbolTable* table, const std::string& name) {
  if (name.empty()) {
    table->cpp_types.emplace_back(new CppType);
    return table->cpp_types.back().get();
  }
  auto entry = table->cpp_type_map.find(name);
  if (entry != table->cpp_type_map.end())
    return entry->second;
  table->cpp_types.emplace_back(new CppType);
  table->cpp_type_map.emplace(name, table->cpp_types.back().get());
  return table->cpp_types.back().get();
}

bool IncludeGroupMembersInEnum(CppSymbolTable* table,
                               const CddlSymbolTable& cddl_table,
                               CppType* cpp_type,
                               const CddlGroup& group);

bool IncludeGroupMembersInSubEnum(CppSymbolTable* table,
                                  const CddlSymbolTable& cddl_table,
                                  CppType* cpp_type,
                                  const std::string& name) {
  auto group_entry = cddl_table.group_map.find(name);
  if (group_entry == cddl_table.group_map.end()) {
    return false;
  }
  if (group_entry->second->entries.size() != 1 ||
      group_entry->second->entries[0]->which !=
          CddlGroup::Entry::Which::kGroup) {
    return false;
  }
  CppType* sub_enum = GetCppType(table, name);
  if (sub_enum->which == CppType::Which::kUninitialized) {
    sub_enum->InitEnum();
    sub_enum->name = name;
    if (!IncludeGroupMembersInEnum(table, cddl_table, sub_enum,
                                   *group_entry->second->entries[0]->group)) {
      return false;
    }
  }
  cpp_type->enum_type.sub_members.push_back(sub_enum);
  return true;
}

bool IncludeGroupMembersInEnum(CppSymbolTable* table,
                               const CddlSymbolTable& cddl_table,
                               CppType* cpp_type,
                               const CddlGroup& group) {
  for (const auto& x : group.entries) {
    if (x->HasOccurrenceOperator() ||
        x->which != CddlGroup::Entry::Which::kType) {
      return false;
    }
    if (x->type.value->which == CddlType::Which::kValue &&
        !x->type.opt_key.empty()) {
      cpp_type->enum_type.members.emplace_back(
          x->type.opt_key, atoi(x->type.value->value.c_str()));
    } else if (x->type.value->which == CddlType::Which::kId) {
      IncludeGroupMembersInSubEnum(table, cddl_table, cpp_type,
                                   x->type.value->id);
    } else {
      return false;
    }
  }
  return true;
}

CppType* MakeCppType(CppSymbolTable* table,
                     const CddlSymbolTable& cddl_table,
                     const std::string& name,
                     const CddlType& type);

bool AddMembersToStruct(
    CppSymbolTable* table,
    const CddlSymbolTable& cddl_table,
    CppType* cpp_type,
    const std::vector<std::unique_ptr<CddlGroup::Entry>>& entries) {
  for (const auto& x : entries) {
    if (x->which == CddlGroup::Entry::Which::kType) {
      if (x->type.opt_key.empty()) {
        // If the represented node has no text (ie - it's code generated) then
        // it must have an inner type that is based on the user input. If this
        // one looks as expected, process it recursively.
        if (x->type.value->which != CddlType::Which::kId ||
            x->HasOccurrenceOperator()) {
          return false;
        }
        auto group_entry = cddl_table.group_map.find(x->type.value->id);
        if (group_entry == cddl_table.group_map.end())
          return false;
        if (group_entry->second->entries.size() != 1 ||
            group_entry->second->entries[0]->which !=
                CddlGroup::Entry::Which::kGroup) {
          return false;
        }
        if (!AddMembersToStruct(
                table, cddl_table, cpp_type,
                group_entry->second->entries[0]->group->entries)) {
          return false;
        }
      } else {
        // Here it is a real type definition - so process it as such.
        CppType* member_type =
            MakeCppType(table, cddl_table,
                        cpp_type->name + std::string("_") + x->type.opt_key,
                        *x->type.value);
        if (!member_type)
          return false;
        if (member_type->name.empty())
          member_type->name = x->type.opt_key;
        if (x->opt_occurrence_min ==
                CddlGroup::Entry::kOccurrenceMinUnbounded &&
            x->opt_occurrence_max == 1) {
          // Create an "optional" type, with sub-type being the type that is
          // optional. This corresponds with occurrence operator '?'.
          table->cpp_types.emplace_back(new CppType);
          CppType* optional_type = table->cpp_types.back().get();
          optional_type->which = CppType::Which::kOptional;
          optional_type->optional_type = member_type;
          cpp_type->struct_type.members.emplace_back(
              x->type.opt_key, x->type.integer_key, optional_type);
        } else {
          cpp_type->struct_type.members.emplace_back(
              x->type.opt_key, x->type.integer_key, member_type);
        }
      }
    } else {
      // If it's not a type, it's a group so add its members recursuvely.
      if (!AddMembersToStruct(table, cddl_table, cpp_type, x->group->entries))
        return false;
    }
  }
  return true;
}

CppType* MakeCppType(CppSymbolTable* table,
                     const CddlSymbolTable& cddl_table,
                     const std::string& name,
                     const CddlType& type) {
  CppType* cpp_type = nullptr;
  switch (type.which) {
    case CddlType::Which::kId: {
      if (type.id == "uint") {
        cpp_type = GetCppType(table, name);
        cpp_type->which = CppType::Which::kUint64;
      } else if (type.id == "text") {
        cpp_type = GetCppType(table, name);
        cpp_type->which = CppType::Which::kString;
      } else if (type.id == "bytes") {
        cpp_type = GetCppType(table, name);
        cpp_type->InitBytes();
        if (type.op == CddlType::Op::kSize) {
          size_t size = 0;
          if (!absl::SimpleAtoi(type.constraint_type->value, &size)) {
            return nullptr;
          }
          cpp_type->bytes_type.fixed_size = size;
        }
      } else {
        cpp_type = GetCppType(table, type.id);
      }
    } break;
    case CddlType::Which::kMap: {
      cpp_type = GetCppType(table, name);
      cpp_type->InitStruct();
      cpp_type->struct_type.key_type = CppType::Struct::KeyType::kMap;
      cpp_type->name = name;
      if (!AddMembersToStruct(table, cddl_table, cpp_type, type.map->entries))
        return nullptr;
    } break;
    case CddlType::Which::kArray: {
      cpp_type = GetCppType(table, name);
      if (type.array->entries.size() == 1 &&
          type.array->entries[0]->HasOccurrenceOperator()) {
        cpp_type->InitVector();
        cpp_type->vector_type.min_length =
            type.array->entries[0]->opt_occurrence_min;
        cpp_type->vector_type.max_length =
            type.array->entries[0]->opt_occurrence_max;
        cpp_type->vector_type.element_type =
            GetCppType(table, type.array->entries[0]->type.value->id);
      } else {
        cpp_type->InitStruct();
        cpp_type->struct_type.key_type = CppType::Struct::KeyType::kArray;
        cpp_type->name = name;
        if (!AddMembersToStruct(table, cddl_table, cpp_type,
                                type.map->entries)) {
          return nullptr;
        }
      }
    } break;
    case CddlType::Which::kGroupChoice: {
      cpp_type = GetCppType(table, name);
      cpp_type->InitEnum();
      cpp_type->name = name;
      if (!IncludeGroupMembersInEnum(table, cddl_table, cpp_type,
                                     *type.group_choice)) {
        return nullptr;
      }
    } break;
    case CddlType::Which::kGroupnameChoice: {
      cpp_type = GetCppType(table, name);
      cpp_type->InitEnum();
      cpp_type->name = name;
      if (!IncludeGroupMembersInSubEnum(table, cddl_table, cpp_type, type.id)) {
        return nullptr;
      }
    } break;
    case CddlType::Which::kDirectChoice: {
      cpp_type = GetCppType(table, name);
      cpp_type->InitDiscriminatedUnion();
      for (const auto* cddl_choice : type.direct_choice) {
        CppType* member = MakeCppType(table, cddl_table, "", *cddl_choice);
        if (!member)
          return nullptr;
        cpp_type->discriminated_union.members.push_back(member);
      }
      return cpp_type;
    } break;
    case CddlType::Which::kTaggedType: {
      cpp_type = GetCppType(table, name);
      cpp_type->which = CppType::Which::kTaggedType;
      cpp_type->tagged_type.tag = type.tagged_type.tag_value;
      cpp_type->tagged_type.real_type =
          MakeCppType(table, cddl_table, "", *type.tagged_type.type);
    } break;
    default:
      return nullptr;
  }

  cpp_type->type_key = type.type_key;
  return cpp_type;
}

void PrePopulateCppTypes(CppSymbolTable* table) {
  std::vector<std::pair<std::string, CppType::Which>> default_types;
  default_types.emplace_back("text", CppType::Which::kString);
  default_types.emplace_back("tstr", CppType::Which::kString);
  default_types.emplace_back("bstr", CppType::Which::kBytes);
  default_types.emplace_back("bytes", CppType::Which::kBytes);
  default_types.emplace_back("uint", CppType::Which::kUint64);

  for (auto& pair : default_types) {
    auto entry = table->cpp_type_map.find(pair.first);
    if (entry != table->cpp_type_map.end())
      continue;
    table->cpp_types.emplace_back(new CppType);
    auto* type = table->cpp_types.back().get();
    type->name = pair.first;
    type->which = pair.second;
    table->cpp_type_map.emplace(pair.first, type);
  }
}

std::pair<bool, CppSymbolTable> BuildCppTypes(
    const CddlSymbolTable& cddl_table) {
  std::pair<bool, CppSymbolTable> result;
  result.first = false;
  PrePopulateCppTypes(&result.second);
  auto& table = result.second;
  for (const auto& type_entry : cddl_table.type_map) {
    if (!MakeCppType(&table, cddl_table, type_entry.first,
                     *type_entry.second)) {
      return result;
    }
  }

  result.first = true;
  return result;
}

bool VerifyUniqueKeysInMember(std::unordered_set<std::string>* keys,
                              const CppType::Struct::CppMember& member) {
  return keys->insert(member.name).second &&
         (!member.integer_key.has_value() ||
          keys->insert(std::to_string(member.integer_key.value())).second);
}

bool HasUniqueKeys(const CppType& type) {
  std::unordered_set<std::string> keys;
  return type.which != CppType::Which::kStruct ||
         absl::c_all_of(type.struct_type.members,
                        [&keys](const CppType::Struct::CppMember& member) {
                          return VerifyUniqueKeysInMember(&keys, member);
                        });
}

bool IsUniqueEnumValue(std::vector<uint64_t>* values, uint64_t v) {
  auto it = std::lower_bound(values->begin(), values->end(), v);
  if (it == values->end() || *it != v) {
    values->insert(it, v);
    return true;
  }
  return false;
}

bool HasUniqueEnumValues(std::vector<uint64_t>* values, const CppType& type) {
  return absl::c_all_of(type.enum_type.sub_members,
                        [values](CppType* sub_member) {
                          return HasUniqueEnumValues(values, *sub_member);
                        }) &&
         absl::c_all_of(
             type.enum_type.members,
             [values](const std::pair<std::string, uint64_t>& member) {
               return IsUniqueEnumValue(values, member.second);
             });
}

bool HasUniqueEnumValues(const CppType& type) {
  std::vector<uint64_t> values;
  return type.which != CppType::Which::kEnum ||
         HasUniqueEnumValues(&values, type);
}

bool ValidateCppTypes(const CppSymbolTable& cpp_symbols) {
  return absl::c_all_of(
      cpp_symbols.cpp_types, [](const std::unique_ptr<CppType>& ptr) {
        return HasUniqueKeys(*ptr) && HasUniqueEnumValues(*ptr);
      });
}

std::string DumpTypeKey(absl::optional<uint64_t> key) {
  if (key != absl::nullopt) {
    return " (type key=\"" + std::to_string(key.value()) + "\")";
  }
  return "";
}

void DumpType(CddlType* type, int indent_level) {
  std::string output = "";
  for (int i = 0; i <= indent_level; ++i)
    output += "--";
  switch (type->which) {
    case CddlType::Which::kDirectChoice:
      output = "kDirectChoice" + DumpTypeKey(type->type_key) + ": ";
      Logger::Log(output);
      for (auto& option : type->direct_choice)
        DumpType(option, indent_level + 1);
      break;
    case CddlType::Which::kValue:
      output += "kValue" + DumpTypeKey(type->type_key) + ": " + type->value;
      Logger::Log(output);
      break;
    case CddlType::Which::kId:
      output += "kId" + DumpTypeKey(type->type_key) + ": " + type->id;
      Logger::Log(output);
      break;
    case CddlType::Which::kMap:
      output += "kMap" + DumpTypeKey(type->type_key) + ": ";
      Logger::Log(output);
      DumpGroup(type->map, indent_level + 1);
      break;
    case CddlType::Which::kArray:
      output += "kArray" + DumpTypeKey(type->type_key) + ": ";
      Logger::Log(output);
      DumpGroup(type->array, indent_level + 1);
      break;
    case CddlType::Which::kGroupChoice:
      output += "kGroupChoice" + DumpTypeKey(type->type_key) + ": ";
      Logger::Log(output);
      DumpGroup(type->group_choice, indent_level + 1);
      break;
    case CddlType::Which::kGroupnameChoice:
      output += "kGroupnameChoice" + DumpTypeKey(type->type_key) + ": ";
      Logger::Log(output);
      break;
    case CddlType::Which::kTaggedType:
      output += "kTaggedType" + DumpTypeKey(type->type_key) + ": " +
                std::to_string(type->tagged_type.tag_value);
      Logger::Log(output);
      DumpType(type->tagged_type.type, indent_level + 1);
      break;
  }
}

void DumpGroup(CddlGroup* group, int indent_level) {
  for (auto& entry : group->entries) {
    std::string output = "";
    for (int i = 0; i <= indent_level; ++i)
      output += "--";
    switch (entry->which) {
      case CddlGroup::Entry::Which::kUninitialized:
        break;
      case CddlGroup::Entry::Which::kType:
        output += "kType:";
        if (entry->HasOccurrenceOperator()) {
          output +=
              "minOccurance: " + std::to_string(entry->opt_occurrence_min) +
              " maxOccurance: " + std::to_string(entry->opt_occurrence_max);
        }
        if (!entry->type.opt_key.empty()) {
          output += " " + entry->type.opt_key + "=>";
        }
        Logger::Log(output);
        DumpType(entry->type.value, indent_level + 1);
        break;
      case CddlGroup::Entry::Which::kGroup:
        if (entry->HasOccurrenceOperator())
          output +=
              "minOccurance: " + std::to_string(entry->opt_occurrence_min) +
              " maxOccurance: " + std::to_string(entry->opt_occurrence_max);
        Logger::Log(output);
        DumpGroup(entry->group, indent_level + 1);
        break;
    }
  }
}

void DumpSymbolTable(CddlSymbolTable* table) {
  for (auto& entry : table->type_map) {
    Logger::Log(entry.first);
    DumpType(entry.second);
  }
  for (auto& entry : table->group_map) {
    Logger::Log(entry.first);
    DumpGroup(entry.second);
  }
}
