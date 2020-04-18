// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_CHROME_CLEANER_STRINGS_STRING16_EMBEDDED_NULLS_H_
#define CHROME_CHROME_CLEANER_STRINGS_STRING16_EMBEDDED_NULLS_H_

#include <stddef.h>
#include <stdint.h>

#include <vector>

#include "base/strings/string16.h"
#include "base/strings/string_piece.h"

namespace chrome_cleaner {

// Holds a string16 with embedded nulls, which is allowed in registry paths.
// Provides a method to access the string as a StringPiece16, in order to
// prevent potential errors due to accidental uses of c_str().
class String16EmbeddedNulls {
 public:
  typedef const wchar_t* const_iterator;

  String16EmbeddedNulls();
  explicit String16EmbeddedNulls(nullptr_t);
  String16EmbeddedNulls(const String16EmbeddedNulls& str);
  String16EmbeddedNulls(const wchar_t* array, size_t size);
  explicit String16EmbeddedNulls(const std::vector<wchar_t>& str);
  explicit String16EmbeddedNulls(const base::string16& str);
  explicit String16EmbeddedNulls(base::StringPiece16 str);

  ~String16EmbeddedNulls();

  String16EmbeddedNulls& operator=(const String16EmbeddedNulls& str);
  bool operator==(const String16EmbeddedNulls& str) const;

  size_t size() const;

  // These methods don't create a copy of the underlying string. Make sure the
  // returned values don't outlive the current object.
  const base::StringPiece16 CastAsStringPiece16() const;
  const wchar_t* CastAsWCharArray() const;
  const uint16_t* CastAsUInt16Array() const;

 private:
  std::vector<wchar_t> data_;
};

}  // namespace chrome_cleaner

#endif  // CHROME_CHROME_CLEANER_STRINGS_STRING16_EMBEDDED_NULLS_H_
