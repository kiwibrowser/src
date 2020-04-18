// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome_elf/third_party_dlls/imes.h"

#include <windows.h>

#include "base/files/file.h"
#include "base/test/test_reg_util_win.h"
#include "base/win/registry.h"
#include "chrome_elf/nt_registry/nt_registry.h"
#include "sandbox/win/src/nt_internals.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace third_party_dlls {
namespace {

constexpr wchar_t kNewGuid[] = L"{12345678-1234-1234-1234-123456789ABC}";

void RegRedirect(nt::ROOT_KEY key,
                 registry_util::RegistryOverrideManager* rom) {
  ASSERT_NE(key, nt::AUTO);
  HKEY root = (key == nt::HKCU ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE);
  base::string16 temp;

  ASSERT_NO_FATAL_FAILURE(rom->OverrideRegistry(root, &temp));
  ASSERT_TRUE(nt::SetTestingOverride(key, temp));
}

void CancelRegRedirect(nt::ROOT_KEY key) {
  ASSERT_NE(key, nt::AUTO);
  ASSERT_TRUE(nt::SetTestingOverride(key, base::string16()));
}

bool GetExistingImeDllPath(std::wstring* path) {
  base::win::RegistryKeyIterator ime_iterator(HKEY_LOCAL_MACHINE,
                                              kImeRegistryKey);

  // Iterate through IMEs registered on the system until finding one with a
  // valid dll path.
  for (; ime_iterator.Valid(); ++ime_iterator) {
    const wchar_t* guid = ime_iterator.Name();
    // Make sure it's a guid.
    if (::wcslen(guid) != kGuidLength)
      continue;

    std::wstring classes_path(kClassesKey);
    classes_path.append(guid);
    classes_path.push_back(L'\\');
    classes_path.append(kClassesSubkey);

    base::win::RegKey classes_key(HKEY_LOCAL_MACHINE, classes_path.c_str(),
                                  KEY_QUERY_VALUE);
    if (!classes_key.Valid())
      continue;

    // Grab the default value.
    if (classes_key.ReadValue(L"", path) != ERROR_SUCCESS)
      continue;

    // Make sure the path to a DLL exists.  Quite common for it not to.
    base::File test(base::FilePath(path->c_str()),
                    base::File::FLAG_OPEN | base::File::FLAG_READ);
    if (test.IsValid())
      return true;
  }

  return false;
}

bool RegisterFakeIme(const wchar_t* new_guid) {
  base::win::RegKey parent_key(HKEY_LOCAL_MACHINE, kImeRegistryKey,
                               KEY_CREATE_SUB_KEY);
  if (!parent_key.Valid())
    return false;
  if (parent_key.CreateKey(new_guid, KEY_WRITE) != ERROR_SUCCESS)
    return false;

  return true;
}

bool AddClassDllPath(const wchar_t* new_guid, const std::wstring& dll_path) {
  base::win::RegKey parent_key(HKEY_LOCAL_MACHINE, kClassesKey,
                               KEY_CREATE_SUB_KEY);
  if (!parent_key.Valid())
    return false;
  if (parent_key.CreateKey(new_guid, KEY_WRITE) != ERROR_SUCCESS)
    return false;
  if (parent_key.CreateKey(kClassesSubkey, KEY_WRITE) != ERROR_SUCCESS)
    return false;
  if (parent_key.WriteValue(L"", dll_path.c_str()) != ERROR_SUCCESS)
    return false;

  return true;
}

//------------------------------------------------------------------------------
// Third-party IME tests
//------------------------------------------------------------------------------

// Test initialization with whatever IMEs are on the current machine.
TEST(ThirdParty, IMEInitExisting) {
  // Init IME!
  EXPECT_EQ(InitIMEs(), IMEStatus::kSuccess);
  DeinitIMEs();
}

// Test initialization with a custom IME to exercise all of the code path for
// non-Microsoft-provided IMEs.
//
// Note: Uses registry test override to protect machine from registry writes.
TEST(ThirdParty, IMEInitNonMs) {
  // 1. Read in any real registry info needed.
  // 2. Enable reg override for test net.
  // 3. Write fake registry data.
  // 4. InitIMEs()
  // 5. Disable reg override.

  // 1. Read in any real registry info needed.
  //    - A dll path to use from an existing MS IME.
  std::wstring existing_ime_dll;
  ASSERT_TRUE(GetExistingImeDllPath(&existing_ime_dll));

  // 2. Enable reg override for test net.
  registry_util::RegistryOverrideManager override_manager;
  ASSERT_NO_FATAL_FAILURE(RegRedirect(nt::HKLM, &override_manager));

  // 3. Add a fake registry IME guid and Class DLL path.
  EXPECT_TRUE(RegisterFakeIme(kNewGuid));
  EXPECT_TRUE(AddClassDllPath(kNewGuid, existing_ime_dll));

  // 4. Init IME!
  EXPECT_EQ(InitIMEs(), IMEStatus::kSuccess);

  // 5. Disable reg override.
  ASSERT_NO_FATAL_FAILURE(CancelRegRedirect(nt::HKLM));

  DeinitIMEs();
}

}  // namespace
}  // namespace third_party_dlls
