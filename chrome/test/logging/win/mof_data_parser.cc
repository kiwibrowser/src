// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/logging/win/mof_data_parser.h"

namespace logging_win {

MofDataParser::MofDataParser(const EVENT_TRACE* event)
    : scan_(reinterpret_cast<const uint8_t*>(event->MofData)),
      length_(event->MofLength) {}

bool MofDataParser::ReadString(base::StringPiece* value) {
  const uint8_t* str_scan = scan_;
  const uint8_t* const str_end = str_scan + length_;
  while (str_scan < str_end && *str_scan != 0)
    ++str_scan;
  if (str_scan == str_end)
    return false;
  size_t string_length = str_scan - scan_;
  bool has_trailing_newline = (string_length > 0 && str_scan[-1] == '\n');
  value->set(reinterpret_cast<const char*>(scan_),
    has_trailing_newline ? string_length - 1 : string_length);
  Advance(string_length + 1);
  return true;
}

}  // namespace logging_win
