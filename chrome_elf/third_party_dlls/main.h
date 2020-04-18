// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//  Developers:
//  - All contents of the third_party_dlls project must be compatible
//    with chrome_elf (early loading framework) requirements.
//
//  - IMPORTANT: all code executed during Init() must be meet the restrictions
//    for DllMain on Windows.
//    https://msdn.microsoft.com/en-us/library/windows/desktop/dn633971.aspx

#ifndef CHROME_ELF_THIRD_PARTY_DLLS_MAIN_H_
#define CHROME_ELF_THIRD_PARTY_DLLS_MAIN_H_

namespace third_party_dlls {

// Third-party DLL control is enabled and initialized in this process.
bool IsThirdPartyInitialized();

// Init Third-Party
// ----------------
// Central initialization for all third-party DLL management. Users only need to
// include this file, and call this one function at startup.
// - This should be called as early as possible, before any undesirable DLLs
//   might be loaded.
// - Ensure elf_crash component has been initialized before calling.
// - This initialization will fail on unsupported versions of Windows.
bool Init();

}  // namespace third_party_dlls

#endif  // CHROME_ELF_THIRD_PARTY_DLLS_MAIN_H_
