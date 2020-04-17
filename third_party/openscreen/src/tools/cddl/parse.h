// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_CDDL_PARSE_H_
#define TOOLS_CDDL_PARSE_H_

#include <stddef.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "absl/strings/string_view.h"
#include "absl/types/optional.h"

struct AstNode {
  // These types all correspond to types in the grammar, which can be found in
  // grammar.abnf.
  enum class Type {
    kRule,
    kTypename,
    kGroupname,
    kAssign,
    kAssignT,
    kAssignG,
    kType,
    kGrpent,
    kType1,
    kType2,
    kValue,
    kGroup,
    kUint,
    kDigit,
    kRangeop,
    kCtlop,
    kGrpchoice,
    kOccur,
    kMemberKey,
    kId,
    kNumber,
    kText,
    kBytes,
    kOther,
  };

  // A node that (along with its' sublings) represents some sub-section of the
  // text represented by this node.
  AstNode* children;

  // Pointer to the next sibling of this node in the siblings linked list.
  AstNode* sibling;

  // Type of node being represented.
  Type type;

  // Text parsed from the CDDL spec to create this node.
  std::string text;

  // Text parsed from another source but used when serializing this node.
  std::string integer_member_key_text;

  // Text parsed from the CDDL spec for the type key.
  absl::optional<std::string> type_key;
};

// Override for << operator to simplify logging.
// NOTE: If a new enum value is added without modifying this operator, it will
// lead to a compilation failure because an enum class is used.
inline std::ostream& operator<<(std::ostream& os, const AstNode::Type& which) {
  switch (which) {
    case AstNode::Type::kRule:
      os << "kRule";
      break;
    case AstNode::Type::kTypename:
      os << "kTypename";
      break;
    case AstNode::Type::kGroupname:
      os << "kGroupname";
      break;
    case AstNode::Type::kAssign:
      os << "kAssign";
      break;
    case AstNode::Type::kAssignT:
      os << "kAssignT";
      break;
    case AstNode::Type::kAssignG:
      os << "kAssignG";
      break;
    case AstNode::Type::kType:
      os << "kType";
      break;
    case AstNode::Type::kGrpent:
      os << "kGrpent";
      break;
    case AstNode::Type::kType1:
      os << "kType1";
      break;
    case AstNode::Type::kType2:
      os << "kType2";
      break;
    case AstNode::Type::kValue:
      os << "kValue";
      break;
    case AstNode::Type::kGroup:
      os << "kGroup";
      break;
    case AstNode::Type::kUint:
      os << "kUint";
      break;
    case AstNode::Type::kDigit:
      os << "kDigit";
      break;
    case AstNode::Type::kRangeop:
      os << "kRangeop";
      break;
    case AstNode::Type::kCtlop:
      os << "kCtlop";
      break;
    case AstNode::Type::kGrpchoice:
      os << "kGrpchoice";
      break;
    case AstNode::Type::kOccur:
      os << "kOccur";
      break;
    case AstNode::Type::kMemberKey:
      os << "kMemberKey";
      break;
    case AstNode::Type::kId:
      os << "kId";
      break;
    case AstNode::Type::kNumber:
      os << "kNumber";
      break;
    case AstNode::Type::kText:
      os << "kText";
      break;
    case AstNode::Type::kBytes:
      os << "kBytes";
      break;
    case AstNode::Type::kOther:
      os << "kOther";
      break;
  }
  return os;
}

struct ParseResult {
  AstNode* root;
  std::vector<std::unique_ptr<AstNode>> nodes;
};

ParseResult ParseCddl(absl::string_view data);
void DumpAst(AstNode* node, int indent_level = 0);

#endif  // TOOLS_CDDL_PARSE_H_
