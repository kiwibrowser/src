// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <Cocoa/Cocoa.h>

#include <dlfcn.h>

#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"

#include "chrome/common/importer/firefox_importer_utils.h"
#include "chrome/utility/importer/nss_decryptor_mac.h"

// Important!! : On OS X the nss3 libraries are compiled with depedencies
// on one another, referenced using dyld's @executable_path directive.
// To make a long story short in order to get the libraries to load, dyld's
// fallback path needs to be set to the directory containing the libraries.
// To do so, the process this function runs in must have the
// DYLD_FALLBACK_LIBRARY_PATH set on startup to said directory.
bool NSSDecryptor::Init(const base::FilePath& dll_path,
                        const base::FilePath& db_path) {
  if (getenv("DYLD_FALLBACK_LIBRARY_PATH") == NULL) {
    LOG(ERROR) << "DYLD_FALLBACK_LIBRARY_PATH variable not set";
    return false;
  }
  base::FilePath nss3_path = dll_path.Append("libnss3.dylib");

  void* nss_3_lib = dlopen(nss3_path.value().c_str(), RTLD_LAZY);
  if (!nss_3_lib) {
    LOG(ERROR) << "Failed to load nss3 lib" << dlerror();
    return false;
  }

  NSS_Init = (NSSInitFunc)dlsym(nss_3_lib, "NSS_Init");
  NSS_Shutdown = (NSSShutdownFunc)dlsym(nss_3_lib, "NSS_Shutdown");
  PK11_GetInternalKeySlot =
      (PK11GetInternalKeySlotFunc)dlsym(nss_3_lib, "PK11_GetInternalKeySlot");
  PK11_CheckUserPassword =
      (PK11CheckUserPasswordFunc)dlsym(nss_3_lib, "PK11_CheckUserPassword");
  PK11_FreeSlot = (PK11FreeSlotFunc)dlsym(nss_3_lib, "PK11_FreeSlot");
  PK11_Authenticate =
      (PK11AuthenticateFunc)dlsym(nss_3_lib, "PK11_Authenticate");
  PK11SDR_Decrypt = (PK11SDRDecryptFunc)dlsym(nss_3_lib, "PK11SDR_Decrypt");
  SECITEM_FreeItem = (SECITEMFreeItemFunc)dlsym(nss_3_lib, "SECITEM_FreeItem");

  if (!NSS_Init || !NSS_Shutdown || !PK11_GetInternalKeySlot ||
      !PK11_CheckUserPassword || !PK11_FreeSlot || !PK11_Authenticate ||
      !PK11SDR_Decrypt || !SECITEM_FreeItem) {
    LOG(ERROR) << "NSS3 importer couldn't find entry points";
    return false;
  }

  SECStatus result = NSS_Init(db_path.value().c_str());

  if (result != SECSuccess) {
    LOG(ERROR) << "NSS_Init Failed returned: " << result;
    return false;
  }

  is_nss_initialized_ = true;
  return true;
}

NSSDecryptor::~NSSDecryptor() {
  if (NSS_Shutdown && is_nss_initialized_) {
    NSS_Shutdown();
    is_nss_initialized_ = false;
  }
}
