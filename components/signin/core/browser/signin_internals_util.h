// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SIGNIN_CORE_BROWSER_SIGNIN_INTERNALS_UTIL_H_
#define COMPONENTS_SIGNIN_CORE_BROWSER_SIGNIN_INTERNALS_UTIL_H_

#include <stddef.h>

#include <map>
#include <memory>
#include <string>

#include "base/values.h"

namespace signin_internals_util {

// Preference prefixes for signin and token values.
extern const char kSigninPrefPrefix[];
extern const char kTokenPrefPrefix[];

// The length of strings returned by GetTruncatedHash() below.
const size_t kTruncateTokenStringLength = 6;

// Helper enums to access fields from SigninStatus (declared below).
enum {
  SIGNIN_FIELDS_BEGIN = 0,
  UNTIMED_FIELDS_BEGIN = SIGNIN_FIELDS_BEGIN
};

enum UntimedSigninStatusField {
  ACCOUNT_ID = UNTIMED_FIELDS_BEGIN,
  GAIA_ID,
  USERNAME,
  UNTIMED_FIELDS_END
};

enum {
  UNTIMED_FIELDS_COUNT = UNTIMED_FIELDS_END - UNTIMED_FIELDS_BEGIN,
  TIMED_FIELDS_BEGIN = UNTIMED_FIELDS_END
};

enum TimedSigninStatusField {
  AUTHENTICATION_RESULT_RECEIVED = TIMED_FIELDS_BEGIN,
  REFRESH_TOKEN_RECEIVED,
  SIGNIN_STARTED,
  SIGNIN_COMPLETED,
  TIMED_FIELDS_END
};

enum {
  TIMED_FIELDS_COUNT = TIMED_FIELDS_END - TIMED_FIELDS_BEGIN,
  SIGNIN_FIELDS_END = TIMED_FIELDS_END,
  SIGNIN_FIELDS_COUNT = SIGNIN_FIELDS_END - SIGNIN_FIELDS_BEGIN
};

// Returns the root preference path for the service. The path should be
// qualified with one of .value, .status or .time to get the respective
// full preference path names.
std::string TokenPrefPath(const std::string& service_name);

// Returns the name of a SigninStatus field.
std::string SigninStatusFieldToString(UntimedSigninStatusField field);
std::string SigninStatusFieldToString(TimedSigninStatusField field);

// An Observer class for authentication and token diagnostic information.
class SigninDiagnosticsObserver {
 public:
  // Credentials and signin related changes.
  virtual void NotifySigninValueChanged(const TimedSigninStatusField& field,
                                        const std::string& value) {}
};

// Gets the first 6 hex characters of the SHA256 hash of the passed in string.
// These are enough to perform equality checks across a single users tokens,
// while preventing outsiders from reverse-engineering the actual token from
// the displayed value.
// Note that for readability (in about:signin-internals), an empty string
// is not hashed, but simply returned as an empty string.
std::string GetTruncatedHash(const std::string& str);

} // namespace signin_internals_util

#endif  // COMPONENTS_SIGNIN_CORE_BROWSER_SIGNIN_INTERNALS_UTIL_H_
