// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/os_crypt/ie7_password_win.h"

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "base/sha1.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "crypto/wincrypt_shim.h"

namespace {

// Structures that IE7/IE8 use to store a username/password.
// Some of the fields might have been incorrectly reverse engineered.
struct PreHeader {
  DWORD pre_header_size;  // Size of this header structure. Always 12.
  DWORD header_size;      // Size of the real Header: sizeof(Header) +
                          // item_count * sizeof(Entry);
  DWORD data_size;        // Size of the data referenced by the entries.
};

struct Header {
  char wick[4];             // The string "WICK". I don't know what it means.
  DWORD fixed_header_size;  // The size of this structure without the entries:
                            // sizeof(Header).
  DWORD item_count;         // Number of entries. Should be even.
  wchar_t two_letters[2];   // Two unknown bytes.
  DWORD unknown[2];         // Two unknown DWORDs.
};

struct Entry {
  DWORD offset;         // Offset where the data referenced by this entry is
                        // located.
  FILETIME time_stamp;  // Timestamp when the password got added.
  DWORD string_length;  // The length of the data string.
};

// Main data structure.
struct PasswordEntry {
  PreHeader pre_header;  // Contains the size of the different sections.
  Header header;         // Contains the number of items.
  Entry entry[1];        // List of entries containing a string. Even-indexed
                         // are usernames, odd are passwords. There may be
                         // several sets saved for a single url hash.
};
}  // namespace

IE7PasswordInfo::IE7PasswordInfo() {
}

IE7PasswordInfo::IE7PasswordInfo(const IE7PasswordInfo& other) = default;

IE7PasswordInfo::~IE7PasswordInfo() {
}

namespace ie7_password {

bool GetUserPassFromData(const std::vector<unsigned char>& data,
                         std::vector<DecryptedCredentials>* credentials) {
  const PasswordEntry* information =
      reinterpret_cast<const PasswordEntry*>(&data.front());

  // Some expected values. If it's not what we expect we don't even try to
  // understand the data.
  if (information->pre_header.pre_header_size != sizeof(PreHeader))
    return false;

  const int entry_count = information->header.item_count;
  if (entry_count % 2)  // Usernames and Passwords
    return false;

  if (information->header.fixed_header_size != sizeof(Header))
    return false;

  const uint8_t* offset_to_data = &data[0] +
                                  information->pre_header.header_size +
                                  information->pre_header.pre_header_size;

  for (int i = 0; i < entry_count / 2; ++i) {
    const Entry* user_entry = &information->entry[2*i];
    const Entry* pass_entry = user_entry+1;

    DecryptedCredentials c;
    c.username = reinterpret_cast<const wchar_t*>(offset_to_data +
                                                  user_entry->offset);
    c.password = reinterpret_cast<const wchar_t*>(offset_to_data +
                                                  pass_entry->offset);
    credentials->push_back(c);
  }
  return true;
}

std::wstring GetUrlHash(const std::wstring& url) {
  std::wstring lower_case_url = base::ToLowerASCII(url);
  // Get a data buffer out of our std::wstring to pass to SHA1HashString.
  std::string url_buffer(
      reinterpret_cast<const char*>(lower_case_url.c_str()),
      (lower_case_url.size() + 1) * sizeof(wchar_t));
  std::string hash_bin = base::SHA1HashString(url_buffer);

  std::wstring url_hash;

  // Transform the buffer to an hexadecimal string.
  unsigned char checksum = 0;
  for (size_t i = 0; i < hash_bin.size(); ++i) {
    // std::string gives signed chars, which mess with StringPrintf and
    // check_sum.
    unsigned char hash_byte = static_cast<unsigned char>(hash_bin[i]);
    checksum += hash_byte;
    url_hash += base::StringPrintf(L"%2.2X", static_cast<unsigned>(hash_byte));
  }
  url_hash += base::StringPrintf(L"%2.2X", checksum);

  return url_hash;
}

bool DecryptPasswords(const std::wstring& url,
                      const std::vector<unsigned char>& data,
                      std::vector<DecryptedCredentials>* credentials) {
  std::wstring lower_case_url = base::ToLowerASCII(url);
  DATA_BLOB input = {0};
  DATA_BLOB output = {0};
  DATA_BLOB url_key = {0};

  input.pbData = const_cast<unsigned char*>(&data.front());
  input.cbData = static_cast<DWORD>((data.size()) *
                                    sizeof(std::string::value_type));

  url_key.pbData = reinterpret_cast<unsigned char*>(
                      const_cast<wchar_t*>(lower_case_url.data()));
  url_key.cbData = static_cast<DWORD>((lower_case_url.size() + 1) *
                                      sizeof(std::wstring::value_type));

  if (CryptUnprotectData(&input, nullptr, &url_key, nullptr, nullptr,
                         CRYPTPROTECT_UI_FORBIDDEN, &output)) {
    // Now that we have the decrypted information, we need to understand it.
    std::vector<unsigned char> decrypted_data;
    decrypted_data.resize(output.cbData);
    memcpy(&decrypted_data.front(), output.pbData, output.cbData);

    GetUserPassFromData(decrypted_data, credentials);

    LocalFree(output.pbData);
    return true;
  }

  return false;
}

}  // namespace ie7_password
