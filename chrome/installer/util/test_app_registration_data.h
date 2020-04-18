// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_UTIL_TEST_APP_REGISTRATION_DATA_H_
#define CHROME_INSTALLER_UTIL_TEST_APP_REGISTRATION_DATA_H_

#include "chrome/installer/util/app_registration_data.h"

class TestAppRegistrationData : public AppRegistrationData {
 public:
  TestAppRegistrationData();
  ~TestAppRegistrationData() override;
  base::string16 GetStateKey() const override;
  base::string16 GetStateMediumKey() const override;
  base::string16 GetVersionKey() const override;
};

#endif  // CHROME_INSTALLER_UTIL_TEST_APP_REGISTRATION_DATA_H_
