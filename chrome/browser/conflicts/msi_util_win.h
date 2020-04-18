// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CONFLICTS_MSI_UTIL_WIN_H_
#define CHROME_BROWSER_CONFLICTS_MSI_UTIL_WIN_H_

#include <vector>

#include "base/callback.h"
#include "base/strings/string16.h"

class MsiUtil {
 public:
  virtual ~MsiUtil() {}

  // Using the Microsoft Installer API, retrieves the path of all the components
  // for a given product. This function should be called on a thread that allows
  // access to the file system. Returns false if any error occured, including if
  // the |product_guid| passed is not a GUID.
  //
  // Note: Marked virtual to allow mocking.
  virtual bool GetMsiComponentPaths(
      const base::string16& product_guid,
      std::vector<base::string16>* component_paths) const;
};

#endif  // CHROME_BROWSER_CONFLICTS_MSI_UTIL_WIN_H_
