// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OS_CRYPT_KEY_STORAGE_LIBSECRET_H_
#define COMPONENTS_OS_CRYPT_KEY_STORAGE_LIBSECRET_H_

#include <string>

#include "base/macros.h"
#include "components/os_crypt/key_storage_linux.h"

// Specialisation of KeyStorageLinux that uses Libsecret.
class KeyStorageLibsecret : public KeyStorageLinux {
 public:
  KeyStorageLibsecret() = default;
  ~KeyStorageLibsecret() override = default;

 protected:
  // KeyStorageLinux
  bool Init() override;
  std::string GetKeyImpl() override;

 private:
  std::string AddRandomPasswordInLibsecret();

  // TODO(crbug.com/639298) Older Chromium releases stored passwords with a
  // problematic schema. Detect password entries with the old schema and migrate
  // them to the new schema. Returns the migrated password or an empty string if
  // none we migrated.
  std::string Migrate();

  DISALLOW_COPY_AND_ASSIGN(KeyStorageLibsecret);
};

#endif  // COMPONENTS_OS_CRYPT_KEY_STORAGE_LIBSECRET_H_
