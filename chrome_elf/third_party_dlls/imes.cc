// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome_elf/third_party_dlls/imes.h"

#include <assert.h>
#include <string.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "chrome_elf/nt_registry/nt_registry.h"
#include "chrome_elf/pe_image_safe/pe_image_safe.h"

namespace third_party_dlls {
namespace {

// Holds third-party IME information.
class IME {
 public:
  // Move constructor
  IME(IME&&) noexcept = default;
  // Move assignment
  IME& operator=(IME&&) noexcept = default;
  // Take ownership of the std::wstrings passed in, to prevent copies.
  IME(std::wstring&& guid,
      DWORD image_size,
      DWORD date_time_stamp,
      std::wstring&& path)
      : image_size_(image_size),
        date_time_stamp_(date_time_stamp),
        guid_(std::move(guid)),
        path_(std::move(path)) {}

  DWORD image_size() { return image_size_; }
  DWORD date_time_stamp() { return date_time_stamp_; }
  const wchar_t* guid() { return guid_.c_str(); }
  const wchar_t* path() { return path_.c_str(); }

 private:
  DWORD image_size_;
  DWORD date_time_stamp_;
  std::wstring guid_;
  std::wstring path_;

  // DISALLOW_COPY_AND_ASSIGN(IME);
  IME(const IME&) = delete;
  IME& operator=(const IME&) = delete;
};

//------------------------------------------------------------------------------
// Private functions
//------------------------------------------------------------------------------

// List of third-party IMEs registered on system.
// NOTE: this list is only initialized once during InitIMEs().
// NOTE: this structure is wrapped in a function to prevent an exit-time dtor.
std::vector<IME>* GetImeVector() {
  static std::vector<IME>* const ime_list = new std::vector<IME>();
  return ime_list;
}

// Returns true if |ime_guid| is the GUID of a built-in Microsoft IME.
bool IsMicrosoftIme(const wchar_t* ime_guid) {
  assert(ime_guid);

  // Debug check hard-coded kGuidLength.
  assert(kGuidLength == ::wcslen(kMicrosoftImeGuids[0]));

  // Debug check the array is sorted.
  assert(std::is_sorted(kMicrosoftImeGuids,
                        kMicrosoftImeGuids + kMicrosoftImeGuidsLength));

  // The binary predicate (compare function) must return true if the first
  // argument is ordered before the second.
  return std::binary_search(
      kMicrosoftImeGuids, kMicrosoftImeGuids + kMicrosoftImeGuidsLength,
      ime_guid, [](const wchar_t* lhs, const wchar_t* rhs) {
        return (::wcsicmp(lhs, rhs) < 0);
      });
}

// Returns the path to the in-proc server DLL for |guid|.
bool GetInprocServerDllPath(const std::wstring& guid, std::wstring* value) {
  assert(value);

  std::wstring subpath(kClassesKey);
  subpath.append(guid);
  subpath.push_back(L'\\');
  subpath.append(kClassesSubkey);
  HANDLE key_handle = nullptr;

  // HKCR is actually a combination view of HKLM and HKCU.  HKLM are defaults
  // for all users and HKCU are user specific and OVERRIDE HKLM classes. So
  // check HKCU first, and only if it doesn't exist there, try HKLM.
  if (!nt::OpenRegKey(nt::HKCU, subpath.c_str(), KEY_QUERY_VALUE, &key_handle,
                      nullptr) &&
      !nt::OpenRegKey(nt::HKLM, subpath.c_str(), KEY_QUERY_VALUE, &key_handle,
                      nullptr)) {
    return false;
  }

  // "(Default)" value string is full path to DLL.
  bool found = nt::QueryRegValueSZ(key_handle, L"", value);
  nt::CloseRegKey(key_handle);

  return found;
}

// Mines DLL information from |path|.
bool GetModuleData(const wchar_t* path,
                   DWORD* size_of_image,
                   DWORD* time_date_stamp) {
  assert(path && size_of_image && time_date_stamp);

  // Only need the headers.
  constexpr DWORD kPageSize = 4096;
  // Don't zero the buffer - waste of time.
  BYTE buffer[kPageSize];

  // INVALID_HANDLE_VALUE alert.
  HANDLE file =
      ::CreateFileW(path, FILE_READ_DATA,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE)
    return false;

  DWORD read = 0;
  if (!::ReadFile(file, &buffer[0], kPageSize, &read, nullptr))
    read = 0;

  // Done with file.
  ::CloseHandle(file);
  if (read != kPageSize)
    return false;

  pe_image_safe::PEImageSafe wrapper(buffer, kPageSize);
  PIMAGE_FILE_HEADER file_header = wrapper.GetFileHeader();
  BYTE* optional_header = wrapper.GetOptionalHeader();
  if (!file_header || !optional_header)
    return false;

// Make sure bitness of the PE matches bitness of self.  If somehow the PE
// read in does not match THIS architecture, eject.
#if defined(_WIN64)
  constexpr pe_image_safe::ImageBitness kSelfBitness =
      pe_image_safe::ImageBitness::k64;
#else
  constexpr pe_image_safe::ImageBitness kSelfBitness =
      pe_image_safe::ImageBitness::k32;
#endif

  pe_image_safe::ImageBitness bitness = wrapper.GetImageBitness();
  if (bitness != kSelfBitness)
    return false;

  *time_date_stamp = file_header->TimeDateStamp;
  *size_of_image =
      reinterpret_cast<PIMAGE_OPTIONAL_HEADER>(optional_header)->SizeOfImage;

  return true;
}

}  // namespace

//------------------------------------------------------------------------------
// Public defines & functions
//------------------------------------------------------------------------------

// This list of IMEs was provided by Microsoft as their own.
const wchar_t* const kMicrosoftImeGuids[] = {
    L"{0000897b-83df-4b96-be07-0fb58b01c4a4}",
    L"{03b5835f-f03c-411b-9ce2-aa23e1171e36}",
    L"{07eb03d6-b001-41df-9192-bf9b841ee71f}",
    L"{3697c5fa-60dd-4b56-92d4-74a569205c16}",
    L"{531fdebf-9b4c-4a43-a2aa-960e8fcdc732}",
    L"{6a498709-e00b-4c45-a018-8f9e4081ae40}",
    L"{78cb5b0e-26ed-4fcc-854c-77e8f3d1aa80}",
    L"{81d4e9c9-1d3b-41bc-9e6c-4b40bf79e35e}",
    L"{8613e14c-d0c0-4161-ac0f-1dd2563286bc}",
    L"{a028ae76-01b1-46c2-99c4-acd9858ae02f}",
    L"{a1e2b86b-924a-4d43-80f6-8a820df7190f}",
    L"{ae6be008-07fb-400d-8beb-337a64f7051f}",
    L"{b115690a-ea02-48d5-a231-e3578d2fdf80}",
    L"{c1ee01f2-b3b6-4a6a-9ddd-e988c088ec82}",
    L"{dcbd6fa8-032f-11d3-b5b1-00c04fc324a1}",
    L"{e429b25a-e5d3-4d1f-9be3-0c608477e3a1}",
    L"{f25e9f57-2fc8-4eb3-a41a-cce5f08541e6}",
    L"{f89e9e58-bd2f-4008-9ac2-0f816c09f4ee}",
    L"{fa445657-9379-11d6-b41a-00065b83ee53}",
};
const uint32_t kMicrosoftImeGuidsLength =
    sizeof(kMicrosoftImeGuids) / sizeof(kMicrosoftImeGuids[0]);

const wchar_t kClassesKey[] = L"SOFTWARE\\Classes\\CLSID\\";
const wchar_t kClassesSubkey[] = L"InProcServer32";
const wchar_t kImeRegistryKey[] = L"SOFTWARE\\Microsoft\\CTF\\TIP";

// Harvest the registered IMEs in the system.
// Keep debug asserts for unexpected behaviour contained in here.
IMEStatus InitIMEs() {
  std::vector<IME>* ime_list = GetImeVector();

  // Debug check: InitIMEs should not be called more than once.
  assert(ime_list->empty());
  ime_list->clear();

  HANDLE key_handle = nullptr;
  NTSTATUS ntstatus = STATUS_UNSUCCESSFUL;

  // Open HKEY_LOCAL_MACHINE, kImeRegistryKey.
  if (!nt::OpenRegKey(nt::HKLM, kImeRegistryKey, KEY_READ, &key_handle,
                      &ntstatus)) {
    if (ntstatus == STATUS_OBJECT_NAME_NOT_FOUND)
      return IMEStatus::kMissingRegKey;
    if (ntstatus == STATUS_ACCESS_DENIED)
      return IMEStatus::kRegAccessDenied;
    return IMEStatus::kErrorGeneric;
  }

  ULONG count = 0;
  if (!nt::QueryRegEnumerationInfo(key_handle, &count)) {
    nt::CloseRegKey(key_handle);
    return IMEStatus::kGenericRegFailure;
  }

  for (ULONG i = 0; i < count; i++) {
    // If index is now bad, continue on.
    std::wstring subkey_name;
    if (!nt::QueryRegSubkey(key_handle, i, &subkey_name))
      continue;

    // Basic verification of expected GUID.
    if (subkey_name.length() != kGuidLength)
      continue;

    // Skip Microsoft IMEs since Chrome won't do anything about those.
    if (IsMicrosoftIme(subkey_name.c_str()))
      continue;

    // Collect DLL info.
    std::wstring dll_path;
    if (!GetInprocServerDllPath(subkey_name, &dll_path))
      continue;

    // Mine info out of the DLL.
    DWORD size = 0;
    DWORD date_time = 0;
    if (!GetModuleData(dll_path.c_str(), &size, &date_time))
      continue;

    // For each guid: store guid, size, timestamp, path.
    ime_list->emplace_back(std::move(subkey_name), size, date_time,
                           std::move(dll_path));
  }

  nt::CloseRegKey(key_handle);
  return IMEStatus::kSuccess;
}

void DeinitIMEs() {
  GetImeVector()->clear();
}

}  // namespace third_party_dlls
