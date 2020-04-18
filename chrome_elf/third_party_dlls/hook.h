// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_ELF_THIRD_PARTY_DLLS_HOOK_H_
#define CHROME_ELF_THIRD_PARTY_DLLS_HOOK_H_

namespace third_party_dlls {

// "static_cast<int>(HookStatus::value)" to access underlying value.
enum class HookStatus {
  kSuccess = 0,
  kInitImportsFailure = 1,
  kUnsupportedOs = 2,
  kVirtualProtectFail = 3,
  kApplyHookFail = 4,
  COUNT
};

// Apply hook.
// - Ensure the rest of third_party_dlls is initialized before hooking.
HookStatus ApplyHook();

}  // namespace third_party_dlls

#endif  // CHROME_ELF_THIRD_PARTY_DLLS_HOOK_H_
