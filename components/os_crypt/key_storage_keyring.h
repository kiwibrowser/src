// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OS_CRYPT_KEY_STORAGE_KEYRING_H_
#define COMPONENTS_OS_CRYPT_KEY_STORAGE_KEYRING_H_

#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/os_crypt/key_storage_linux.h"

namespace base {
class SingleThreadTaskRunner;
}  // namespace base

// Specialisation of KeyStorageLinux that uses Libsecret.
class KeyStorageKeyring : public KeyStorageLinux {
 public:
  explicit KeyStorageKeyring(
      scoped_refptr<base::SingleThreadTaskRunner> main_thread_runner);
  ~KeyStorageKeyring() override;

 protected:
  // KeyStorageLinux
  base::SequencedTaskRunner* GetTaskRunner() override;
  bool Init() override;
  std::string GetKeyImpl() override;

 private:
  // Generate a random string and store it as OScrypt's new password.
  std::string AddRandomPasswordInKeyring();

  // Keyring calls need to originate from the main thread.
  scoped_refptr<base::SingleThreadTaskRunner> main_thread_runner_;

  DISALLOW_COPY_AND_ASSIGN(KeyStorageKeyring);
};

#endif  // COMPONENTS_OS_CRYPT_KEY_STORAGE_KEYRING_H_
