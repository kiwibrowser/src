// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/chrome_content_client.h"

#if defined(GOOGLE_CHROME_BUILD)
const char ChromeContentClient::kNotPresent[] = "internal-not-yet-present";
const char ChromeContentClient::kPDFExtensionPluginName[] = "Chrome PDF Viewer";
const char ChromeContentClient::kPDFInternalPluginName[] = "Chrome PDF Plugin";
#else
const char ChromeContentClient::kPDFExtensionPluginName[] =
    "Chromium PDF Viewer";
const char ChromeContentClient::kPDFInternalPluginName[] =
    "Chromium PDF Plugin";
#endif
const char ChromeContentClient::kPDFPluginPath[] = "internal-pdf-viewer";
