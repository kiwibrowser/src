// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PDF_PDF_EXTENSION_UTIL_H_
#define CHROME_BROWSER_PDF_PDF_EXTENSION_UTIL_H_

#include <string>

namespace pdf_extension_util {

// Return the extensions manifest for PDF. The manifest is loaded from
// browser_resources.grd and certain fields are replaced based on what chrome
// flags are enabled.
std::string GetManifest();

}  // namespace pdf_extension_util

#endif  // CHROME_BROWSER_PDF_PDF_EXTENSION_UTIL_H_
