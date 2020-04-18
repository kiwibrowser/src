// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_ELF_THIRD_PARTY_DLLS_IMES_H_
#define CHROME_ELF_THIRD_PARTY_DLLS_IMES_H_

#include <stdint.h>

namespace third_party_dlls {

// "static_cast<int>(IMEStatus::value)" to access underlying value.
enum class IMEStatus {
  kSuccess = 0,
  kErrorGeneric = 1,
  kMissingRegKey = 2,
  kRegAccessDenied = 3,
  kGenericRegFailure = 4,
  COUNT
};

// This list of IMEs was provided by Microsoft as their own.
extern const wchar_t* const kMicrosoftImeGuids[];
// The number of elements in the kMicrosoftImeGuids array.
extern const uint32_t kMicrosoftImeGuidsLength;
// The length of a GUID string.  (Hard-coded for compile time.)
enum { kGuidLength = 38 };

// Constant registry locations for IME information.
extern const wchar_t kClassesKey[];
extern const wchar_t kClassesSubkey[];
extern const wchar_t kImeRegistryKey[];

// Initialize internal list of registered IMEs.
IMEStatus InitIMEs();

// Removes initialization for use by tests, or cleanup on failure.
void DeinitIMEs();

}  // namespace third_party_dlls

#endif  // CHROME_ELF_THIRD_PARTY_DLLS_IMES_H_
