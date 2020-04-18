// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Brand-specific constants and install modes for Chromium.

#include <stdlib.h>

#include "chrome/app/chrome_dll_resource.h"
#include "chrome/common/chrome_icon_resources_win.h"
#include "chrome/install_static/install_modes.h"

namespace install_static {

const wchar_t kCompanyPathName[] = L"";

const wchar_t kProductPathName[] = L"Chromium";

const size_t kProductPathNameLength = _countof(kProductPathName) - 1;

// No integration with Google Update, so no app GUID.
const wchar_t kBinariesAppGuid[] = L"";

const wchar_t kBinariesPathName[] = L"Chromium Binaries";

const InstallConstants kInstallModes[] = {
    // The primary (and only) install mode for Chromium.
    {
        sizeof(kInstallModes[0]),
        CHROMIUM_INDEX,  // The one and only mode for Chromium.
        "",              // No install switch for the primary install mode.
        L"",             // Empty install_suffix for the primary install mode.
        L"",             // No logo suffix for the primary install mode.
        L"",          // Empty app_guid since no integraion with Google Update.
        L"Chromium",  // A distinct base_app_name.
        L"Chromium",  // A distinct base_app_id.
        L"ChromiumHTM",                             // ProgID prefix.
        L"Chromium HTML Document",                  // ProgID description.
        L"{7D2B3E1D-D096-4594-9D8F-A6667F12E0AC}",  // Active Setup GUID.
        L"{A2DF06F9-A21A-44A8-8A99-8B9C84F29160}",  // CommandExecuteImpl CLSID.
        {0x635EFA6F,
         0x08D6,
         0x4EC9,
         {0xBD, 0x14, 0x8A, 0x0F, 0xDE, 0x97, 0x51,
          0x59}},  // Toast Activator CLSID.
        {0xD133B120,
         0x6DB4,
         0x4D6B,
         {0x8B, 0xFE, 0x83, 0xBF, 0x8C, 0xA1, 0xB1, 0xB0}},  // Elevator CLSID.
        L"",  // Empty default channel name since no update integration.
        ChannelStrategy::UNSUPPORTED,
        true,   // Supports system-level installs.
        true,   // Supports in-product set as default browser UX.
        false,  // Does not support retention experiments.
        true,   // Supported multi-install.
        icon_resources::kApplicationIndex,  // App icon resource index.
        IDR_MAINFRAME,                      // App icon resource id.
    },
};

static_assert(_countof(kInstallModes) == NUM_INSTALL_MODES,
              "Imbalance between kInstallModes and InstallConstantIndex");

}  // namespace install_static
