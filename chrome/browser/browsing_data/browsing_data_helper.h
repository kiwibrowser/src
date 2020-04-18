// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Defines methods relevant to all code that wants to work with browsing data.

#ifndef CHROME_BROWSER_BROWSING_DATA_BROWSING_DATA_HELPER_H_
#define CHROME_BROWSER_BROWSING_DATA_BROWSING_DATA_HELPER_H_

#include <string>

#include "base/macros.h"

class GURL;

// TODO(crbug.com/668114): DEPRECATED. Remove this class.
// The primary functionality of testing origin type masks has moved to
// BrowsingDataRemover. The secondary functionality of recognizing web schemes
// storing browsing data has moved to url::GetWebStorageSchemes();
// or alternatively, it can also be retrieved from BrowsingDataRemover by
// testing the ORIGIN_TYPE_UNPROTECTED_ORIGIN | ORIGIN_TYPE_PROTECTED_ORIGIN
// origin mask. This class now merely forwards the functionality to several
// helper classes in the browsing_data codebase.
class BrowsingDataHelper {
 public:
  // Returns true iff the provided scheme is (really) web safe, and suitable
  // for treatment as "browsing data". This relies on the definition of web safe
  // in ChildProcessSecurityPolicy, but excluding schemes like
  // `chrome-extension`.
  static bool IsWebScheme(const std::string& scheme);
  static bool HasWebScheme(const GURL& origin);

  // Returns true iff the provided scheme is an extension.
  static bool IsExtensionScheme(const std::string& scheme);
  static bool HasExtensionScheme(const GURL& origin);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(BrowsingDataHelper);
};

#endif  // CHROME_BROWSER_BROWSING_DATA_BROWSING_DATA_HELPER_H_
