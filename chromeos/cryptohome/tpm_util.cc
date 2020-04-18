// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/cryptohome/tpm_util.h"

#include <stdint.h>

#include "base/logging.h"
#include "chromeos/dbus/cryptohome_client.h"
#include "chromeos/dbus/dbus_thread_manager.h"

namespace chromeos {
namespace tpm_util {

bool TpmIsEnabled() {
  bool result = false;
  DBusThreadManager::Get()->GetCryptohomeClient()->CallTpmIsEnabledAndBlock(
      &result);
  return result;
}

bool TpmIsOwned() {
  bool result = false;
  DBusThreadManager::Get()->GetCryptohomeClient()->CallTpmIsOwnedAndBlock(
      &result);
  return result;
}

bool TpmIsBeingOwned() {
  bool result = false;
  DBusThreadManager::Get()->GetCryptohomeClient()->CallTpmIsBeingOwnedAndBlock(
      &result);
  return result;
}

bool InstallAttributesGet(const std::string& name, std::string* value) {
  std::vector<uint8_t> buf;
  bool success = false;
  DBusThreadManager::Get()->GetCryptohomeClient()->InstallAttributesGet(
      name, &buf, &success);
  if (success) {
    // Cryptohome returns 'buf' with a terminating '\0' character.
    DCHECK(!buf.empty());
    DCHECK_EQ(buf.back(), 0);
    value->assign(reinterpret_cast<char*>(buf.data()), buf.size() - 1);
  }
  return success;
}

bool InstallAttributesSet(const std::string& name, const std::string& value) {
  std::vector<uint8_t> buf(value.c_str(), value.c_str() + value.size() + 1);
  bool success = false;
  DBusThreadManager::Get()->GetCryptohomeClient()->InstallAttributesSet(
      name, buf, &success);
  return success;
}

bool InstallAttributesFinalize() {
  bool success = false;
  DBusThreadManager::Get()->GetCryptohomeClient()->InstallAttributesFinalize(
      &success);
  return success;
}

bool InstallAttributesIsInvalid() {
  bool result = false;
  DBusThreadManager::Get()->GetCryptohomeClient()->InstallAttributesIsInvalid(
      &result);
  return result;
}

bool InstallAttributesIsFirstInstall() {
  bool result = false;
  DBusThreadManager::Get()
      ->GetCryptohomeClient()
      ->InstallAttributesIsFirstInstall(&result);
  return result;
}

}  // namespace tpm_util
}  // namespace chromeos
