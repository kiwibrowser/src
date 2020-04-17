// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_CDDL_SEMA_H_
#define TOOLS_CDDL_SEMA_H_

#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/types/optional.h"
#include "tools/cddl/parse.h"

struct CddlGroup;

// Represents a type defined in CDDL.
struct CddlType {
  // Type of assignment being represented by this CDDL node.
  enum class Which {
    kDirectChoice,
    kValue,
    kId,
    kMap,
    kArray,
    kGroupChoice,
    kGroupnameChoice,
    kTaggedType,
  };
  enum class Op {
    kNone,            // not specified
    kInclusiveRange,  // ..
    kExclusiveRange,  // ...
    kSize,            // .size
    kBits,            // .bits
    kRegexp,          // .regexp
    kCbor,            // .cbor
    kCborseq,         // .cborseq
    kWithin,          // .within
    kAnd,             // .and
    kLess,            // .lt
    kLessOrEqual,     // .lt
    kGreater,         // .gt
    kGreaterOrEqual,  // .ge
    kEqual,           // .eq
    kNotEqual,        // .ne
    kDefault,         // .default
  };
  struct TaggedType {
    uint64_t tag_value;
    CddlType* type;
  };
  CddlType();
  ~CddlType();

  Which which;
  union {
    // A direct choice comes from the CDDL syntax "a-type / b-type / c-type".
    // This is in contrast to a "group choice".
    std::vector<CddlType*> direct_choice;

    // A literal value (number, text, or bytes) stored as its original string
    // form.
    std::string value;

    std::string id;
    CddlGroup* map;
    CddlGroup* array;

    // A group choice comes from the CDDL syntax:
    // a-group = (
    //   key1: uint, key2: text //
    //   key3: float, key4: bytes //
    //   key5: text
    // )
    CddlGroup* group_choice;
    TaggedType tagged_type;
  };

  Op op;
  CddlType* constraint_type;
  absl::optional<uint64_t> type_key;
};

// Override for << operator to simplify logging.
// NOTE: If a new enum value is added without modifying this operator, it will
// lead to a compilation failure because an enum class is used.
inline std::ostream& operator<<(std::ostream& os,
                                const CddlType::Which& which) {
  switch (which) {
    case CddlType::Which::kDirectChoice:
      os << "kDirectChoice";
      break;
    case CddlType::Which::kValue:
      os << "kValue";
      break;
    case CddlType::Which::kId:
      os << "kId";
      break;
    case CddlType::Which::kMap:
      os << "kMap";
      break;
    case CddlType::Which::kArray:
      os << "kArray";
      break;
    case CddlType::Which::kGroupChoice:
      os << "kGroupChoice";
      break;
    case CddlType::Which::kGroupnameChoice:
      os << "kGroupnameChoice";
      break;
    case CddlType::Which::kTaggedType:
      os << "kTaggedType";
      break;
  }
  return os;
}

// Represets a group defined in CDDL.
// TODO(btolsch): group choices
struct CddlGroup {
  struct Entry {
    enum class Which {
      kUninitialized = 0,
      kType,
      kGroup,
    };
    struct EntryType {
      std::string opt_key;
      absl::optional<uint64_t> integer_key;
      CddlType* value;
    };
    Entry();
    ~Entry();

    // Minimum number of times that this entry can be repeated.
    uint32_t opt_occurrence_min;

    // Maximum number of times that this entry can be repeated.
    uint32_t opt_occurrence_max;

    // Signifies whether an occurrence opperator is present or not.
    bool occurrence_specified;

    // Value to represent when opt_occurrence_min is unbounded.
    static constexpr uint32_t kOccurrenceMinUnbounded = 0;

    // Value to represent when opt_occurrence_max is unbounded.
    static constexpr uint32_t kOccurrenceMaxUnbounded =
        std::numeric_limits<uint32_t>::max();

    Which which = Which::kUninitialized;
    union {
      EntryType type;
      CddlGroup* group;
    };

    bool HasOccurrenceOperator() const { return occurrence_specified; }
  };

  std::vector<std::unique_ptr<Entry>> entries;
};

// Override for << operator to simplify logging.
// NOTE: If a new enum value is added without modifying this operator, it will
// lead to a compilation failure because an enum class is used.
inline std::ostream& operator<<(std::ostream& os,
                                const CddlGroup::Entry::Which& which) {
  switch (which) {
    case CddlGroup::Entry::Which::kUninitialized:
      os << "kUninitialized";
      break;
    case CddlGroup::Entry::Which::kType:
      os << "kType";
      break;
    case CddlGroup::Entry::Which::kGroup:
      os << "kGroup";
      break;
  }
  return os;
}

// Represents all CDDL definitions.
struct CddlSymbolTable {
  // Set of all CDDL types.
  std::vector<std::unique_ptr<CddlType>> types;

  // Set of all CDDL groups.
  std::vector<std::unique_ptr<CddlGroup>> groups;

  // Map from name of a type to the object that represents it.
  std::map<std::string, CddlType*> type_map;

  // Map from name of a group to the object that represents it.
  std::map<std::string, CddlGroup*> group_map;
};

// Represents a C++ Type, as translated from CDDL.
struct CppType {
  // Data type for this C++ type.
  enum class Which {
    kUninitialized = 0,
    kUint64,
    kString,
    kBytes,
    kVector,
    kEnum,
    kStruct,
    kOptional,
    kDiscriminatedUnion,
    kTaggedType,
  };

  struct Vector {
    CppType* element_type;

    // Minimum length for the vector.
    uint32_t min_length;

    // Maximum length for the vector.
    uint32_t max_length;

    // Value to represent when opt_occurrence_min is unbounded.
    static constexpr uint32_t kMinLengthUnbounded =
        CddlGroup::Entry::kOccurrenceMinUnbounded;

    // Value to represent when opt_occurrence_max is unbounded.
    static constexpr uint32_t kMaxLengthUnbounded =
        CddlGroup::Entry::kOccurrenceMaxUnbounded;
  };

  struct Enum {
    std::string name;
    std::vector<CppType*> sub_members;
    std::vector<std::pair<std::string, uint64_t>> members;
  };

  // Represents a C++ Struct.
  struct Struct {
    enum class KeyType {
      kMap,
      kArray,
      kPlainGroup,
    };
    // Contains a member of a C++ Struct.
    struct CppMember {
      // Constructs a new CppMember from the required fields. This constructor
      // is needed for vector::emplace_back(...).
      CppMember(std::string name,
                absl::optional<uint64_t> integer_key,
                CppType* type) {
        this->name = std::move(name);
        this->integer_key = integer_key;
        this->type = type;
      }

      // Name visible to callers of the generated C++ methods.
      std::string name;

      // When present, this key is used in place of the name for serialialized
      // messages. This should only be the case for integer-keyed group entries.
      absl::optional<uint64_t> integer_key;

      // C++ Type this member represents.
      CppType* type;
    };

    // Set of all members in this Struct.
    std::vector<CppMember> members;

    // Type of data structure being represented.
    KeyType key_type;
  };

  struct DiscriminatedUnion {
    std::vector<CppType*> members;
  };

  struct Bytes {
    absl::optional<size_t> fixed_size;
  };

  struct TaggedType {
    uint64_t tag;
    CppType* real_type;
  };

  CppType();
  ~CppType();

  void InitVector();
  void InitEnum();
  void InitStruct();
  void InitDiscriminatedUnion();
  void InitBytes();

  Which which = Which::kUninitialized;
  std::string name;
  absl::optional<uint64_t> type_key;
  union {
    Vector vector_type;
    Enum enum_type;
    Struct struct_type;
    CppType* optional_type;
    DiscriminatedUnion discriminated_union;
    Bytes bytes_type;
    TaggedType tagged_type;
  };
};

// Override for << operator to simplify logging.
// NOTE: If a new enum value is added without modifying this operator, it will
// lead to a compilation failure because an enum class is used.
inline std::ostream& operator<<(std::ostream& os, const CppType::Which& which) {
  switch (which) {
    case CppType::Which::kUint64:
      os << "kUint64";
      break;
    case CppType::Which::kString:
      os << "kString";
      break;
    case CppType::Which::kBytes:
      os << "kBytes";
      break;
    case CppType::Which::kVector:
      os << "kVector";
      break;
    case CppType::Which::kEnum:
      os << "kEnum";
      break;
    case CppType::Which::kStruct:
      os << "kStruct";
      break;
    case CppType::Which::kOptional:
      os << "kOptional";
      break;
    case CppType::Which::kDiscriminatedUnion:
      os << "kDiscriminatedUnion";
      break;
    case CppType::Which::kTaggedType:
      os << "kTaggedType";
      break;
    case CppType::Which::kUninitialized:
      os << "kUninitialized";
      break;
  }
  return os;
}

struct CppSymbolTable {
 public:
  std::vector<std::unique_ptr<CppType>> cpp_types;
  std::map<std::string, CppType*> cpp_type_map;

  std::vector<CppType*> TypesWithId();

 private:
  std::vector<CppType*> TypesWithId_;
};

std::pair<bool, CddlSymbolTable> BuildSymbolTable(const AstNode& rules);
std::pair<bool, CppSymbolTable> BuildCppTypes(
    const CddlSymbolTable& cddl_table);
bool ValidateCppTypes(const CppSymbolTable& cpp_symbols);
void DumpType(CddlType* type, int indent_level = 0);
void DumpGroup(CddlGroup* group, int indent_level = 0);
void DumpSymbolTable(CddlSymbolTable* table);

#endif  // TOOLS_CDDL_SEMA_H_
