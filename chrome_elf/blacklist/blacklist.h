// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_ELF_BLACKLIST_BLACKLIST_H_
#define CHROME_ELF_BLACKLIST_BLACKLIST_H_

#if defined(_WIN64)
#include "sandbox/win/src/sandbox_nt_types.h"
#endif

#include <stddef.h>

#include <string>

namespace blacklist {

// Max size of the DLL blacklist.
const size_t kTroublesomeDllsMaxCount = 64;

// The DLL blacklist.
extern const wchar_t* g_troublesome_dlls[kTroublesomeDllsMaxCount];

#if defined(_WIN64)
extern NtMapViewOfSectionFunction g_nt_map_view_of_section_func;
#endif

// Attempts to leave a beacon in the current user's registry hive. If the
// blacklist beacon doesn't say it is enabled or there are any other errors when
// creating the beacon, returns false. Otherwise returns true. The intent of the
// beacon is to act as an extra failure mode protection whereby if Chrome
// repeatedly fails to start during blacklist setup, it will skip blacklisting
// on the subsequent run.
bool LeaveSetupBeacon();

// Looks for the setup running beacon that LeaveSetupBeacon() creates and resets
// it to to show the setup was successful.
// Returns true if the beacon was successfully set to BLACKLIST_ENABLED.
bool ResetBeacon();

// Return the size of the current blacklist.
extern "C" int BlacklistSize();

// Returns if true if the blacklist has been initialized.
extern "C" bool IsBlacklistInitialized();

// Adds the given dll name to the blacklist. Returns true if the dll name is in
// the blacklist when this returns, false on error. Note that this will copy
// |dll_name| and will leak it on exit if the string is not subsequently removed
// using RemoveDllFromBlacklist.
// Exposed for testing only, this shouldn't be exported from chrome_elf.dll.
extern "C" bool AddDllToBlacklist(const wchar_t* dll_name);

// Removes the given dll name from the blacklist. Returns true if it was
// removed, false on error.
// Exposed for testing only, this shouldn't be exported from chrome_elf.dll.
extern "C" bool RemoveDllFromBlacklist(const wchar_t* dll_name);

// Returns a list of all the dlls that have been successfully blocked by the
// blacklist via blocked_dlls, if there is enough space (according to |size|).
// |size| will always be modified to be the number of dlls that were blocked.
// The caller doesn't own the strings and isn't expected to free them. These
// strings won't be hanging unless RemoveDllFromBlacklist is called, but it
// is only exposed in tests (and should stay that way).
extern "C" void SuccessfullyBlocked(const wchar_t** blocked_dlls, int* size);

// Record that the dll at the given index was blocked.
extern "C" void BlockedDll(size_t blocked_index);

// Legacy match function.
// Returns the index of the blacklist found in |g_troublesome_dlls|, or -1.
int DllMatch(const std::wstring& module_name);

// New wrapper for above match function.
// Returns true if a matching name is found in the legacy blacklist.
// Note: |module_name| must be an ASCII encoded string.
bool DllMatch(const std::string& module_name);

// Initializes the DLL blacklist in the current process. This should be called
// before any undesirable DLLs might be loaded. If |force| is set to true, then
// initialization will take place even if a beacon is present. This is useful
// for tests.
bool Initialize(bool force);

}  // namespace blacklist

#endif  // CHROME_ELF_BLACKLIST_BLACKLIST_H_
