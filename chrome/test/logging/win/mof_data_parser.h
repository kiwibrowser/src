// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_LOGGING_WIN_MOF_DATA_PARSER_H_
#define CHROME_TEST_LOGGING_WIN_MOF_DATA_PARSER_H_

#include <windows.h>
#include <evntrace.h>
#include <stddef.h>
#include <stdint.h>
#include <wmistr.h>

#include "base/strings/string_piece.h"

namespace logging_win {

// A parser for Mof data found in an EVENT_TRACE object as formatted by
// Chromium-related classes.  Instances have an implicit cursor that scans the
// data.  Callers invoke Read* methods to extract primitive data types values or
// pointers to complex data types (arrays and strings).  In the latter case, the
// pointers are only valid for the lifetime of the underlying event.
class MofDataParser {
 public:
  explicit MofDataParser(const EVENT_TRACE* event);

  bool ReadDWORD(DWORD* value) {
    return ReadPrimitive(value);
  }

  bool ReadInt(int* value) {
    return ReadPrimitive(value);
  }

  bool ReadPointer(intptr_t* value) {
    return ReadPrimitive(value);
  }

  // Populates |values| with a pointer to an array of |size| pointer-sized
  // values in the data.
  bool ReadPointerArray(DWORD size, const intptr_t** values) {
    return ReadPrimitiveArray(size, values);
  }

  // Populates |value| with a pointer to an arbitrary data structure at the
  // current position.
  template<typename T> bool ReadStructure(const T** value) {
    if (length_ < sizeof(**value))
      return false;
    *value = reinterpret_cast<const T*>(scan_);
    Advance(sizeof(**value));
    return true;
  }

  // Sets |value| such that it points to the string in the data at the current
  // position.  A trailing newline, if present, is not included in the returned
  // piece.  The returned piece is not null-terminated.
  bool ReadString(base::StringPiece* value);

  bool empty() { return length_ == 0; }

 private:
  void Advance(size_t num_bytes) {
    scan_ += num_bytes;
    length_ -= num_bytes;
  }

  template<typename T> bool ReadPrimitive(T* value) {
    if (length_ < sizeof(*value))
      return false;
    *value = *reinterpret_cast<const T*>(scan_);
    Advance(sizeof(*value));
    return true;
  }

  template<typename T> bool ReadPrimitiveArray(DWORD size, const T** values) {
    if (length_ < sizeof(**values) * size)
      return false;
    *values = reinterpret_cast<const T*>(scan_);
    Advance(sizeof(**values) * size);
    return true;
  }

  const uint8_t* scan_;
  uint32_t length_;
};

}  // namespace logging_win

#endif  // CHROME_TEST_LOGGING_WIN_MOF_DATA_PARSER_H_
