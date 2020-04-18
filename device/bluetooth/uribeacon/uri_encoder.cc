// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "uri_encoder.h"

using base::StringPiece;

namespace {

struct expansion {
  uint8_t code;
  const char* value;
};

// The two following data structures are the expansions code tables for URI
// encoding described in the following specification:
// https://github.com/google/uribeacon/blob/master/specification/AdvertisingMode.md

// For the prefix of the URI.
struct expansion prefix_expansions_list[] = {
    {0, "http://www."},
    {1, "https://www."},
    {2, "http://"},
    {3, "https://"},
    {4, "urn:uuid:"},
};

// For the remaining part of the URI.
struct expansion expansions_list[] = {
    {0, ".com/"},
    {1, ".org/"},
    {2, ".edu/"},
    {3, ".net/"},
    {4, ".info/"},
    {5, ".biz/"},
    {6, ".gov/"},
    {7, ".com"},
    {8, ".org"},
    {9, ".edu"},
    {10, ".net"},
    {11, ".info"},
    {12, ".biz"},
    {13, ".gov"},
};

struct expansion* CommonLookupExpansionByValue(struct expansion* table,
                                               int table_length,
                                               const std::string& input,
                                               int input_index) {
  int found = -1;
  int found_length = -1;

  for (int k = 0; k < table_length; k++) {
    const char* value = table[k].value;
    int len = static_cast<int>(strlen(table[k].value));
    if (input_index + len <= static_cast<int>(input.size())) {
      if (len > found_length && strncmp(&input[input_index], value, len) == 0) {
        found = k;
        found_length = len;
      }
    }
  }
  if (found == -1)
    return NULL;
  return &table[found];
}

struct expansion* LookupExpansionByValue(const std::string& input,
                                         int input_index) {
  return CommonLookupExpansionByValue(
      expansions_list, arraysize(expansions_list), input, input_index);
}

struct expansion* LookupPrefixExpansionByValue(const std::string& input,
                                               int input_index) {
  return CommonLookupExpansionByValue(prefix_expansions_list,
                                      arraysize(prefix_expansions_list), input,
                                      input_index);
}

struct expansion* LookupExpansionByCode(const std::vector<uint8_t>& input,
                                        int input_index) {
  if (input[input_index] >= arraysize(expansions_list))
    return NULL;
  return &expansions_list[input[input_index]];
}

struct expansion* LookupPrefixExpansionByCode(const std::vector<uint8_t>& input,
                                              int input_index) {
  if (input[input_index] >= arraysize(prefix_expansions_list))
    return NULL;
  return &prefix_expansions_list[input[input_index]];
}

}  // namespace

void device::EncodeUriBeaconUri(const std::string& input,
                                std::vector<uint8_t>& output) {
  int i = 0;
  while (i < static_cast<int>(input.size())) {
    struct expansion* exp;
    if (i == 0)
      exp = LookupPrefixExpansionByValue(input, i);
    else
      exp = LookupExpansionByValue(input, i);
    if (exp == NULL) {
      output.push_back(static_cast<uint8_t>(input[i]));
      i++;
    } else {
      output.push_back(exp->code);
      i += static_cast<int>(strlen(exp->value));
    }
  }
}

void device::DecodeUriBeaconUri(const std::vector<uint8_t>& input,
                                std::string& output) {
  int length = static_cast<int>(input.size());
  for (int i = 0; i < length; i++) {
    struct expansion* exp;
    if (i == 0)
      exp = LookupPrefixExpansionByCode(input, i);
    else
      exp = LookupExpansionByCode(input, i);
    if (exp == NULL)
      output.push_back(static_cast<char>(input[i]));
    else
      output.append(exp->value);
  }
}
