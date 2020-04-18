// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OS_CRYPT_IE7_PASSWORD_WIN_H_
#define COMPONENTS_OS_CRYPT_IE7_PASSWORD_WIN_H_

#include <windows.h>
#include <string>
#include <vector>

#include "base/time/time.h"

// Contains the information read from the IE7/IE8 Storage2 key in the registry.
struct IE7PasswordInfo {
  IE7PasswordInfo();
  IE7PasswordInfo(const IE7PasswordInfo& other);
  ~IE7PasswordInfo();

  // Hash of the url.
  std::wstring url_hash;

  // Encrypted data containing the username, password and some more undocumented
  // fields.
  std::vector<unsigned char> encrypted_data;

  // When the login was imported.
  base::Time date_created;
};

namespace ie7_password {

struct DecryptedCredentials {
  std::wstring username;
  std::wstring password;
};

// Parses a data structure to find passwords and usernames.
// The collection of bytes in |data| is interpreted as a special PasswordEntry
// structure. IE saves the login information as a binary dump of this structure.
// Credentials extracted from |data| end up in |credentials|.
bool GetUserPassFromData(const std::vector<unsigned char>& data,
                         std::vector<DecryptedCredentials>* credentials);

// Decrypts usernames and passwords for a given data vector using the url as
// the key.
// Output ends up in |credentials|.
bool DecryptPasswords(const std::wstring& url,
                      const std::vector<unsigned char>& data,
                      std::vector<DecryptedCredentials>* credentials);

// Returns the hash of a url.
std::wstring GetUrlHash(const std::wstring& url);

}  // namespace ie7_password

#endif  // COMPONENTS_OS_CRYPT_IE7_PASSWORD_WIN_H_
