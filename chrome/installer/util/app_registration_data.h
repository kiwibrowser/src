// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_UTIL_APP_REGISTRATION_DATA_H_
#define CHROME_INSTALLER_UTIL_APP_REGISTRATION_DATA_H_

#include "base/strings/string16.h"

// An interface to retrieve product-specific key paths to communicate with
// Google Update via registry.
class AppRegistrationData {
 public:
  virtual ~AppRegistrationData() {}
  virtual base::string16 GetStateKey() const = 0;
  virtual base::string16 GetStateMediumKey() const = 0;
  virtual base::string16 GetVersionKey() const = 0;
};

#endif  // CHROME_INSTALLER_UTIL_APP_REGISTRATION_DATA_H_
