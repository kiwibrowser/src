// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <windows.h>

#include <stdlib.h>

#include "chrome/install_static/product_install_details.h"
#include "chrome_elf/third_party_dlls/main.h"

//------------------------------------------------------------------------------
// PUBLIC
//------------------------------------------------------------------------------

// Good ol' main.
// - Init third_party_dlls, which will apply a hook to NtMapViewOfSection.
// - Attempt to load a specific DLL.
//
// Returns:
// - Negative values in case of unexpected error.
// - 0 for successful DLL load.
// - 1 for failed DLL load.
int main(int argc, char** argv) {
  if (third_party_dlls::IsThirdPartyInitialized())
    _exit(-1);

  install_static::InitializeProductDetailsForPrimaryModule();

  if (!third_party_dlls::Init())
    _exit(-2);

  return 0;
}
