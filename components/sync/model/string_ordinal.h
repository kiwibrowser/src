// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_MODEL_STRING_ORDINAL_H_
#define COMPONENTS_SYNC_MODEL_STRING_ORDINAL_H_

#include <stddef.h>
#include <stdint.h>

#include "components/sync/base/ordinal.h"

namespace syncer {

// A StringOrdinal is an Ordinal with range 'a'-'z' for printability
// of its internal byte string representation.  (Think of
// StringOrdinal as being short for PrintableStringOrdinal.)  It
// should be used for data types that want to maintain one or more
// orderings for nodes.
//
// Since StringOrdinals contain only printable characters, it is safe
// to store as a string in a protobuf.

struct StringOrdinalTraits {
  static const uint8_t kZeroDigit = 'a';
  static const uint8_t kMaxDigit = 'z';
  static const size_t kMinLength = 1;
};

using StringOrdinal = Ordinal<StringOrdinalTraits>;

static_assert(StringOrdinal::kZeroDigit == 'a',
              "StringOrdinal has incorrect zero digit");
static_assert(StringOrdinal::kOneDigit == 'b',
              "StringOrdinal has incorrect one digit");
static_assert(StringOrdinal::kMidDigit == 'n',
              "StringOrdinal has incorrect mid digit");
static_assert(StringOrdinal::kMaxDigit == 'z',
              "StringOrdinal has incorrect max digit");
static_assert(StringOrdinal::kMidDigitValue == 13,
              "StringOrdinal has incorrect mid digit value");
static_assert(StringOrdinal::kMaxDigitValue == 25,
              "StringOrdinal has incorrect max digit value");
static_assert(StringOrdinal::kRadix == 26, "StringOrdinal has incorrect radix");

}  // namespace syncer

#endif  // COMPONENTS_SYNC_MODEL_STRING_ORDINAL_H_
