// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// The local-auth module allows for user authentication in the case when
// on-line authentication is not possible (e.g. there is no network
// connection).

#ifndef CHROME_BROWSER_SIGNIN_LOCAL_AUTH_H_
#define CHROME_BROWSER_SIGNIN_LOCAL_AUTH_H_

#include <stddef.h>

#include <string>

#include "base/gtest_prod_util.h"

class LocalAuthTest;
class Profile;
class ProfileAttributesEntry;

namespace user_prefs {
class PrefRegistrySyncable;
}

class LocalAuth {
 public:
  static void RegisterLocalAuthPrefs(
      user_prefs::PrefRegistrySyncable* registry);

  static void SetLocalAuthCredentials(ProfileAttributesEntry* entry,
                                      const std::string& password);

  static void SetLocalAuthCredentials(const Profile* profile,
                                      const std::string& password);

  static bool ValidateLocalAuthCredentials(ProfileAttributesEntry* entry,
                                           const std::string& password);

  static bool ValidateLocalAuthCredentials(const Profile* profile,
                                           const std::string& password);

 private:
  FRIEND_TEST_ALL_PREFIXES(LocalAuthTest, SetUpgradeAndCheckCredentials);
  FRIEND_TEST_ALL_PREFIXES(LocalAuthTest, TruncateStringEvenly);
  FRIEND_TEST_ALL_PREFIXES(LocalAuthTest, TruncateStringUnevenly);

  // Return only the first |len_bits| bits of the string |str|. Defined here for
  // testing.
  static std::string TruncateStringByBits(const std::string& str,
                                          const size_t len_bits);

  static void SetLocalAuthCredentialsWithEncoding(ProfileAttributesEntry* entry,
                                                  const std::string& password,
                                                  char encoding_version);
};

#endif  // CHROME_BROWSER_SIGNIN_LOCAL_AUTH_H_
