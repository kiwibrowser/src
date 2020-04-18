// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_CHROME_CLEANER_STRINGS_STRING_UTIL_H_
#define CHROME_CHROME_CLEANER_STRINGS_STRING_UTIL_H_

#include <wchar.h>

#include <set>
#include <string>

#include "base/strings/string16.h"

namespace chrome_cleaner {

struct String16InsensitiveLess {
  bool operator()(const base::string16& str1,
                  const base::string16& str2) const {
    return _wcsicmp(str1.c_str(), str2.c_str()) < 0;
  }
};

typedef std::set<base::string16, String16InsensitiveLess>
    String16CaseInsensitiveSet;

// A function that returns true if |pattern| matches |value|, for some
// definition of "matches".
typedef bool (*String16Matcher)(const base::string16& value,
                                const base::string16& pattern);

// Returns true when both strings are equal, ignoring the string case.
bool String16EqualsCaseInsensitive(const base::string16& str1,
                                   const base::string16& str2);

// Returns true when |value| contains an occurrence of |substring|, ignoring
// the string case.
bool String16ContainsCaseInsensitive(const base::string16& value,
                                     const base::string16& substring);

// Splits |value| into a set of strings separated by any characters in
// |delimiters|, and returns true if any entry of the set is matched by
// |matcher|.
bool String16SetMatchEntry(const base::string16& value,
                           const base::string16& delimiters,
                           const base::string16& substring,
                           String16Matcher matcher);

// Returns true if |text| matches |pattern|. |pattern| can contain can
// wild-cards like * and ?. |escape_char| is an escape character for * and ?.
// The ? wild-card matches exactly 1 character, while the * wild-card matches 0
// or more characters.
bool String16WildcardMatchInsensitive(const base::string16& text,
                                      const base::string16& pattern,
                                      const wchar_t escape_char);

// Returns a copy of |input| without any invalid UTF8 characters.
std::string RemoveInvalidUTF8Chars(const std::string& input);

}  // namespace chrome_cleaner

#endif  // CHROME_CHROME_CLEANER_STRINGS_STRING_UTIL_H_
