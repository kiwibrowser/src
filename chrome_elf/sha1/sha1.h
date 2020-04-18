// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//------------------------------------------------------------------------------
// * This code is taken from base/sha1, with small changes.
//------------------------------------------------------------------------------

#ifndef CHROME_ELF_SHA1_SHA1_H_
#define CHROME_ELF_SHA1_SHA1_H_

#include <stddef.h>

#include <string>

namespace elf_sha1 {

// Length in bytes of a SHA-1 hash.
constexpr size_t kSHA1Length = 20;

// Compare two hashes.
// - Returns -1 if |first| < |second|
// - Returns 0 if |first| == |second|
// - Returns 1 if |first| > |second|
// Note: Arguments should be kSHA1Length long.
int CompareHashes(const uint8_t* first, const uint8_t* second);

// Computes the SHA1 hash of the input string |str| and returns the full
// hash.  The returned SHA1 will be 20 bytes in length.
std::string SHA1HashString(const std::string& str);

}  // namespace elf_sha1

#endif  // CHROME_ELF_SHA1_SHA1_H_
