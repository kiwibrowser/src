// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string.h>

#include "third_party/hunspell/google/bdict.h"

// static
bool hunspell::BDict::Verify(const char* bdict_data, size_t bdict_length) {
  if (bdict_length <= sizeof(hunspell::BDict::Header))
    return false;

  const BDict::Header* header =
      reinterpret_cast<const hunspell::BDict::Header*>(bdict_data);
  if (header->signature != hunspell::BDict::SIGNATURE ||
      header->major_version > hunspell::BDict::MAJOR_VERSION ||
      header->dic_offset > bdict_length)
    return false;

  // Get the affix header, make sure there is enough room for it.
  if (header->aff_offset + sizeof(hunspell::BDict::AffHeader) > bdict_length)
    return false;

  // Make sure there is enough room for the affix group count dword.
  const hunspell::BDict::AffHeader* aff_header =
      reinterpret_cast<const hunspell::BDict::AffHeader*>(
          &bdict_data[header->aff_offset]);
  if (aff_header->affix_group_offset + sizeof(uint32_t) > bdict_length)
    return false;

  // The new BDICT header has a MD5 digest of the dictionary data. Compare the
  // MD5 digest of the data with the one in the BDICT header.
  if (header->major_version >= 2) {
    base::MD5Digest digest;
    base::MD5Sum(aff_header, bdict_length - header->aff_offset, &digest);
    if (memcmp(&digest, &header->digest, sizeof(digest)))
      return false;
  }

  return true;
}
