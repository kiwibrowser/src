// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/util/self_reg_work_item.h"

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "chrome/installer/util/logging_installer.h"

// Default registration export names.
const char kDefaultRegistrationEntryPoint[] = "DllRegisterServer";
const char kDefaultUnregistrationEntryPoint[] = "DllUnregisterServer";

// User-level registration export names.
const char kUserRegistrationEntryPoint[] = "DllRegisterUserServer";
const char kUserUnregistrationEntryPoint[] = "DllUnregisterUserServer";

SelfRegWorkItem::SelfRegWorkItem(const std::wstring& dll_path,
                                 bool do_register,
                                 bool user_level_registration)
    : dll_path_(dll_path),
      do_register_(do_register),
      user_level_registration_(user_level_registration) {
}

SelfRegWorkItem::~SelfRegWorkItem() {
}

// This is designed to unmux error codes that may be shoe-horned in to HRESULT
// return codes by ORing a number into the top four bits of the facility code
// Any number thus found will be returned in |error_code|. The "cleaned"
// HRESULT is then returned.
//
// This only has an effect if the customer bit is set in the HRESULT, if it is
// not set then *error_code will be unchanged and the original HRESULT is
// returned.
//
// Note that this will do the wrong thing for high-valued facility codes.
HRESULT UnMuxHRESULTErrorCode(HRESULT hr, int* error_code) {
  DCHECK(error_code);
  if (hr & (1 << 29)) {
    *error_code = (hr & 0x07800000) >> 23;
    return hr & 0xF87FFFFF;
  }

  return hr;
}

bool SelfRegWorkItem::RegisterDll(bool do_register) {
  VLOG(1) << "COM " << (do_register ? "registration of " : "unregistration of ")
          << dll_path_;

  HMODULE dll_module = ::LoadLibraryEx(dll_path_.c_str(), NULL,
                                       LOAD_WITH_ALTERED_SEARCH_PATH);
  bool success = false;
  if (NULL != dll_module) {
    typedef HRESULT (WINAPI* RegisterFunc)();
    RegisterFunc register_server_func = NULL;
    if (do_register) {
      register_server_func = reinterpret_cast<RegisterFunc>(
          ::GetProcAddress(dll_module, user_level_registration_ ?
              kUserRegistrationEntryPoint : kDefaultRegistrationEntryPoint));
    } else {
      register_server_func = reinterpret_cast<RegisterFunc>(
          ::GetProcAddress(dll_module, user_level_registration_ ?
              kUserUnregistrationEntryPoint :
              kDefaultUnregistrationEntryPoint));
    }

    if (NULL != register_server_func) {
      HRESULT hr = register_server_func();
      success = SUCCEEDED(hr);
      if (!success) {
        int error_code = 0;
        HRESULT unmuxed_hr = UnMuxHRESULTErrorCode(hr, &error_code);
        LOG(ERROR) << "Failed to " << (do_register ? "register" : "unregister")
                   << " DLL at " << dll_path_.c_str()
                   << ", hr=" << base::StringPrintf(" 0x%08lX", unmuxed_hr)
                   << ", code=" << error_code;
      }
    } else {
      LOG(ERROR) << "COM registration export function not found";
    }
    ::FreeLibrary(dll_module);
  } else {
    PLOG(WARNING) << "Failed to load: " << dll_path_;
  }
  return success;
}

bool SelfRegWorkItem::DoImpl() {
  return RegisterDll(do_register_);
}

void SelfRegWorkItem::RollbackImpl() {
  RegisterDll(!do_register_);
}
