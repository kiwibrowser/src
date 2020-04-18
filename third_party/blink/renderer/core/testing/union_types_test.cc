// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/testing/union_types_test.h"

#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"

namespace blink {

void UnionTypesTest::doubleOrStringOrStringSequenceAttribute(
    DoubleOrStringOrStringSequence& double_or_string_or_string_sequence) {
  switch (attribute_type_) {
    case kSpecificTypeNone:
      // Default value is zero (of double).
      double_or_string_or_string_sequence.SetDouble(0);
      break;
    case kSpecificTypeDouble:
      double_or_string_or_string_sequence.SetDouble(attribute_double_);
      break;
    case kSpecificTypeString:
      double_or_string_or_string_sequence.SetString(attribute_string_);
      break;
    case kSpecificTypeStringSequence:
      double_or_string_or_string_sequence.SetStringSequence(
          attribute_string_sequence_);
      break;
    default:
      NOTREACHED();
  }
}

void UnionTypesTest::setDoubleOrStringOrStringSequenceAttribute(
    const DoubleOrStringOrStringSequence& double_or_string_or_string_sequence) {
  if (double_or_string_or_string_sequence.IsDouble()) {
    attribute_double_ = double_or_string_or_string_sequence.GetAsDouble();
    attribute_type_ = kSpecificTypeDouble;
  } else if (double_or_string_or_string_sequence.IsString()) {
    attribute_string_ = double_or_string_or_string_sequence.GetAsString();
    attribute_type_ = kSpecificTypeString;
  } else if (double_or_string_or_string_sequence.IsStringSequence()) {
    attribute_string_sequence_ =
        double_or_string_or_string_sequence.GetAsStringSequence();
    attribute_type_ = kSpecificTypeStringSequence;
  } else {
    NOTREACHED();
  }
}

String UnionTypesTest::doubleOrStringArg(DoubleOrString& double_or_string) {
  if (double_or_string.IsNull())
    return "null is passed";
  if (double_or_string.IsDouble()) {
    return "double is passed: " +
           String::NumberToStringECMAScript(double_or_string.GetAsDouble());
  }
  if (double_or_string.IsString())
    return "string is passed: " + double_or_string.GetAsString();
  NOTREACHED();
  return String();
}

String UnionTypesTest::doubleOrInternalEnumArg(
    DoubleOrInternalEnum& double_or_internal_enum) {
  if (double_or_internal_enum.IsDouble()) {
    return "double is passed: " + String::NumberToStringECMAScript(
                                      double_or_internal_enum.GetAsDouble());
  }
  if (double_or_internal_enum.IsInternalEnum()) {
    return "InternalEnum is passed: " +
           double_or_internal_enum.GetAsInternalEnum();
  }
  NOTREACHED();
  return String();
}

String UnionTypesTest::doubleOrStringSequenceArg(
    HeapVector<DoubleOrString>& sequence) {
  if (!sequence.size())
    return "";

  StringBuilder builder;
  for (DoubleOrString& double_or_string : sequence) {
    DCHECK(!double_or_string.IsNull());
    if (double_or_string.IsDouble()) {
      builder.Append("double: ");
      builder.Append(
          String::NumberToStringECMAScript(double_or_string.GetAsDouble()));
    } else if (double_or_string.IsString()) {
      builder.Append("string: ");
      builder.Append(double_or_string.GetAsString());
    } else {
      NOTREACHED();
    }
    builder.Append(", ");
  }
  return builder.Substring(0, builder.length() - 2);
}

String UnionTypesTest::nodeListOrElementArg(
    NodeListOrElement& node_list_or_element) {
  DCHECK(!node_list_or_element.IsNull());
  return nodeListOrElementOrNullArg(node_list_or_element);
}

String UnionTypesTest::nodeListOrElementOrNullArg(
    NodeListOrElement& node_list_or_element_or_null) {
  if (node_list_or_element_or_null.IsNull())
    return "null or undefined is passed";
  if (node_list_or_element_or_null.IsNodeList())
    return "nodelist is passed";
  if (node_list_or_element_or_null.IsElement())
    return "element is passed";
  NOTREACHED();
  return String();
}

String UnionTypesTest::doubleOrStringOrStringSequenceArg(
    const DoubleOrStringOrStringSequence& double_or_string_or_string_sequence) {
  if (double_or_string_or_string_sequence.IsNull())
    return "null";

  if (double_or_string_or_string_sequence.IsDouble()) {
    return "double: " + String::NumberToStringECMAScript(
                            double_or_string_or_string_sequence.GetAsDouble());
  }

  if (double_or_string_or_string_sequence.IsString())
    return "string: " + double_or_string_or_string_sequence.GetAsString();

  DCHECK(double_or_string_or_string_sequence.IsStringSequence());
  const Vector<String>& sequence =
      double_or_string_or_string_sequence.GetAsStringSequence();
  if (!sequence.size())
    return "sequence: []";
  StringBuilder builder;
  builder.Append("sequence: [");
  for (const String& item : sequence) {
    DCHECK(!item.IsNull());
    builder.Append(item);
    builder.Append(", ");
  }
  return builder.Substring(0, builder.length() - 2) + "]";
}

}  // namespace blink
